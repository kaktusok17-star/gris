#include "download.h"
#include "shell.h"

int download_to_file(const char *url, const char *dest) {
    char *argv[] = {
        "curl", "-fsSL", "-o", (char *)dest, (char *)url, NULL
    };
    return shell_run(argv) == 0 ? 0 : -1;
}