#ifndef GRIS_DEPS_H
#define GRIS_DEPS_H

#include "pkg.h"
#include "util.h"

/* Прочитать .PKGINFO из .gris-файла, не распаковывая его целиком.
   0 = ok, -1 = ошибка. */
int peek_pkginfo(const char *gris_file, PkgInfo *out);

/* Найти .gris-файл пакета name в каталоге dir.
   Возвращает malloc'd путь или NULL. */
char *deps_find_gris(const char *dir, const char *name);

/* Выделить имя из "bar>=1.0" → "bar". 0 = ok. */
int deps_parse_name(const char *dep, char *out, size_t size);

/* Каталог файла. malloc'd. */
char *deps_dirname(const char *path);

/* Узел графа установки. */
typedef struct {
    char   *name;
    char   *path;
    PkgInfo info;
    int     state;    /* 0=new, 1=visiting, 2=done */
} DepNode;

/* Разрешить граф зависимостей, начиная с файла gris_file.
   Заполняет Vec of DepNode в порядке "зависимости сначала".
   0 = ok, -1 = ошибка. */
int deps_resolve(const char *gris_file, Vec *nodes);

#endif