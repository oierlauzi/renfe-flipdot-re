/* See timesrc.h. */

#include "timesrc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "esp_log.h"
#include "esp_netif_sntp.h"

static const char *TAG = "timesrc";

static bool synced;
static bool sntp_running;
static time_t last_sync;

static void on_sync(struct timeval *tv)
{
    (void)tv;
    synced = true;
    last_sync = time(NULL);
    ESP_LOGI(TAG, "clock set from NTP");
}

esp_err_t timesrc_init(void)
{
    setenv("TZ", TZ_SPAIN, 1);
    tzset();

    /* If a battery-backed chip is ever fitted, this is where it seeds the
     * internal RTC -- before the network is up, which is its whole point. */
    struct tm ext;
    if (timesrc_external(&ext)) {
        time_t t = mktime(&ext);
        struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
        if (settimeofday(&tv, NULL) != 0)
            return ESP_FAIL;
        synced = true;
        ESP_LOGI(TAG, "clock set from the external RTC");
    }
    return ESP_OK;
}

void timesrc_start_ntp(void)
{
    if (sntp_running)
        return;
    esp_sntp_config_t cfg = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    cfg.sync_cb = on_sync;
    cfg.start = true;
    if (esp_netif_sntp_init(&cfg) == ESP_OK) {
        sntp_running = true;
        ESP_LOGI(TAG, "SNTP started against pool.ntp.org");
    }
}

bool timesrc_ok(void)
{
    if (synced)
        return true;
    /* A plausible wall-clock value means something set it -- a resync after a
     * soft reboot, say. 2020-01-01. */
    return time(NULL) > 1577836800;
}

const char *timesrc_describe(void)
{
    static char buf[64];
    if (!timesrc_ok())
        return "not set: waiting for NTP";
    if (synced) {
        long age = (long)(time(NULL) - last_sync);
        snprintf(buf, sizeof buf, "NTP, last sync %ld s ago", age);
        return buf;
    }
    return "internal RTC, not yet resynced";
}

bool timesrc_local(struct tm *out)
{
    if (!timesrc_ok())
        return false;
    time_t now = time(NULL);
    localtime_r(&now, out);
    return true;
}

int timesrc_ms_to_next_minute(void)
{
    time_t now = time(NULL);
    struct tm tm;
    localtime_r(&now, &tm);
    int secs = 60 - tm.tm_sec;
    if (secs <= 0)
        secs = 60;
    return secs * 1000;
}

bool timesrc_parse_hhmm(const char *s, int *hour, int *minute)
{
    if (!s)
        return false;
    int h, m;
    if (sscanf(s, "%d:%d", &h, &m) != 2)
        return false;
    if (h < 0 || h > 23 || m < 0 || m > 59)
        return false;
    *hour = h;
    *minute = m;
    return true;
}

/* Both counters work in minutes-of-the-day, so neither needs a date and the
 * arithmetic stays symmetric: one subtraction, opposite signs. */
static int now_minutes(void)
{
    struct tm tm;
    if (!timesrc_local(&tm))
        return -1;
    return tm.tm_hour * 60 + tm.tm_min;
}

int timesrc_minutes_until(int hour, int minute)
{
    int now = now_minutes();
    if (now < 0)
        return -1;
    int target = hour * 60 + minute;
    int delta = target - now;
    if (delta < 0)
        delta += 24 * 60;             /* the next occurrence, tomorrow */
    return delta;
}

int timesrc_minutes_since(int hour, int minute)
{
    int now = now_minutes();
    if (now < 0)
        return -1;
    int target = hour * 60 + minute;
    int delta = now - target;
    if (delta < 0)
        delta += 24 * 60;             /* the last occurrence, yesterday */
    return delta;
}

bool timesrc_external(struct tm *out)
{
    (void)out;
    /* No DS3231 fitted. Adding one means an I2C read here and two GPIOs in
     * Kconfig; nothing else in the project changes. */
    return false;
}
