/* A push-button to ground: the way back in.
 *
 * Held for a few seconds it erases the stored configuration and reboots, which
 * puts the board back on its own access point with the default settings.
 *
 * This exists because there is deliberately no automatic escape any more. Wrong
 * WiFi credentials mean the board retries forever and never offers an AP -- the
 * alternative, falling back after a few failures, could not tell a mistyped
 * passphrase from a router rebooting at three in the morning, and answered both
 * by taking the sign off the network until someone power-cycled it. A physical
 * button cannot be triggered by a network outage.
 */
#pragma once

#include "esp_err.h"

/* Configures the GPIO as an input with its internal pull-up and starts the
 * poll task. A GPIO of -1 disables the feature. */
esp_err_t button_init(void);
