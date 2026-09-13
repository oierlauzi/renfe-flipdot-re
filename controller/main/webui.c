/* See webui.h. */

#include "webui.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"

#include "config.h"
#include "cp437.h"
#include "modes.h"
#include "netcfg.h"
#include "panel.h"
#include "timesrc.h"

static const char *TAG = "webui";

/* Shown once at the top of the next page: a validation failure, or what a
 * button did. */
static char notice[192];

/* ------------------------------------------------------------------ escaping */

void html_escape(const char *in, char *out, size_t cap)
{
    size_t w = 0;
    if (!cap)
        return;
    for (const unsigned char *p = (const unsigned char *)(in ? in : ""); *p; p++) {
        const char *rep = NULL;
        switch (*p) {
        case '&':  rep = "&amp;";  break;
        case '<':  rep = "&lt;";   break;
        case '>':  rep = "&gt;";   break;
        case '"':  rep = "&quot;"; break;
        default: break;
        }
        if (rep) {
            size_t n = strlen(rep);
            if (w + n + 1 >= cap)
                break;
            memcpy(out + w, rep, n);
            w += n;
        } else {
            if (w + 2 >= cap)
                break;
            out[w++] = (char)*p;
        }
    }
    out[w] = '\0';
}

/* ------------------------------------------------------------ form parsing */

#define FORM_MAX 24
#define BODY_MAX 2048

typedef struct {
    char body[BODY_MAX];
    struct { char *k, *v; } kv[FORM_MAX];
    int n;
} form_t;

static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* application/x-www-form-urlencoded, decoded in place. esp_http_server has no
 * parser for this, so here is the whole of it. */
static void url_decode(char *s)
{
    char *w = s;
    for (char *p = s; *p; p++) {
        if (*p == '+') {
            *w++ = ' ';
        } else if (*p == '%' && hexval(p[1]) >= 0 && hexval(p[2]) >= 0) {
            *w++ = (char)(hexval(p[1]) * 16 + hexval(p[2]));
            p += 2;
        } else {
            *w++ = *p;
        }
    }
    *w = '\0';
}

static esp_err_t form_read(httpd_req_t *r, form_t *f)
{
    f->n = 0;
    size_t len = r->content_len;
    if (len >= sizeof f->body)
        len = sizeof f->body - 1;

    size_t got = 0;
    while (got < len) {
        int k = httpd_req_recv(r, f->body + got, len - got);
        if (k <= 0)
            return ESP_FAIL;
        got += (size_t)k;
    }
    f->body[got] = '\0';

    char *p = f->body;
    while (*p && f->n < FORM_MAX) {
        char *amp = strchr(p, '&');
        if (amp)
            *amp = '\0';
        char *eq = strchr(p, '=');
        if (eq) {
            *eq = '\0';
            url_decode(p);
            url_decode(eq + 1);
            f->kv[f->n].k = p;
            f->kv[f->n].v = eq + 1;
            f->n++;
        }
        if (!amp)
            break;
        p = amp + 1;
    }
    return ESP_OK;
}

static const char *form_get(const form_t *f, const char *key)
{
    for (int i = 0; i < f->n; i++)
        if (strcmp(f->kv[i].k, key) == 0)
            return f->kv[i].v;
    return NULL;
}

/* ------------------------------------------------------------------ pieces */

static void send(httpd_req_t *r, const char *s)
{
    httpd_resp_sendstr_chunk(r, s);
}

