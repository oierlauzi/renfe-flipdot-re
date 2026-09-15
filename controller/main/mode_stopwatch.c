/* Countdown and count-up: stopwatches driven by start, pause and reset.
 *
 * Unlike time-until and time-since, these measure a duration from a button
 * press, on the monotonic clock, so they work before NTP has synced and are not
 * disturbed when it does. The arithmetic is in stopwatch.c, where it is tested on
 * the host; this file is only the wiring to the panel and the web page.
 *
 * Minutes, not seconds, for the same reason as everywhere else: a display that
 * repaints every second flips dots sixty times as often as the sign ever did.
 * The web page shows seconds, because a web page does not wear out.
 *
 * Run state lives in RAM. It survives switching to another mode and back -- the
 * watch keeps running in the background, since it is computed from timestamps
 * rather than ticked -- and it survives saving settings. It does NOT survive a
 * reboot: the monotonic clock restarts from zero, and there is no way to know
 * how long the board was off. Both watches come back stopped.
 */

#include <stdio.h>
#include <string.h>

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

#include "config.h"
#include "modes.h"
#include "stopwatch.h"

static stopwatch_t up, down;

/* render() runs on the scheduler task and action() on the HTTP server's, and a
 * render can also change state -- a countdown stops itself at zero. The critical
 * sections are a few integer operations, never I/O. */
static portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;

static int64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

typedef enum { OP_START, OP_PAUSE, OP_RESET, OP_UNKNOWN } op_t;

static op_t parse_op(const char *name)
{
    if (strcmp(name, "start") == 0) return OP_START;
    if (strcmp(name, "pause") == 0) return OP_PAUSE;
    if (strcmp(name, "reset") == 0) return OP_RESET;
    return OP_UNKNOWN;
}

/* "1:04:37" or "4:37", for the web page only. */
static void format_hms(int64_t ms, char *out, size_t cap)
{
    int64_t s = ms / 1000;
    int h = (int)(s / 3600), m = (int)((s / 60) % 60), sec = (int)(s % 60);
    if (h)
        snprintf(out, cap, "%d:%02d:%02d", h, m, sec);
    else
        snprintf(out, cap, "%d:%02d", m, sec);
}

static void button(httpd_req_t *r, const char *op, const char *label)
{
    char buf[128];
    snprintf(buf, sizeof buf,
             "<button name=\"do\" value=\"%s\">%s</button> ", op, label);
    httpd_resp_sendstr_chunk(r, buf);
}

/* ---------------------------------------------------------------- countdown */

static int64_t duration_ms(bool *valid)
{
    int m;
    *valid = sw_parse_minutes(cfg_get(CFG_CD_DUR), &m);
    return *valid ? (int64_t)m * 60000 : 0;
}

static uint32_t down_render(char out[PANEL_CHARS + 1])
{
    bool valid;
    int64_t dur = duration_ms(&valid);
    if (!valid) {
        layout("TIEMPO NO VALIDO", out);
        return 0;
    }

    int64_t now = now_ms();
    uint32_t due;
    portENTER_CRITICAL(&lock);
    int mins = sw_down_minutes(&down, dur, now, &due);
    portEXIT_CRITICAL(&lock);

    mode_counter_line(cfg_get(CFG_CD_LABEL), mins, out);
    return due;
}

static void down_form(httpd_req_t *r)
{
    mode_row_text(r, cfg_name(CFG_CD_LABEL), "Etiqueta", cfg_get(CFG_CD_LABEL),
                  16, NULL);
    mode_row_text(r, cfg_name(CFG_CD_DUR), "Duración", cfg_get(CFG_CD_DUR),
                  5, "minutos, o H:MM. Se aplica al guardar, incluso con la "
                     "cuenta en marcha.");
}

