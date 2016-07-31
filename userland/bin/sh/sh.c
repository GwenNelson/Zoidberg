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

int main() {
    char* cwd;
    size_t len = 0;
    ssize_t read=0;    
    char* input_line=NULL;

    while(running) {
       cwd = "/";  // mock for now
       snprintf(hostname_str,1024,"%s","zoidberg"); // also mock
       snprintf(prompt_str, 4096, unix_prompt, login_name, hostname_str, cwd);
       printf(prompt_str);
       __getline(&input_line,&len,stdin);
       printf("\n");
    }

}
