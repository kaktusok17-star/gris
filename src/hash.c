#include "hash.h"
#include "shell.h"
#include "util.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int hash_file_sha256(const char *path, char out_hex[65]) {
#if defined(__APPLE__)
    char *argv[] = { "shasum", "-a", "256", (char *)path, NULL };
#else
    char *argv[] = { "sha256sum", (char *)path, NULL };
#endif

    char  *out = NULL;
    int    rc  = shell_capture(argv, &out, NULL);
    if (rc != 0) { free(out); return -1; }

    char  *p = out;
    while (*p == ' ' || *p == '\t') p++;

    size_t i = 0;
    while (i < 64 && isxdigit((unsigned char)p[i])) i++;
    if (i != 64) { free(out); return -1; }

    memcpy(out_hex, p, 64);
    out_hex[64] = '\0';
    free(out);
    return 0;
}