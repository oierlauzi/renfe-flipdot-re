/* A fixed line of text.
 *
 * Truncated at 16 characters, never scrolled. A marquee would flip most of the
 * 560 dots on every step, which is precisely the wear this panel was not built
 * for -- it was designed to repaint when a train's ETA changed, differentially,
 * about once a minute. The web UI shows a column ruler so the cut is visible
 * before it reaches the panel.
 */

#include <string.h>

#include "config.h"
#include "modes.h"

static uint32_t render(char out[PANEL_CHARS + 1])
{
    layout(cfg_get(CFG_MSG_TEXT), out);
    return 0;                         /* static: only configuration changes it */
}

static void form(httpd_req_t *r)
{
    mode_row_text(r, cfg_name(CFG_MSG_TEXT), "Texto", cfg_get(CFG_MSG_TEXT),
                  64, "«|» es un hueco elástico: <code>ATOCHA|7</code>");
}

const panel_mode_t mode_message = {
    .id     = "message",
    .label  = "Mensaje",
    .render = render,
    .form   = form,
};
