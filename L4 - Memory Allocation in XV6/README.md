# CS 1550 Lab 4 - Lazy Memory Allocation For Xv6

One of the many neat tricks an OS can play with page table hardware is the lazy allocation of heap memory. Xv6 applications ask the kernel for heap memory using the `sbrk()` system call, which allocates physical memory and maps it into the process's virtual address space. Some programs allocate memory but never use it, for example, to implement large sparse arrays. Sophisticated kernels delay the allocation of each page of heap memory until the application tries to use that page -- as signaled by a page fault. You'll add this lazy allocation feature to Xv6 in this lab.

## PART 0: Setting Up the XV6 Development Environment (Same as in previous labs)

We will use the Thoth server (thoth.cs.pitt.edu) for this lab. You may also use the CS Department's Linux cluster (linux.cs.pitt.edu), Pitt's Linux cluster (linux-ts.it.pitt.edu), or the Docker Images provided on Canvas as backup in case you have a problem accessing Thoth. You can connect to Thoth without having to connect to Pitt VPN. However, to access the Linux clusters, you must connect to [Pitt VPN](https://www.technology.pitt.edu/services/pittnet-vpn-pulse-secure). 

1. These instructions will help you clone the Xv6 code of the lab from your GitHub Classroom private repository into your home folder on Thoth or the Linux clusters.
2. If your account has less than 500 MB of AFS free space, you can increase your disk quota from the [Accounts Self-Service](https://accounts.pitt.edu/Unix/) page or from the [Pitt IT Help Desk](https://www.technology.pitt.edu/help-desk/247-it-help-desk).
3. We recommend using VS Code to connect to the servers. You can find instructions on how to do that in the Ed discussion board for the class. Log in to the server using your Pitt account. From a UNIX box, you can type: ssh ksm73@thoth.cs.pitt.edu, assuming ksm73 is your Pitt ID. From Windows, you may use PuTTY or the PowerShell ssh client.
4. Ensure you are in a private directory (e.g., your private folder in your AFS space).
5. From the server's command prompt, run the following command to download the Xv6 source code for the lab: 
`git clone https://github.com/cs1550-2244/cs1550-lab4-USERNAME.git`, where USERNAME is your GitHub username used when accepting the Github Classroom assignment.
6. If prompted, enter your GitHub username and [your GitHub personal access token](https://docs.github.com/en/github/authenticating-to-github/creating-a-personal-access-token). By [integrating GitHub with VS Code](https://code.visualstudio.com/docs/sourcecontrol/github), you may be able to skip the need to generate and use an access token.
7. Change the working directory into the xv6 folder using `cd cs1550-lab2-USERNAME`, where again USERNAME is your GitHub username, and `cd xv6`. Then, type `make qemu-nox` to compile and run Xv6. To exit, press `CTRL+a` _then_ `x`.
8. **It is essential for all labs and projects of this class that you commit frequently into your GitHub Classroom repository.**

##	PART 1: ELIMINATE ALLOCATION FROM SBRK()

Your first task is to delete page allocation from the `sbrk(n)` system call implementation, function `sys_sbrk()` in `sysproc.c`. The `sbrk(n)` system call grows (or shrinks) the process's heap by `n` bytes, and then returns the start of the newly allocated region (i.e., the old size). Your new `sbrk(n)` should increment the process's size (`proc->sz`) by `n` and return the old size. It should not allocate memory -- so you should delete the call to `growproc()`.

_Try to guess what the result of this modification will be: what will break?_

Make this modification, boot Xv6, and type `echo hi` to the shell. You should see something like this:

```
init: starting sh
$ echo hi
pid 3 sh: trap 14 err 6 on cpu 0 eip 0x12f1 addr 0x4004--kill proc
$ 
```

The `"pid 3 sh: trap..."` message is from the kernel trap handler in `trap.c`; it has caught a page fault (trap 14, or `T_PGFLT`), which the Xv6 kernel does not know how to handle. Make sure you understand why this page fault occurs. The `"addr 0x4004"` indicates that the virtual address that caused the page fault is `0x4004`.

##	PART 2: LAZY ALLOCATION

Modify the code in `trap.c` to respond to a page fault from user space by mapping a newly allocated page of physical memory at the faulting address and then returning back to user space to let the process continue executing. You should add your code before the `cprintf` call that produced the `"pid 3 sh: trap 14"` message, preferably as another case of the switch statement. Your code is not required to cover all corner cases and error situations; it just needs to be good enough to let `sh` run simple commands like echo and ls.

**Hint**: look at the `cprintf` arguments to see how to find the virtual address that caused the page fault.

**Hint**: steal code from `allocuvm()` in `vm.c`, which is what `sbrk()` calls (via `growproc()`).

**Hint**: use `PGROUNDDOWN(va)` to round the faulting virtual address down to a page boundary.

**Hint**: `break` or `return` to avoid falling down to `cprintf` and `proc->killed = 1`.

**Hint**: you'll need to call `mappages()`. To do this, you'll need to delete the `static` keyword in the declaration of `mappages()` in `vm.c`, and you'll need to declare `mappages()` in `trap.c`. Add the following declaration to `trap.c` before any call to `mappages()`:
```c
      int mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm);
```

**Hint**: you can check whether a fault is a page fault by checking if `tf->trapno` is equal to `T_PGFLT` in `trap()`.

If all goes well, your lazy allocation code should result in `echo hi` working correctly and printing g`hi`. You should get at least one page-fault (and thus lazy allocation) in the shell, and perhaps two.

By the way, this is not an entirely correct implementation. For a list of problems, see the challenges below. You don't need to worry about these for this lab.

Challenges: 

-	Handle negative `sbrk()` arguments. 
-	Handle error cases such as `sbrk()` arguments that are too large. 
-	Verify that `fork()` and `exit()` work even if some `sbrk()`'d addresses have no memory allocated for them. 
-	Correctly handle faults on the invalid page below the stack. 
-	Make sure that kernel use of not-yet-allocated user addresses works -- for example, if a program passes an `sbrk()`-allocated address to `read()`.

## Debugging 

Here is a way to run `gdb` on Xv6:
1.	Start Xv6 with the following command:
				`make qemu-nox-gdb`
2.	Note the name of the server to which you are connected. You can find the name in the server prompt. Examples of server names are thoth, thompson, kernighan, and ritchie. (Do you recognize the relationship between some of these names and the Linux operating system?)
3.	Open a separate SSH connection (e.g., using a new terminal (with split screen) in VS Code, another PuTTY session, or another terminal) into the server name that you got in the previous step (e.g., thoth.cs.pitt.edu).
4.	Change the directory into the folder that has your Xv6 code.
5.	Run gdb using the command:
				`gdb -iex "set auto-load safe-path ."`
6.	Feel free to use your GDB skills. For example, you can place a breakpoint at a given line in the Xv6 source code:
				`(gdb) b proc.c:215`
7.	Then continue running the kernel using the command
				`(gdb) c`
8.	To debug a user program (e.g., echo), you can add the symbol table inside the user program to the GDB session as follows (note the underscore in front of the program name)
`(gdb) add-symbol-file _echo 0`
9.	You would then be able to add breakpoints in the user program: 
`(gdb) b echo.c:7`
10.	Using the `next` or `n` command in the GDB prompt, you can trace inside kernel code and out to the user code.
11.	Here is an excellent page about [gdb commands](https://visualgdb.com/gdbreference/commands/).

## SUBMISSION INSTRUCTIONS

Please submit your GitHub Classroom repository to GradeScope. You should modify the following files only:
-	`sysproc.c`
-	`trap.c`
-	`vm.c`

Your submission will be graded by compiling and running it by the autograder.

