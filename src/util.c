#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Логгер ─────────────────────────────────────────────────── */
int gris_debug = 0;

static void vmsg(const char *prefix, const char *fmt, va_list ap) {
    fputs(prefix, stderr);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
}

void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vmsg("gris: error: ", fmt, ap);
    va_end(ap);
    exit(1);
}

void warn(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vmsg("gris: warning: ", fmt, ap);
    va_end(ap);
}

void log_debug(const char *fmt, ...) {
    if (!gris_debug) return;
    va_list ap;
    va_start(ap, fmt);
    vmsg("gris: debug: ", fmt, ap);
    va_end(ap);
}

/* ── Память ─────────────────────────────────────────────────── */
void *xmalloc(size_t n) {
    if (n == 0) n = 1;
    void *p = malloc(n);
    if (!p) die("out of memory (%zu bytes)", n);
    return p;
}

void *xcalloc(size_t nmemb, size_t size) {
    void *p = calloc(nmemb, size);
    if (!p) die("out of memory (%zu * %zu bytes)", nmemb, size);
    return p;
}

void *xrealloc(void *p, size_t n) {
    if (n == 0) n = 1;
    void *q = realloc(p, n);
    if (!q) die("out of memory (%zu bytes)", n);
    return q;
}

char *xstrdup(const char *s) {
    char *p = strdup(s);
    if (!p) die("out of memory");
    return p;
}

char *xstrndup(const char *s, size_t n) {
    char *p = strndup(s, n);
    if (!p) die("out of memory");
    return p;
}

/* ── Вектор ─────────────────────────────────────────────────── */
void vec_init(Vec *v, size_t elem_size) {
    v->data = NULL;
    v->len  = 0;
    v->cap  = 0;
    v->elem = elem_size;
}

void *vec_push(Vec *v) {
    if (v->len == v->cap) {
        v->cap = v->cap ? v->cap * 2 : 16;
        v->data = xrealloc(v->data, v->cap * v->elem);
    }
    void *slot = (char *)v->data + v->len * v->elem;
    memset(slot, 0, v->elem);
    v->len++;
    return slot;
}

void *vec_get(const Vec *v, size_t i) {
    if (i >= v->len)
        die("vec_get: index %zu out of range (%zu)", i, v->len);
    return (char *)v->data + i * v->elem;
}

void vec_free(Vec *v) {
    free(v->data);
    v->data = NULL;
    v->len = v->cap = 0;
}

/* ── Строки ─────────────────────────────────────────────────── */
char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *end = s + strlen(s);
    while (end > s && (end[-1] == ' '  || end[-1] == '\t' ||
                       end[-1] == '\n' || end[-1] == '\r'))
        *--end = '\0';
    return s;
}

int starts_with(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

int ends_with(const char *s, const char *suffix) {
    size_t ls = strlen(s), lf = strlen(suffix);
    return ls >= lf && strcmp(s + ls - lf, suffix) == 0;
}