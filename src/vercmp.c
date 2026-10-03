#include "vercmp.h"

#include <ctype.h>
#include <string.h>

static const char *skip_seps(const char *p) {
    while (*p && !isalnum((unsigned char)*p)) p++;
    return p;
}

int vercmp(const char *a, const char *b) {
    while (*a && *b) {
        a = skip_seps(a);
        b = skip_seps(b);
        if (!*a || !*b) break;

        int a_digit = isdigit((unsigned char)*a);
        int b_digit = isdigit((unsigned char)*b);

        if (a_digit && b_digit) {
            const char *a0 = a;
            const char *b0 = b;
            while (isdigit((unsigned char)*a)) a++;
            while (isdigit((unsigned char)*b)) b++;

            while (a0 < a && *a0 == '0') a0++;
            while (b0 < b && *b0 == '0') b0++;

            size_t alen = (size_t)(a - a0);
            size_t blen = (size_t)(b - b0);

            if (alen != blen) return alen < blen ? -1 : 1;
            int c = strncmp(a0, b0, alen);
            if (c) return c < 0 ? -1 : 1;
        } else if (!a_digit && !b_digit) {
            const char *a0 = a;
            const char *b0 = b;
            while (isalpha((unsigned char)*a)) a++;
            while (isalpha((unsigned char)*b)) b++;

            size_t alen = (size_t)(a - a0);
            size_t blen = (size_t)(b - b0);
            size_t m    = alen < blen ? alen : blen;

            int c = strncmp(a0, b0, m);
            if (c) return c < 0 ? -1 : 1;
            if (alen != blen) return alen < blen ? -1 : 1;
        } else {
            /* цифра vs буква: цифра больше */
            return a_digit ? 1 : -1;
        }
    }

    a = skip_seps(a);
    b = skip_seps(b);

    if (*a && !*b) return  1;
    if (!*a && *b) return -1;
    return 0;
}