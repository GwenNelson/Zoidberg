#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

void stop_startup(char* errmsg) {
     printf("\n\n **** STARTUP ABORTED ****\n");
     printf("Could not start the system, reason: %s\n");
     printf("System will now hang\n");
     for(;;);
}

int main() {
    printf("[init] starting system\n");
    printf("[init] Sanity checks:\n");

    pid_t my_pid = getpid();
    printf("       [+] PID=%d\n",my_pid);
    if(my_pid!=1) stop_startup("/sbin/init must be PID 1");
    
    printf("       [+] vfork() test\n");
    
    return 0;
}
