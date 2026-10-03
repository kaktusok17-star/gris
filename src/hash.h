#ifndef GRIS_HASH_H
#define GRIS_HASH_H

/* SHA-256 файла в hex. out_hex — минимум 65 байт (64 + NUL).
   0 = ok, -1 = ошибка. */
int hash_file_sha256(const char *path, char out_hex[65]);

#endif