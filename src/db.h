#ifndef GRIS_DB_H
#define GRIS_DB_H

#include "pkg.h"
#include "util.h"

char *db_path(const char *sub);
char *db_pkg_dir(const char *name);
int  db_list_installed(Vec *out);
int  db_read_pkginfo (const char *name, PkgInfo *out);
int  db_read_filelist(const char *name, Vec *out);
int  db_is_installed(const char *name);

#endif