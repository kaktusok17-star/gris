#ifndef GRIS_FS_H
#define GRIS_FS_H

int fs_exists (const char *path);
int fs_mkdir_p(const char *path, unsigned mode);
int fs_rm_rf  (const char *path);
int fs_copy   (const char *src, const char *dst);

#endif