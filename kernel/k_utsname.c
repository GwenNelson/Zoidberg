#include "zoidberg_version.h"
#define IN_UTSNAME
#include "k_utsname.h"

struct utsname zoidberg_uname = {
    .sysname  = "zoidberg",
    .nodename = "localhost",
    .release  = ZOIDBERG_VERSION,
    .version  = "build "ZOIDBERG_BUILD" "ZOIDBERG_BUILDDATE,
    .machine  = "x86_64"
};

int uname(struct utsname *buf) {
    *buf = zoidberg_uname;
    return 0;
}
