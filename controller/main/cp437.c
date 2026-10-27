/* See cp437.h. A direct port of tools/pit100.py lines 51-122, plus layout().
 *
 * This file deliberately depends on nothing but the C library, so that
 * controller/test can compile it on the host and check it against the Python.
 */

#include "cp437.h"

#include <string.h>

/* ------------------------------------------------------------------ layout */

void layout(const char *spec, char out[PANEL_CHARS + 1])
{
    memset(out, ' ', PANEL_CHARS);
    out[PANEL_CHARS] = '\0';
    if (!spec)
        return;

    const char *bar = strchr(spec, '|');
    if (!bar) {
        size_t n = strlen(spec);
        if (n > PANEL_CHARS)
            n = PANEL_CHARS;
        memcpy(out, spec, n);
        return;
    }

    size_t ln = (size_t)(bar - spec);
    const char *right = bar + 1;
    size_t rn = strlen(right);

    /* The right field is the one the "min" sticker continues, so it survives
     * intact and the left is what gets cut. Keep at least one space between
     * them, so a truncated label never runs into the number -- the same rule
     * web-replica/realtime.js applies with PANEL_CELLS - mins.length - 1. */
    if (rn > PANEL_CHARS)
        rn = PANEL_CHARS;
    size_t room = PANEL_CHARS - rn;
    if (ln && rn && room)
        room--;                       /* reserve the separating space */
    if (ln > room)
        ln = room;

    memcpy(out, spec, ln);
    memcpy(out + PANEL_CHARS - rn, right, rn);
}

/* ---------------------------------------------------------------- encoding */

/* CP437 0x80-0xFF as Unicode code points, in byte order. Python's cp437 codec
 * is the reference; controller/test proves this agrees with it. */
static const uint16_t HIGH[128] = {
    0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,  /* 80 */
    0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,  /* 88 */
    0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,  /* 90 */
    0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,  /* 98 */
    0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,  /* A0 */
    0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,  /* A8 */
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,  /* B0 */
    0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,  /* B8 */
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,  /* C0 */
    0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,  /* C8 */
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,  /* D0 */
    0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,  /* D8 */
    0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,  /* E0 */
    0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,  /* E8 */
    0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,  /* F0 */
    0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0,  /* F8 */
};

/* CP437 carries lowercase accented vowels but almost no uppercase ones. Station
 * names are usually uppercase, so this bites immediately: fold to the
 * unaccented letter. Exactly tools/pit100.py:54-56, so the two agree byte for
 * byte -- controller/test proves it. */
static const struct { uint16_t cp; char to; } FOLD[] = {
    { 0x00C1, 'A' }, { 0x00CD, 'I' }, { 0x00D3, 'O' }, { 0x00DA, 'U' },
    { 0x00C0, 'A' }, { 0x00C8, 'E' }, { 0x00CC, 'I' }, { 0x00D2, 'O' },
    { 0x00D9, 'U' }, { 0x00C2, 'A' }, { 0x00CA, 'E' }, { 0x00CE, 'I' },
    { 0x00D4, 'O' }, { 0x00DB, 'U' }, { 0x00CF, 'I' },
};

/* A 7-bit link strips bit 7 in hardware, so a CP437 byte at or above 0x80
 * arrives as byte & 0x7F -- a different glyph entirely, with no error anywhere.
 * E-acute at 0x90 would land on 0x10, a right-pointing triangle; a-acute at
 * 0xA0 would land on 0x20, a space.
 *
 * Only n-tilde and N-tilde survive, because the ROM duplicates them at exactly
 * the codes the stripping produces -- the one pair Spanish cannot do without,
 * put where a 7-bit link would deliver them. docs/5-the-panel.md.
 *
 * Everything else accented is folded to its unaccented letter here, which is a
 * visible approximation rather than a silent corruption. A CP437 byte at or
 * above 0x80 that is not in this table is refused outright. */
static const struct { uint8_t byte; char to; } FOLD_7BIT[] = {
    { 0x80, 'C' }, { 0x81, 'u' }, { 0x82, 'e' }, { 0x83, 'a' }, { 0x84, 'a' },
    { 0x85, 'a' }, { 0x86, 'a' }, { 0x87, 'c' }, { 0x88, 'e' }, { 0x89, 'e' },
    { 0x8A, 'e' }, { 0x8B, 'i' }, { 0x8C, 'i' }, { 0x8D, 'i' }, { 0x8E, 'A' },
    { 0x8F, 'A' }, { 0x90, 'E' }, { 0x93, 'o' }, { 0x94, 'o' }, { 0x95, 'o' },
    { 0x96, 'u' }, { 0x97, 'u' }, { 0x98, 'y' }, { 0x99, 'O' }, { 0x9A, 'U' },
    { 0xA0, 'a' }, { 0xA1, 'i' }, { 0xA2, 'o' }, { 0xA3, 'u' },
};

