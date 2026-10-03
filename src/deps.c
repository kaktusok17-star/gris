#include "deps.h"
#include "shell.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── файловый режим (для локальных .gris) ───────────────────── */
int peek_pkginfo(const char *gris_file, PkgInfo *out) {
    char *argv[] = {
        "tar", "-xOJf", (char *)gris_file, ".PKGINFO", NULL
    };
    char *text = NULL;
    if (shell_capture(argv, &text, NULL) != 0) {
        free(text);
        return -1;
    }
    int rc = pkginfo_parse_buf(text, out);
    free(text);
    return rc;
}

char *deps_dirname(const char *path) {
    char *p = xstrdup(path);
    char *s = strrchr(p, '/');
    if (!s) { free(p); return xstrdup("."); }
    *s = '\0';
    if (p[0] == '\0') { free(p); return xstrdup("/"); }
    return p;
}

int deps_parse_name(const char *dep, char *out, size_t size) {
    size_t i = 0;
    while (dep[i] && i < size - 1 &&
           dep[i] != '>' && dep[i] != '<' && dep[i] != '=' &&
           dep[i] != ' ' && dep[i] != '\t') {
        out[i] = dep[i];
        i++;
    }
    out[i] = '\0';
    return i > 0 ? 0 : -1;
}

char *deps_find_gris(const char *dir, const char *name) {
    DIR *d = opendir(dir);
    if (!d) return NULL;

    size_t name_len = strlen(name);
    char  *result   = NULL;

    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (strncmp(e->d_name, name, name_len) != 0) continue;
        if (e->d_name[name_len] != '-') continue;
        if (!ends_with(e->d_name, ".gris")) continue;

        size_t n = strlen(dir) + 1 + strlen(e->d_name) + 1;
        result = xmalloc(n);
        snprintf(result, n, "%s/%s", dir, e->d_name);
        break;
    }
    closedir(d);
    return result;
}

static DepNode *find_node(Vec *nodes, const char *name) {
    for (size_t i = 0; i < nodes->len; i++) {
        DepNode *n = vec_get(nodes, i);
        if (strcmp(n->name, name) == 0) return n;
    }
    return NULL;
}

static int resolve_recursive(const char *gris_file, Vec *nodes) {
    PkgInfo info;
    if (peek_pkginfo(gris_file, &info) != 0) {
        fprintf(stderr, "gris: не могу прочитать .PKGINFO из %s\n", gris_file);
        return -1;
    }

    DepNode *existing = find_node(nodes, info.name);
    if (existing) {
        if (existing->state == 1) {
            fprintf(stderr, "gris: циклическая зависимость: %s\n", info.name);
            pkginfo_free(&info);
            return -1;
        }
        pkginfo_free(&info);
        return 0;
    }

    DepNode *n  = vec_push(nodes);
    n->name     = xstrdup(info.name);
    n->path     = xstrdup(gris_file);
    n->info     = info;
    n->state    = 1;

    char *dir = deps_dirname(gris_file);

    for (size_t i = 0; i < n->info.ndepends; i++) {
        char dep_name[256];
        if (deps_parse_name(n->info.depends[i], dep_name, sizeof(dep_name)) != 0)
            continue;

        char *dep_file = deps_find_gris(dir, dep_name);
        if (!dep_file) {
            fprintf(stderr,
                "gris: не найдена зависимость '%s' (нужна пакету '%s') в %s\n",
                dep_name, n->name, dir);
            free(dir);
            return -1;
        }

        if (resolve_recursive(dep_file, nodes) != 0) {
            free(dep_file);
            free(dir);
            return -1;
        }
        free(dep_file);
    }

    n->state = 2;
    free(dir);
    return 0;
}

int deps_resolve(const char *gris_file, Vec *nodes) {
    return resolve_recursive(gris_file, nodes);
}

/* ── режим по индексу (для install по имени) ────────────────── */
static int resolve_index_rec(Index *idx, const char *name,
                             Vec *order, Vec *in_progress) {
    /* Уже разрешено? */
    for (size_t i = 0; i < order->len; i++) {
        IndexEntry *e = *(IndexEntry **)vec_get(order, i);
        if (strcmp(e->name, name) == 0) return 0;
    }

    /* В процессе? → цикл */
    for (size_t i = 0; i < in_progress->len; i++) {
        char *n = *(char **)vec_get(in_progress, i);
        if (strcmp(n, name) == 0) {
            fprintf(stderr, "gris: циклическая зависимость: %s\n", name);
            return -1;
        }
    }

    IndexEntry *e = index_find(idx, name);
    if (!e) {
        fprintf(stderr, "gris: пакет '%s' не найден в индексе\n", name);
        return -1;
    }

    *(char **)vec_push(in_progress) = e->name;

    char *deps = xstrdup(e->deps);
    char *save = NULL;
    for (char *tok = strtok_r(deps, " \t", &save); tok;
         tok = strtok_r(NULL, " \t", &save)) {
        char dep_name[256];
        if (deps_parse_name(tok, dep_name, sizeof(dep_name)) != 0) continue;
        if (resolve_index_rec(idx, dep_name, order, in_progress) != 0) {
            free(deps);
            return -1;
        }
    }
    free(deps);

    in_progress->len--;
    *(IndexEntry **)vec_push(order) = e;
    return 0;
}

int deps_resolve_index(Index *idx, const char *name, Vec *out_order) {
    Vec in_progress;
    vec_init(&in_progress, sizeof(char *));
    int rc = resolve_index_rec(idx, name, out_order, &in_progress);
    vec_free(&in_progress);
    return rc;
}