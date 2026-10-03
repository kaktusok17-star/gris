#ifndef GRIS_DOWNLOAD_H
#define GRIS_DOWNLOAD_H

/* Скачать URL в файл dest. 0 = ok, -1 = ошибка. */
int download_to_file(const char *url, const char *dest);

#endif