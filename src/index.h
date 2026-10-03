#ifndef GRIS_INDEX_H
#define GRIS_INDEX_H

#include "util.h"

typedef struct {
    char *name;
    char *version;
    char *arch;
    char *deps;      /* строка как есть: "bar>=2.0 baz" */
    long  size;
    char *url;
    char *sha256;
} IndexEntry;

typedef struct {
    Vec entries;     /* Vec of IndexEntry* */
} Index;

/* Загрузить core.index. 0 = ok, -1 = ошибка. */
int  index_load(const char *path, Index *out);
void index_free(Index *idx);

/* Найти по имени. NULL если нет. */
IndexEntry *index_find(Index *idx, const char *name);

#endif