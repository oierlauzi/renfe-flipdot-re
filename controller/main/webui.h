/* The configuration web UI: plain HTML forms, no JavaScript.
 *
 * Handlers never touch the UART. They write configuration and poke the
 * scheduler; panel.c decides what actually reaches the panel and when.
 */
#pragma once

#include <stddef.h>

#include "esp_err.h"

esp_err_t webui_init(void);

/* Escape &, <, > and " for HTML. Truncates rather than overflowing. */
void html_escape(const char *in, char *out, size_t cap);
