#include "download.h"
#include "fs.h"
#include "shell.h"

#include <string.h>

int download_to_file(const char *url, const char *dest) {
    /* file:// — простое копирование, без внешних утилит */
    if (strncmp(url, "file://", 7) == 0) {
        const char *src = url + 7;
        return fs_copy(src, dest) == 0 ? 0 : -1;
    }

    /* HTTP/HTTPS — через curl с прогресс-баром. */
    char *argv[] = {
        "curl", "-fL", "--progress-bar",
        "--output", (char *)dest,
        (char *)url, NULL
    };
    return shell_run(argv) == 0 ? 0 : -1;
}