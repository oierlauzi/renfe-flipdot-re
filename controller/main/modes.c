/* See modes.h. The mode table and the scheduler that drives it. */

#include "modes.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config.h"
#include "panel.h"
#include "webui.h"

static const char *TAG = "modes";

extern const panel_mode_t mode_message;
extern const panel_mode_t mode_clock;
extern const panel_mode_t mode_countdown;
extern const panel_mode_t mode_countup;
extern const panel_mode_t mode_playlist;
extern const panel_mode_t mode_blank;

/* Order is the order of the radio buttons in the web UI. Adding a mode is one
 * line here plus its file in main/CMakeLists.txt. */
const panel_mode_t *const MODES[] = {
    &mode_message,
    &mode_clock,
    &mode_countdown,
    &mode_countup,
    &mode_playlist,
    &mode_blank,
};

const int MODE_COUNT = (int)(sizeof MODES / sizeof MODES[0]);

static TaskHandle_t     sched;
static const panel_mode_t *current;

/* ------------------------------------------------------------------ lookup */

static const panel_mode_t *by_id(const char *id)
{
    for (int i = 0; i < MODE_COUNT; i++)
        if (strcmp(MODES[i]->id, id) == 0)
            return MODES[i];
    return MODES[0];
}

const panel_mode_t *mode_active(void)
{
    if (!current)
        current = by_id(cfg_get(CFG_MODE));
    return current;
}

void mode_refresh(void)
{
    const panel_mode_t *next = by_id(cfg_get(CFG_MODE));
    if (next == current)
        return;
    if (current && current->stop)
        current->stop();
    current = next;
    if (current->start)
        current->start();
    ESP_LOGI(TAG, "mode -> %s", current->id);
}

void mode_notify(void)
{
    if (sched)
        xTaskNotifyGive(sched);
}

/* --------------------------------------------------------------- scheduler */

static void scheduler_task(void *arg)
{
    (void)arg;
    const panel_mode_t *m = mode_active();
    if (m->start)
        m->start();

    for (;;) {
        m = mode_active();

        char frame[PANEL_CHARS + 1];
        uint32_t due = m->render(frame);
        panel_set(frame);

        /* due == 0 means the line only changes when the configuration does, so
         * wait for a notification rather than spinning on a timer. */
        TickType_t wait = due ? pdMS_TO_TICKS(due) : portMAX_DELAY;
        ulTaskNotifyTake(pdTRUE, wait);
    }
}

esp_err_t modes_init(void)
{
    current = by_id(cfg_get(CFG_MODE));
    return xTaskCreate(scheduler_task, "modes", 4096, NULL, 4, &sched) == pdPASS
         ? ESP_OK : ESP_ERR_NO_MEM;
}

/* ----------------------------------------------------------------- helpers */

void mode_counter_line(const char *label, int minutes, char out[PANEL_CHARS + 1])
{
    char num[8];
    if (minutes < 0)
        snprintf(num, sizeof num, "--");
    else if (minutes < 100)
        snprintf(num, sizeof num, "%d", minutes);
    else {
        /* Past 99 the field would eat the label, so switch to hours: "1h40"
         * still reads correctly with the "min" sticker after it. */
        int h = minutes / 60, m = minutes % 60;
        if (h > 99) { h = 99; m = 59; }
        snprintf(num, sizeof num, "%dh%02d", h, m);
    }

    char spec[PANEL_CHARS * 2 + 8];
    snprintf(spec, sizeof spec, "%s|%s", label ? label : "", num);
    layout(spec, out);
}

void mode_row_text(httpd_req_t *r, const char *key, const char *label,
                   const char *value, int maxlen, const char *hint)
{
    char esc[512];
    char buf[900];
    html_escape(value, esc, sizeof esc);
    snprintf(buf, sizeof buf,
             "<tr><th>%s<td><input name=\"%s\" value=\"%s\" maxlength=\"%d\" "
             "size=\"32\">%s%s%s",
             label, key, esc, maxlen,
             hint ? " <small>" : "", hint ? hint : "", hint ? "</small>" : "");
    httpd_resp_sendstr_chunk(r, buf);
}
