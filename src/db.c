#include "db.h"
#include "config.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static char *join(const char *a, const char *b) {
    size_t la = strlen(a), lb = strlen(b);
    int need_slash = (la > 0 && a[la - 1] != '/');
    size_t n = la + (need_slash ? 1 : 0) + lb + 1;
    char *p = xmalloc(n);
    snprintf(p, n, "%s%s%s", a, need_slash ? "/" : "", b);
    return p;
}

char *db_path(const char *sub) {
    return join(g_cfg.db_dir, sub);
}

char *db_pkg_dir(const char *name) {
    char *installed = db_path("installed");
    char *dir       = join(installed, name);
    free(installed);
    return dir;
}

int db_list_installed(Vec *out) {
    char *dir = db_path("installed");
    DIR  *d   = opendir(dir);
    if (!d) { free(dir); return 0; }

    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;

        char *sub = join(dir, e->d_name);
        struct stat st;
        if (stat(sub, &st) == 0 && S_ISDIR(st.st_mode))
            *(char **)vec_push(out) = xstrdup(e->d_name);
        free(sub);
    }

    closedir(d);
    free(dir);
    return 0;
}

int db_read_pkginfo(const char *name, PkgInfo *out) {
    char *dir  = db_pkg_dir(name);
    char *path = join(dir, "PKGINFO");
    int   rc   = pkginfo_parse(path, out);
    free(path);
    free(dir);
    return rc;
}

int db_read_filelist(const char *name, Vec *out) {
    char *dir  = db_pkg_dir(name);
    char *path = join(dir, "FILELIST");
    int   rc   = filelist_parse(path, out);
    free(path);
    free(dir);
    return rc;
}

int db_is_installed(const char *name) {
    char *dir = db_pkg_dir(name);
    struct stat st;
    int   rc  = (stat(dir, &st) == 0 && S_ISDIR(st.st_mode));
    free(dir);
    return rc;
}