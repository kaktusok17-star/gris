#include "gris.h"
#include "config.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(FILE *out) {
    fprintf(out,
        "gris %s — Aethel Linux package manager\n"
        "\n"
        "Short flags (pacman-style):\n"
        "  -S <pkg>...       install from repository\n"
        "  -Sy               refresh repository index\n"
        "  -Syu              refresh and upgrade system\n"
        "  -Ss <regex>       search repository\n"
        "  -Sc               clean cache\n"
        "  -R <pkg>...       remove packages\n"
        "  -R --force <pkg>  remove forcefully\n"
        "  -Q                list installed\n"
        "  -Qi <pkg>         show package info\n"
        "  -Ql <pkg>         list package files\n"
        "\n"
        "Long commands:\n"
        "  install <pkg|file>   install\n"
        "  remove  <pkg>...     remove\n"
        "  upgrade              upgrade system\n"
        "  sync                 refresh index\n"
        "  search  <regex>      search\n"
        "  list                 list installed\n"
        "  info    <pkg>        show info\n"
        "  files   <pkg>        list files\n"
        "  build   <DESTDIR>    build .gris from tree\n"
        "  clean                clean cache\n"
        "\n"
        "Options:\n"
        "  -h, --help       this help\n"
        "  -v, --version    version\n"
        "\n"
        "Environment:\n"
        "  GRIS_ROOT        system root (default /)\n"
        "  GRIS_DB          db directory (default $GRIS_ROOT/var/lib/gris)\n"
        "  GRIS_REPO        repository URL\n"
        "  GRIS_DEBUG=1     verbose output\n",
        GRIS_VERSION);
}

static int collect_args(int argc, char **argv, int start,
                        char **out, int max, int *force) {
    int n = 0;
    *force = 0;
    for (int i = start; i < argc && n < max; i++) {
        if (strcmp(argv[i], "--force") == 0) {
            *force = 1;
        } else {
            out[n++] = argv[i];
        }
    }
    return n;
}

static int handle_short_flags(int argc, char **argv) {
    const char *flags = argv[1] + 1;

    char op = 0;
    int  refresh = 0, upgrade = 0, search = 0;
    int  info = 0, list = 0, clean = 0;

    for (const char *p = flags; *p; p++) {
        switch (*p) {
            case 'S': op = 'S'; break;
            case 'R': op = 'R'; break;
            case 'Q': op = 'Q'; break;
            case 'y': refresh = 1; break;
            case 'u': upgrade = 1; break;
            case 's': search  = 1; break;
            case 'i': info    = 1; break;
            case 'l': list    = 1; break;
            case 'c': clean   = 1; break;
            default:
                fprintf(stderr, "gris: unknown flag: -%c\n", *p);
                usage(stderr);
                return 2;
        }
    }

    char *args[64];
    int   force = 0;
    int   n = collect_args(argc, argv, 2, args, 64, &force);

    if (op == 'S') {
        if (clean)              return cmd_clean(n, args);
        if (refresh && upgrade) {
            int rc = cmd_sync(0, NULL);
            if (rc != 0) return rc;
            return cmd_upgrade(0, NULL);
        }
        if (refresh)            return cmd_sync(n, args);
        if (upgrade)            return cmd_upgrade(n, args);
        if (search)             return cmd_search(n, args);
        if (n == 0) {
            fprintf(stderr, "gris: -S requires a package name\n");
            return 2;
        }
        return cmd_install(n, args);
    }

    if (op == 'R') {
        if (n == 0) {
            fprintf(stderr, "gris: -R requires a package name\n");
            return 2;
        }
        if (force) {
            char *rargv[65];
            rargv[0] = "--force";
            for (int i = 0; i < n; i++) rargv[i + 1] = args[i];
            return cmd_remove(n + 1, rargv);
        }
        return cmd_remove(n, args);
    }

    if (op == 'Q') {
        if (info)               return cmd_info(n, args);
        if (list)               return cmd_files(n, args);
        return cmd_list(n, args);
    }

    fprintf(stderr, "gris: specify operation: -S, -R or -Q\n");
    return 2;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(stderr);
        return 2;
    }

    const char *cmd = argv[1];
    if (!strcmp(cmd, "-h") || !strcmp(cmd, "--help")) {
        usage(stdout);
        return 0;
    }
    if (!strcmp(cmd, "-v") || !strcmp(cmd, "--version")) {
        printf("gris %s\n", GRIS_VERSION);
        return 0;
    }

    config_init();

    int rc;

    if (cmd[0] == '-' && cmd[1] != '-' && cmd[1] != '\0') {
        rc = handle_short_flags(argc, argv);
    } else {
        int n = argc - 2;
        char **args = argv + 2;

        if      (!strcmp(cmd, "sync"))    rc = cmd_sync(n, args);
        else if (!strcmp(cmd, "install")) rc = cmd_install(n, args);
        else if (!strcmp(cmd, "remove"))  rc = cmd_remove(n, args);
        else if (!strcmp(cmd, "upgrade")) rc = cmd_upgrade(n, args);
        else if (!strcmp(cmd, "info"))    rc = cmd_info(n, args);
        else if (!strcmp(cmd, "list"))    rc = cmd_list(n, args);
        else if (!strcmp(cmd, "search"))  rc = cmd_search(n, args);
        else if (!strcmp(cmd, "files"))   rc = cmd_files(n, args);
        else if (!strcmp(cmd, "clean"))   rc = cmd_clean(n, args);
        else if (!strcmp(cmd, "build"))   rc = cmd_build(n, args);
        else {
            fprintf(stderr, "gris: unknown command: %s\n", cmd);
            usage(stderr);
            config_free();
            return 2;
        }
    }

    config_free();
    return rc;
}