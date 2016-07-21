#include <syscalls>
main() {
   write(1,"Init\n",5);
   new pid = fork();
   if(pid != 0) {
      write(1,"parent\n",7);
   } else {
      write(1,"child!\n",7);
   }
   for(;;) {}
}