/* Decode one UTF-8 sequence. Returns bytes consumed, or 0 if malformed. */
static size_t utf8_next(const char *s, uint32_t *cp)
{
    const uint8_t *p = (const uint8_t *)s;
    if (p[0] < 0x80) {
        *cp = p[0];
        return 1;
    }
    if ((p[0] & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) {
        *cp = ((uint32_t)(p[0] & 0x1F) << 6) | (p[1] & 0x3F);
        return *cp < 0x80 ? 0 : 2;
    }
    if ((p[0] & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
        *cp = ((uint32_t)(p[0] & 0x0F) << 12) | ((uint32_t)(p[1] & 0x3F) << 6)
            | (p[2] & 0x3F);
        return *cp < 0x800 ? 0 : 3;
    }
    if ((p[0] & 0xF8) == 0xF0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80
        && (p[3] & 0xC0) == 0x80) {
        *cp = ((uint32_t)(p[0] & 0x07) << 18) | ((uint32_t)(p[1] & 0x3F) << 12)
            | ((uint32_t)(p[2] & 0x3F) << 6) | (p[3] & 0x3F);
        return 4;                     /* no CP437 glyph, but decode it cleanly */
    }
    return 0;
}

int cp437_encode(const char *utf8, uint8_t *out, size_t cap,
                 bool seven_bit, const char **why)
{
    /* Shown on the web page, so in Spanish. ROM 0x24 and 0x25 hold n-tilde and
     * N-tilde, which is why the first two exist: see docs/5-the-panel.md. */
    static const char *ERR_DOLLAR =
        "«$» no existe en este panel: en su lugar sale «ñ»";
    static const char *ERR_PERCENT =
        "«%» no existe en este panel: en su lugar sale «Ñ»";
    static const char *ERR_UTF8  = "la codificación del texto no es válida";
    static const char *ERR_ROOM  = "es demasiado largo";
    static const char *ERR_GLYPH = "tiene un carácter que el panel no puede mostrar";
    static const char *ERR_SEVEN =
        "tiene un carácter que no pasa por un enlace de 7 bits "
        "(solo sobreviven ñ y Ñ)";

    if (why)
        *why = NULL;
    size_t n = 0;

    for (const char *s = utf8; *s; ) {
        uint32_t cp;
        size_t adv = utf8_next(s, &cp);
        if (!adv) {
            if (why) *why = ERR_UTF8;
            return -1;
        }
        s += adv;

        for (size_t i = 0; i < sizeof FOLD / sizeof FOLD[0]; i++)
            if (FOLD[i].cp == cp) {
                cp = (uint32_t)FOLD[i].to;
                break;
            }

        if (cp == '$') { if (why) *why = ERR_DOLLAR;  return -1; }
        if (cp == '%') { if (why) *why = ERR_PERCENT; return -1; }

        uint8_t b;
        if (cp < 0x80) {
            b = (uint8_t)cp;
        } else {
            size_t i = 0;
            while (i < 128 && HIGH[i] != cp)
                i++;
            if (i == 128) {
                if (why) *why = ERR_GLYPH;
                return -1;
            }
            b = (uint8_t)(0x80 + i);
        }

        if (seven_bit && b >= 0x80) {
            if (b == 0xA4) {
                b = 0x24;             /* n-tilde, the ROM's own duplicate */
            } else if (b == 0xA5) {
                b = 0x25;             /* N-tilde */
            } else {
                char sub = '\0';
                for (size_t i = 0; i < sizeof FOLD_7BIT / sizeof FOLD_7BIT[0]; i++)
                    if (FOLD_7BIT[i].byte == b) {
                        sub = FOLD_7BIT[i].to;
                        break;
                    }
                if (!sub) {
                    if (why) *why = ERR_SEVEN;
                    return -1;
                }
                b = (uint8_t)sub;
            }
        }

        if (n >= cap) {
            if (why) *why = ERR_ROOM;
            return -1;
        }
        out[n++] = b;
    }
    return (int)n;
}

size_t cp437_escape_controls(const uint8_t *in, size_t n,
                            uint8_t *out, size_t cap)
{
    /* Handler B's dispatcher consumes 0x00-0x1F as commands, so a bare control
     * byte is silently eaten rather than stored. EOT '3' <byte> stores the next
     * byte verbatim whatever it is. tools/pit100.py:59-75. */
    size_t w = 0;
    for (size_t i = 0; i < n; i++) {
        if (in[i] < 0x20) {
            if (w + 2 > cap)
                return w;
            out[w++] = 0x04;          /* EOT */
            out[w++] = 0x33;          /* '3' */
        }
        if (w >= cap)
            return w;
        out[w++] = in[i];
    }
    return w;
}

bool cp437_check(const char *utf8, const char **why)
{
    uint8_t scratch[BUFFER_COLS * 2];
    return cp437_encode(utf8, scratch, sizeof scratch, true, why) >= 0;
}
