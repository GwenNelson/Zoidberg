#include <stdio.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <string.h>

char* shell_path="initrd:/bin/sh";

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

    struct utsname uname_buf;
    uname(&uname_buf);
    printf("       [+] sysname=%s\n",uname_buf.sysname);
    if(strncmp(uname_buf.sysname,"zoidberg",8) != 0) stop_startup("wrong OS, should run on zoidberg");

    pid_t test_pid = vfork();
    printf("       [+] hello from vfork() PID %d\n",test_pid);
    if(test_pid != 0) {
       for(;;);
    }

    printf("[init] Will spawn shell from %s\n",shell_path);
    pid_t shell_pid = spawn(shell_path);
    // TODO - wait() for shell

    for(;;);
}
