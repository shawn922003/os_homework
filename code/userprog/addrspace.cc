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
    // 開啟要載入的執行檔
    OpenFile *executable = kernel->fileSystem->Open(fileName);
    NoffHeader noffH;  // NoffHeader 用於儲存執行檔的檔頭，包含程式段的資訊
    unsigned int size;

    // 如果無法開啟檔案，則回傳失敗
    if (executable == NULL)
    {
        cerr << "Unable to open file " << fileName << "\n";
        return FALSE;
    }
    
    // 讀取執行檔的檔頭至 noffH 結構中
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    
    // 檢查 noffMagic 欄位以確保該檔案為合法的 Nachos 可執行檔格式
    if ((noffH.noffMagic != NOFFMAGIC) &&
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);  // 如果檔頭的字節順序不對，則調整為當前主機的順序
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // 計算地址空間的大小
    // 計算方式為程式碼區段 + 已初始化資料區段 + 未初始化資料區段 + 用戶堆疊大小
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size + UserStackSize;

    // 將 size 向上取整為頁面數
    numPages = divRoundUp(size, PageSize);
    //	cout << "number of pages of " << fileName<< " is "<<numPages<<endl;
    size = numPages * PageSize;

    numPages = divRoundUp(size, PageSize);

    // 建立頁面表並分配物理頁框給虛擬頁面
    for (unsigned int i = 0, j = 0; i < numPages; i++)
    {
        pageTable[i].virtualPage = i;  // 設定虛擬頁面號
        while (j < NumPhysPages && AddrSpace::usedPhyPage[j] == true) // 找到空閒的物理頁框
            j++;
        AddrSpace::usedPhyPage[j] = true;  // 標記該物理頁框為已使用
        pageTable[i].physicalPage = j;  // 設定物理頁框號
        pageTable[i].valid = true;
        pageTable[i].use = false;
        pageTable[i].dirty = false;
        pageTable[i].readOnly = false;
    }

    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages); // 確認記憶體足夠以避免過大的程式

    DEBUG(dbgAddr, "Initializing address space: " << numPages << ", " << size);

    // 複製程式碼和資料區段到主記憶體
    // 若程式碼區段的大小大於 0，則載入程式碼段
    if (noffH.code.size > 0)
    {
        DEBUG(dbgAddr, "Initializing code segment.");
        DEBUG(dbgAddr, noffH.code.virtualAddr << ", " << noffH.code.size);
        
        // 將程式碼段從檔案載入至對應的物理記憶體位置
        // 1. `pageTable[noffH.code.virtualAddr / PageSize].physicalPage`：
        //    將虛擬位址轉換成對應的頁數（即 `noffH.code.virtualAddr / PageSize`），
        //    再透過頁表找到對應的physical frame number。
        // 2. `pageTable[noffH.code.virtualAddr / PageSize].physicalPage * PageSize`：
        //    取得該物理頁框的起始位址。
        // 3. `(noffH.code.virtualAddr % PageSize)`：
        //    將虛擬位址中的頁內偏移量加入計算，以確保載入到正確的記憶體位址。
        // 4. `noffH.code.size`：
        //    表示要載入的程式碼段的大小。
        // 5. `noffH.code.inFileAddr`：
        //    此為檔案內的偏移量，用於定位程式碼段在執行檔中的位置。
        executable->ReadAt(
            &(kernel->machine->mainMemory[pageTable[noffH.code.virtualAddr / PageSize].physicalPage * PageSize + (noffH.code.virtualAddr % PageSize)]),
            noffH.code.size, noffH.code.inFileAddr);
    }

    // 若已初始化資料區段的大小大於 0，則載入已初始化資料段
    if (noffH.initData.size > 0)
    {
        DEBUG(dbgAddr, "Initializing data segment.");
        DEBUG(dbgAddr, noffH.initData.virtualAddr << ", " << noffH.initData.size);
        
        // 將已初始化資料段從檔案載入至對應的物理記憶體位置
        // 1. `pageTable[noffH.initData.virtualAddr / PageSize].physicalPage`：
        //    將資料段的虛擬位址轉換為對應的頁數，
        //    再透過頁表找到對應的物理頁框號。
        // 2. `pageTable[noffH.initData.virtualAddr / PageSize].physicalPage * PageSize`：
        //    計算該物理頁框的起始位址。
        // 3. `(noffH.initData.virtualAddr % PageSize)`：
        //    取得頁內偏移量，以確保載入到正確的記憶體位址。
        // 4. `noffH.initData.size`：
        //    已初始化資料段的大小，表示要載入的資料量。
        // 5. `noffH.initData.inFileAddr`：
        //    此為檔案內的偏移量，用於定位資料段在執行檔中的位置。
        executable->ReadAt(
            &(kernel->machine->mainMemory[pageTable[noffH.initData.virtualAddr / PageSize].physicalPage * PageSize + (noffH.initData.virtualAddr % PageSize)]),
            noffH.initData.size, noffH.initData.inFileAddr);
    }


    delete executable; // 關閉檔案
    return TRUE;       // 成功載入
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
    pageTable = kernel->machine->pageTable;
    numPages = kernel->machine->pageTableSize;
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
