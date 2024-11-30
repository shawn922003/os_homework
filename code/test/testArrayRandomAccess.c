#include "syscall.h"

// Initialized global array
int sum = 0; // 16 bytes
// Uninitialized global array
int arr[2048]; // 8192 bytes

main()
{
    // Local variables (in stack)
    int n;
    // int sum = 0;
    
    sum += 1980;
    for (n = 0; n <= sizeof(arr) / sizeof(int); n++)
    {
        // arr[n] = n;
        if (n % 500 == 0)
            sum += n;
    }
    arr[1] = 10;
    PrintInt(sum);
}
