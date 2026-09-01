/*
 * test_compat.cpp — Tests unitaires qt6-compat.h
 * qt6sermo 1.0.0 — haplo-dialog — GPL-2.0-or-later
 *
 * Tests :
 *   1. g_strdup_printf()    → allocation + format correct
 *   2. g_strdup()           → copie correcte, NULL-safe
 *   3. g_strlcpy()          → borne correcte, null-terminé
 *   4. g_strlcat()          → borne correcte, null-terminé
 *   5. gint32/gint64 sizes  → _Static_assert vérifié à compile-time
 *   6. snprintf_safe()      → format complexe
 *   7. g_malloc/g_free      → pas de crash
 *   8. g_warning            → ne crashe pas (stderr)
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <stdint.h>

#include "../src/qt6-compat.h"

static int failures = 0;

#define EXPECT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL [%s:%d] %s\n", __FILE__, __LINE__, (msg)); \
            failures++; \
        } else { \
            printf("PASS  %s\n", (msg)); \
        } \
    } while (0)

int main()
{
    printf("=== test_compat (qt6-compat.h) ===\n");

    /* 1. g_strdup_printf */
    {
        gchar *s = g_strdup_printf("val=%d str=%s", 42, "hello");
        EXPECT(s != NULL, "g_strdup_printf returns non-NULL");
        EXPECT(strcmp(s, "val=42 str=hello") == 0, "g_strdup_printf correct format");
        g_free(s);
    }

    /* 2. g_strdup */
    {
        gchar *s = g_strdup("test");
        EXPECT(s != NULL, "g_strdup returns non-NULL");
        EXPECT(strcmp(s, "test") == 0, "g_strdup correct copy");
        g_free(s);

        gchar *n = g_strdup(NULL);
        EXPECT(n == NULL, "g_strdup(NULL) returns NULL");
    }

    /* 3. g_strlcpy */
    {
        char dst[8];
        gsize r = g_strlcpy(dst, "hello world", sizeof(dst));
        EXPECT(r == strlen("hello world"), "g_strlcpy returns source length");
        EXPECT(dst[7] == '\0', "g_strlcpy always null-terminates");
        EXPECT(strncmp(dst, "hello w", 7) == 0, "g_strlcpy truncates correctly");
    }

    /* 4. g_strlcat */
    {
        char dst[16];
        g_strlcpy(dst, "foo", sizeof(dst));
        gsize r = g_strlcat(dst, "bar", sizeof(dst));
        EXPECT(r == 6, "g_strlcat returns combined length");
        EXPECT(strcmp(dst, "foobar") == 0, "g_strlcat concatenates correctly");
    }

    /* 5. Type sizes (validated at compile-time by _Static_assert, runtime check here) */
    EXPECT(sizeof(gint32)  == 4, "sizeof(gint32) == 4");
    EXPECT(sizeof(guint32) == 4, "sizeof(guint32) == 4");
    EXPECT(sizeof(gint64)  == 8, "sizeof(gint64) == 8");
    EXPECT(sizeof(guint64) == 8, "sizeof(guint64) == 8");

    /* 6. snprintf_safe with complex format */
    {
        gchar *s = g_strdup_printf("%.3f %05d %s", 3.14159, 7, "end");
        EXPECT(s != NULL, "g_strdup_printf complex format non-NULL");
        EXPECT(strstr(s, "3.142") != NULL, "g_strdup_printf float format");
        EXPECT(strstr(s, "00007") != NULL, "g_strdup_printf zero-padded int");
        EXPECT(strstr(s, "end") != NULL, "g_strdup_printf string part");
        g_free(s);
    }

    /* 7. g_malloc / g_free */
    {
        gpointer p = g_malloc(128);
        EXPECT(p != NULL, "g_malloc(128) returns non-NULL");
        memset(p, 0xAB, 128);
        g_free(p);
        printf("PASS  g_free without crash\n");

        gpointer p0 = g_malloc0(64);
        EXPECT(p0 != NULL, "g_malloc0(64) returns non-NULL");
        char *c = (char*)p0;
        int all_zero = 1;
        for (int i = 0; i < 64; i++) if (c[i] != 0) { all_zero = 0; break; }
        EXPECT(all_zero, "g_malloc0 zero-initializes");
        g_free(p0);
    }

    /* 8. g_warning — must not crash */
    g_warning("test warning %d", 99);
    printf("PASS  g_warning does not crash\n");

    /* 9. g_return_if_fail / g_return_val_if_fail (compile-time) */
    /* Ces macros sont juste des assertions — on vérifie qu'elles compilent */
    {
        auto fn = [](int x) -> int {
            g_return_val_if_fail(x >= 0, -1);
            return x * 2;
        };
        EXPECT(fn(5) == 10,  "g_return_val_if_fail: passes when condition true");
        EXPECT(fn(-1) == -1, "g_return_val_if_fail: returns early when false");
    }

    printf("\n=== Résultat : %d échec(s) ===\n", failures);
    return failures > 0 ? 1 : 0;
}
