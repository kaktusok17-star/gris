#ifndef GRIS_UTIL_H
#define GRIS_UTIL_H

#include <stddef.h>

/* ── Логгер ─────────────────────────────────────────────────── */
extern int gris_debug;

#if defined(__GNUC__) || defined(__clang__)
#  define GRIS_PRINTF(a, b) __attribute__((format(printf, a, b)))
#  define GRIS_NORETURN    __attribute__((noreturn))
#else
#  define GRIS_PRINTF(a, b)
#  define GRIS_NORETURN
#endif

GRIS_NORETURN void die(const char *fmt, ...) GRIS_PRINTF(1, 2);
void warn(const char *fmt, ...) GRIS_PRINTF(1, 2);
void log_debug(const char *fmt, ...) GRIS_PRINTF(1, 2);

/* ── Память ─────────────────────────────────────────────────── */
void *xmalloc(size_t n);
void *xcalloc(size_t nmemb, size_t size);
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);
char *xstrndup(const char *s, size_t n);

/* ── Динамический массив ────────────────────────────────────── */
typedef struct {
    void   *data;
    size_t  len;
    size_t  cap;
    size_t  elem;
} Vec;

void  vec_init(Vec *v, size_t elem_size);
void *vec_push(Vec *v);
void *vec_get(const Vec *v, size_t i);
void  vec_free(Vec *v);

/* ── Строки ─────────────────────────────────────────────────── */
char *trim(char *s);
int   starts_with(const char *s, const char *prefix);
int   ends_with(const char *s, const char *suffix);

#endif