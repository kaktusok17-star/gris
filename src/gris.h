#ifndef GRIS_H
#define GRIS_H

#define GRIS_VERSION "0.1.0"

/* Все команды возвращают код выхода процесса. */
int cmd_sync   (int argc, char **argv);
int cmd_install(int argc, char **argv);
int cmd_remove (int argc, char **argv);
int cmd_upgrade(int argc, char **argv);
int cmd_info   (int argc, char **argv);
int cmd_list   (int argc, char **argv);
int cmd_search (int argc, char **argv);
int cmd_files  (int argc, char **argv);
int cmd_clean  (int argc, char **argv);

#endif