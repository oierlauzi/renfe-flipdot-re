/* Date and time, in 16 columns.
 *
 * The formatter is local rather than strftime because newlib carries no es_ES
 * locale, so %a and %b would come out in English. The Spanish abbreviations are
 * all plain ASCII, which matters more than it looks: CP437 has no uppercase
 * accented vowels at all, and the one it does have -- E-acute at 0x90 -- does
 * not survive the 7-bit link either (0x90 & 0x7F is a triangle). So MIE and SAB
 * are written unaccented by necessity, not by choice.
 *
 * There is no seconds directive, deliberately. A panel that repaints every
 * second would be flipping dots 60 times more often than the sign ever did in
 * service, and it is loud.
 */

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "modes.h"
#include "timesrc.h"
#include "webui.h"

/* tm_wday is 0 = Sunday. */
static const char *DAY_SHORT[7] = { "DOM", "LUN", "MAR", "MIE", "JUE", "VIE", "SAB" };
static const char *DAY_LONG[7]  = { "DOMINGO", "LUNES", "MARTES", "MIERCOLES",
                                    "JUEVES", "VIERNES", "SABADO" };
static const char *MON_SHORT[12] = { "ENE", "FEB", "MAR", "ABR", "MAY", "JUN",
                                     "JUL", "AGO", "SEP", "OCT", "NOV", "DIC" };
static const char *MON_LONG[12]  = { "ENERO", "FEBRERO", "MARZO", "ABRIL",
                                     "MAYO", "JUNIO", "JULIO", "AGOSTO",
                                     "SEPTIEMBRE", "OCTUBRE", "NOVIEMBRE",
                                     "DICIEMBRE" };

struct preset { const char *tpl, *shows; };
static const struct preset PRESETS[] = {
    { "%a %d %b|%Hh%M", "LUN 13 SEP 18h24" },
    { "%d-%m-%Y|%Hh%M", "13-09-2026 18h24" },
    { "%d %b|%Hh%M",    "13 SEP     18h24" },
};
#define PRESET_COUNT ((int)(sizeof PRESETS / sizeof PRESETS[0]))

/* Expand the template. '|' passes through untouched, for layout() to use. */
static void expand(const char *tpl, const struct tm *tm, char *out, size_t cap)
{
    size_t w = 0;
    for (const char *p = tpl; *p && w + 1 < cap; ) {
        if (*p != '%') {
            out[w++] = *p++;
            continue;
        }
        p++;
        char frag[16];
        frag[0] = '\0';
        switch (*p) {
        case 'a': snprintf(frag, sizeof frag, "%s", DAY_SHORT[tm->tm_wday % 7]); break;
        case 'A': snprintf(frag, sizeof frag, "%s", DAY_LONG[tm->tm_wday % 7]);  break;
        case 'b': snprintf(frag, sizeof frag, "%s", MON_SHORT[tm->tm_mon % 12]); break;
        case 'B': snprintf(frag, sizeof frag, "%s", MON_LONG[tm->tm_mon % 12]);  break;
        case 'd': snprintf(frag, sizeof frag, "%02d", tm->tm_mday);              break;
        case 'e': snprintf(frag, sizeof frag, "%2d",  tm->tm_mday);              break;
        case 'm': snprintf(frag, sizeof frag, "%02d", tm->tm_mon + 1);           break;
        case 'Y': snprintf(frag, sizeof frag, "%04d", tm->tm_year + 1900);       break;
        case 'y': snprintf(frag, sizeof frag, "%02d", (tm->tm_year + 1900) % 100); break;
        case 'H': snprintf(frag, sizeof frag, "%02d", tm->tm_hour);              break;
        case 'I': snprintf(frag, sizeof frag, "%02d",
                           tm->tm_hour % 12 == 0 ? 12 : tm->tm_hour % 12);       break;
        case 'M': snprintf(frag, sizeof frag, "%02d", tm->tm_min);               break;
        case '\0': goto done;
        default:  frag[0] = *p; frag[1] = '\0';   break;   /* unknown: literal */
        }
        p++;
        for (const char *f = frag; *f && w + 1 < cap; f++)
            out[w++] = *f;
    }
done:
    out[w] = '\0';
}

static uint32_t render(char out[PANEL_CHARS + 1])
{
    struct tm tm;
    if (!timesrc_local(&tm)) {
        layout("SIN HORA", out);
        return 5000;                  /* look again shortly; NTP may land */
    }
    char expanded[PANEL_CHARS * 4];
    expand(cfg_get(CFG_CLK_TPL), &tm, expanded, sizeof expanded);
    layout(expanded, out);
    return (uint32_t)timesrc_ms_to_next_minute();
}

static void form(httpd_req_t *r)
{
    char buf[512];

    /* No JavaScript, so the preset cannot fill the text box. Instead the select
     * wins when it is set to anything, and is ignored when left on "keep". */
    httpd_resp_sendstr_chunk(r,
        "<tr><th>Formato<td><select name=\"clk.preset\">"
        "<option value=\"\">&mdash; mantener la plantilla de abajo &mdash;</option>");
    for (int i = 0; i < PRESET_COUNT; i++) {
        char esc[64];
        html_escape(PRESETS[i].tpl, esc, sizeof esc);
        snprintf(buf, sizeof buf, "<option value=\"%s\">%s</option>",
                 esc, PRESETS[i].shows);
        httpd_resp_sendstr_chunk(r, buf);
    }
    httpd_resp_sendstr_chunk(r, "</select>");

    mode_row_text(r, cfg_name(CFG_CLK_TPL), "Plantilla", cfg_get(CFG_CLK_TPL), 48,
                  "%a %A %b %B %d %e %m %Y %y %H %I %M, y «|» para el hueco. "
                  "Sin segundos, a propósito.");

    httpd_resp_sendstr_chunk(r, "<tr><th>Origen de la hora<td>");
    html_escape(timesrc_describe(), buf, sizeof buf);
    httpd_resp_sendstr_chunk(r, buf);
}

const panel_mode_t mode_clock = {
    .id     = "clock",
    .label  = "Fecha y hora",
    .render = render,
    .form   = form,
};
