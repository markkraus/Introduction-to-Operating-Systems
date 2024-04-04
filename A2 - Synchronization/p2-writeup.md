# Deadlock-Free Solution Analysis

My program appears to be deadlock-free. Here are my reasons for why that is the case.

## Mutex Usage
Mutex locks are employed to ensure mutual exclusion in critical sections of code - primarily used for accessing and updating shared variables used across threads. It prevents multiple threads from concurrently accessing shared resources at the same time which would result in a race condition.

## Condition Variables
Condition variables synchronize threads. This ensures that threads do not remain indefinitely blocked when the program is run. I made sure to carefully awaken visitors and guides when specifc scenarios took place. For example, I put the guide to sleep when another guide is giving a tour in the museum, and awaken the sleeping guide when the touring guide signals to them that they have concluded their tour. Another example is when a guide arrives right away (before any visitors arrive). The guide will enter the museum but sleep until a visitor arrives. 10 visitors will wake up the guide, but the guide will only proceed when the 10th visitor shows up, or else they just go back to sleep.

## Conclusion

Of course, I partly believe my program is deadlock-free because I haven't ran my completed program and encountered a deadlock. But due to the many different combinations that guests/guides could arrive, it is possible that I have oversight and have a deadlock somewhere in my code based on a given sequence. However, this number of combinations is very large and therefore I am unable to test every one. In total, from my human-error prone persepctive, I believe my program is deadlock-free.