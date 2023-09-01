# NachOS 4.0

**Now Compatible with 64-bit Linux Machines**

This is a modified version of NachOS (Original NachOS available at https://homes.cs.washington.edu/~tom/nachos/) which has undergone various patches to make it compatible with modern Linux machines.

The original readme file has been relocated to [README.Original.md](README.Original.md)

In this project, only the `Halt()` system call from the original NachOS and our sample `PrintInt()` system call have been implemented. Attempting to use other unimplemented system calls will result in a core dump.

As of now, this project has been tested and confirmed to work on the latest LTS version of Ubuntu Linux (22.04)

*Please note that the non-LTS versions of Ubuntu (or Debian-based) have not been tested but are expected to work. However, we do not provide support for non Ubuntu LTS versions.*

# Table of Contents
- [Prerequisite](#Prerequisite)
  - [For 64-bit Ubuntu Linux](#For-64-bit-Ubuntu-Linux)
  - [For 32-bit Ubuntu Linux](#For-32-bit-Ubuntu-Linux)
  - [Download Project](#Download-Project)
  - [Install Cross-Compiler for NachOS](#Install-Cross-Compiler-for-NachOS)
- [Building NachOS](#Building-NachOS)
- [Testing NachOS](#Testing-NachOS)
- [Make Usage](#Make-Usage)
- [NachOS Usage](#NachOS-Usage)
- [Debug Flag](#Debug-Flag)
- [Adding User Program](#Adding-User-Program)
- [References](#References)

# Prerequisite

## For 64-bit Ubuntu Linux

Enable i386 architecture first

```shell
$ sudo dpkg --add-architectures i386 
```

Preform system upgrade

```shell
$ sudo apt update; sudo apt dist-upgrade
```

Install the dependency (csh/git/compiler)

```shell
$ sudo apt install csh git build-essential gcc-multilib g++multilib gdb gdb-multiarch
```

## For 32-bit Ubuntu Linux

Preform system upgrade

```shell
$ sudo apt update; sudo apt dist-upgrade
```

Install the dependency (csh/git/compiler)

```shell
$ sudo apt install csh git build-essential gdb
```

## Download Project

Clone the project repository

```shell
$ git clone https://github.com/wynn1212/NachOS
```

## Install Cross-Compiler for NachOS

Navigate to the `NachOS` directory, then copy the cross-compiler to the system directory

```shell
$ cd NachOS
NachOS$ sudo cp -r usr /
```

# Building NachOS

Navigate to the `code` directory in `NachOS`, then run `make` to build NachOS

```shell
NachOS$ cd code
NachOS/code$ make
```

**Note 1:** Whenever you make modifications to the source code, it is essential to rebuild the entire NachOS system.

**Note 2:** If your modifications are limited to the test directory, you can simply execute `make` within the test directory to build your program. However, if you have made changes to system calls or other core components, it is still necessary to rebuild the complete NachOS system.

# Testing NachOS

After a successful build, you should find the `nachos` executable in the `NachOS/code/userprog/` directory and your `test1` program in the `NachOS/code/test/` directory.

To run `test1` in NachOS, execute the following command:

```shell
NachOS/code$ ./userprog/nachos -e ./test/test1
```

*Please note that you should be inside the `NachOS/code` directory. Otherwise, you should specify the actual location accordingly.*

If you see this output, it indicates that you have successfully run `test1` in NachOS.

```shell
NachOS/code$ ./userprog/nachos -e ./test/test1
Total threads number is 1
Thread ./test/test1 is executing.
Print integer:9
Print integer:8
Print integer:7
Print integer:6
return value:0
No threads ready or runnable, and no pending interrupts.
Assuming the program completed.
Machine halting!

Ticks: total 200, idle 66, system 40, user 94
Disk I/O: reads 0, writes 0
Console I/O: reads 0, writes 0
Paging: faults 0
Network I/O: packets received 0, sent 0
NachOS/code$
```

# Make Usage

- `make`: This command builds the entire NachOS system and user programs in the `test` directory.
- `make clean`: This command cleans up NachOS build files and executables, while keeping the executable user programs in the `test` directory.
- `make distclean`: This command cleans up NachOS build files and executables, and also removes the executable user programs in the `test` directory.
- `make print`: This command prints the source code and Makefile of NachOS to the printer. (deprecated, preserved for historical purposes)

If you're in the `test` directory:

- `make`: This command builds the user programs in the current directory.

# NachOS Usage

- `./nachos [-d debugFlags]`: Causes certain debugging messages to be printed, where legal `debugFlags` are
  - `+`: turn on all debug messages
  - `t`: threads
  - `s`: locks, semaphores, condition vars
  - `i`: interrupt emulation
  - `m`: machine emulation (USER_PROGRAM)
  - `d`: disk emulation (FILESYS)
  - `f`: file system (FILESYS)
  - `a`: address spaces (USER_PROGRAM)
  - `n`: network emulation (NETWORK)
    - E.G. `./nachos -d +`: will turn on all debug messages
- `./nachos [-e] filename`: Execute user program in `filename`
  - E.G. `./nachos -e file1 -e file2`: executing file1 and file2.
- `./nachos [-h]`: Prints help message
- `./nachos [-m int]`: Sets this machine's host id in `int` (needed for the network)
  - E.G. `./nachos -m 1`: Sets this machine's host id to 1
- `./nachos [-n float]`: Sets the network reliability in `float`
  - E.G. `./nachos -n 1`: Sets the network reliability to 1
- `./nachos [-rs randomSeed]`: Sets random seed in `randomSeed`
  - E.G. `./nachos -rs 123`: Sets random seed to 123
- `./nachos [-s]`: Print machine status during the machine is on. (`debugUserProg = TRUE` in `userprog/userkernel.cc` )
- `./nachos [-u]`: Prints entire set of legal flags
- `./nachos [-z]`: Prints copyright string

# Debug Flag

In `NachOS/code/lib/debug.h` There is pre-defined debugging flags
- debug.h:20

```cpp
// The pre-defined debugging flags are:

const char dbgAll = '+';        // turn on all debug messages
const char dbgThread = 't';        // threads
const char dbgSynch = 's';        // locks, semaphores, condition vars
const char dbgInt = 'i';         // interrupt emulation
const char dbgMach = 'm';         // machine emulation (USER_PROGRAM)
const char dbgDisk = 'd';         // disk emulation (FILESYS)
const char dbgFile = 'f';         // file system (FILESYS)
const char dbgAddr = 'a';         // address spaces (USER_PROGRAM)
const char dbgNet = 'n';         // network emulation (NETWORK)
```

For example in `NachOS/code/threads/main.cc`
- main.cc:76

```cpp
DEBUG(dbgThread, "Entering main");
```

If you run either `./nachos -d +` or `./nachos -d t`, you will see the `Entering main` debug message displayed during the execution of NachOS.

```shell
NachOS/code$ ./userprog/nachos -d t
Entering main
Total threads number is 0
...(skipped)...
NachOS/code$ ./userprog/nachos -d +
Entering main
Scheduling interrupt handler the timer at time = 100
...(skipped)...
NachOS/code$
```

# Adding User Program

To add your program to NachOS, follow these steps:

1. Place your source code in the `NachOS/code/test` directory.
2. Add a line in the `NachOS/code/test/Makefile` file to specify your program for compilation.

To illustrate the process of adding, for example, `test1` to NachOS, follow these steps:

1. Place `test1.c` source file into `NachOS/code/test/` directory 
2. Modify the `NachOS/code/test/Makefile` file to add a `test1` make target, as shown below:
  - Makefile:67
  
  ```makefile
  test1: test1.o start.o
      $(LD) $(LDFLAGS) start.o test1.o -o test1.coff
      ../bin/coff2noff test1.coff test1
  ```
  
3. Ensure that you add the `test1` make target at the end of the `all` make target, as shown below:
  - Makefile:36
  
  ```makefile
  all: halt shell matmult sort test1
  ```
  
This process will allow you to include and build `test1` in NachOS.

# References

- https://homes.cs.washington.edu/~tom/nachos/ - Original NachOS 4.0 source code and documents.
- [GitHub - connlabtw/NachOS2020](https://github.com/connlabtw/NachOS2020) - This repository originated as a fork and includes the `PrintInt()` system call implementation. 
- https://blog.csdn.net/Aloneingchild/article/details/115339992 - A guide that provides a solution for building NachOS on 64-bit Linux systems.
- [GitHub - Yan-Hau/NachOS](https://github.com/Yan-Hau/NachOS/tree/master#readme) - Another solution for building NachOS on 64-bit Linux systems. This repository includes information about the required dependency packages for NachOS.
  - If you want to run NachOS on Docker or WSL2, you can follow the provided guide.
  - If you're interested in learning how to use gdb (GNU Debugger) with Visual Studio Code for NachOS development, there's also a guide available.
  - **Please be aware that this repository uses NachOS 4.1, which has a different source code structure compared to our NachOS 4.0. Additionally, it does not have the `PrintInt()` system call implemented, which means you'll need to implement the `PrintInt()` system call yourself if you choose to use this version.**
