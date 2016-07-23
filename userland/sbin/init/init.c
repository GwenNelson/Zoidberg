#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

char* shell_path="/bin/sh";

void stop_startup(char* errmsg) {
     printf("\n\n **** STARTUP ABORTED ****\n");
     printf("Could not start the system, reason: %s\n",errmsg);
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
    int ret = vfork();
    if(ret<0) stop_startup("vfork() is not working!");
    if(ret!=0) {
       printf("           [+] in parent, child PID is %d\n",ret);
    } else {
       printf("           [+] in child\n");
//       exit(0);
       stop_startup("exit() did not work!");
    }


    printf("[init] Will spawn shell from %s\n",shell_path);
    pid_t shell_pid = vfork();
    if(shell_pid==0) {
       char* shellargv[] = {shell_path,NULL};
       char* shellenv[]  = {NULL};
       execve(shell_path,shellargv,shellenv);
    } else {
       for(;;); // TODO - wait() for shell
    }

    printf("[init] PANIC - should not ever reach here!\n");
    for(;;);
}
