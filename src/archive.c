#include "archive.h"
#include "shell.h"

int archive_extract(const char *gris_file, const char *destdir) {
    char *argv[] = {
        "tar", "-xJf", (char *)gris_file, "-C", (char *)destdir, NULL
    };
    return shell_run(argv) == 0 ? 0 : -1;
}