#include <stdio.h>
#include <sys/sysinfo.h>


int main() {
  printf("This machine has %d core(s).\n", get_nprocs_conf());
  return 0;
}

