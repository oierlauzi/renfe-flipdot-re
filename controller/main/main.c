/* A replacement upstream controller for the PAGE Iberica PI-T100-M.
 *
 * The display computes nothing -- no clock, no countdown, no arithmetic on
 * message content. Whatever drives it formats all 16 characters and re-sends
 * the line when it changes. So this is not a serial bridge: it is the thing
 * that decides what the sign says.
 *
 * See ../README.md for the wiring and the web UI, and ../../docs/ for how the
 * protocol was derived.
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config.h"
#include "modes.h"
#include "netcfg.h"
#include "panel.h"
#include "timesrc.h"
#include "webui.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "PI-T100-M controller starting");

    /* Order matters: configuration first, because everything else reads it;
     * the panel next, so the link is up and identified before the network can
     * start changing settings underneath it. */
    ESP_ERROR_CHECK(cfg_init());
    ESP_ERROR_CHECK(timesrc_init());
    ESP_ERROR_CHECK(panel_init());

    ESP_ERROR_CHECK(netcfg_init());
    ESP_ERROR_CHECK(webui_init());

    /* Last, so the first frame is painted onto a link that is already known to
     * work -- or that has already logged why it does not. */
    ESP_ERROR_CHECK(modes_init());

    ESP_LOGI(TAG, "up: mode '%s'", mode_active()->id);
}
