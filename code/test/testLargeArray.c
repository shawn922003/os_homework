#include "syscall.h"

// 44 pages
// stack size = 1024
// uninitialized data segment size = 4096
// initialized data segment size = 16
// code segment size = 432
// total size = 432 + 16 + 4096 + 1024 = 5568
// total pages = 5568 / 128 = 43.5 => 44

// Initialized global variable
int sum = 0;
// Uninitialized global array
int arr[1024]; // 4096 bytes

main()
{
    // Local variables (in stack)
    int n;

    for (n = 0; n < sizeof(arr) / sizeof(int); n++)
    {
        arr[n] = n;
        sum += arr[n];
    }
    PrintInt(sum);
}
