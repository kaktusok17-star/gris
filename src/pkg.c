#include "pkg.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── helpers ────────────────────────────────────────────────── */
static int is_hex(const char *s, size_t len) {
    for (size_t i = 0; i < len; i++)
        if (!isxdigit((unsigned char)s[i])) return 0;
    return 1;
}

static void set_str(char **dst, const char *val) {
    free(*dst);
    *dst = xstrdup(val);
}

static void add_dep(PkgInfo *p, const char *dep) {
    p->depends = xrealloc(p->depends, (p->ndepends + 1) * sizeof(char *));
    p->depends[p->ndepends++] = xstrdup(dep);
}

/* ── .PKGINFO ───────────────────────────────────────────────── */
int pkginfo_parse(const char *path, PkgInfo *out) {
    memset(out, 0, sizeof(*out));

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char  *line = NULL;
    size_t cap  = 0;

    while (getline(&line, &cap, f) != -1) {
        char *s = trim(line);
        if (!*s || *s == '#') continue;

        char *eq = strstr(s, " = ");
        if (!eq) continue;
        *eq = '\0';
        char *key = s;
        char *val = eq + 3;

        if      (!strcmp(key, "pkgname"))   set_str(&out->name, val);
        else if (!strcmp(key, "pkgver"))    set_str(&out->version, val);
        else if (!strcmp(key, "arch"))      set_str(&out->arch, val);
        else if (!strcmp(key, "pkgdesc"))   set_str(&out->desc, val);
        else if (!strcmp(key, "url"))       set_str(&out->url, val);
        else if (!strcmp(key, "license"))   set_str(&out->license, val);
        else if (!strcmp(key, "sha256"))    set_str(&out->sha256, val);
        else if (!strcmp(key, "size"))      out->size      = atol(val);
        else if (!strcmp(key, "builddate")) out->builddate = atol(val);
        else if (!strcmp(key, "packager"))  set_str(&out->packager, val);
        else if (!strcmp(key, "depends")) {
            char *save = NULL;
            for (char *tok = strtok_r(val, " \t", &save); tok;
                 tok = strtok_r(NULL, " \t", &save))
                add_dep(out, tok);
        }
    }

    free(line);
    fclose(f);
    return out->name ? 0 : -1;
}

void pkginfo_free(PkgInfo *p) {
    free(p->name);
    free(p->version);
    free(p->arch);
    free(p->desc);
    free(p->url);
    free(p->license);
    free(p->sha256);
    free(p->packager);
    for (size_t i = 0; i < p->ndepends; i++)
        free(p->depends[i]);
    free(p->depends);
    memset(p, 0, sizeof(*p));
}

void pkginfo_print(const PkgInfo *p) {
    if (p->name)      printf("Name        : %s\n", p->name);
    if (p->version)   printf("Version     : %s\n", p->version);
    if (p->arch)      printf("Architecture: %s\n", p->arch);
    if (p->desc)      printf("Description : %s\n", p->desc);
    if (p->url)       printf("URL         : %s\n", p->url);
    if (p->license)   printf("License     : %s\n", p->license);
    if (p->size)      printf("Size        : %ld\n", p->size);
    if (p->builddate) printf("Build date  : %ld\n", p->builddate);
    if (p->packager)  printf("Packager    : %s\n", p->packager);
    if (p->ndepends) {
        printf("Depends     :");
        for (size_t i = 0; i < p->ndepends; i++)
            printf(" %s", p->depends[i]);
        printf("\n");
    }
}

/* ── .FILELIST ──────────────────────────────────────────────── */
int filelist_parse(const char *path, Vec *out) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char  *line = NULL;
    size_t cap  = 0;

    while (getline(&line, &cap, f) != -1) {
        char *s = trim(line);
        if (!*s || *s == '#') continue;

        FileEntry *fe = vec_push(out);
        fe->path = NULL;
        fe->hash = NULL;

        char *sep = strpbrk(s, " \t");
        if (sep && (size_t)(sep - s) == 64 && is_hex(s, 64)) {
            *sep = '\0';
            fe->hash = xstrdup(s);
            fe->path = xstrdup(trim(sep + 1));
        } else {
            fe->path = xstrdup(s);
        }
    }

    free(line);
    fclose(f);
    return 0;
}