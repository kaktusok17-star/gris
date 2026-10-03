#include "gris.h"
#include "config.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

static void usage(FILE *out) {
    fprintf(out,
        "gris %s — пакетный менеджер Aethel Linux\n"
        "\n"
        "Использование:\n"
        "  gris <команда> [аргументы]\n"
        "\n"
        "Команды:\n"
        "  sync                 обновить индексы репозиториев\n"
        "  install <pkg>...     установить пакеты\n"
        "  remove  <pkg>...     удалить пакеты\n"
        "  upgrade              обновить установленные пакеты\n"
        "  info    <pkg>        показать информацию о пакете\n"
        "  list                 список установленных пакетов\n"
        "  search  <regex>      поиск в репозиториях\n"
        "  files   <pkg>        файлы пакета\n"
        "  clean                очистить кэш\n"
        "\n"
        "Опции:\n"
        "  -h, --help           эта справка\n"
        "  -v, --version        версия\n"
        "\n"
        "Переменные окружения:\n"
        "  GRIS_ROOT            корень системы (по умолчанию /)\n"
        "  GRIS_DB              каталог БД (по умолчанию $GRIS_ROOT/var/lib/gris)\n"
        "  GRIS_REPO            URL репозитория\n"
        "  GRIS_DEBUG=1         подробный лог\n",
        GRIS_VERSION);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(stderr);
        return 2;
    }

    const char *cmd = argv[1];
    if (!strcmp(cmd, "-h") || !strcmp(cmd, "--help")) {
        usage(stdout);
        return 0;
    }
    if (!strcmp(cmd, "-v") || !strcmp(cmd, "--version")) {
        printf("gris %s\n", GRIS_VERSION);
        return 0;
    }

    config_init();

    int n = argc - 2;
    char **args = argv + 2;
    int rc;

    if      (!strcmp(cmd, "sync"))    rc = cmd_sync(n, args);
    else if (!strcmp(cmd, "install")) rc = cmd_install(n, args);
    else if (!strcmp(cmd, "remove"))  rc = cmd_remove(n, args);
    else if (!strcmp(cmd, "upgrade")) rc = cmd_upgrade(n, args);
    else if (!strcmp(cmd, "info"))    rc = cmd_info(n, args);
    else if (!strcmp(cmd, "list"))    rc = cmd_list(n, args);
    else if (!strcmp(cmd, "search"))  rc = cmd_search(n, args);
    else if (!strcmp(cmd, "files"))   rc = cmd_files(n, args);
    else if (!strcmp(cmd, "clean"))   rc = cmd_clean(n, args);
    else {
        fprintf(stderr, "gris: неизвестная команда: %s\n", cmd);
        usage(stderr);
        config_free();
        return 2;
    }

    config_free();
    return rc;
}