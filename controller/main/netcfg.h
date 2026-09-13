/* WiFi.
 *
 * Station mode from the stored credentials, retrying for ever. The board serves
 * its own open access point only when no credentials are stored at all, which is
 * first boot and after a configuration reset.
 *
 * It never falls back there on its own. A failure count cannot tell a mistyped
 * passphrase from a router rebooting, and treating the second as the first takes
 * the sign off the network until someone power-cycles it. The way back in when
 * the credentials really are wrong is the reset button -- see button.h.
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
