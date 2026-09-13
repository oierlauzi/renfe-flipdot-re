/* Where the clock comes from.
 *
 * NTP sets it, the ESP32's internal RTC carries it across WiFi outages and soft
 * reboots. It is lost on a power cycle, which is acceptable here: a hardware
 * sweep drives every dot black at power-on anyway, so the panel has nothing to
 * preserve either.
 *
 * timesrc_external() is the seam for a battery-backed DS3231 on I2C. It is
 * declared so the call sites exist; it reports "absent" until one is fitted.
 */
#pragma once

#include <stdbool.h>
#include <time.h>

#include "esp_err.h"

/* Spain (mainland), with the EU DST rules built in. */
#define TZ_SPAIN "CET-1CEST,M3.5.0,M10.5.0/3"

/* Always succeeds today -- setting the zone cannot fail and no external RTC is
 * fitted -- but returns a code so every init in main.c reads the same way, and
 * so fitting a DS3231 here does not change the call site. */
esp_err_t timesrc_init(void);

/* Start SNTP. Safe to call again when the network comes back. */
void timesrc_start_ntp(void);

/* True once the clock has been set from some source and can be trusted. */
bool timesrc_ok(void);

/* A one-line description of where the time came from, for the status page. */
const char *timesrc_describe(void);

/* Local broken-down time. Returns false if the clock has never been set. */
bool timesrc_local(struct tm *out);

/* Seconds until the next minute boundary, 1..60. What the clock modes wait on:
 * repainting on the minute is the whole refresh budget this panel gets. */
int timesrc_ms_to_next_minute(void);

/* Read a "HH:MM" string. Returns false if it is not a valid time of day. */
bool timesrc_parse_hhmm(const char *s, int *hour, int *minute);

/* Minutes from now until the next occurrence of hour:minute (always 0..1439),
 * and minutes since its last occurrence. The pair is what the two counter modes
 * are built on. */
int timesrc_minutes_until(int hour, int minute);
int timesrc_minutes_since(int hour, int minute);

/* Battery-backed external RTC. Always false until a DS3231 is fitted. */
bool timesrc_external(struct tm *out);
