#include "gris.h"

#include <stdio.h>

static int not_implemented(const char *name) {
    fprintf(stderr, "gris: %s: не реализовано (следующий шаг)\n", name);
    return 1;
}

int cmd_sync   (int argc, char **argv) { (void)argc; (void)argv; return not_implemented("sync");    }
int cmd_install(int argc, char **argv) { (void)argc; (void)argv; return not_implemented("install"); }
int cmd_remove (int argc, char **argv) { (void)argc; (void)argv; return not_implemented("remove");  }
int cmd_upgrade(int argc, char **argv) { (void)argc; (void)argv; return not_implemented("upgrade"); }
int cmd_info   (int argc, char **argv) { (void)argc; (void)argv; return not_implemented("info");    }
int cmd_list   (int argc, char **argv) { (void)argc; (void)argv; return not_implemented("list");    }
int cmd_search (int argc, char **argv) { (void)argc; (void)argv; return not_implemented("search");  }
int cmd_files  (int argc, char **argv) { (void)argc; (void)argv; return not_implemented("files");   }
int cmd_clean  (int argc, char **argv) { (void)argc; (void)argv; return not_implemented("clean");   }