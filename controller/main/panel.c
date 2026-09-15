/* See panel.h. */

#include "panel.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "config.h"
#include "pit100.h"

static const char *TAG = "panel";

typedef enum {
    REQ_FRAME,        /* want[] changed */
    REQ_FILL,
    REQ_CLEAR,
    REQ_CYCLE,
    REQ_RECONFIG,
    REQ_IDENTIFY,
    REQ_VERIFY,
} req_kind_t;

typedef struct {
    req_kind_t kind;
    int        arg;
} panel_req_t;

static void post(req_kind_t kind, int arg);

static pit100_t         dev;
static QueueHandle_t    q;
static SemaphoreHandle_t mux;          /* guards want[] and st */
static char             want[PANEL_CHARS + 1];
static panel_stats_t    st;

/* ------------------------------------------------------------- bookkeeping */

static void note_result(esp_err_t err, const char *why)
{
    xSemaphoreTake(mux, portMAX_DELAY);
    pit100_hex(dev.tx, dev.tx_n, st.tx_hex, sizeof st.tx_hex);
    pit100_hex(dev.rx, dev.rx_n, st.rx_hex, sizeof st.rx_hex);
    st.last_ok = (err == ESP_OK);
    if (err == ESP_OK)
        snprintf(st.last_err, sizeof st.last_err, "correcto");
    else
        snprintf(st.last_err, sizeof st.last_err, "%s", why ? why : esp_err_to_name(err));
    if (err == ESP_ERR_TIMEOUT)
        st.timeouts++;
    else if (err == ESP_FAIL)
        st.naks++;
    xSemaphoreGive(mux);

    if (err == ESP_OK)
        ESP_LOGI(TAG, "TX %s", st.tx_hex);
    else
        ESP_LOGW(TAG, "TX %s -> %s", st.tx_hex, st.last_err);
}

static uint32_t settle_ms(void)
{
    return (uint32_t)cfg_get_int(CFG_PANEL_SETTLE);
}

/* --------------------------------------------------------------- the moves */

/* One repaint. The byte sequence and the reason for each step are in
 * docs/serial-protocol.md; the two RS are what make it work at all. */
static void do_frame(void)
{
    char frame[PANEL_CHARS + 1];
    xSemaphoreTake(mux, portMAX_DELAY);
    memcpy(frame, want, sizeof frame);
    bool same = memcmp(frame, st.frame, PANEL_CHARS) == 0;
    xSemaphoreGive(mux);

    if (same) {
        xSemaphoreTake(mux, portMAX_DELAY);
        st.skipped++;
        xSemaphoreGive(mux);
        return;                        /* the dots are already right */
    }

    pit100_begin(&dev);                /* RS, then flush */
    pit100_select(&dev);               /* GS <addr> */
    pit100_clear(&dev);                /* CAN: the whole 64-column buffer */
    const char *why = NULL;
    esp_err_t err = pit100_message(&dev, frame, &why);
    pit100_deselect(&dev);             /* RS: releases the repaint interlock */

    note_result(err, why);

    if (err == ESP_OK) {
        xSemaphoreTake(mux, portMAX_DELAY);
        memcpy(st.frame, frame, sizeof frame);
        st.paints++;
        xSemaphoreGive(mux);
        vTaskDelay(pdMS_TO_TICKS(settle_ms()));
    }
}

/* A bare command with no message body: CAN or FF. */
static void do_simple(bool fill)
{
    pit100_begin(&dev);
    pit100_select(&dev);
    if (fill)
        pit100_fill(&dev);
    else
        pit100_clear(&dev);
    pit100_deselect(&dev);

    note_result(ESP_OK, NULL);

    xSemaphoreTake(mux, portMAX_DELAY);
    /* Neither command leaves the panel showing our idea of the frame, so the
     * next panel_set() must not be skipped as a no-op. */
    memset(st.frame, fill ? '#' : ' ', PANEL_CHARS);
    st.paints++;
    xSemaphoreGive(mux);

    vTaskDelay(pdMS_TO_TICKS(settle_ms()));
}

static void do_cycle(int cycles)
{
    if (cycles < 1)
        cycles = 1;
    if (cycles > 20)
        cycles = 20;
    for (int i = 0; i < cycles; i++) {
        ESP_LOGI(TAG, "exercise %d/%d", i + 1, cycles);
        do_simple(true);
        do_simple(false);
    }
    /* Put the mode's frame back. want[] still holds it, so this only has to
     * forget what is on the panel and ask for a repaint -- calling panel_set()
     * with want[] itself would copy the buffer onto itself. */
    xSemaphoreTake(mux, portMAX_DELAY);
    memset(st.frame, 0, sizeof st.frame);
    xSemaphoreGive(mux);
    post(REQ_FRAME, 0);
}

static void do_identify(void)
{
    char version[16] = "";
    pit100_begin(&dev);
    pit100_select(&dev);
    esp_err_t err = pit100_identify(&dev, version, sizeof version);
    pit100_deselect(&dev);

    xSemaphoreTake(mux, portMAX_DELAY);
    snprintf(st.version, sizeof st.version, "%s",
             err == ESP_OK ? version : "sin respuesta");
    xSemaphoreGive(mux);

    note_result(err, err == ESP_OK ? NULL : "identificación: sin respuesta");
    ESP_LOGI(TAG, "identify -> '%s'", version);
}

