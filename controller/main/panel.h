/* The panel as a resource: one task owns the UART, everything else asks.
 *
 * The panel is slow (a 3 s settle), loud, and mechanically finite, so it gets
 * exactly one writer. Modes and web handlers call panel_set() with the 16
 * characters they want and return immediately; the task decides whether that is
 * worth a transaction at all.
 *
 * Three things fall out of that and are the reason for this layer:
 *   - identical frames are never sent, so a static message costs nothing;
 *   - the settle delay bounds the global repaint rate, whatever a mode asks for;
 *   - exercise buttons and config changes serialise with repaints instead of
 *     racing them.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "cp437.h"

typedef struct {
    char     frame[PANEL_CHARS + 1];   /* what is on the panel now */
    char     version[16];              /* SUB reply, "AK0052.139U1" when healthy */
    char     tx_hex[300];              /* last transaction, for the status page */
    char     rx_hex[300];
    char     last_err[96];
    bool     last_ok;
    uint32_t paints;                   /* transactions that moved dots */
    uint32_t skipped;                  /* frames identical to what is shown */
    uint32_t naks;
    uint32_t timeouts;
} panel_stats_t;

esp_err_t panel_init(void);

/* Ask for these 16 characters. Coalescing: the newest request wins, and one
 * identical to what is displayed is dropped by the task. */
void panel_set(const char *frame);

/* Deliberately move every dot, to find stuck ones. `cycles` of fill/clear;
 * 0 means a single fill. These are the one exception to the wear rule -- a
 * button pressed on purpose, never something a mode can leave running. */
void panel_fill(void);
void panel_clear(void);
void panel_cycle(int cycles);

/* Re-read the line settings and address from config and apply them. */
void panel_reconfigure(void);

/* SUB, the known-answer health check. Result lands in stats.version. */
void panel_identify(void);

/* VT 00 + SI: read the shadow buffer back and compare with what was sent.
 * Result lands in stats.last_err / last_ok. */
void panel_verify(void);

void panel_stats(panel_stats_t *out);
