#include <syscalls>
main() {
   write(1,"Init\r",5);
   new pid = fork();
   write(pid,"fork!\r",6);
   while(1) {}
}

