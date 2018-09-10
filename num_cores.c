#include <stdio.h>
#include <sys/sysinfo.h>

int main()
{
  printf("This machine has %d cores.", get_nprocs());
  return 0;
}
