#include "types.h" 
#include "stat.h" 
#include "user.h" 
#include "fcntl.h"

//We want Child 1 to execute first, then Child 2, and finally Parent.
int main() {
  init_lock();
  init_cv();
  int fd = open("flag", O_RDWR | O_CREATE); 
  int ret = fork(); //fork the first child
  if(ret < 0) {
    printf(1, "Error forking first child.\n");
  } else if (ret == 0) { 
    sleep(5);
    lock();
    printf(1, "Child 1 Executing\n"); 
    write(fd, "done", 4); 
    cv_broadcast(); 
    unlock();
  } else {
    ret = fork(); //fork the second
    if(ret < 0) {
      printf(1, "Error forking second child.\n");
     } else if(ret == 0) { 
      lock(); 
      struct stat stats; 
      fstat(fd, &stats);
      printf(1, "file size = %d\n", stats.size);
      while(stats.size <= 0){ 
        cv_wait(); 
        fstat(fd, &stats);
        printf(1, "file size = %d\n", stats.size);
      }
      printf(1, "Child 2 Executing\n");
      unlock();
    } else {
      lock(); 
      printf(1, "Parent Waiting\n");
      unlock();
      int i;
      for(i=0; i< 2; i++) 
        wait();
      printf(1, "Children completed\n"); 
      printf(1, "Parent Executing\n"); 
      printf(1, "Parent exiting.\n");
    }
  }
  close(fd);
  unlink("flag"); 
  exit();
}