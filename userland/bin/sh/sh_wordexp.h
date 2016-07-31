#ifndef	_SH_WORDEXP_H
#define	_SH_WORDEXP_H

#include <sys/types.h>

#define WRDE_DOOFFS  1
#define WRDE_APPEND  2
#define WRDE_NOCMD   4
#define WRDE_REUSE   8
#define WRDE_SHOWERR 16
#define WRDE_UNDEF   32

typedef struct {
	size_t we_wordc;
	char **we_wordv;
	size_t we_offs;
} wordexp_t;

#define WRDE_NOSYS   -1
#define WRDE_NOSPACE 1
#define WRDE_BADCHAR 2
#define WRDE_BADVAL  3
#define WRDE_CMDSUB  4
#define WRDE_SYNTAX  5

int sh_wordexp(const char *s, wordexp_t *we, int flags);

void sh_wordfree (wordexp_t *);


#endif