static void sendf(httpd_req_t *r, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

static void sendf(httpd_req_t *r, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    httpd_resp_sendstr_chunk(r, buf);
}

static const char *STYLE =
    "<style>"
    "body{font:14px/1.5 monospace;margin:1.5em;max-width:44em}"
    "table{border-collapse:collapse;margin:.5em 0}"
    "th,td{text-align:left;padding:.25em .6em;vertical-align:top}"
    "th{font-weight:normal;color:#555;white-space:nowrap}"
    "fieldset{border:1px solid #bbb;margin:1.2em 0;padding:.4em 1em 1em}"
    "legend{padding:0 .4em}"
    "pre{background:#f4f4f4;padding:.6em;overflow-x:auto}"
    "small{color:#666}"
    ".bad{color:#a00}"
    "</style>";

static void head(httpd_req_t *r, const char *title)
{
    httpd_resp_set_type(r, "text/html; charset=utf-8");
    sendf(r, "<!doctype html><meta charset=\"utf-8\">"
             "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
             "<title>%s</title>%s<h1>%s</h1>", title, STYLE, title);
    if (notice[0]) {
        char esc[sizeof notice * 6];
        html_escape(notice, esc, sizeof esc);
        sendf(r, "<p class=\"bad\"><strong>%s</strong>", esc);
        notice[0] = '\0';
    }
}

/* The exact 16 characters that will be sent, under a ruler, so a truncation or
 * an unencodable character is visible before it reaches the panel. */
static void frame_preview(httpd_req_t *r, const char *frame)
{
    /* Pad to the full width: before the first paint the frame is empty, and an
     * empty string would collapse the line and misalign it under the ruler. */
    char padded[PANEL_CHARS + 1];
    snprintf(padded, sizeof padded, "%-*.*s", PANEL_CHARS, PANEL_CHARS,
             frame ? frame : "");

    char esc[PANEL_CHARS * 6 + 1];
    html_escape(padded, esc, sizeof esc);
    sendf(r, "<pre>     0123456789012345\n    \"%s\"  &rarr; min</pre>", esc);
}

/* ------------------------------------------------------------- status page */

static esp_err_t get_root(httpd_req_t *r)
{
    panel_stats_t st;
    panel_stats(&st);
    const panel_mode_t *active = mode_active();

    head(r, "Flip-dot controller");

    /* --- status --- */
    send(r, "<fieldset><legend>Status</legend><table>");
    {
        char esc[128];
        html_escape(st.version, esc, sizeof esc);
        sendf(r, "<tr><th>Display<td>%s %s", esc,
              strcmp(st.version, "AK0052.139U1") == 0
                  ? "<small>(the expected answer)</small>"
                  : "<small class=\"bad\">(expected AK0052.139U1)</small>");
        html_escape(st.last_err, esc, sizeof esc);
        sendf(r, "<tr><th>Last result<td>%s%s%s",
              st.last_ok ? "" : "<span class=\"bad\">", esc,
              st.last_ok ? "" : "</span>");
        sendf(r, "<tr><th>Last TX<td><small>%s</small>", st.tx_hex);
        sendf(r, "<tr><th>Last RX<td><small>%s</small>",
              st.rx_hex[0] ? st.rx_hex : "(nothing)");
        sendf(r, "<tr><th>Painted<td>%" PRIu32 " <small>(%" PRIu32 " skipped as "
                 "identical, %" PRIu32 " NAK, %" PRIu32 " silent)</small>",
              st.paints, st.skipped, st.naks, st.timeouts);
        html_escape(timesrc_describe(), esc, sizeof esc);
        sendf(r, "<tr><th>Clock<td>%s", esc);
        html_escape(netcfg_status(), esc, sizeof esc);
        sendf(r, "<tr><th>Network<td>%s", esc);
    }
    send(r, "</table>");
    frame_preview(r, st.frame);
    send(r, "</fieldset>");

    /* --- mode and its settings, one form --- */
    send(r, "<form method=\"post\" action=\"/save\">"
            "<fieldset><legend>Mode</legend><table>");
    for (int i = 0; i < MODE_COUNT; i++)
        sendf(r, "<tr><th><label><input type=\"radio\" name=\"mode\" value=\"%s\"%s> "
                 "%s</label>",
              MODES[i]->id, MODES[i] == active ? " checked" : "", MODES[i]->label);
    send(r, "</table></fieldset>");

    sendf(r, "<fieldset><legend>%s</legend><table>", active->label);
    if (active->form)
        active->form(r);
    send(r, "</table></fieldset>");

    /* --- panel link --- */
    send(r, "<fieldset><legend>Panel link</legend><table>");
    mode_row_text(r, cfg_name(CFG_PANEL_ADDR), "Address",
                  cfg_get(CFG_PANEL_ADDR), 2, "0-31; this unit is 1");
    mode_row_text(r, cfg_name(CFG_PANEL_BAUD), "Baud",
                  cfg_get(CFG_PANEL_BAUD), 4, "300-9600; this unit is 1200");
    mode_row_text(r, cfg_name(CFG_PANEL_BITS), "Data bits",
                  cfg_get(CFG_PANEL_BITS), 1, "7 or 8");
    mode_row_text(r, cfg_name(CFG_PANEL_PARITY), "Parity",
                  cfg_get(CFG_PANEL_PARITY), 1, "E, O or N");
    mode_row_text(r, cfg_name(CFG_PANEL_SETTLE), "Settle ms",
                  cfg_get(CFG_PANEL_SETTLE), 5,
                  "the dots are mechanical; this also caps the repaint rate");
    send(r, "</table><p><button type=\"submit\">Save</button></fieldset></form>");

    /* --- exercise --- */
    send(r, "<form method=\"post\" action=\"/exercise\">"
            "<fieldset><legend>Exercise</legend>"
            "<p><small>Moves every dot on purpose, to find stuck ones. This is "
            "the one thing here that ignores the wear rule, which is why it is "
            "a button and not a mode.</small>"
            "<p><button name=\"what\" value=\"fill\">Fill</button> "
            "<button name=\"what\" value=\"clear\">Clear</button> "
            "<button name=\"what\" value=\"cycle\">Cycle</button> "
            "<input name=\"cycles\" value=\"3\" size=\"2\" maxlength=\"2\"> times"
            "<p><button name=\"what\" value=\"identify\">Identify</button> "
            "<button name=\"what\" value=\"verify\">Read the panel back</button>"
            "</fieldset></form>");

    /* --- wifi --- */
    send(r, "<form method=\"post\" action=\"/save\">"
            "<fieldset><legend>Network</legend><table>");
    mode_row_text(r, cfg_name(CFG_WIFI_SSID), "SSID", cfg_get(CFG_WIFI_SSID),
                  32, NULL);
    mode_row_text(r, cfg_name(CFG_WIFI_PASS), "Passphrase",
                  cfg_get(CFG_WIFI_PASS), 63, NULL);
    mode_row_text(r, cfg_name(CFG_WIFI_HOST), "Hostname",
                  cfg_get(CFG_WIFI_HOST), 24, "reachable as &lt;name&gt;.local");
    send(r, "</table><p><button type=\"submit\">Save</button> "
            "<small>a changed network needs a reboot to take effect</small>");

    /* There is no automatic escape any more: the board retries for ever rather
     * than deciding for itself that the network is wrong. So the page has to say
     * what the way back in is, on the page where the mistake gets made. */
#if CONFIG_FLIPDOT_RESET_GPIO >= 0
    sendf(r, "<p><small><strong>Get this wrong and you will lose this page.</strong> "
             "The board keeps retrying and never starts its own access point on "
             "its own, so the way back is the reset button on GPIO%d: hold it for "
             "%d seconds to erase all configuration and reboot onto "
             "<code>FLIPDOT-XXXX</code>.</small>",
          CONFIG_FLIPDOT_RESET_GPIO, CONFIG_FLIPDOT_RESET_HOLD_MS / 1000);
#else
    send(r, "<p><small class=\"bad\"><strong>Get this wrong and you will lose "
            "this page.</strong> No reset button is configured and the board "
            "never starts its own access point on its own, so recovering means "
            "reflashing. Check the passphrase.</small>");
#endif

    send(r, "</fieldset></form>");

    send(r, "<p><small>PAGE Ib&eacute;rica PI-T100-M &middot; "
            "16 characters &middot; CP437 &middot; "
            "<code>$</code> and <code>%</code> are <code>&ntilde;</code> and "
            "<code>&Ntilde;</code> on this panel</small>");

    httpd_resp_sendstr_chunk(r, NULL);
    return ESP_OK;
}

/* -------------------------------------------------------------- save */

/* Keys whose value ends up on the panel, so they must encode cleanly. */
static bool is_panel_text(cfg_key_t k)
{
    return k == CFG_MSG_TEXT || k == CFG_CLK_TPL || k == CFG_CD_LABEL
        || k == CFG_CU_LABEL || k == CFG_PL_ITEMS;
}

static esp_err_t post_save(httpd_req_t *r)
{
    form_t f;
    if (form_read(r, &f) != ESP_OK) {
        httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "unreadable form");
        return ESP_FAIL;
    }

    bool panel_changed = false;

    for (int i = 0; i < f.n; i++) {
        cfg_key_t k = cfg_key_by_name(f.kv[i].k);
        if (k == CFG_COUNT)
            continue;                 /* not ours: ignore rather than store */

        if (is_panel_text(k)) {
            const char *why = NULL;
            if (!cp437_check(f.kv[i].v, &why)) {
                snprintf(notice, sizeof notice, "%s not saved: %s",
                         f.kv[i].k, why ? why : "cannot be shown on this panel");
                continue;
            }
        }
        cfg_set(k, f.kv[i].v);
        if (k == CFG_PANEL_ADDR || k == CFG_PANEL_BAUD || k == CFG_PANEL_BITS
            || k == CFG_PANEL_PARITY)
            panel_changed = true;
    }

    /* The clock preset select, which is not a stored key: it exists only to
     * fill in the template without JavaScript, and wins when it is set. */
    const char *preset = form_get(&f, "clk.preset");
    if (preset && *preset)
        cfg_set(CFG_CLK_TPL, preset);

    if (panel_changed)
        panel_reconfigure();
    mode_refresh();
    mode_notify();

    httpd_resp_set_status(r, "303 See Other");
    httpd_resp_set_hdr(r, "Location", "/");
    httpd_resp_send(r, NULL, 0);
    return ESP_OK;
}

