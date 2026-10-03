#ifndef GRIS_DEPS_H
#define GRIS_DEPS_H

#include "index.h"
#include "pkg.h"
#include "util.h"

/* Прочитать .PKGINFO из .gris-файла, не распаковывая его целиком. */
int peek_pkginfo(const char *gris_file, PkgInfo *out);

/* Найти .gris-файл пакета name в каталоге dir. malloc'd. */
char *deps_find_gris(const char *dir, const char *name);

/* Выделить имя из "bar>=1.0" → "bar". 0 = ok. */
int deps_parse_name(const char *dep, char *out, size_t size);

/* Каталог файла. malloc'd. */
char *deps_dirname(const char *path);

/* Узел файлового графа. */
typedef struct {
    char   *name;
    char   *path;
    PkgInfo info;
    int     state;
} DepNode;

/* Разрешение по локальным .gris-файлам. Vec of DepNode. */
int deps_resolve(const char *gris_file, Vec *nodes);

/* Разрешение по индексу репозитория. Vec of IndexEntry* в порядке
   "зависимости сначала". 0 = ok, -1 = не найдено / цикл. */
int deps_resolve_index(Index *idx, const char *name, Vec *out_order);

#endif