#ifndef GRIS_SHELL_H
#define GRIS_SHELL_H

#include <stddef.h>

/* Запустить внешнюю команду, дождаться окончания. Вернуть код выхода. */
int shell_run(char *const argv[]);

/* Запустить, поймать stdout. *out — malloc'd, вызывающий free(). */
int shell_capture(char *const argv[], char **out, size_t *out_len);

#endif