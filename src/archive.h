#ifndef GRIS_ARCHIVE_H
#define GRIS_ARCHIVE_H

/* Распаковать .gris в destdir. 0 = ok, -1 = ошибка. */
int archive_extract(const char *gris_file, const char *destdir);

#endif