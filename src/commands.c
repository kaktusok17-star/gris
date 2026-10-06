#include "gris.h"
#include "archive.h"
#include "config.h"
#include "db.h"
#include "deps.h"
#include "download.h"
#include "fs.h"
#include "hash.h"
#include "index.h"
#include "pkg.h"
#include "shell.h"
#include "util.h"
#include "vercmp.h"

#include <dirent.h>
#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int g_force_remove = 0;

int cmd_sync(int argc, char **argv) {
    (void)argc; (void)argv;
    char *repo_dir = db_path("repo");
    if (fs_mkdir_p(repo_dir, 0755) != 0) {
        fprintf(stderr, "gris: cannot create %s\n", repo_dir);
        free(repo_dir); return 1;
    }
    char url[4096];
    snprintf(url, sizeof(url), "%s/core.index", g_cfg.repo_url);
    char index_path[4096];
    snprintf(index_path, sizeof(index_path), "%s/core.index", repo_dir);
    printf("syncing: %s\n", url);
    if (download_to_file(url, index_path) != 0) {
        fprintf(stderr, "gris: cannot download index\n");
        free(repo_dir); return 1;
    }
    printf("ok\n");
    free(repo_dir);
    return 0;
}

int cmd_clean(int argc, char **argv) {
    (void)argc; (void)argv;
    char *cache = db_path("cache");
    if (fs_exists(cache) && fs_rm_rf(cache) != 0) {
        fprintf(stderr, "gris: cannot clean %s\n", cache);
        free(cache); return 1;
    }
    free(cache);
    printf("cache cleaned\n");
    return 0;
}

int cmd_search(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "gris: search: regex required\n");
        return 2;
    }
    regex_t re;
    if (regcomp(&re, argv[0], REG_EXTENDED | REG_NOSUB) != 0) {
        fprintf(stderr, "gris: invalid regex: %s\n", argv[0]);
        return 2;
    }
    char *index_path = db_path("repo/core.index");
    Index idx;
    if (index_load(index_path, &idx) != 0) {
        fprintf(stderr, "gris: cannot read %s. Run 'gris sync'.\n", index_path);
        free(index_path); regfree(&re); return 1;
    }
    free(index_path);
    int found = 0;
    for (size_t i = 0; i < idx.entries.len; i++) {
        IndexEntry *e = *(IndexEntry **)vec_get(&idx.entries, i);
        if (regexec(&re, e->name, 0, NULL, 0) == 0) {
            printf("%-20s %-12s %ld\n", e->name, e->version, e->size);
            found++;
        }
    }
    index_free(&idx);
    regfree(&re);
    return found > 0 ? 0 : 1;
}

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

int cmd_info(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "gris: info: package name required\n");
        return 2;
    }
    PkgInfo pi;
    if (db_read_pkginfo(argv[0], &pi) != 0) {
        fprintf(stderr, "gris: info: package '%s' is not installed\n", argv[0]);
        return 1;
    }
    pkginfo_print(&pi);
    pkginfo_free(&pi);
    return 0;
}

static int files_from_db(const char *name, Vec *out) {
    return db_read_filelist(name, out);
}

static int files_from_repo(const char *name, Vec *out) {
    char *index_path = db_path("repo/core.index");
    Index idx;
    if (index_load(index_path, &idx) != 0) {
        fprintf(stderr,
            "gris: files: package '%s' not installed, no index. Run 'gris sync'.\n",
            name);
        free(index_path); return -1;
    }
    free(index_path);
    IndexEntry *e = index_find(&idx, name);
    if (!e) {
        fprintf(stderr, "gris: files: package '%s' is not installed and not in index\n", name);
        index_free(&idx);
        return -1;
    }
    char *cache_dir = db_path("cache");
    fs_mkdir_p(cache_dir, 0755);
    const char *basename = strrchr(e->url, '/');
    basename = basename ? basename + 1 : e->url;
    char cached[4096];
    snprintf(cached, sizeof(cached), "%s/%s", cache_dir, basename);
    if (!fs_exists(cached)) {
        printf("downloading: %s\n", e->url);
        if (download_to_file(e->url, cached) != 0) {
            fprintf(stderr, "gris: cannot download %s\n", e->url);
            free(cache_dir); index_free(&idx); return -1;
        }
    }
    free(cache_dir);
    char *targv[] = { "tar", "-xOJf", cached, "--wildcards", "*FILELIST", NULL };
    char *text = NULL;
    if (shell_capture(targv, &text, NULL) != 0) {
        fprintf(stderr, "gris: cannot read .FILELIST from %s\n", cached);
        free(text); index_free(&idx); return -1;
    }
    filelist_parse_buf(text, out);
    free(text);
    index_free(&idx);
    return 0;
}

