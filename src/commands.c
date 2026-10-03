#include "gris.h"
#include "db.h"
#include "pkg.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int not_implemented(const char *name) {
    fprintf(stderr, "gris: %s: не реализовано (следующий шаг)\n", name);
    return 1;
}

/* ── заглушки ────────────────────────────────────────────────── */
int cmd_sync   (int argc, char **argv) { (void)argc; (void)argv; return not_implemented("sync");    }
int cmd_install(int argc, char **argv) { (void)argc; (void)argv; return not_implemented("install"); }
int cmd_remove (int argc, char **argv) { (void)argc; (void)argv; return not_implemented("remove");  }
int cmd_upgrade(int argc, char **argv) { (void)argc; (void)argv; return not_implemented("upgrade"); }
int cmd_search (int argc, char **argv) { (void)argc; (void)argv; return not_implemented("search");  }
int cmd_clean  (int argc, char **argv) { (void)argc; (void)argv; return not_implemented("clean");   }

/* ── list ────────────────────────────────────────────────────── */
int cmd_list(int argc, char **argv) {
    (void)argc; (void)argv;

    Vec v;
    vec_init(&v, sizeof(char *));
    db_list_installed(&v);

    for (size_t i = 0; i < v.len; i++) {
        char *name = *(char **)vec_get(&v, i);

        PkgInfo pi;
        if (db_read_pkginfo(name, &pi) == 0) {
            printf("%s %s\n", name, pi.version ? pi.version : "?");
            pkginfo_free(&pi);
        } else {
            printf("%s\n", name);
        }
        free(name);
    }
    vec_free(&v);
    return 0;
}

/* ── info ────────────────────────────────────────────────────── */
int cmd_info(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "gris: info: укажите имя пакета\n");
        return 2;
    }

    PkgInfo pi;
    if (db_read_pkginfo(argv[0], &pi) != 0) {
        fprintf(stderr, "gris: info: пакет '%s' не установлен\n", argv[0]);
        return 1;
    }
    pkginfo_print(&pi);
    pkginfo_free(&pi);
    return 0;
}

/* ── files ───────────────────────────────────────────────────── */
int cmd_files(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "gris: files: укажите имя пакета\n");
        return 2;
    }

    Vec v;
    vec_init(&v, sizeof(FileEntry));
    if (db_read_filelist(argv[0], &v) != 0) {
        fprintf(stderr, "gris: files: пакет '%s' не установлен\n", argv[0]);
        vec_free(&v);
        return 1;
    }

    for (size_t i = 0; i < v.len; i++) {
        FileEntry *fe = vec_get(&v, i);
        printf("%s\n", fe->path);
        free(fe->path);
        free(fe->hash);
    }
    vec_free(&v);
    return 0;
}