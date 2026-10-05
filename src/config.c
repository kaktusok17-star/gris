#include "config.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GrisConfig g_cfg;

static char *path_join(const char *a, const char *b) {
    size_t la = strlen(a);
    int need_slash = (la > 0 && a[la - 1] != '/');
    size_t n = la + (need_slash ? 1 : 0) + strlen(b) + 1;
    char *p = xmalloc(n);
    snprintf(p, n, "%s%s%s", a, need_slash ? "/" : "", b);
    return p;
}

void config_init(void) {
    const char *root = getenv("GRIS_ROOT");
    g_cfg.root = xstrdup((root && *root) ? root : "/");

    const char *db = getenv("GRIS_DB");
    g_cfg.db_dir = (db && *db) ? xstrdup(db)
                               : path_join(g_cfg.root, "var/lib/gris");

    const char *repo = getenv("GRIS_REPO");
    g_cfg.repo_url = xstrdup((repo && *repo)
        ? repo
        : "https://kaktusok17-star.github.io/aethel-repo/core/x86_64");

    const char *dbg = getenv("GRIS_DEBUG");
    g_cfg.debug = (dbg && *dbg && strcmp(dbg, "0") != 0);

    gris_debug = g_cfg.debug;

    log_debug("root     = %s", g_cfg.root);
    log_debug("db_dir   = %s", g_cfg.db_dir);
    log_debug("repo_url = %s", g_cfg.repo_url);
}

void config_free(void) {
    free(g_cfg.root);
    free(g_cfg.db_dir);
    free(g_cfg.repo_url);
    g_cfg.root = g_cfg.db_dir = g_cfg.repo_url = NULL;
}