static void down_controls(httpd_req_t *r)
{
    bool valid;
    int64_t dur = duration_ms(&valid);
    int64_t now = now_ms();

    portENTER_CRITICAL(&lock);
    stopwatch_t snap = down;
    portEXIT_CRITICAL(&lock);

    int64_t el  = sw_elapsed_ms(&snap, now);
    int64_t rem = dur - el;
    if (rem < 0)
        rem = 0;
    bool finished = valid && rem == 0;

    char left[16], total[16], buf[256];
    format_hms(rem, left, sizeof left);
    format_hms(dur, total, sizeof total);

    const char *state = !valid       ? "la duración no es válida"
                      : finished     ? "terminada"
                      : snap.running ? "en marcha"
                      : el > 0       ? "en pausa"
                                     : "lista";
    snprintf(buf, sizeof buf,
             "<p>quedan <strong>%s</strong> de %s &mdash; %s</p><p>",
             left, total, state);
    httpd_resp_sendstr_chunk(r, buf);

    if (snap.running && !finished)
        button(r, "pause", "Pausa");
    else
        button(r, "start", finished ? "Volver a empezar"
                         : el > 0   ? "Reanudar" : "Iniciar");
    button(r, "reset", "Poner a cero");
    httpd_resp_sendstr_chunk(r,
        "<small>la página no se actualiza sola: recargar para ver el avance</small>");
}

static const char *down_action(const char *name)
{
    op_t op = parse_op(name);
    if (op == OP_UNKNOWN)
        return "control desconocido";

    bool valid;
    int64_t dur = duration_ms(&valid);
    if (op == OP_START && !valid)
        return "no se puede iniciar: la duración no es válida";

    int64_t now = now_ms();
    portENTER_CRITICAL(&lock);
    switch (op) {
    case OP_START:
        /* A finished countdown restarts from the top rather than doing nothing,
         * which is what a Start button on a timer that reads 0 is for. */
        if (sw_down_finished(&down, dur, now))
            sw_reset(&down);
        sw_start(&down, now);
        break;
    case OP_PAUSE: sw_pause(&down, now); break;
    case OP_RESET: sw_reset(&down);      break;
    default: break;
    }
    portEXIT_CRITICAL(&lock);
    return NULL;
}

const panel_mode_t mode_countdown = {
    .id       = "countdown",
    .label    = "Cuenta atrás",
    .render   = down_render,
    .form     = down_form,
    .controls = down_controls,
    .action   = down_action,
};

/* ----------------------------------------------------------------- count-up */

static uint32_t up_render(char out[PANEL_CHARS + 1])
{
    int64_t now = now_ms();
    uint32_t due;
    portENTER_CRITICAL(&lock);
    int mins = sw_up_minutes(&up, now, &due);
    portEXIT_CRITICAL(&lock);

    mode_counter_line(cfg_get(CFG_CU_LABEL), mins, out);
    return due;
}

static void up_form(httpd_req_t *r)
{
    mode_row_text(r, cfg_name(CFG_CU_LABEL), "Etiqueta", cfg_get(CFG_CU_LABEL),
                  16, NULL);
}

static void up_controls(httpd_req_t *r)
{
    int64_t now = now_ms();
    portENTER_CRITICAL(&lock);
    stopwatch_t snap = up;
    portEXIT_CRITICAL(&lock);

    int64_t el = sw_elapsed_ms(&snap, now);
    char elapsed[16], buf[200];
    format_hms(el, elapsed, sizeof elapsed);

    snprintf(buf, sizeof buf,
             "<p>llevamos <strong>%s</strong> &mdash; %s</p><p>",
             elapsed, snap.running ? "en marcha" : el > 0 ? "en pausa" : "listo");
    httpd_resp_sendstr_chunk(r, buf);

    if (snap.running)
        button(r, "pause", "Pausa");
    else
        button(r, "start", el > 0 ? "Reanudar" : "Iniciar");
    button(r, "reset", "Poner a cero");
    httpd_resp_sendstr_chunk(r,
        "<small>la página no se actualiza sola: recargar para ver el avance</small>");
}

static const char *up_action(const char *name)
{
    op_t op = parse_op(name);
    if (op == OP_UNKNOWN)
        return "control desconocido";

    int64_t now = now_ms();
    portENTER_CRITICAL(&lock);
    switch (op) {
    case OP_START: sw_start(&up, now); break;
    case OP_PAUSE: sw_pause(&up, now); break;
    case OP_RESET: sw_reset(&up);      break;
    default: break;
    }
    portEXIT_CRITICAL(&lock);
    return NULL;
}

const panel_mode_t mode_countup = {
    .id       = "countup",
    .label    = "Cronómetro",
    .render   = up_render,
    .form     = up_form,
    .controls = up_controls,
    .action   = up_action,
};
