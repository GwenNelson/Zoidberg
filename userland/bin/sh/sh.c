#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>


static char* unix_prompt = "%s@%s:%s# ";
static char  prompt_str[4096];
static char  hostname_str[1024];
static char*  login_name = "root"; // mock
static int running=1;
static char* cwd;

int main() {
    cwd = malloc(512);
    size_t len = 0;
    ssize_t read=0;    
    char* input_line=NULL;
    
    sys_chdir("/");

    while(running) {
       sys_getcwd(cwd,512);
       snprintf(hostname_str,1024,"%s","zoidberg"); // also mock
       snprintf(prompt_str, 4096, unix_prompt, login_name, hostname_str, cwd);
       printf(prompt_str);
       __getline(&input_line,&len,stdin);
       if(strncmp(input_line,"cd ",3)==0) {
          sys_chdir(strtok(input_line+3,"\n"));
       } else {
       }

    }

}