static void do_verify(void)
{
    uint8_t shadow[BUFFER_COLS + 8];
    pit100_begin(&dev);
    pit100_select(&dev);
    pit100_goto(&dev, 0, 0);
    int n = pit100_read_screen(&dev, shadow, sizeof shadow);
    pit100_deselect(&dev);

    char seen[PANEL_CHARS + 1];
    memset(seen, ' ', PANEL_CHARS);
    seen[PANEL_CHARS] = '\0';
    for (int i = 0; i < n && i < PANEL_CHARS; i++)
        seen[i] = (shadow[i] >= 0x20 && shadow[i] < 0x7F) ? (char)shadow[i] : '?';

    xSemaphoreTake(mux, portMAX_DELAY);
    bool match = memcmp(seen, st.frame, PANEL_CHARS) == 0;
    st.last_ok = match;
    if (n <= 0)
        snprintf(st.last_err, sizeof st.last_err, "lectura: sin respuesta");
    else if (match)
        snprintf(st.last_err, sizeof st.last_err,
                 "lectura: el panel muestra lo enviado");
    else
        snprintf(st.last_err, sizeof st.last_err,
                 "lectura: el panel muestra «%s»", seen);
    pit100_hex(dev.rx, dev.rx_n, st.rx_hex, sizeof st.rx_hex);
    xSemaphoreGive(mux);
}

static void do_reconfig(void)
{
    dev.address = cfg_get_int(CFG_PANEL_ADDR);
    esp_err_t err = pit100_set_line(&dev, cfg_get_int(CFG_PANEL_BAUD),
                                    cfg_get_int(CFG_PANEL_BITS),
                                    cfg_get_char(CFG_PANEL_PARITY));
    ESP_LOGI(TAG, "line now %d %d%c1, address %d (%s)",
             dev.baud, dev.bits, dev.parity, dev.address, esp_err_to_name(err));
    /* Force the next frame through: the panel may well be showing something
     * else entirely if the address just changed. */
    xSemaphoreTake(mux, portMAX_DELAY);
    memset(st.frame, 0, sizeof st.frame);
    xSemaphoreGive(mux);
}

/* ----------------------------------------------------------------- the task */

static void panel_task(void *arg)
{
    (void)arg;
    do_identify();

    for (;;) {
        panel_req_t r;
        if (xQueueReceive(q, &r, portMAX_DELAY) != pdTRUE)
            continue;
        switch (r.kind) {
        case REQ_FRAME:    do_frame();          break;
        case REQ_FILL:     do_simple(true);     break;
        case REQ_CLEAR:    do_simple(false);    break;
        case REQ_CYCLE:    do_cycle(r.arg);     break;
        case REQ_RECONFIG: do_reconfig();       break;
        case REQ_IDENTIFY: do_identify();       break;
        case REQ_VERIFY:   do_verify();         break;
        }
    }
}

/* -------------------------------------------------------------------- API */

static void post(req_kind_t kind, int arg)
{
    panel_req_t r = { .kind = kind, .arg = arg };
    /* A full queue means at least eight requests are already pending, and any
     * REQ_FRAME among them will read the newest want[], so dropping this one
     * loses nothing. */
    xQueueSend(q, &r, 0);
}

esp_err_t panel_init(void)
{
    mux = xSemaphoreCreateMutex();
    q   = xQueueCreate(8, sizeof(panel_req_t));
    if (!mux || !q)
        return ESP_ERR_NO_MEM;

    memset(&st, 0, sizeof st);
    memset(st.frame, 0, sizeof st.frame);     /* unknown: force the first paint */
    snprintf(st.version, sizeof st.version, "?");
    snprintf(st.last_err, sizeof st.last_err, "aún sin usar");

    layout("", want);

    dev.address = cfg_get_int(CFG_PANEL_ADDR);
    dev.baud    = cfg_get_int(CFG_PANEL_BAUD);
    dev.bits    = cfg_get_int(CFG_PANEL_BITS);
    dev.parity  = cfg_get_char(CFG_PANEL_PARITY);

    esp_err_t err = pit100_init(&dev, CONFIG_FLIPDOT_UART_NUM,
                                CONFIG_FLIPDOT_TX_GPIO, CONFIG_FLIPDOT_RX_GPIO);
    if (err != ESP_OK)
        return err;

    return xTaskCreate(panel_task, "panel", 4096, NULL, 5, NULL) == pdPASS
         ? ESP_OK : ESP_ERR_NO_MEM;
}

void panel_set(const char *frame)
{
    xSemaphoreTake(mux, portMAX_DELAY);
    snprintf(want, sizeof want, "%-*.*s", PANEL_CHARS, PANEL_CHARS,
             frame ? frame : "");
    xSemaphoreGive(mux);
    post(REQ_FRAME, 0);
}

void panel_fill(void)            { post(REQ_FILL, 0); }
void panel_clear(void)           { post(REQ_CLEAR, 0); }
void panel_cycle(int cycles)     { post(REQ_CYCLE, cycles); }
void panel_reconfigure(void)     { post(REQ_RECONFIG, 0); }
void panel_identify(void)        { post(REQ_IDENTIFY, 0); }
void panel_verify(void)          { post(REQ_VERIFY, 0); }

void panel_stats(panel_stats_t *out)
{
    xSemaphoreTake(mux, portMAX_DELAY);
    *out = st;
    xSemaphoreGive(mux);
}
