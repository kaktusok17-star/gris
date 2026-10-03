#include <stdio.h>
#include <string.h>

#define GRIS_VERSION "0.1.0"

static void usage(FILE *out) {
    fprintf(out,
        "gris %s — пакетный менеджер Aethel Linux\n"
        "\n"
        "Использование:\n"
        "  gris <команда> [аргументы]\n"
        "\n"
        "Опции:\n"
        "  -h, --help       эта справка\n"
        "  -v, --version    версия\n",
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

    fprintf(stderr, "gris: неизвестная команда: %s\n", cmd);
    usage(stderr);
    return 2;
}