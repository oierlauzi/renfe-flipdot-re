/* Configuration, in one NVS namespace, cached in RAM.
 *
 * Every value is stored as a string and parsed on read, which keeps the web
 * form, the defaults table and NVS all speaking one type. The cache exists so
 * that render() -- called on every minute boundary -- never touches flash.
 *
 * NVS keys are capped at 15 characters by the API; the names below stay inside
 * that and the limit is asserted at init.
 */
#pragma once

#include <stdbool.h>

#include "esp_err.h"

typedef enum {
    CFG_WIFI_SSID,
    CFG_WIFI_PASS,
    CFG_WIFI_HOST,

    CFG_MODE,             /* the active mode's id */

    CFG_MSG_TEXT,
    CFG_CLK_TPL,
    CFG_CD_LABEL,
    CFG_CD_TIME,          /* "HH:MM" -- counts down to its next occurrence */
    CFG_CU_LABEL,
    CFG_CU_TIME,          /* "HH:MM" -- counts up from its last occurrence */
    CFG_PL_ITEMS,         /* one line per entry */
    CFG_PL_DWELL,         /* seconds, floored at 30 */

    CFG_PANEL_ADDR,
    CFG_PANEL_BAUD,
    CFG_PANEL_BITS,
    CFG_PANEL_PARITY,     /* "E", "O" or "N" */
    CFG_PANEL_SETTLE,     /* milliseconds */

    CFG_COUNT
} cfg_key_t;

esp_err_t cfg_init(void);

const char *cfg_get(cfg_key_t k);
int         cfg_get_int(cfg_key_t k);
char        cfg_get_char(cfg_key_t k);

/* Set and persist. Values are clamped where the key has limits. */
esp_err_t cfg_set(cfg_key_t k, const char *value);

/* Erase every stored key, so the next boot comes up on the defaults -- which
 * means no WiFi credentials, and therefore the fallback access point. What the
 * reset button does. The RAM cache is reloaded with the defaults too, so a
 * caller that does not reboot immediately still sees sane values. */
esp_err_t cfg_factory_reset(void);

/* Look a key up by its NVS name, for form handling. Returns CFG_COUNT if the
 * name is not one of ours -- so an unexpected form field is ignored, not
 * written. */
cfg_key_t cfg_key_by_name(const char *name);

const char *cfg_name(cfg_key_t k);