int cmd_files(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "gris: files: package name required\n");
        return 2;
    }
    const char *name = argv[0];
    Vec v;
    vec_init(&v, sizeof(FileEntry));
    int rc = db_is_installed(name) ? files_from_db(name, &v) : files_from_repo(name, &v);
    if (rc != 0) { vec_free(&v); return 1; }
    for (size_t i = 0; i < v.len; i++) {
        FileEntry *fe = vec_get(&v, i);
        printf("%s\n", fe->path);
        free(fe->path); free(fe->hash);
    }
    vec_free(&v);
    return 0;
}

static int verify_sidecar_sha256(const char *gris_file) {
    char sidecar[4096];
    snprintf(sidecar, sizeof(sidecar), "%s.sha256", gris_file);
    if (!fs_exists(sidecar)) return 0;
    char expected[65];
    FILE *f = fopen(sidecar, "r");
    if (!f) return 0;
    int got = fscanf(f, "%64s", expected);
    fclose(f);
    if (got != 1) {
        fprintf(stderr, "gris: empty .sha256: %s\n", sidecar);
        return -1;
    }
    char actual[65];
    if (hash_file_sha256(gris_file, actual) != 0) {
        fprintf(stderr, "gris: cannot compute sha256 for %s\n", gris_file);
        return -1;
    }
    if (strcmp(expected, actual) != 0) {
        fprintf(stderr, "gris: sha256 mismatch for %s\n", gris_file);
        fprintf(stderr, "  expected: %s\n", expected);
        fprintf(stderr, "  got:      %s\n", actual);
        return -1;
    }
    return 0;
}

static int install_one(const char *gris_file) {
    if (!fs_exists(gris_file)) {
        fprintf(stderr, "gris: install: file not found: %s\n", gris_file);
        return -1;
    }
    if (verify_sidecar_sha256(gris_file) != 0) return -1;
    char tmp[] = "/tmp/gris-install-XXXXXX";
    if (!mkdtemp(tmp)) {
        fprintf(stderr, "gris: mkdtemp: %s\n", strerror(errno));
        return -1;
    }
    if (archive_extract(gris_file, tmp) != 0) {
        fprintf(stderr, "gris: install: cannot unpack %s\n", gris_file);
        fs_rm_rf(tmp); return -1;
    }
    char pi_path[4096], fl_path[4096];
    snprintf(pi_path, sizeof(pi_path), "%s/.PKGINFO",  tmp);
    snprintf(fl_path, sizeof(fl_path), "%s/.FILELIST", tmp);
    PkgInfo pi;
    if (pkginfo_parse(pi_path, &pi) != 0 || !pi.name || !pi.version) {
        fprintf(stderr, "gris: install: no valid .PKGINFO in %s\n", gris_file);
        pkginfo_free(&pi); fs_rm_rf(tmp); return -1;
    }
    if (db_is_installed(pi.name)) {
        fprintf(stderr, "gris: install: %s is already installed\n", pi.name);
        pkginfo_free(&pi); fs_rm_rf(tmp); return -1;
    }
    Vec files;
    vec_init(&files, sizeof(FileEntry));
    if (filelist_parse(fl_path, &files) != 0) {
        fprintf(stderr, "gris: install: no .FILELIST in %s\n", gris_file);
        pkginfo_free(&pi); fs_rm_rf(tmp); return -1;
    }
    for (size_t i = 0; i < files.len; i++) {
        FileEntry *fe = vec_get(&files, i);
        char src[4096], dst[4096];
        snprintf(src, sizeof(src), "%s/%s", tmp,        fe->path);
        snprintf(dst, sizeof(dst), "%s/%s", g_cfg.root, fe->path);
        char *slash = strrchr(dst, '/');
        if (slash) { *slash = '\0'; fs_mkdir_p(dst, 0755); *slash = '/'; }
        struct stat st;
        if (lstat(src, &st) < 0) {
            fprintf(stderr, "gris: install: file from FILELIST missing: %s\n", fe->path);
            continue;
        }
        if (S_ISLNK(st.st_mode)) {
            char target[4096];
            ssize_t n = readlink(src, target, sizeof(target) - 1);
            if (n > 0) {
                target[n] = '\0';
                unlink(dst);
                if (symlink(target, dst) != 0)
                    fprintf(stderr, "gris: install: cannot symlink %s\n", fe->path);
            }
        } else if (S_ISDIR(st.st_mode)) {
            fs_mkdir_p(dst, (unsigned)(st.st_mode & 07777));
        } else {
            if (fs_copy(src, dst) != 0)
                fprintf(stderr, "gris: install: cannot copy %s\n", fe->path);
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
        free(fe->path); free(fe->hash);
    }
    vec_free(&files);
    free(pkg_dir);
    pkginfo_free(&pi);
    fs_rm_rf(tmp);
    return 0;
}

static int install_by_name(const char *name) {
    if (db_is_installed(name)) {
        fprintf(stderr, "gris: install: %s is already installed\n", name);
        return -1;
    }
    char *index_path = db_path("repo/core.index");
    Index idx;
    if (index_load(index_path, &idx) != 0) {
        fprintf(stderr, "gris: cannot read %s. Run 'gris sync'.\n", index_path);
        free(index_path); return -1;
    }
    free(index_path);
    Vec order;
    vec_init(&order, sizeof(IndexEntry *));
    if (deps_resolve_index(&idx, name, &order) != 0) {
        vec_free(&order); index_free(&idx); return -1;
    }
    char *cache_dir = db_path("cache");
    fs_mkdir_p(cache_dir, 0755);
    int rc = 0;
    for (size_t i = 0; i < order.len; i++) {
        IndexEntry *e = *(IndexEntry **)vec_get(&order, i);
        if (db_is_installed(e->name)) continue;
        const char *basename = strrchr(e->url, '/');
        basename = basename ? basename + 1 : e->url;
        char cached[4096];
        snprintf(cached, sizeof(cached), "%s/%s", cache_dir, basename);
        if (!fs_exists(cached)) {
            printf("downloading: %s\n", e->url);
            if (download_to_file(e->url, cached) != 0) {
                fprintf(stderr, "gris: cannot download %s\n", e->url);
                rc = 1; break;
            }
        }
        if (e->sha256 && *e->sha256) {
            char sidecar[4200];
            snprintf(sidecar, sizeof(sidecar), "%s.sha256", cached);
            FILE *f = fopen(sidecar, "w");
            if (f) { fprintf(f, "%s\n", e->sha256); fclose(f); }
        }
        if (install_one(cached) != 0) { rc = 1; break; }
    }
    free(cache_dir);
    vec_free(&order);
    index_free(&idx);
    return rc;
}

int cmd_install(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "gris: install: specify .gris file or package name\n");
        return 2;
    }
    int rc = 0;
    for (int i = 0; i < argc; i++) {
        if (ends_with(argv[i], ".gris")) {
            Vec nodes;
            vec_init(&nodes, sizeof(DepNode));
            if (deps_resolve(argv[i], &nodes) != 0) {
                vec_free(&nodes); rc = 1; continue;
            }
            for (size_t j = 0; j < nodes.len; j++) {
                DepNode *n = vec_get(&nodes, j);
                if (install_one(n->path) != 0) rc = 1;
            }
            vec_free(&nodes);
        } else {
            if (install_by_name(argv[i]) != 0) rc = 1;
        }
    }
    return rc;
}

