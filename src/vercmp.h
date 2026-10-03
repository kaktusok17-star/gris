#ifndef GRIS_VERCMP_H
#define GRIS_VERCMP_H

/* Сравнить две версии в стиле rpmvercmp:
   возвращает <0 если a<b, 0 если равны, >0 если a>b. */
int vercmp(const char *a, const char *b);

#endif