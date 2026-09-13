/* See button.h. */

#include "button.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config.h"
#include "panel.h"

static const char *TAG = "button";

#define POLL_MS 50

/* No debounce logic beyond this: a reset has to be held for seconds, so contact
 * bounce lasting milliseconds cannot reach the threshold on its own, and a
 * bounce part-way through simply restarts the count. */
static void do_reset(void)
{
    ESP_LOGW(TAG, "reset button held: erasing the stored configuration");

    /* Erase first. It is the part that matters, and if the power goes now the
     * board still comes up in a known state. */
    esp_err_t err = cfg_factory_reset();
    if (err != ESP_OK)
        ESP_LOGE(TAG, "erase failed: %s", esp_err_to_name(err));

    /* Then say so on the panel, which is the only output the person holding the
     * button is looking at. Long enough for the transaction and the settle. */
    panel_set(err == ESP_OK ? "CONFIG BORRADA" : "ERROR AL BORRAR");
    vTaskDelay(pdMS_TO_TICKS(6000));

    ESP_LOGW(TAG, "restarting");
    esp_restart();
}

static void button_task(void *arg)
{
    const int gpio  = CONFIG_FLIPDOT_RESET_GPIO;
    const int need  = CONFIG_FLIPDOT_RESET_HOLD_MS / POLL_MS;
    const int notice = need / 2;
    int held = 0;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(POLL_MS));

        /* Pulled up, so the button shorts it to ground: low means pressed. */
        if (gpio_get_level(gpio) != 0) {
            if (held && held < need)
                ESP_LOGI(TAG, "released after %d ms, nothing done",
                         held * POLL_MS);
            held = 0;
            continue;
        }

        held++;
        if (held == notice)
            ESP_LOGW(TAG, "reset button down; keep holding to erase "
                          "the configuration");
        else if (held == need)
            do_reset();                /* does not return */
    }
}

esp_err_t button_init(void)
{
    const int gpio = CONFIG_FLIPDOT_RESET_GPIO;
    if (gpio < 0) {
        ESP_LOGW(TAG, "no reset button configured; a wrong passphrase will "
                      "need a reflash");
        return ESP_OK;
    }

    /* GPIO34-39 are input-only and have no internal pull-up at all, so the pin
     * would float and read as a button held down for ever. */
    if (gpio >= 34)
        ESP_LOGW(TAG, "GPIO%d has no internal pull-up; fit an external one to "
                      "3.3 V or the board will reset itself", gpio);

    gpio_config_t io = {
        .pin_bit_mask = 1ULL << gpio,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&io);
    if (err != ESP_OK)
        return err;

    ESP_LOGI(TAG, "reset button on GPIO%d, hold %d ms (reads %d now)",
             gpio, CONFIG_FLIPDOT_RESET_HOLD_MS, gpio_get_level(gpio));

    return xTaskCreate(button_task, "button", 3072, NULL, 3, NULL) == pdPASS
         ? ESP_OK : ESP_ERR_NO_MEM;
}
