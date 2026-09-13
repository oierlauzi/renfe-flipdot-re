/* See config.h. */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "config";

#define NAMESPACE "flipdot"
#define VALUE_MAX 256

typedef struct {
    const char *name;                 /* the NVS key, 15 characters or fewer */
    const char *def;
    int         lo, hi;               /* numeric clamp; lo == hi means no clamp */
} entry_t;

static const entry_t TABLE[CFG_COUNT] = {
    [CFG_WIFI_SSID]    = { "wifi.ssid",    "",              0, 0 },
    [CFG_WIFI_PASS]    = { "wifi.pass",    "",              0, 0 },
    [CFG_WIFI_HOST]    = { "wifi.host",    "flipdot",       0, 0 },

    [CFG_MODE]         = { "mode",         "message",       0, 0 },

    [CFG_MSG_TEXT]     = { "msg.text",     "HOLA MUNDO",    0, 0 },

    /* 3 + 1 + 2 + 1 + 3 + 1 + 5 = exactly 16 columns, and the time lands hard
     * against the right edge where the "min" vinyl sticker continues it. */
    [CFG_CLK_TPL]      = { "clk.tpl",      "%a %d %b|%Hh%M", 0, 0 },

    [CFG_CD_LABEL]     = { "cd.label",     "CHAMARTIN",     0, 0 },
    [CFG_CD_TIME]      = { "cd.time",      "08:30",         0, 0 },
    [CFG_CU_LABEL]     = { "cu.label",     "DESDE",         0, 0 },
    [CFG_CU_TIME]      = { "cu.time",      "08:00",         0, 0 },

    [CFG_PL_ITEMS]     = { "pl.items",     "HOLA MUNDO\nRENFE CERCANIAS", 0, 0 },
    /* Floored at 30 s deliberately: a short dwell turns the playlist into a
     * scroller, which is exactly the wear the panel was never built for. */
    [CFG_PL_DWELL]     = { "pl.dwell",     "60",           30, 3600 },

    [CFG_PANEL_ADDR]   = { "panel.addr",   "1",             0, 31 },
    [CFG_PANEL_BAUD]   = { "panel.baud",   "1200",        300, 9600 },
    [CFG_PANEL_BITS]   = { "panel.bits",   "7",             7, 8 },
    [CFG_PANEL_PARITY] = { "panel.parity", "E",             0, 0 },
    [CFG_PANEL_SETTLE] = { "panel.settle", "3000",        500, 20000 },
};

static char cache[CFG_COUNT][VALUE_MAX];
static nvs_handle_t nvs;

const char *cfg_name(cfg_key_t k)
{
    return (k < CFG_COUNT) ? TABLE[k].name : "?";
}

esp_err_t cfg_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK)
        return err;

    if ((err = nvs_open(NAMESPACE, NVS_READWRITE, &nvs)) != ESP_OK)
        return err;

    for (int k = 0; k < CFG_COUNT; k++) {
        if (strlen(TABLE[k].name) > 15) {         /* NVS_KEY_NAME_MAX_SIZE - 1 */
            ESP_LOGE(TAG, "key '%s' exceeds the 15-character NVS limit",
                     TABLE[k].name);
            return ESP_ERR_INVALID_ARG;
        }
        size_t len = VALUE_MAX;
        if (nvs_get_str(nvs, TABLE[k].name, cache[k], &len) != ESP_OK)
            snprintf(cache[k], VALUE_MAX, "%s", TABLE[k].def);
    }
    ESP_LOGI(TAG, "loaded %d keys; mode '%s'", CFG_COUNT, cache[CFG_MODE]);
    return ESP_OK;
}

const char *cfg_get(cfg_key_t k)
{
    return (k < CFG_COUNT) ? cache[k] : "";
}

int cfg_get_int(cfg_key_t k)
{
    return (k < CFG_COUNT) ? atoi(cache[k]) : 0;
}

char cfg_get_char(cfg_key_t k)
{
    return (k < CFG_COUNT) ? cache[k][0] : '\0';
}

esp_err_t cfg_set(cfg_key_t k, const char *value)
{
    if (k >= CFG_COUNT || !value)
        return ESP_ERR_INVALID_ARG;

    char buf[VALUE_MAX];
    snprintf(buf, sizeof buf, "%s", value);

    const entry_t *e = &TABLE[k];
    if (e->lo != e->hi) {
        int v = atoi(buf);
        if (v < e->lo) v = e->lo;
        if (v > e->hi) v = e->hi;
        snprintf(buf, sizeof buf, "%d", v);
    }

    if (strcmp(cache[k], buf) == 0)
        return ESP_OK;                /* unchanged: no flash write */

    snprintf(cache[k], VALUE_MAX, "%s", buf);
    esp_err_t err = nvs_set_str(nvs, e->name, buf);
    if (err == ESP_OK)
        err = nvs_commit(nvs);
    return err;
}

esp_err_t cfg_factory_reset(void)
{
    esp_err_t err = nvs_erase_all(nvs);
    if (err == ESP_OK)
        err = nvs_commit(nvs);

    for (int k = 0; k < CFG_COUNT; k++)
        snprintf(cache[k], VALUE_MAX, "%s", TABLE[k].def);

    ESP_LOGW(TAG, "configuration erased (%s)", esp_err_to_name(err));
    return err;
}

cfg_key_t cfg_key_by_name(const char *name)
{
    for (int k = 0; k < CFG_COUNT; k++)
        if (strcmp(TABLE[k].name, name) == 0)
            return (cfg_key_t)k;
    return CFG_COUNT;
}