static void find_dependents(const char *target, Vec *dependents) {
    Vec installed;
    vec_init(&installed, sizeof(char *));
    db_list_installed(&installed);
    for (size_t i = 0; i < installed.len; i++) {
        char *name = *(char **)vec_get(&installed, i);
        if (strcmp(name, target) == 0) { free(name); continue; }
        PkgInfo pi;
        if (db_read_pkginfo(name, &pi) == 0) {
            for (size_t j = 0; j < pi.ndepends; j++) {
                char dep_name[256];
                if (deps_parse_name(pi.depends[j], dep_name, sizeof(dep_name)) == 0
                    && strcmp(dep_name, target) == 0) {
                    *(char **)vec_push(dependents) = xstrdup(name);
                    break;
                }
            }
            pkginfo_free(&pi);
        }
        free(name);
    }
    vec_free(&installed);
}

static int remove_one(const char *name) {
    if (!db_is_installed(name)) {
        fprintf(stderr, "gris: remove: package '%s' is not installed\n", name);
        return -1;
    }
    if (!g_force_remove) {
        Vec dependents;
        vec_init(&dependents, sizeof(char *));
        find_dependents(name, &dependents);
        if (dependents.len > 0) {
            fprintf(stderr, "gris: remove: package '%s' is required by:\n", name);
            for (size_t i = 0; i < dependents.len; i++) {
                char *dep = *(char **)vec_get(&dependents, i);
                fprintf(stderr, "  - %s\n", dep);
                free(dep);
            }
            fprintf(stderr, "Use --force to remove anyway.\n");
            vec_free(&dependents);
            return -1;
        }
        vec_free(&dependents);
    }
    Vec files;
    vec_init(&files, sizeof(FileEntry));
    if (db_read_filelist(name, &files) != 0) {
        fprintf(stderr, "gris: remove: cannot read FILELIST of '%s'\n", name);
        vec_free(&files); return -1;
    }
    for (size_t i = 0; i < files.len; i++) {
        FileEntry *fe = vec_get(&files, i);
        char path[4096];
        snprintf(path, sizeof(path), "%s/%s", g_cfg.root, fe->path);
        if (unlink(path) != 0 && errno != ENOENT)
            fprintf(stderr, "gris: remove: %s: %s\n", path, strerror(errno));
        free(fe->path); free(fe->hash);
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
        fprintf(stderr, "gris: remove: package name required\n");
        return 2;
    }
    char *names[64];
    int   nnames = 0;
    for (int i = 0; i < argc && nnames < 64; i++) {
        if (strcmp(argv[i], "--force") == 0) {
            g_force_remove = 1;
        } else {
            names[nnames++] = argv[i];
        }
    }
    if (nnames == 0) {
        fprintf(stderr, "gris: remove: package name required\n");
        return 2;
    }
    int rc = 0;
    for (int i = 0; i < nnames; i++)
        if (remove_one(names[i]) != 0) rc = 1;
    return rc;
}

