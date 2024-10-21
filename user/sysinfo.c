#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/sysinfo.h"
#include "user/user.h"


int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    fprintf(2, "Usage: sysinfo files...\n");
    exit(1);
  }

  struct sysinfo info;
  sysinfo(&info);
  exit(0);
}
