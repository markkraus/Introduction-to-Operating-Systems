Project 2: Synchronization
==========================

To submit your project code, submit your GitHub Classroom repository to Gradescope. **Only the following files will be used for grading**:

- `museumsim.c`
- `p2-writeup.md` (the writeup in [GitHub Markdown](https://guides.github.com/features/mastering-markdown/))

Don't forget to name your writeup `p2-writeup.md`.

- Due date: Friday 3/22/2024 at 11:59 pm EST
- Late due date: Sunday 3/24/2024 at 11:59 pm EST

---


Project Overview
----------------

Imagine that the **CS1550 museum** has been established to showcase old operating systems from all over the history of computing. Imagine also that several CS1550 students have volunteered to work as tour guides to answer the questions of the museum visitors and help them run the showcased operating systems.

The museum is hosted in a room that can hold up to 10 visitors and 1 tour guide at a time. To not overwhelm the volunteer tour guides, who already have lots of work to do in the CS1550 class, each volunteer guide is limited to supervising 10 visitors daily, and leaves afterwards. A visitor arrives, waits (if necessary) to tour the museum, tours the museum, and leaves. A tour guide arrives, waits (if necessary) to enter the museum, enters the museum, waits until allowed to leave, and leaves. 

Synchronization
---------------

The CS1550 museum involves multiple independently acting entities &ndash; namely, the visitors and the tour guides. We will use our thread synchronization skills to simulate the operation of the CS1550 museum. In particular, you will use mutex locks, condition variables, and (optionally) barriers to simulate the **safe museum tour problem**, whereby visitors and guides are modeled as threads that need to synchronize in such a way that all of the following constraints are met:

- Before entering the museum, each visitor must wait until a guide enters.
- Before entering the museum, each guide must wait until ten visitors arrive and the museum is empty.
- The guide inside the museum cannot leave until all visitors inside the museum tour and leave.
- Visitors inside the museum should take a tour *concurrently*, not sequentially.

To enforce the above conditions, you will use variables shared between the visitor and guide **threads**. Please remember from Project 1 that any time we share data between two or more processes or threads, we risk having race conditions. In race conditions, the shared data could become corrupted. We have discussed various mechanisms (e.g., Mutex lock) to ensure critical regions are guarded to avoid race conditions.

Your job is to write a multi-threaded program that:

- always satisfies the above constraints
- does NOT use busy waiting
- does NOT cause race conditions
- does NOT cause a deadlock

In deadlocks, the threads end up waiting indefinitely on each other. A deadlock happens, for example, when a guide and ten visitors arrive at an empty museum, and they wait outside forever.

To implement this project, you cannot use busy waiting or timed `sleep()`. This means spinlocks and looping until a memory value changes are **not allowed**, and any `sleep()` call is **not allowed**. Instead, you must use the POSIX threading primitives discussed below and explained in [this document](README.libpthread.md).


POSIX threading API
-------------------

The POSIX threading library (`libpthread`) allows implementing multi-threaded applications and synchronization primitives on various platforms, such as Windows (via `winpthread`), Linux and MacOS. A discussion on how to use its various features follows. Depending on how you implement your project, you may use any or all of the allowed features, but not all will be required to produce a working implementation.

Please look at the [README for POSIX threads](README.libpthread.md) for information on the functions you may use.


Project details
---------------

You are to write C code for four functions:
- `museum_init`
- `museum_destroy`
- `visitor`
- `guide`

### Constants

Some constants you will use while implementing the program are defined as macros and will be referenced using these names in the following code.

```c
#define VISITORS_PER_GUIDE 10
#define GUIDES_ALLOWED_INSIDE 1
```

### Shared data

Because we need variables to be shared across multiple threads, we must have a safe place to put the synchronization constructs (e.g., Mutexes and Condition Variables) and the variables. This safe place is provided for you in a global structure accessible from any thread.

```c
struct shared_data {
	// Add any relevant synchronization constructs and shared state here.
	// For example:
	//     pthread_mutex_t mutex;
}

static struct shared_data shared;
```


### `museum_init` and `museum_destroy`

To set up and tear down the shared variables and synchronization constructs for your implementation, `museum_init` will be called before any threads are created, and `museum_destroy` will be called after all threads are done executing.

In `museum_init`, you must initialize the synchronization constructs in your shared data section. As an example:

```c
void museum_init(int num_guides, int num_visitors)
{
	pthread_mutex_init(&shared.mutex, NULL);

}
```

In `museum_destroy`, you will do the reverse, destroying all the synchronization constructs you made, and potentially freeing any memory you have allocated (e.g., using `malloc()`).
```c
void museum_destroy()
{
	pthread_mutex_destroy(&shared.mutex);
}
```

### `visitor`

In `visitor`, you will implement the visitor arrival, touring, and leaving sequence.

```c
void visitor(int id)
{
}
```

The visitor thread will indicate that it has arrived at the museum as soon as it has started. The visitor thread must **wait** until there is a tour guide and nine other visitors so they can enter as a group.

Unlike in a real museum, in the CS1550 museum, guides and visitors can enter the museum in any order as long as the constraints are maintained.

Once the visitor thread has entered the museum, it should indicate it is touring by calling the `visitor_tours()` function mentioned below. Visitors inside the museum should tour the museum concurrently. Once a visitor thread is done touring, it should indicate that it is leaving by calling `visitor_leaves()`, and after that, it may do anything else necessary to leave. Visitors do not wait before leaving. Each visitor's departure should **not** depend on any other visitor leaving or on a tour guide leaving.

Three functions are provided by the driver program to automatically test your code.

```c
void visitor_arrives(int id);
void visitor_tours(int id);
void visitor_leaves(int id);
```

You must call these functions in the correct order and at the correct times to receive full credit. The respective functions will print the following messages to the console:

```
Visitor V arrives at time T.
Visitor V tours the museum at time T.
Visitor V leaves the museum at time T.
```

To simulate touring, `visitor_tours` will also sleep for a configurable amount of time. The default when debugging is two seconds, but you can adjust this to help you test your implementation.

The driver program will automatically measure the elapsed time for you; you do not need to measure it in your code.

### `guide`

In `guide`, you will implement the tour guide arrival, entering, and leaving sequence.

```c
void guide(int id)
{
}
```

Like the visitor thread, the tour guide thread will indicate that it has arrived as soon as it has started. The guide thread must **wait** until there is a group of ten visitors before it enters.

Once the tour guide is inside the museum, it should indicate that it has entered. Then, it must wait until all ten visitors tour the museum and leave. After that, the guide thread immediately leaves to make room for the next group.

As with the visitor, three functions are provided by the driver program to automatically test your code.

```c
void guide_arrives(int id);
void guide_enters(int id);
void guide_leaves(int id);
```

You must call these functions in the correct order and at the correct times to receive full credit. The respective functions will print the following messages to the console:

```
Guide G arrives at time T.
Guide G enters the museum at time T.
Guide G leaves the museum at time T.
```


Testing your code
-----------------

As you implement your project, you are going to want a way to automatically build and debug your code. To do this, run `make debug` from the project directory. If all goes well, your output could be similar to the following:

```
INFO: Starting run

Visitor 0 arrives at time 0.
Visitor 1 arrives at time 0.
Guide 0 arrives at time 0.
Visitor 2 arrives at time 0.
Visitor 3 arrives at time 0.
Visitor 4 arrives at time 0.
Visitor 5 arrives at time 0.
Visitor 6 arrives at time 0.
Visitor 7 arrives at time 0.
Visitor 8 arrives at time 0.
Visitor 9 arrives at time 0.

Guide 0 enters the museum at time 0.
Visitor 0 tours the museum at time 0.
Visitor 1 tours the museum at time 0.
Visitor 2 tours the museum at time 0.
Visitor 3 tours the museum at time 0.
Visitor 7 tours the museum at time 0.
Visitor 5 tours the museum at time 0.
Visitor 6 tours the museum at time 0.
Visitor 4 tours the museum at time 0.
Visitor 8 tours the museum at time 0.
Visitor 9 tours the museum at time 0.
Visitor 0 leaves the museum at time 2.
Visitor 1 leaves the museum at time 2.
Visitor 2 leaves the museum at time 2.
Visitor 3 leaves the museum at time 2.
Visitor 5 leaves the museum at time 2.
Visitor 7 leaves the museum at time 2.
Visitor 6 leaves the museum at time 2.
Visitor 4 leaves the museum at time 2.
Visitor 8 leaves the museum at time 2.
Visitor 9 leaves the museum at time 2.
Guide 0 leaves the museum at time 2.
INFO: Finishing run
```

Several variables can be set on the command line to influence the behavior of the debug program. The defaults are given below. Fractional values are not allowed.

```bash
# The total number of visitors that will arrive.
num_visitors=10

# The total number of guides that will arrive.
num_guides=1

# The % chance that a visitor will arrive immediately after the previous one.
visitor_cluster_probability=100

# The % chance that a guide will arrive immediately after the previous one.
guide_cluster_probability=100

# The delay (in microseconds) between visitors if they do not arrive immediately
# after each other.
visitor_arrival_delay=1000000

# The delay (in microseconds) between guides if they do not arrive immediately
# after each other.
guide_arrival_delay=1000000

# The duration (in microseconds) of a visitor tour.
visitor_tour_duration=2000000
```

For example, you may change the tour duration, number of visitors, and number of guides as follows.
```bash
visitor_tour_duration=1000000 num_visitors=20 num_guides=2 make debug
```

To automatically test your program, run `make test`. The test program will automatically test your code as fast as possible and generate a lot of output. To stress your implementation, the delays are small and randomized or skipped altogether. Synchronization is left to the mercy of your program.

The test program depends on your correct implementation of `museum_init` and `museum_destroy` to reset the state of the simulation after each run. If you are encountering strange errors, you should check to see that you have properly cleaned up after the run.

The test program runs these cases with randomized delays:
|Visitors|Guides|
|--------|------|
|10|1|
|20|2|
|30|3|
|40|4|
|50|5|
|60|6|
|70|7|
|80|8|
|90|9|
|100|10|

The number of visitors is always ten times the number of guides; No threads will be left waiting outside forever.

Setting up the sources 
----------------------

These instructions will help you clone the starter code for the project from your GitHub Classroom private repository into Thoth.
1. Click on the GitHub Classroom link for the project on Canvas. After you link your GitHub username to the course and accept the assignment, a private repository with the name `cs1550-2244/cs1550-project2-GITHUB_USERNAME.git`, where `GITHUB_USERNAME` is your GitHub username that you have used when accepting the GitHub Classroom assignment, will be created for you.
2. Log in to Thoth using your Pitt account. From a UNIX box, you can type: `ssh PITT_USERNAME@thoth.cs.pitt.edu`, assuming `PITT_USERNAME` is your Pitt ID. From Windows, you may use PuTTY or the PowerShell ssh client.
3. Make sure that you are in a private directory (e.g., your `private` AFS folder or `/u/OSLab/PITT_USERNAME` on Thoth).
4. [Generate and use personal access tokens](https://docs.github.com/en/github/authenticating-to-github/creating-a-personal-access-token) instead of passwords. By [integrating GitHub with VS Code](https://code.visualstudio.com/docs/sourcecontrol/github), you may be able to skip the need to generate and use an access token.
5. From the server's command prompt, run the following command to download the starter code of the project:
`git clone https://github.com/cs1550-2224/cs1550-project2-GITHUB_USERNAME.git` where `GITHUB_USERNAME` is your GitHub username.
7. Change the directory into the project folder using `cd cs1550-project2-GITHUB_USERNAME/project2`.


Debugging tips
--------------

### Using `gdb`

To debug your program using `gdb`, first run `make` to build it, then run `gdb` as follows.

```
$ make
make: Nothing to be done for 'all'.

$ gdb ./museumsim
GNU gdb (Ubuntu 9.2-0ubuntu1~20.04) 9.2
Copyright (C) 2020 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
Type "show copying" and "show warranty" for details.
This GDB was configured as "x86_64-linux-gnu".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
    <http://www.gnu.org/software/gdb/documentation/>.

For help, type "help".
Type "apropos word" to search for commands related to "word"...
Reading symbols from museumsim...
(gdb)
```

To influence the program execution, you can set its environment variables before the program starts:
```
(gdb) set env visitor_tour_duration=1000000
(gdb) r
Starting program: /u/OSLab/skhattab/cs1550-project2/museumsim
```
To investigate a deadlock situation, you can attach `gdb` to the process by first grabbing the process id then running `gdb` and using the `attach` command. 
1. Press `CTRL+Z` to put the deadlocked program in the background.
2. run `ps`
3. Identify the process ID for `museumsim`
4. Run `gdb ./museumsim`
5. Type `attach <PID>` there `PID` is the process ID you got in step 3.
You can then run `thread apply all bt` in gdb to show where each thread is waiting. You’ll want to examine the code lines in `museumsim.c`.

To dig deeper:

1.  Run `thr 2` to switch to a particular thread (e.g., thread `2`).
2.  Run `bt` to get a stack trace, and `frame 5` (to jump to stack frame `5`, for example) to jump to the point in `museumsim.c` where you called some blocking construct.

### Using Valgrind

Valgrind includes two tools that may be useful for you in debugging the project.

The first tool is Memcheck, a memory error detector. You can read more about Memcheck [here](https://www.valgrind.org/docs/manual/mc-manual.html).
```
$ make
make: Nothing to be done for 'all'.

$ valgrind ./museumsim
==2369201== Memcheck, a memory error detector
==2369201== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
==2369201== Using Valgrind-3.15.0 and LibVEX; rerun with -h for copyright info
==2369201== Command: ./museumsim
==2369201==

...

==2369201==
==2369201== HEAP SUMMARY:
==2369201==     in use at exit: 0 bytes in 0 blocks
==2369201==   total heap usage: 11,060 allocs, 11,060 frees, 191,312 bytes allocated
==2369201==
==2369201== All heap blocks were freed -- no leaks are possible
==2369201==
==2369201== For lists of detected and suppressed errors, rerun with: -s
==2369201== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

The second tool is Helgrind, a thread error detector. You can read more about Helgrind [here](https://valgrind.org/docs/manual/hg-manual.html).

```
$ make
make: Nothing to be done for 'all'.

$ valgrind --tool=helgrind ./museumsim
==1485705== Helgrind, a thread error detector
==1485705== Copyright (C) 2007-2017, and GNU GPL'd, by OpenWorks LLP et al.
==1485705== Using Valgrind-3.15.0 and LibVEX; rerun with -h for copyright info
==1485705== Command: ./test
==1485705==

...

==1485705==
==1485705== Use --history-level=approx or =none to gain increased speed, at
==1485705== the cost of reduced accuracy of conflicting-access information
==1485705== For lists of detected and suppressed errors, rerun with: -s
==1485705== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

File Backups
------------

The `/u/OSLab/` partition is **not** part of AFS space. Thus, any files you modify under these folders are not backed up. If there is a catastrophic disk failure, all of your work will be irrecoverably lost. As such, it is our recommendation that you:

**Commit and push all the files you change to your GitHub repository frequently!**

**BE FOREWARNED:** Loss of work not backed up is not grounds for an extension.


Submission and Writeup
----------------------

We will use an automatic grader for Project 2. It runs similar cases to the ones in `make test`. You can test your code on the autograder before the deadline. You get unlimited attempts until the deadline. You need to submit your GitHub repository to Gradescope. **You may not change the folder structure and file locations**.

- Your repository should also contain a short report (200-300 words) in [GitHub Markdown](https://guides.github.com/features/mastering-markdown/) named `p2-writeup.md` at the root of the repository that answers the following question:
__Indicate if your solution is deadlock-free and informally explain why.__


Grading rubric
--------------

The rubric items can be found on the project submission page on Gradescope. Non-compiling code gets **zero** points.

|Item|Grade|
|----|-----|
|Test cases on the autograder|80%|
|Comments and style|10%|
|Report|10%|

The following penalty points, among others, may be deducted using manual grading:

|Item|Grade|
|----|-----|
|Use of sleep calls for synchronization|-25%|
|Use of busy waiting or spinlocks|-25%|
|Unprotected shared variable access|-20%|

**If the autograder score is below 48 points (out of 80), a partial grade will be assigned using manual grading of your code. The maximum score that you can get in this manual grading is 48 points. Again, non-compiling code gets **zero** points.**

Please note that the score that you get from the autograder is not your final score. We still do manual grading. We may discover bugs and mistakes that were not caught by the test scripts and take penalty points off. Please use the autograder (or the local test harness `make test`) only as a tool to get immediate feedback and discover bugs in your program. Please note that certain bugs (e.g., deadlocks) in your program may or may not manifest themselves when the test cases are run on your program. It may be the case that the same exact code fails in some tests then the same code passes the same tests when resubmitted. The fact that a test once fails should make you debug your code, not simply resubmit and hope that the situation that caused the bug won't happen the next time around.
