#include "shell.h"
#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int shell_run(char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) die("fork: %s", strerror(errno));

    if (pid == 0) {
        execvp(argv[0], argv);
        fprintf(stderr, "gris: exec %s: %s\n", argv[0], strerror(errno));
        _exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) < 0)
        die("waitpid: %s", strerror(errno));

    if (WIFEXITED(status))   return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return -1;
}

int shell_capture(char *const argv[], char **out, size_t *out_len) {
    int fds[2];
    if (pipe(fds) < 0) die("pipe: %s", strerror(errno));

    pid_t pid = fork();
    if (pid < 0) die("fork: %s", strerror(errno));

    if (pid == 0) {
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        close(fds[1]);
        execvp(argv[0], argv);
        fprintf(stderr, "gris: exec %s: %s\n", argv[0], strerror(errno));
        _exit(127);
    }

    close(fds[1]);

    size_t cap = 4096, len = 0;
    char  *buf = xmalloc(cap);
    ssize_t n;
    while ((n = read(fds[0], buf + len, cap - len - 1)) > 0) {
        len += (size_t)n;
        if (len + 1 >= cap) {
            cap *= 2;
            buf = xrealloc(buf, cap);
        }
    }
    close(fds[0]);
    buf[len] = '\0';

    int status;
    waitpid(pid, &status, 0);

    *out = buf;
    if (out_len) *out_len = len;

    if (WIFEXITED(status)) return WEXITSTATUS(status);
    return -1;
}