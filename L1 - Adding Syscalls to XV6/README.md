# CS 1550 Lab 1
## PART 1: Setting Up the Xv6 Development Environment

We will use the Thoth server (thoth.cs.pitt.edu), the CS Department's Linux cluster (linux.cs.pitt.edu), or Pitt's Linux cluster (linux-ts.it.pitt.edu) as backup in case you have a problem accessing Thoth. 
You can connect to Thoth without having to connect to Pitt VPN. However, to access the Linux clusters, you must connect to [Pitt VPN](https://www.technology.pitt.edu/services/pittnet-vpn-pulse-secure). 

1. These instructions will help you clone the Xv6 code of the lab from your GitHub Classroom private repository into your home folder on Thoth or the Linux cluster.
2. Whenever the amount of AFS free space in your account is less than 500MB, you can increase your disk quota from the [Accounts Self-Service](https://accounts.pitt.edu/Unix/) page.
3. Log in to the server using your Pitt account. From a UNIX box, you can type: ssh ksm73@thoth.cs.pitt.edu, assuming ksm73 is your Pitt ID. From Windows, you may use PuTTY or the PowerShell ssh client. We recommend using VS Code to connect to the servers. You can find instructions on how to do that in the discussion board for the class.
4. Ensure you are in a private directory (your private folder in your AFS space (preferred) or /u/PITT_ID folder on Thoth).
5. From the server's command prompt, run the following command to download the Xv6 source code for the lab: 
`git clone https://github.com/cs1550-2244/cs1550-lab1-USERNAME.git`, where USERNAME is the GitHub username you used to accept the GitHub Classroom assignment.
6. [Generate and use personal access tokens](https://docs.github.com/en/github/authenticating-to-github/creating-a-personal-access-token) instead of passwords. By [integrating GitHub with VS Code](https://code.visualstudio.com/docs/sourcecontrol/github), you may be able to skip the need to generate and use an access token.
7. Change the directory into the `xv6` folder using `cd cs1550-lab1-USERNAME` where again USERNAME is your GitHub username and `cd xv6`, then type `make qemu-nox` to compile and run Xv6. To exit, press `CTRL+a` _then_ `x`.
8. **It is vital for all labs and projects of this class that you frequently commit your code into your GitHub Classroom repository.**

## PART 2: Adding A System Call to Xv6

You'll then add a new system call called `getcount` to Xv6, which, when passed a valid system call number (listed in the file `syscall.h`) as an argument, will return the number of times the referenced system call was invoked by the calling process. For instance, consider the following test program (`getcount.c`) and its expected output (note that each character printed using `printf` results in a separate call to the `write` syscall):
```c
#include "types.h"
#include "user.h"
#include "syscall.h"
int  main(int argc, char *argv[])
{
    printf(1, "initial fork count %d\n", getcount(SYS_fork));
    if (fork() == 0) {
        printf(1, "child fork count %d\n", getcount(SYS_fork));
        printf(1, "child write count %d\n", getcount(SYS_write));
    } else {
        wait();
        printf(1, "parent fork count %d\n", getcount(SYS_fork));
        printf(1, "parent write count %d\n", getcount(SYS_write));
    }
    printf(1, "wait count %d\n", getcount(SYS_wait));
    exit();
}
```

Output:
```
initial fork count 0
child fork count 0
child write count 19
wait count 0
parent fork count 1
parent write count 41
wait count 1
```

## Hints
You will need to modify several files for this lab, but the number of lines of code you'll add is relatively tiny. At a minimum, you'll need to alter `syscall.h`, `syscall.c`, `user.h`, and `usys.S` to implement the new system call.  It may be helpful to trace how some other system call is implemented (e.g., `uptime`) for clues. 
You may want to update `struct proc`, located in `proc.h`, to add a data structure (e.g., an array of `int`s) to track the number of invocations for each syscall by the process. Since entries in the process table are reused, you need to reset your data structure when a process terminates; you may want to add code to reset the array contents to zeros in the `exit` or `fork` functions in `proc.c`.
Chapter 3 of the [Xv6 book](https://pdos.csail.mit.edu/6.828/2012/xv6/book-rev7.pdf) contains details on traps and system calls (though most of the low-level details won't be necessary for you to complete this lab).

## Testing

Because the program depends on your implementation, it isn't compiled into Xv6 by default. When you're ready to test, create the `getcount.c` file and add `getcount` to `UPROGS` declaration in the `Makefile`. To test your code, run the `getcount` executable, which is based on the program above, after booting into Xv6. 
Note that while the getcount example only prints out counts for three different system calls, your implementation should support all the system calls listed in `syscall.h`. It is an excellent exercise to add tests to the test program, located in `getcount.c`, for other system calls.

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
8.	To debug a user program (e.g., getcount), you can add the symbol table inside the user program to the GDB session as follows (note the underscore in front of the program name)
`(gdb) add-symbol-file _getcount 0`
9.	You would then be able to add breakpoints in the user program: 
`(gdb) b getcount.c:7`
10.	Using the `next` or `n` command in the GDB prompt, you can trace inside kernel code and out to the user code.
11.	Here is an excellent page about [gdb commands](https://visualgdb.com/gdbreference/commands/).

## Lab Submission Instructions

Submit your Github repository to GradeScope. You should modify the following files **only**:
- `syscall.h`
- `syscall.c`
- `user.h`
- `usys.S`
- `proc.h`
- `proc.c`
- `sysproc.c`
- `Makefile`

Your submission will be auto-graded by compiling and running the test program more than once. 

