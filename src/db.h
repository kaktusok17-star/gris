#ifndef GRIS_DB_H
#define GRIS_DB_H

#include "pkg.h"
#include "util.h"

/* Путь к каталогу установленного пакета. malloc'd, вызывающий free(). */
char *db_pkg_dir(const char *name);

/* Vec of char* — имена установленных пакетов. */
int  db_list_installed(Vec *out);

/* Чтение PKGINFO/FILELIST установленного пакета. 0 = ok, -1 = нет. */
int  db_read_pkginfo (const char *name, PkgInfo *out);
int  db_read_filelist(const char *name, Vec *out);

int  db_is_installed(const char *name);

#endif