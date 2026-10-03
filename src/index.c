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

        /* формат: name|version|arch|deps|size|url|sha256 */
        IndexEntry *e = xcalloc(1, sizeof(IndexEntry));
        char *save = NULL;
        char *tok;
        int   field = 0;
        int   bad   = 0;

        for (tok = strtok_r(s, "|", &save); tok;
             tok = strtok_r(NULL, "|", &save)) {
            switch (field++) {
                case 0: e->name    = xstrdup(tok); break;
                case 1: e->version = xstrdup(tok); break;
                case 2: e->arch    = xstrdup(tok); break;
                case 3: e->deps    = xstrdup(tok); break;
                case 4: e->size    = atol(tok);    break;
                case 5: e->url     = xstrdup(tok); break;
                case 6: e->sha256  = xstrdup(tok); break;
                default: bad = 1; break;
            }
        }

        if (field < 6 || bad) {
            entry_free(e);
            free(e);
            continue;
        }
        if (!e->deps)    e->deps    = xstrdup("");
        if (!e->sha256)  e->sha256  = xstrdup("");

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