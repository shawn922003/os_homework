#include "syscall.h"

// 75 pages
// stack size = 1024
// uninitialized data segment size = 8192
// initialized data segment size = 16
// code segment size = 352

// Initialized global array
int sum = 0; // 16 bytes
// Uninitialized global array
int arr[2048]; // 8192 bytes

main()
{
    // Local variables (in stack)
    int n;
    // int sum = 0;

    for (n = 0; n <= sizeof(arr) / sizeof(int); n++)
    {
        arr[n] = n;
        sum += arr[n];
        // PrintInt(n);
    }
}
