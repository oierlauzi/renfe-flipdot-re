/* WiFi, with a way back in when the credentials are wrong.
 *
 * Station mode from the stored credentials; after three failed attempts it
 * falls back to its own access point serving the same web UI, so a mistyped
 * passphrase never means digging the board out and reflashing it.
 */
#pragma once

#include <stdbool.h>

#include "esp_err.h"

esp_err_t netcfg_init(void);

bool        netcfg_is_ap(void);      /* true when serving from the fallback AP */
const char *netcfg_ip(void);         /* dotted quad, or "-" */
const char *netcfg_ssid(void);       /* the network joined, or the AP's own name */
int         netcfg_rssi(void);       /* dBm in station mode, 0 otherwise */
const char *netcfg_status(void);     /* one line for the status page */
