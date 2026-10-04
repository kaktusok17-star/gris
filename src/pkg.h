#ifndef GRIS_PKG_H
#define GRIS_PKG_H

#include "util.h"
#include <sys/types.h>

typedef struct {
    char  *name;
    char  *version;
    char  *arch;
    char  *desc;
    char  *url;
    char  *license;
    char  *sha256;
    long   size;
    long   builddate;
    char  *packager;
    char **depends;
    size_t ndepends;
} PkgInfo;

typedef struct {
    char   *path;
    char   *hash;
    size_t  size;
    mode_t  mode;
} FileEntry;

/* .PKGINFO: 0 = ok, -1 = не открывается / нет pkgname */
int  pkginfo_parse    (const char *path, PkgInfo *out);
int  pkginfo_parse_buf(const char *text, PkgInfo *out);
void pkginfo_free(PkgInfo *p);
void pkginfo_print(const PkgInfo *p);

/* .FILELIST: 0 = ok, -1 = не открывается. out: Vec of FileEntry */
int  filelist_parse    (const char *path, Vec *out);
int  filelist_parse_buf(const char *text, Vec *out);

#endif