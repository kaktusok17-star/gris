#include "index.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void entry_free(IndexEntry *e) {
    free(e->name);
    free(e->version);
    free(e->arch);
    free(e->deps);
    free(e->url);
    free(e->sha256);
}

/* Ручной split по '|'. В отличие от strtok, сохраняет пустые поля.
   Возвращает число найденных полей (до max). */
static int split_pipe(char *s, char **fields, int max) {
    int n = 0;
    char *p = s;
    while (n < max) {
        fields[n++] = p;
        char *bar = strchr(p, '|');
        if (!bar) break;
        *bar = '\0';
        p = bar + 1;
    }
    return n;
}

int index_load(const char *path, Index *out) {
    memset(out, 0, sizeof(*out));
    vec_init(&out->entries, sizeof(IndexEntry *));

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char  *line = NULL;
    size_t cap  = 0;

    while (getline(&line, &cap, f) != -1) {
        char *s = trim(line);
        if (!*s || *s == '#') continue;

        char *fields[7] = { NULL };
        int   nf = split_pipe(s, fields, 7);
        if (nf < 6) continue;

        IndexEntry *e = xcalloc(1, sizeof(IndexEntry));
        e->name    = xstrdup(fields[0] ? fields[0] : "");
        e->version = xstrdup(fields[1] ? fields[1] : "");
        e->arch    = xstrdup(fields[2] ? fields[2] : "");
        e->deps    = xstrdup(fields[3] ? fields[3] : "");
        e->size    = fields[4] ? atol(fields[4]) : 0;
        e->url     = xstrdup(fields[5] ? fields[5] : "");
        e->sha256  = xstrdup((nf > 6 && fields[6]) ? fields[6] : "");

        *(IndexEntry **)vec_push(&out->entries) = e;
    }

    free(line);
    fclose(f);
    return 0;
}

void index_free(Index *idx) {
    for (size_t i = 0; i < idx->entries.len; i++) {
        IndexEntry *e = *(IndexEntry **)vec_get(&idx->entries, i);
        entry_free(e);
        free(e);
    }
    vec_free(&idx->entries);
}

IndexEntry *index_find(Index *idx, const char *name) {
    for (size_t i = 0; i < idx->entries.len; i++) {
        IndexEntry *e = *(IndexEntry **)vec_get(&idx->entries, i);
        if (strcmp(e->name, name) == 0) return e;
    }
    return NULL;
}