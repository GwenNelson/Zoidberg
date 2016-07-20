#include <syscalls>
main() {
   write(1,"Init\r",5);
   new pid = fork();
   if(pid == 0) {
      write(1,"parent\r",7);
   } else {
      write(1,"child!\r",7);
   }
   while(1) {}
}

