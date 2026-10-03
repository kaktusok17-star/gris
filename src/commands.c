#include "gris.h"
#include "archive.h"
#include "config.h"
#include "db.h"
#include "deps.h"
#include "fs.h"
#include "hash.h"
#include "pkg.h"
#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int not_implemented(const char *name) {
    fprintf(stderr, "gris: %s: не реализовано (следующий шаг)\n", name);
    return 1;
}

int cmd_sync   (int argc, char **argv) { (void)argc; (void)argv; return not_implemented("sync");    }
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

/* ── sha256 sidecar ──────────────────────────────────────────── */
static int verify_sidecar_sha256(const char *gris_file) {
    char sidecar[4096];
    snprintf(sidecar, sizeof(sidecar), "%s.sha256", gris_file);

    if (!fs_exists(sidecar)) return 0;      /* нет сайдкара — пропускаем */

    char expected[65];
    FILE *f = fopen(sidecar, "r");
    if (!f) return 0;
    int got = fscanf(f, "%64s", expected);
    fclose(f);
    if (got != 1) {
        fprintf(stderr, "gris: пустой .sha256: %s\n", sidecar);
        return -1;
    }

    char actual[65];
    if (hash_file_sha256(gris_file, actual) != 0) {
        fprintf(stderr, "gris: не могу посчитать sha256 для %s\n", gris_file);
        return -1;
    }

    if (strcmp(expected, actual) != 0) {
        fprintf(stderr, "gris: sha256 не совпал для %s\n", gris_file);
        fprintf(stderr, "  ожидался: %s\n", expected);
        fprintf(stderr, "  получен:  %s\n", actual);
        return -1;
    }
    return 0;
}

/* ── install ─────────────────────────────────────────────────── */
static int install_one(const char *gris_file) {
    if (!fs_exists(gris_file)) {
        fprintf(stderr, "gris: install: файл не найден: %s\n", gris_file);
        return -1;
    }

    if (verify_sidecar_sha256(gris_file) != 0)
        return -1;

    char tmp[] = "/tmp/gris-install-XXXXXX";
    if (!mkdtemp(tmp)) {
        fprintf(stderr, "gris: mkdtemp: %s\n", strerror(errno));
        return -1;
    }

    if (archive_extract(gris_file, tmp) != 0) {
        fprintf(stderr, "gris: install: не могу распаковать %s\n", gris_file);
        fs_rm_rf(tmp);
        return -1;
    }

    char pi_path[4096], fl_path[4096];
    snprintf(pi_path, sizeof(pi_path), "%s/.PKGINFO",  tmp);
    snprintf(fl_path, sizeof(fl_path), "%s/.FILELIST", tmp);

    PkgInfo pi;
    if (pkginfo_parse(pi_path, &pi) != 0 || !pi.name || !pi.version) {
        fprintf(stderr, "gris: install: нет корректного .PKGINFO в %s\n", gris_file);
        pkginfo_free(&pi);
        fs_rm_rf(tmp);
        return -1;
    }

    if (db_is_installed(pi.name)) {
        fprintf(stderr, "gris: install: %s уже установлен\n", pi.name);
        pkginfo_free(&pi);
        fs_rm_rf(tmp);
        return -1;
    }

    Vec files;
    vec_init(&files, sizeof(FileEntry));
    if (filelist_parse(fl_path, &files) != 0) {
        fprintf(stderr, "gris: install: нет .FILELIST в %s\n", gris_file);
        pkginfo_free(&pi);
        fs_rm_rf(tmp);
        return -1;
    }

    for (size_t i = 0; i < files.len; i++) {
        FileEntry *fe = vec_get(&files, i);

        char src[4096], dst[4096];
        snprintf(src, sizeof(src), "%s/%s", tmp,        fe->path);
        snprintf(dst, sizeof(dst), "%s/%s", g_cfg.root, fe->path);

        char *slash = strrchr(dst, '/');
        if (slash) {
            *slash = '\0';
            fs_mkdir_p(dst, 0755);
            *slash = '/';
        }

        struct stat st;
        if (stat(src, &st) < 0) {
            fprintf(stderr, "gris: install: файл из FILELIST не найден: %s\n",
                    fe->path);
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            fs_mkdir_p(dst, (unsigned)(st.st_mode & 07777));
        } else {
            if (fs_copy(src, dst) != 0)
                fprintf(stderr, "gris: install: не могу скопировать %s\n",
                        fe->path);
        }
    }

    char *pkg_dir = db_pkg_dir(pi.name);
    fs_mkdir_p(pkg_dir, 0755);

    char db_pi[4096], db_fl[4096];
    snprintf(db_pi, sizeof(db_pi), "%s/PKGINFO",  pkg_dir);
    snprintf(db_fl, sizeof(db_fl), "%s/FILELIST", pkg_dir);
    fs_copy(pi_path, db_pi);
    fs_copy(fl_path, db_fl);

    printf("installed: %s %s\n", pi.name, pi.version);

    for (size_t i = 0; i < files.len; i++) {
        FileEntry *fe = vec_get(&files, i);
        free(fe->path);
        free(fe->hash);
    }
    vec_free(&files);
    free(pkg_dir);
    pkginfo_free(&pi);
    fs_rm_rf(tmp);
    return 0;
}

int cmd_install(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "gris: install: укажите .gris файл\n");
        return 2;
    }

    /* Разрешаем граф зависимостей для всех входных файлов. */
    Vec nodes;
    vec_init(&nodes, sizeof(DepNode));

    for (int i = 0; i < argc; i++) {
        if (deps_resolve(argv[i], &nodes) != 0) {
            vec_free(&nodes);
            return 1;
        }
    }

    /* Устанавливаем в порядке появления (зависимости — уже первыми). */
    int rc = 0;
    for (size_t i = 0; i < nodes.len; i++) {
        DepNode *n = vec_get(&nodes, i);
        if (install_one(n->path) != 0) rc = 1;
    }

    vec_free(&nodes);
    return rc;
}

/* ── remove ──────────────────────────────────────────────────── */
static int remove_one(const char *name) {
    if (!db_is_installed(name)) {
        fprintf(stderr, "gris: remove: пакет '%s' не установлен\n", name);
        return -1;
    }

    Vec files;
    vec_init(&files, sizeof(FileEntry));
    if (db_read_filelist(name, &files) != 0) {
        fprintf(stderr, "gris: remove: не могу прочитать FILELIST пакета '%s'\n", name);
        vec_free(&files);
        return -1;
    }

    for (size_t i = 0; i < files.len; i++) {
        FileEntry *fe = vec_get(&files, i);
        char path[4096];
        snprintf(path, sizeof(path), "%s/%s", g_cfg.root, fe->path);

        if (unlink(path) != 0 && errno != ENOENT)
            fprintf(stderr, "gris: remove: %s: %s\n", path, strerror(errno));

        free(fe->path);
        free(fe->hash);
    }
    vec_free(&files);

    char *dir = db_pkg_dir(name);
    fs_rm_rf(dir);
    free(dir);

    printf("removed: %s\n", name);
    return 0;
}

int cmd_remove(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "gris: remove: укажите имя пакета\n");
        return 2;
    }

    int rc = 0;
    for (int i = 0; i < argc; i++)
        if (remove_one(argv[i]) != 0) rc = 1;
    return rc;
}