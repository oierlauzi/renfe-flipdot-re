/* Text for the panel: CP437 with this ROM's quirks, and the 16-column layout.
 *
 * Ported from tools/pit100.py (encode, escape_controls) and docs/font.md.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PANEL_CHARS 16   /* the flip-dot line */
#define BUFFER_COLS 64   /* the message buffer behind it, 0xE001-0xE040 */

/* Lay a spec out into exactly PANEL_CHARS characters, space padded.
 *
 *   layout("CHAMARTIN|7")   -> "CHAMARTIN      7"
 *   layout("|18h24")        -> "           18h24"
 *   layout("HOLA")          -> "HOLA            "
 *
 * The first '|' is an elastic gap that expands to fill the line; the field
 * after it is the one held against the right edge, where the "min" vinyl
 * sticker continues it. The right field wins if the two cannot both fit: the
 * left is truncated, always leaving at least one space between them.
 *
 * Text without a '|' is left-aligned and truncated at PANEL_CHARS.
 * `out` receives PANEL_CHARS characters plus a NUL.
 */
void layout(const char *spec, char out[PANEL_CHARS + 1]);

/* Encode UTF-8 into the panel's byte set.
 *
 * Folds the uppercase accented vowels CP437 lacks, maps Unicode to CP437, and
 * on a 7-bit link maps n-tilde and N-tilde onto their 0x24/0x25 duplicates.
 * Returns the number of bytes written, or -1 with `why` set to a one-line
 * explanation (an unencodable character, or '$'/'%' -- which are n-tilde and
 * N-tilde on this panel, not currency).
 *
 * Control codes are NOT escaped here; that belongs to the command stream. Use
 * cp437_escape_controls().
 */
int cp437_encode(const char *utf8, uint8_t *out, size_t cap,
                 bool seven_bit, const char **why);

/* Wrap every byte below 0x20 in the EOT '3' literal escape, which is the only
 * way to put CP437's control-code glyphs on the panel. Returns bytes written.
 */
size_t cp437_escape_controls(const uint8_t *in, size_t n,
                             uint8_t *out, size_t cap);

/* True if the text encodes cleanly. `why` is set when it does not, and is
 * suitable for showing in the web UI.
 */
bool cp437_check(const char *utf8, const char **why);
