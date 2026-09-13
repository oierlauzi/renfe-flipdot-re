/* See netcfg.h. */

#include "netcfg.h"

#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "mdns.h"

#include "config.h"
#include "timesrc.h"

static const char *TAG = "netcfg";

#define MAX_ATTEMPTS 3

static bool ap_mode;
static bool got_ip;
static int  attempts;
static char ip_str[16] = "-";
static char ssid_str[33] = "-";

static esp_err_t start_ap(void);

/* Propagate the first failure. Every step here is a precondition for the next,
 * so there is nothing useful to do but stop and report which one broke. */
#define TRY(call) do {                                                        \
        esp_err_t _e = (call);                                                \
        if (_e != ESP_OK) {                                                   \
            ESP_LOGE(TAG, "%s: %s", #call, esp_err_to_name(_e));              \
            return _e;                                                        \
        }                                                                     \
    } while (0)

/* The wifi_config_t SSID and password fields are fixed-size and need not be
 * NUL-terminated, so copy by length rather than with snprintf -- which would
 * either warn about truncation or silently drop the last character. */
static void copy_field(uint8_t *dst, size_t cap, const char *src)
{
    size_t n = strlen(src);
    if (n > cap)
        n = cap;
    memset(dst, 0, cap);
    memcpy(dst, src, n);
}

static void on_wifi(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        got_ip = false;
        snprintf(ip_str, sizeof ip_str, "-");
        if (ap_mode)
            return;
        if (++attempts <= MAX_ATTEMPTS) {
            ESP_LOGW(TAG, "association failed, attempt %d/%d", attempts, MAX_ATTEMPTS);
            esp_wifi_connect();
        } else {
            ESP_LOGW(TAG, "giving up on '%s'; starting the fallback AP",
                     cfg_get(CFG_WIFI_SSID));
            /* Nowhere to propagate to from an event handler, and nothing to
             * retry: if even the AP will not start, the board needs a reboot. */
            esp_err_t e = start_ap();
            if (e != ESP_OK)
                ESP_LOGE(TAG, "the fallback AP failed too: %s",
                         esp_err_to_name(e));
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = (ip_event_got_ip_t *)data;
        snprintf(ip_str, sizeof ip_str, IPSTR, IP2STR(&e->ip_info.ip));
        got_ip = true;
        attempts = 0;
        ESP_LOGI(TAG, "joined '%s' as %s", cfg_get(CFG_WIFI_SSID), ip_str);
        timesrc_start_ntp();
    }
}

static void ap_name(char *out, size_t cap)
{
    uint8_t mac[6] = { 0 };
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    snprintf(out, cap, "FLIPDOT-%02X%02X", mac[4], mac[5]);
}

static esp_err_t start_ap(void)
{
    ap_mode = true;
    ap_name(ssid_str, sizeof ssid_str);
    snprintf(ip_str, sizeof ip_str, "192.168.4.1");

    wifi_config_t wc = { 0 };
    copy_field(wc.ap.ssid, sizeof wc.ap.ssid, ssid_str);
    wc.ap.ssid_len = (uint8_t)strlen(ssid_str);
    wc.ap.channel = 1;
    wc.ap.max_connection = 4;
    wc.ap.authmode = WIFI_AUTH_OPEN;  /* open: the point is to get back in */

    TRY(esp_wifi_set_mode(WIFI_MODE_AP));
    TRY(esp_wifi_set_config(WIFI_IF_AP, &wc));
    TRY(esp_wifi_start());
    ESP_LOGI(TAG, "AP '%s' up; the web UI is at http://192.168.4.1/", ssid_str);
    return ESP_OK;
}

static esp_err_t start_sta(const char *ssid, const char *pass)
{
    snprintf(ssid_str, sizeof ssid_str, "%s", ssid);

    wifi_config_t wc = { 0 };
    copy_field(wc.sta.ssid, sizeof wc.sta.ssid, ssid);
    copy_field(wc.sta.password, sizeof wc.sta.password, pass);

    TRY(esp_wifi_set_mode(WIFI_MODE_STA));
    TRY(esp_wifi_set_config(WIFI_IF_STA, &wc));
    TRY(esp_wifi_start());
    ESP_LOGI(TAG, "joining '%s'", ssid);
    return ESP_OK;
}

esp_err_t netcfg_init(void)
{
    TRY(esp_netif_init());
    TRY(esp_event_loop_create_default());
    if (!esp_netif_create_default_wifi_sta() || !esp_netif_create_default_wifi_ap())
        return ESP_ERR_NO_MEM;

    wifi_init_config_t ic = WIFI_INIT_CONFIG_DEFAULT();
    TRY(esp_wifi_init(&ic));
    TRY(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, on_wifi, NULL, NULL));
    TRY(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, on_wifi, NULL, NULL));

    const char *ssid = cfg_get(CFG_WIFI_SSID);
    TRY((ssid && *ssid) ? start_sta(ssid, cfg_get(CFG_WIFI_PASS)) : start_ap());

    /* mDNS is the only component fetched from the registry. Removing it costs
     * only the friendly name -- the IP still works -- so a failure here is
     * logged and tolerated rather than propagated. */
    const char *host = cfg_get(CFG_WIFI_HOST);
    esp_err_t merr = mdns_init();
    if (merr == ESP_OK) {
        mdns_hostname_set(host);
        mdns_instance_name_set("Flip-dot controller");
        mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
        ESP_LOGI(TAG, "mDNS: http://%s.local/", host);
    } else {
        ESP_LOGW(TAG, "mDNS unavailable (%s); reach it by IP instead",
                 esp_err_to_name(merr));
    }
    return ESP_OK;
}

bool netcfg_is_ap(void)      { return ap_mode; }
const char *netcfg_ip(void)  { return ip_str; }
const char *netcfg_ssid(void){ return ssid_str; }

int netcfg_rssi(void)
{
    if (ap_mode || !got_ip)
        return 0;
    wifi_ap_record_t ap;
    return esp_wifi_sta_get_ap_info(&ap) == ESP_OK ? ap.rssi : 0;
}

const char *netcfg_status(void)
{
    static char buf[160];
    if (ap_mode)
        snprintf(buf, sizeof buf,
                 "fallback AP '%s' (open) at %s -- set the credentials below",
                 ssid_str, ip_str);
    else if (got_ip)
        snprintf(buf, sizeof buf, "joined '%s' at %s, %d dBm",
                 ssid_str, ip_str, netcfg_rssi());
    else
        snprintf(buf, sizeof buf, "joining '%s', attempt %d/%d",
                 ssid_str, attempts + 1, MAX_ATTEMPTS);
    return buf;
}