/* ---------------------------------------------------------------- exercise */

static esp_err_t post_exercise(httpd_req_t *r)
{
    form_t f;
    if (form_read(r, &f) != ESP_OK) {
        httpd_resp_send_err(r, HTTPD_400_BAD_REQUEST, "unreadable form");
        return ESP_FAIL;
    }
    const char *what = form_get(&f, "what");
    const char *cycles = form_get(&f, "cycles");

    if (!what) {
        /* nothing to do */
    } else if (strcmp(what, "fill") == 0) {
        panel_fill();
    } else if (strcmp(what, "clear") == 0) {
        panel_clear();
    } else if (strcmp(what, "cycle") == 0) {
        panel_cycle(cycles ? atoi(cycles) : 1);
    } else if (strcmp(what, "identify") == 0) {
        panel_identify();
    } else if (strcmp(what, "verify") == 0) {
        panel_verify();
    }
    snprintf(notice, sizeof notice,
             "%s queued; the panel is slow, so reload in a few seconds.",
             what ? what : "nothing");

    httpd_resp_set_status(r, "303 See Other");
    httpd_resp_set_hdr(r, "Location", "/");
    httpd_resp_send(r, NULL, 0);
    return ESP_OK;
}

/* -------------------------------------------------------------------- init */

esp_err_t webui_init(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.lru_purge_enable = true;
    cfg.stack_size = 8192;            /* the status page builds on the stack */

    esp_err_t err = httpd_start(&server, &cfg);
    if (err != ESP_OK)
        return err;

    static const httpd_uri_t root = {
        .uri = "/", .method = HTTP_GET, .handler = get_root };
    static const httpd_uri_t save = {
        .uri = "/save", .method = HTTP_POST, .handler = post_save };
    static const httpd_uri_t exercise = {
        .uri = "/exercise", .method = HTTP_POST, .handler = post_exercise };

    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &save);
    httpd_register_uri_handler(server, &exercise);

    ESP_LOGI(TAG, "web UI on port 80");
    return ESP_OK;
}
