// addrspace.cc
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -n -T 0 option
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.
#include "copyright.h"
#include "main.h"
#include "addrspace.h"
#include "machine.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

bool AddrSpace::usedPhyPage[NumPhysPages] = {0};
TranslationEntry *AddrSpace::usedPhyPageEntry[NumPhysPages] = {nullptr};

static void
SwapHeader(NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Set up the translation from program memory to physical
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//----------------------------------------------------------------------

AddrSpace::AddrSpace()
{
    pageTable = new TranslationEntry[NumPhysPages];
    for (unsigned int i = 0; i < NumPhysPages; i++)
    {
        pageTable[i].virtualPage = i; // for now, virt page # = phys page #
        pageTable[i].physicalPage = i;
        //	pageTable[i].physicalPage = 0;
        pageTable[i].valid = TRUE;
        //	pageTable[i].valid = FALSE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;
    }

    // zero out the entire address space
    //    bzero(kernel->machine->mainMemory, MemorySize);
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    for (int i = 0; i < numPages; i++)
        AddrSpace::usedPhyPage[pageTable[i].physicalPage] = false;
    delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::Load
// 	Load a user program into memory from a file.
//
//	Assumes that the page table has been initialized, and that
//	the object code file is in NOFF format.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

bool AddrSpace::Load(char *fileName)
{
    // 打開執行檔，若無法打開則回傳 FALSE
    OpenFile *executable = kernel->fileSystem->Open(fileName);
    NoffHeader noffH; // 用於儲存 NOFF 格式的檔案頭部資訊
    unsigned int size;

    // 若檔案無法打開，輸出錯誤訊息並返回 FALSE
    if (executable == NULL)
    {
        cerr << "Unable to open file " << fileName << "\n";
        return FALSE;
    }

    // 讀取檔案的頭部資訊至 noffH
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);

    // 檢查 NOFFMAGIC 標誌是否正確，若有字節序問題則修正
    if ((noffH.noffMagic != NOFFMAGIC) &&
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH); // 如果字節序不匹配，交換頭部資訊字節序

    // 確保檔案的 NOFFMAGIC 標誌正確
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // 計算整體的記憶體需求，包括代碼段、初始化數據段、未初始化數據段和堆疊空間
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size + UserStackSize;
    // cout << fileName << " size: " << size << endl;
    // cout << "noffH.uninitData.size: " << noffH.uninitData.size << endl;
    // cout << "noffH.initData.size: " << noffH.initData.size << endl;
    // cout << "noffH.code.size: " << noffH.code.size << endl;

    // 計算所需的頁數，並將總大小對齊到頁的邊界
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    if (numPages > NumPhysPages) {
        // 原本在 AddrSpace() 的初始化在這邊重做，以適應不同大小的程式
        delete [] pageTable;
        pageTable = new TranslationEntry[numPages];
        for (unsigned int i = 0; i < numPages; i++)
        {
            pageTable[i].virtualPage = i; // for now, virt page # = phys page #
            pageTable[i].physicalPage = i;
            //	pageTable[i].physicalPage = 0;
            pageTable[i].valid = TRUE;
            // pageTable[i].valid = FALSE;
            pageTable[i].use = FALSE;
            pageTable[i].dirty = FALSE;
            pageTable[i].readOnly = FALSE;
        }
    }
    
    // 確認所需頁數不超過可用的物理頁數（這是暫時的，直到支援虛擬記憶體為止）
    // ASSERT(numPages <= NumPhysPages);

    DEBUG(dbgAddr, "Initializing address space. # of pages = " << numPages << ", " << size << " bytes.");

    DEBUG(dbgAddr, "Loading program into memory...");
    // 為暫存緩衝區分配記憶體，用於儲存代碼和數據段
    char *tempBuffer = new char[noffH.code.size + noffH.initData.size + noffH.uninitData.size];

    // 將代碼段從檔案讀取到緩衝區中的指定位置
    if (noffH.code.size > 0) {
        executable->ReadAt(tempBuffer + noffH.code.virtualAddr, noffH.code.size, noffH.code.inFileAddr);
        DEBUG(dbgAddr, "Code segment is at: " << noffH.code.virtualAddr << " with size: " << noffH.code.size);
    }

    // 將初始化數據段從檔案讀取到緩衝區中的指定位置
    if (noffH.initData.size > 0) {
        DEBUG(dbgAddr, "Initialized data segment is at: " << (unsigned int)noffH.initData.virtualAddr << " with size: " << noffH.initData.size);
        executable->ReadAt(tempBuffer + noffH.initData.virtualAddr, noffH.initData.size, noffH.initData.inFileAddr);
        // cout << (int)tempBuffer << endl;
        // cout << (int)(tempBuffer + noffH.initData.virtualAddr) << endl;
    }

    if (noffH.uninitData.size > 0) {
        DEBUG(dbgAddr, "Uninitialized data segment is at: " << (unsigned int)noffH.uninitData.virtualAddr << " with size: " << noffH.uninitData.size);
        // executable->ReadAt(tempBuffer + noffH.uninitData.virtualAddr, noffH.uninitData.size, noffH.uninitData.inFileAddr);
    }

    unsigned int offset = 0; // 記錄當前加載的偏移量

    // 將每一頁的資料從緩衝區加載到主記憶體或磁碟
    for (unsigned int page = 0; page < numPages; page++)
    {
        // 對於此檔案以page為單位做計算
        int filePageIndex = 0;

        // 找到第一個可用的物理頁框
        while (filePageIndex < NumPhysPages && AddrSpace::usedPhyPage[filePageIndex] == true)
            filePageIndex++;

        if (filePageIndex < NumPhysPages)
        {
            // 若找到可用的物理頁框，將資料從暫存緩衝區加載到主記憶體
            memcpy(&(kernel->machine->mainMemory[filePageIndex * PageSize]), tempBuffer + offset, PageSize);

            AddrSpace::usedPhyPage[filePageIndex] = true;                  // 標記該頁框為已使用
            AddrSpace::usedPhyPageEntry[filePageIndex] = &pageTable[page]; // 記錄對應的頁表項
            pageTable[page].physicalPage = filePageIndex;                  // 設定頁表的物理頁號
            pageTable[page].valid = true;                                 // 標記頁表的有效位
            pageTable[page].diskPage = -1;                                // 表示該頁於磁碟的位置未定
            pageTable[page].lastUsedTime = kernel->stats->totalTicks;     // 設定初始值
        }
        else
        {
            // 若無可用的物理頁框，將頁面寫入交換區（swap disk）
            int diskSector = kernel->synchDisk->numUsedSectors++;
            
            TranslationEntry* tmp = pageTable;
            DEBUG(dbgAddr, "Before WriteSector: pageTable = " << pageTable);
            kernel->synchDisk->WriteSector(diskSector, tempBuffer + offset);
            DEBUG(dbgAddr, "After WriteSector: pageTable = " << pageTable);
            DEBUG(dbgAddr, "Before == After : " << (pageTable == tmp));
            // 暫時解法
            // pageTable = tmp;

            // 這邊會 Segmentation fault
            pageTable[page].diskPage = diskSector; // 紀錄該頁在磁碟的區段編號
            pageTable[page].valid = false;         // 標記頁表的有效位為假
        }

        // 更新偏移量以處理下一頁
        offset += PageSize;

        // 初始化頁表的其他屬性
        pageTable[page].use = false;      // 尚未被使用
        pageTable[page].dirty = false;    // 頁面未被修改
        pageTable[page].readOnly = false; // 頁面可讀寫
    }

    // 釋放暫存緩衝區的記憶體
    delete [] tempBuffer;

    // 關閉檔案以釋放資源
    delete executable;

    return TRUE; // 成功加載檔案
}

//----------------------------------------------------------------------
// AddrSpace::Execute
// 	Run a user program.  Load the executable into memory, then
//	(for now) use our own thread to run it.
//
//	"fileName" is the file containing the object code to load into memory
//----------------------------------------------------------------------

void AddrSpace::Execute(char *fileName)
{
    if (!Load(fileName))
    {
        cout << "inside !Load(FileName)" << endl;
        return; // executable not found
    }

    // kernel->currentThread->space = this;
    this->InitRegisters(); // set the initial register values
    this->RestoreState();  // load page table register

    kernel->machine->Run(); // jump to the user progam

    ASSERTNOTREACHED(); // machine->Run never returns;
                        // the address space exits
                        // by doing the syscall "exit"
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void AddrSpace::InitRegisters()
{
    Machine *machine = kernel->machine;
    int i;

    for (i = 0; i < NumTotalRegs; i++)
        machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we don't
    // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG(dbgAddr, "Initializing stack pointer: " << numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, don't need to save anything!
//----------------------------------------------------------------------

void AddrSpace::SaveState()
{
    // 其實可以全都不用做?
    if (kernel->machine->pageTable == nullptr) {
        kernel->machine->pageTable = pageTable;
        kernel->machine->pageTableSize = numPages;
    }
    else {
        pageTable = kernel->machine->pageTable;
        numPages = kernel->machine->pageTableSize;
    }
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState()
{
    kernel->machine->pageTable = pageTable;
    kernel->machine->pageTableSize = numPages;
}
