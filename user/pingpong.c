#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int p1[2];
  int p2[2];
  char buf[4];
  int m, n;

  pipe(p1);
  pipe(p2);
  if(fork() == 0){

    close(p1[1]);
    close(p2[0]);

    n = read(p1[0], buf, 4);
    close(p1[0]);

    if(n > 0){
        printf("%d: received ping\n", getpid());
        write(p2[1], "pong", 4);
        close(p2[1]);
    }

    exit(0);
  }
  else{
    
    close(p1[0]);
    close(p2[1]);

    write(p1[1], "ping", 4);
    close(p1[1]);

    m = read(p2[0], buf, 4);
    close(p2[0]);

    if(m > 0){
        printf("%d: received pong\n", getpid());
    }
  }
  exit(0);
}