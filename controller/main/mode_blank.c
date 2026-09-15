/* Nothing on the panel, on purpose.
 *
 * Distinct from the clear button: this is a state the controller stays in, so
 * no mode is quietly still running and waiting to repaint.
 */

#include "modes.h"

static uint32_t render(char out[PANEL_CHARS + 1])
{
    layout("", out);
    return 0;
}

static void form(httpd_req_t *r)
{
    httpd_resp_sendstr_chunk(r,
        "<tr><th>&nbsp;<td><small>El panel se mantiene vacío. No se repinta nada.</small>");
}

const panel_mode_t mode_blank = {
    .id     = "blank",
    .label  = "En blanco",
    .render = render,
    .form   = form,
};
