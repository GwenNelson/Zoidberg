#include <stdlib.h>
#include <stdio.h>
#include <sys/utsname.h>

int main(int argc, char** argv) {
    struct utsname *buf=malloc(sizeof(struct utsname));
    uname(buf);
    if(argc==1) {
       printf("%s\n",buf->sysname);
       free(buf);
       return 0;
    }
    extern char *optarg;
    extern int optind;
    if(argc>=2) {
       int c=0;
       while((c = getopt(argc, argv, "asnrvmpio")) != -1) {
          switch(c) {
             case 'a':
               printf("%s %s %s %s %s",
                      buf->sysname,
                      buf->nodename,
                      buf->release,
                      buf->version,
                      buf->machine);
             break;
             case 's':
               printf("%s ",buf->sysname);
             break;
             case 'n':
               printf("%s ",buf->nodename);
             break;
             case 'r':
               printf("%s ",buf->release);
             break;
             case 'v':
               printf("%s ",buf->version);
             break;
             case 'm':
               printf("%s ",buf->machine);
             break;
             case 'p':
               printf("unknown ");
             break;
             case 'i':
               printf("unknown");
             break;
             case 'o':
               printf("%s ",buf->sysname);
             break;
          }
       }
       printf("\n");

    }

    free(buf);
    return 0;
}
