// translate.cc
//	Routines to translate virtual addresses to physical addresses.
//	Software sets up a table of legal translations.  We look up
//	in the table on every memory reference to find the true physical
//	memory location.
//
// Two types of translation are supported here.
//
//	Linear page table -- the virtual page # is used as an index
//	into the table, to find the physical page #.
//
//	Translation lookaside buffer -- associative lookup in the table
//	to find an entry with the same virtual page #.  If found,
//	this entry is used for the translation.
//	If not, it traps to software with an exception.
//
//	In practice, the TLB is much smaller than the amount of physical
//	memory (16 entries is common on a machine that has 1000's of
//	pages).  Thus, there must also be a backup translation scheme
//	(such as page tables), but the hardware doesn't need to know
//	anything at all about that.
//
//	Note that the contents of the TLB are specific to an address space.
//	If the address space changes, so does the contents of the TLB!
//
// DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"

// Routines for converting Words and Short Words to and from the
// simulated machine's format of little endian.  These end up
// being NOPs when the host machine is also little endian (DEC and Intel).

unsigned int
WordToHost(unsigned int word)
{
#ifdef HOST_IS_BIG_ENDIAN
    register unsigned long result;
    result = (word >> 24) & 0x000000ff;
    result |= (word >> 8) & 0x0000ff00;
    result |= (word << 8) & 0x00ff0000;
    result |= (word << 24) & 0xff000000;
    return result;
#else
    return word;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned short
ShortToHost(unsigned short shortword)
{
#ifdef HOST_IS_BIG_ENDIAN
    register unsigned short result;
    result = (shortword << 8) & 0xff00;
    result |= (shortword >> 8) & 0x00ff;
    return result;
#else
    return shortword;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned int
WordToMachine(unsigned int word) { return WordToHost(word); }

unsigned short
ShortToMachine(unsigned short shortword) { return ShortToHost(shortword); }

//----------------------------------------------------------------------
// Machine::ReadMem
//      Read "size" (1, 2, or 4) bytes of virtual memory at "addr" into
//	the location pointed to by "value".
//
//   	Returns FALSE if the translation step from virtual to physical memory
//   	failed.
//
//	"addr" -- the virtual address to read from
//	"size" -- the number of bytes to read (1, 2, or 4)
//	"value" -- the place to write the result
//----------------------------------------------------------------------

bool Machine::ReadMem(int addr, int size, int *value)
{
    int data;
    ExceptionType exception;
    int physicalAddress;

    DEBUG(dbgAddr, "Reading VA " << addr << ", size " << size);

    exception = Translate(addr, &physicalAddress, size, FALSE);
    if (exception != NoException)
    {
        RaiseException(exception, addr);
        return FALSE;
    }
    switch (size)
    {
    case 1:
        data = mainMemory[physicalAddress];
        *value = data;
        break;

    case 2:
        data = *(unsigned short *)&mainMemory[physicalAddress];
        *value = ShortToHost(data);
        break;

    case 4:
        data = *(unsigned int *)&mainMemory[physicalAddress];
        *value = WordToHost(data);
        break;

    default:
        ASSERT(FALSE);
    }

    DEBUG(dbgAddr, "\tvalue read = " << *value);
    return (TRUE);
}

//----------------------------------------------------------------------
// Machine::WriteMem
//      Write "size" (1, 2, or 4) bytes of the contents of "value" into
//	virtual memory at location "addr".
//
//   	Returns FALSE if the translation step from virtual to physical memory
//   	failed.
//
//	"addr" -- the virtual address to write to
//	"size" -- the number of bytes to be written (1, 2, or 4)
//	"value" -- the data to be written
//----------------------------------------------------------------------

bool Machine::WriteMem(int addr, int size, int value)
{
    ExceptionType exception;
    int physicalAddress;

    DEBUG(dbgAddr, "Writing VA " << addr << ", size " << size << ", value " << value);

    exception = Translate(addr, &physicalAddress, size, TRUE);
    if (exception != NoException)
    {
        RaiseException(exception, addr);
        return FALSE;
    }
    switch (size)
    {
    case 1:
        mainMemory[physicalAddress] = (unsigned char)(value & 0xff);
        break;

    case 2:
        *(unsigned short *)&mainMemory[physicalAddress] = ShortToMachine((unsigned short)(value & 0xffff));
        break;

    case 4:
        *(unsigned int *)&mainMemory[physicalAddress] = WordToMachine((unsigned int)value);
        break;

    default:
        ASSERT(FALSE);
    }

    return TRUE;
}

//----------------------------------------------------------------------
// Machine::Translate
// 	Translate a virtual address into a physical address, using
//	either a page table or a TLB.  Check for alignment and all sorts
//	of other errors, and if everything is ok, set the use/dirty bits in
//	the translation table entry, and store the translated physical
//	address in "physAddr".  If there was an error, returns the type
//	of the exception.
//
//	"virtAddr" -- the virtual address to translate
//	"physAddr" -- the place to store the physical address
//	"size" -- the amount of memory being read or written
// 	"writing" -- if TRUE, check the "read-only" bit in the TLB
//----------------------------------------------------------------------

// Machine::Translate 函數
// 此函數將虛擬位址轉換為物理位址，並處理不同的例外狀況
// 輸入參數：
// - int virtAddr：虛擬位址
// - int *physAddr：轉換後的物理位址指標
// - int size：存取的大小（1、2 或 4 bytes）
// - bool writing：是否為寫操作
ExceptionType Machine::Translate(int virtAddr, int *physAddr, int size, bool writing)
{
    int i;
    unsigned int vpn, offset;
    TranslationEntry *entry;
    unsigned int pageFrame;

    // 記錄此次轉換的操作類型（讀取或寫入）
    DEBUG(dbgAddr, "\tTranslate " << virtAddr << (writing ? " , write" : " , read"));

    // 檢查位址對齊錯誤
    // 如果 size 是 4 bytes，虛擬位址應以 4-byte 對齊（最後兩位必須是 00），
    // 如果 size 是 2 bytes，虛擬位址應以 2-byte 對齊（最後一位必須是 0）。
    if (((size == 4) && (virtAddr & 0x3)) || ((size == 2) && (virtAddr & 0x1)))
    {
        DEBUG(dbgAddr, "Alignment problem at " << virtAddr << ", size " << size);
        return AddressErrorException;  // 返回位址錯誤例外
    }

    // 系統必須有 TLB 或 Page Table，但不能同時擁有兩者。
    ASSERT(tlb == NULL || pageTable == NULL);  // 若條件不成立會觸發 ASSERT
    ASSERT(tlb != NULL || pageTable != NULL);  // 必須至少有 TLB 或 Page Table 其中之一

    // 計算虛擬頁面號 (vpn) 和頁內偏移量 (offset)
    // - `vpn`：虛擬頁面號，由虛擬位址除以 PageSize 得到
    // - `offset`：頁內偏移量，虛擬位址對 PageSize 取餘數
    vpn = (unsigned)virtAddr / PageSize;
    offset = (unsigned)virtAddr % PageSize;

    // 判斷是使用 TLB 還是 Page Table 來進行轉換
    if (tlb == NULL)
    { // 使用 Page Table，vpn 作為索引
        // 檢查 vpn 是否超出 Page Table 的大小
        if (vpn >= pageTableSize)
        {
            DEBUG(dbgAddr, "Illegal virtual page # " << virtAddr);
            return AddressErrorException;  // 返回位址錯誤例外
        }
        else if (!pageTable[vpn].valid)  // 檢查頁面是否有效
        {
            std::cout << "PageFaultException" << std::endl;
            // return PageFaultException;
            /* 		Add Page fault code here		*/
            // 若頁面無效，則表示發生頁面錯誤（未在此處實作處理）
        }
        entry = &pageTable[vpn];  // 取得 Page Table 中該虛擬頁面的翻譯項目
    }
    else
    { // 使用 TLB 進行查詢
        // 遍歷 TLB，尋找對應的虛擬頁面
        for (entry = NULL, i = 0; i < TLBSize; i++)
        {
            if (tlb[i].valid && (tlb[i].virtualPage == vpn))  // 若找到有效且匹配的頁面
            {
                entry = &tlb[i];  // 找到對應的翻譯項目
                break;
            }
        }
        // 如果在 TLB 中未找到對應的頁面
        if (entry == NULL)
        {
            DEBUG(dbgAddr, "Invalid TLB entry for this virtual page!");
            return PageFaultException;  // 返回頁面錯誤例外（實際上為 TLB 錯誤）
                                        // 頁面可能在記憶體中，但不在 TLB 中
        }
    }

    // 檢查頁面的讀寫權限
    if (entry->readOnly && writing)  // 如果頁面為唯讀，但進行的是寫操作
    {
        DEBUG(dbgAddr, "Write to read-only page at " << virtAddr);
        return ReadOnlyException;  // 返回唯讀例外
    }
    pageFrame = entry->physicalPage;  // 取得頁框號

    // 檢查頁框號是否合法（頁框號不應超出物理頁數限制）
    if (pageFrame >= NumPhysPages)
    {
        DEBUG(dbgAddr, "Illegal pageframe " << pageFrame);
        return BusErrorException;  // 返回總線錯誤例外
    }

    // 更新使用位與修改位
    entry->use = TRUE;  // 設置該頁面的使用位為 TRUE，表示該頁面被存取
    if (writing)        // 若為寫操作
        entry->dirty = TRUE;  // 設置修改位為 TRUE，表示該頁面內容已被修改

    // 計算物理位址
    *physAddr = pageFrame * PageSize + offset;  // 物理位址為頁框號乘以頁面大小再加上頁內偏移量

    // 驗證物理位址是否在合法範圍內
    ASSERT((*physAddr >= 0) && ((*physAddr + size) <= MemorySize));
    DEBUG(dbgAddr, "phys addr = " << *physAddr);  // 輸出物理位址的除錯訊息

    return NoException;  // 成功完成轉換，返回 NoException
}
