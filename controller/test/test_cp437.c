/* Host-side checks for cp437.c: no ESP32, no panel.
 *
 *   ./test_cp437 layout        the 16-column layout rules, asserted here
 *   ./test_cp437 sevenbit      the 7-bit link rules, asserted here
 *   ./test_cp437 encode        stdin lines -> hex on stdout, for the Makefile
 *                              to diff against tools/pit100.py
 *
 * The split is deliberate. Plain CP437 encoding and the accent fold are checked
 * against the Python, because a typo in a 128-entry table is exactly what a
 * second implementation catches. The 7-bit behaviour is checked here instead,
 * because it is where this encoder intentionally differs: pit100.py never had to
 * refuse a character for being unable to cross the link.
 */

#include <stdio.h>
#include <string.h>

#include "cp437.h"

static int failures;

static void expect_layout(const char *spec, const char *want)
{
    char got[PANEL_CHARS + 1];
    layout(spec, got);
    if (strcmp(got, want) != 0) {
        printf("FAIL layout(\"%s\")\n  want \"%s\"\n  got  \"%s\"\n",
               spec, want, got);
        failures++;
    }
}

static void expect_hex(const char *text, bool seven_bit, const char *want)
{
    uint8_t buf[64];
    const char *why = NULL;
    int n = cp437_encode(text, buf, sizeof buf, seven_bit, &why);
    char got[200] = "";
    if (n < 0)
        snprintf(got, sizeof got, "REFUSED");
    else
        for (int i = 0; i < n; i++)
            snprintf(got + strlen(got), sizeof got - strlen(got),
                     i ? " %02X" : "%02X", buf[i]);
    if (strcmp(got, want) != 0) {
        printf("FAIL encode(\"%s\", seven_bit=%d)\n  want %s\n  got  %s\n",
               text, (int)seven_bit, want, got);
        failures++;
    }
}

static void test_layout(void)
{
    /* The two shipped clock presets, both exactly 16 columns with a single
     * space of gap -- which is what makes "18h24" land against the sticker. */
    expect_layout("LUN 13 SEP|18h24", "LUN 13 SEP 18h24");
    expect_layout("13-09-2026|18h24", "13-09-2026 18h24");

    /* The counter layout: label left, minutes hard right. */
    expect_layout("CHAMARTIN|7",      "CHAMARTIN      7");
    expect_layout("ATOCHA|12",        "ATOCHA        12");

    /* A leading bar right-aligns. */
    expect_layout("|18h24",           "           18h24");

    /* No bar: left-aligned, space padded. */
    expect_layout("HOLA",             "HOLA            ");
    expect_layout("",                 "                ");

    /* Exactly full, with and without a bar. */
    expect_layout("0123456789ABCDEF", "0123456789ABCDEF");
    expect_layout("0123456789A|CDEF", "0123456789A CDEF");

    /* Over-long without a bar: truncated. */
    expect_layout("0123456789ABCDEFGHI", "0123456789ABCDEF");

    /* The right field wins and at least one space is kept, so a long label can
     * never run into the number. */
    expect_layout("PRINCIPE PIO XX|7",  "PRINCIPE PIO X 7");
    expect_layout("ABCDEFGHIJKLMNOP|7", "ABCDEFGHIJKLMN 7");
    expect_layout("ABCDEFGHIJKLMNOP|12", "ABCDEFGHIJKLM 12");

    /* A right field that fills the line leaves no room for the label at all. */
    expect_layout("LABEL|0123456789ABCDEF", "0123456789ABCDEF");
}

static void test_sevenbit(void)
{
    /* The deliberate duplicates: the one pair Spanish cannot do without. */
    expect_hex("Ñ",  true,  "25");
    expect_hex("ñ",  true,  "24");
    expect_hex("Ñ",  false, "A5");
    expect_hex("ñ",  false, "A4");

    /* Everything else above 0x80 is folded to an unaccented letter rather than
     * silently arriving as some other glyph. */
    expect_hex("É",  true,  "45");    /* 'E', not 0x90 -> 0x10 triangle */
    expect_hex("á",  true,  "61");    /* 'a', not 0xA0 -> 0x20 space */
    expect_hex("Ü",  true,  "55");
    expect_hex("é",  true,  "65");

    /* Without the 7-bit constraint the real CP437 codes come through. */
    expect_hex("É",  false, "90");
    expect_hex("á",  false, "A0");
    expect_hex("é",  false, "82");

    /* Uppercase accented vowels CP437 lacks are folded either way. */
    expect_hex("CHAMARTÍN", false, "43 48 41 4D 41 52 54 49 4E");
    expect_hex("CHAMARTÍN", true,  "43 48 41 4D 41 52 54 49 4E");

    /* ROM 0x24 and 0x25 are n-tilde and N-tilde, so these cannot be sent. */
    expect_hex("$", true, "REFUSED");
    expect_hex("%", true, "REFUSED");
    expect_hex("100$", false, "REFUSED");

    /* A glyph with no fold and no way across the link is refused, not mangled. */
    expect_hex("█", true, "REFUSED");
    expect_hex("█", false, "DB");

    /* Not in CP437 at all. */
    expect_hex("→", false, "REFUSED");

    /* Control codes are escaped only in the command stream, not here. */
    uint8_t esc[16];
    const uint8_t raw[] = { 'A', 0x01, 'B' };
    size_t n = cp437_escape_controls(raw, sizeof raw, esc, sizeof esc);
    const uint8_t want[] = { 'A', 0x04, 0x33, 0x01, 'B' };
    if (n != sizeof want || memcmp(esc, want, n) != 0) {
        printf("FAIL escape_controls: expected 41 04 33 01 42\n");
        failures++;
    }
}

/* stdin lines -> the hex the panel would receive, for the Python comparison.
 * seven_bit is off here: that is the half the two implementations share. */
static void filter(void)
{
    char line[256];
    while (fgets(line, sizeof line, stdin)) {
        size_t n = strlen(line);
        while (n && (line[n - 1] == '\n' || line[n - 1] == '\r'))
            line[--n] = '\0';

        uint8_t buf[128];
        const char *why = NULL;
        int k = cp437_encode(line, buf, sizeof buf, false, &why);
        if (k < 0) {
            printf("REFUSED\n");
            continue;
        }
        for (int i = 0; i < k; i++)
            printf(i ? " %02X" : "%02X", buf[i]);
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    const char *what = argc > 1 ? argv[1] : "layout";

    if (strcmp(what, "encode") == 0) {
        filter();
        return 0;
    }
    if (strcmp(what, "layout") == 0) {
        test_layout();
        if (!failures)
            printf("cp437: layout rules ok\n");
    } else if (strcmp(what, "sevenbit") == 0) {
        test_sevenbit();
        if (!failures)
            printf("cp437: 7-bit link rules ok\n");
    } else {
        fprintf(stderr, "usage: %s layout|sevenbit|encode\n", argv[0]);
        return 2;
    }
    return failures ? 1 : 0;
}