int cmd_upgrade(int argc, char **argv) {
    (void)argc; (void)argv;
    char *index_path = db_path("repo/core.index");
    Index idx;
    if (index_load(index_path, &idx) != 0) {
        fprintf(stderr, "gris: cannot read %s. Run 'gris sync'.\n", index_path);
        free(index_path); return 1;
    }
    free(index_path);
    Vec installed;
    vec_init(&installed, sizeof(char *));
    db_list_installed(&installed);
    int rc = 0;
    int upgraded = 0;
    for (size_t i = 0; i < installed.len; i++) {
        char *name = *(char **)vec_get(&installed, i);
        PkgInfo pi;
        if (db_read_pkginfo(name, &pi) != 0) { free(name); continue; }
        IndexEntry *e = index_find(&idx, name);
        if (!e) { pkginfo_free(&pi); free(name); continue; }
        if (vercmp(e->version, pi.version) > 0) {
            printf("upgrading: %s (%s -> %s)\n", name, pi.version, e->version);
            g_force_remove = 1;
            char *rargv[] = { name };
            if (cmd_remove(1, rargv) != 0) { rc = 1; upgraded = -1; }
            else if (install_by_name(name) != 0) { rc = 1; upgraded = -1; }
            else upgraded++;
            g_force_remove = 0;
        }
        pkginfo_free(&pi);
        free(name);
        if (upgraded < 0) break;
    }
    if (upgraded == 0)
        printf("everything up to date\n");
    vec_free(&installed);
    index_free(&idx);
    return rc;
}

static void build_walk(const char *dir, const char *prefix, FILE *flist) {
    DIR *d = opendir(dir);
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        char full[4096];
        snprintf(full, sizeof(full), "%s/%s", dir, e->d_name);
        char rel[4096];
        if (prefix[0])
            snprintf(rel, sizeof(rel), "%s/%s", prefix, e->d_name);
        else
            snprintf(rel, sizeof(rel), "%s", e->d_name);
        struct stat st;
        if (lstat(full, &st) < 0) continue;
        if (S_ISDIR(st.st_mode)) {
            build_walk(full, rel, flist);
        } else if (S_ISREG(st.st_mode)) {
            char hash[65];
            if (hash_file_sha256(full, hash) == 0)
                fprintf(flist, "%s  %s\n", hash, rel);
        }
    }
    closedir(d);
}

int cmd_build(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "gris: build: DESTDIR required\n");
        return 2;
    }
    const char *destdir = argv[0];
    char pi_path[4096];
    snprintf(pi_path, sizeof(pi_path), "%s/.PKGINFO", destdir);
    PkgInfo pi;
    if (pkginfo_parse(pi_path, &pi) != 0 || !pi.name || !pi.version) {
        fprintf(stderr, "gris: build: no valid .PKGINFO in %s\n", destdir);
        pkginfo_free(&pi); return 1;
    }
    char fl_path[4096];
    snprintf(fl_path, sizeof(fl_path), "%s/.FILELIST", destdir);
    FILE *fl = fopen(fl_path, "w");
    if (!fl) {
        fprintf(stderr, "gris: build: cannot create %s\n", fl_path);
        pkginfo_free(&pi); return 1;
    }
    build_walk(destdir, "", fl);
    fclose(fl);
    const char *arch = pi.arch ? pi.arch : "x86_64";
    char        output[4096];
    snprintf(output, sizeof(output), "%s-%s-%s.gris", pi.name, pi.version, arch);
    char *targv[] = {
        "tar", "-cJf", output, "-C", (char *)destdir, ".", NULL
    };
    printf("building: %s\n", output);
    int rc = shell_run(targv);
    if (rc != 0) {
        fprintf(stderr, "gris: build: tar returned %d\n", rc);
        pkginfo_free(&pi); return 1;
    }
    char final_hash[65];
    if (hash_file_sha256(output, final_hash) == 0)
        printf("sha256: %s\n", final_hash);
    printf("ok\n");
    pkginfo_free(&pi);
    return 0;
}