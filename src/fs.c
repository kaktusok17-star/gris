#include "fs.h"
#include "util.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int fs_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

int fs_mkdir_p(const char *path, unsigned mode) {
    if (!*path) return -1;
    char  *tmp = xstrdup(path);
    size_t n   = strlen(tmp);

    while (n > 1 && tmp[n - 1] == '/') tmp[--n] = '\0';

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, mode) < 0 && errno != EEXIST) {
                free(tmp); return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, mode) < 0 && errno != EEXIST) {
        free(tmp); return -1;
    }
    free(tmp);
    return 0;
}

int fs_rm_rf(const char *path) {
    struct stat st;
    if (lstat(path, &st) < 0) {
        if (errno == ENOENT) return 0;
        return -1;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *d = opendir(path);
        if (!d) return -1;

        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
            size_t n = strlen(path) + 1 + strlen(e->d_name) + 1;
            char  *sub = xmalloc(n);
            snprintf(sub, n, "%s/%s", path, e->d_name);
            fs_rm_rf(sub);
            free(sub);
        }
        closedir(d);
        return rmdir(path);
    }
    return unlink(path);
}

int fs_copy(const char *src, const char *dst) {
    int in = open(src, O_RDONLY);
    if (in < 0) return -1;

    struct stat st;
    if (fstat(in, &st) < 0) { close(in); return -1; }

    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 07777);
    if (out < 0) { close(in); return -1; }

    char    buf[65536];
    ssize_t n;
    while ((n = read(in, buf, sizeof(buf))) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = write(out, buf + written, (size_t)(n - written));
            if (w < 0) { close(in); close(out); return -1; }
            written += w;
        }
    }
    close(in);
    close(out);
    return n < 0 ? -1 : 0;
}