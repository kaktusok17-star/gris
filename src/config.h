#ifndef GRIS_CONFIG_H
#define GRIS_CONFIG_H

typedef struct {
    char *root;       /* GRIS_ROOT   — по умолчанию "/"        */
    char *db_dir;     /* GRIS_DB     — по умолчанию $root/var/lib/gris */
    char *repo_url;   /* GRIS_REPO   — URL репозитория          */
    int   debug;      /* GRIS_DEBUG=1 — подробный лог           */
} GrisConfig;

extern GrisConfig g_cfg;

void config_init(void);
void config_free(void);

#endif