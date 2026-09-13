/* See pit100.h. */

#include "pit100.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "pit100";

#define RX_RING 256

/* Error codes returned after a NAK. 0x30 and 0x32 arrive bare; anything else is
 * followed by two detail bytes. tools/pit100.py:41-44. */
static const char *nak_text(uint8_t code)
{
    switch (code) {
    case 0x30: return "buffer overflow";
    case 0x32: return "unspecified failure";
    default:   return "unknown error code";
    }
}

static bool bare_error(uint8_t code)
{
    return code == 0x30 || code == 0x32;
}

/* ------------------------------------------------------------------- setup */

static uart_config_t line_config(const pit100_t *d)
{
    /* Accept a lowercase letter from the web form: parity is typed by hand, and
     * silently falling through to "none" on an 'e' would look like a dead link
     * rather than a typo. */
    char parity = (d->parity >= 'a' && d->parity <= 'z')
                ? (char)(d->parity - 'a' + 'A') : d->parity;
    uart_config_t c = {
        .baud_rate = d->baud,
        .data_bits = d->bits == 7 ? UART_DATA_7_BITS : UART_DATA_8_BITS,
        .parity    = parity == 'E' ? UART_PARITY_EVEN
                   : parity == 'O' ? UART_PARITY_ODD
                                   : UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    return c;
}

static void recompute_char_time(pit100_t *d)
{
    /* start + data + parity + stop, in microseconds. tools/pit100.py:432. */
    int bits = 1 + d->bits + ((d->parity | 0x20) == 'n' ? 0 : 1) + 1;
    d->char_us = (uint32_t)((1000000LL * bits) / (d->baud > 0 ? d->baud : 1));
}

esp_err_t pit100_init(pit100_t *d, uart_port_t port, int tx_gpio, int rx_gpio)
{
    d->port = port;
    recompute_char_time(d);

    uart_config_t cfg = line_config(d);
    esp_err_t err = uart_driver_install(port, RX_RING, 0, 0, NULL, 0);
    if (err != ESP_OK)
        return err;
    if ((err = uart_param_config(port, &cfg)) != ESP_OK)
        return err;
    if ((err = uart_set_pin(port, tx_gpio, rx_gpio,
                            UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE)) != ESP_OK)
        return err;

    ESP_LOGI(TAG, "UART%d on TX=%d RX=%d at %d %d%c1, address %d",
             port, tx_gpio, rx_gpio, d->baud, d->bits, d->parity, d->address);
    return ESP_OK;
}

esp_err_t pit100_set_line(pit100_t *d, int baud, int bits, char parity)
{
    d->baud = baud;
    d->bits = bits;
    d->parity = parity;
    recompute_char_time(d);
    uart_config_t cfg = line_config(d);
    return uart_param_config(d->port, &cfg);
}

uint8_t pit100_address_byte(int address)
{
    if (address < 0 || address == PIT_BROADCAST || address > 0x1F)
        return PIT_BROADCAST;
    return (address & 0x10) ? (uint8_t)(0x40 | (address & 0x0F))
                            : (uint8_t)(0x30 | address);
}

/* ------------------------------------------------------------------ output */

static void emit(pit100_t *d, const uint8_t *b, size_t n)
{
    uart_write_bytes(d->port, (const char *)b, n);
    size_t room = sizeof d->tx - d->tx_n;
    if (room) {
        size_t k = n < room ? n : room;
        memcpy(d->tx + d->tx_n, b, k);
        d->tx_n += k;
    }
}

static void emit1(pit100_t *d, uint8_t b)
{
    emit(d, &b, 1);
}

void pit100_select(pit100_t *d)
{
    uint8_t f[2] = { PIT_GS, pit100_address_byte(d->address) };
    emit(d, f, 2);
}

void pit100_deselect(pit100_t *d)
{
    /* Not optional if you want to see anything. The repaint task spins at ROM
     * 0x130B for as long as the selected flag (0xA040) is set, so a selected
     * unit accepts and ACKs text but never moves a dot. In emulation, without
     * this the render pass runs zero times. tools/pit100.py:150-158. */
    emit1(d, PIT_RS);
}

void pit100_clear(pit100_t *d)  { emit1(d, PIT_CAN); }
void pit100_fill(pit100_t *d)   { emit1(d, PIT_FF);  }
void pit100_reset(pit100_t *d)  { emit1(d, PIT_DLE); }

void pit100_goto(pit100_t *d, int row, int col)
{
    uint8_t f[3] = { PIT_VT, (uint8_t)(0x30 + row), (uint8_t)(0x30 + col) };
    emit(d, f, 3);
}

void pit100_begin(pit100_t *d)
{
    d->tx_n = 0;
    d->rx_n = 0;
    /* A unit left mid-command by an earlier session eats the next byte as a
     * parameter, so STX never resets the write pointer and the text lands at a
     * stale one -- which surfaces as a spurious "buffer overflow" NAK. RS first
     * costs nothing. tools/say.py:40-43. */
    pit100_deselect(d);
    uart_wait_tx_done(d->port, pdMS_TO_TICKS(500));
    uart_flush_input(d->port);
}

/* ------------------------------------------------------------------- input */

/* Remove any 18 18 1E 15 <addr> status frame. It belongs to receive handler C
 * and the unit does not emit it unprompted, but stripping it is harmless.
 * tools/pit100.py:362-377. */
static size_t strip_status_frames(uint8_t *b, size_t n)
{
    static const uint8_t PREFIX[4] = { PIT_CAN, PIT_CAN, PIT_RS, PIT_NAK };
    size_t w = 0;
    for (size_t i = 0; i < n; ) {
        if (i + 5 <= n && memcmp(b + i, PREFIX, 4) == 0) {
            i += 5;
            continue;
        }
        b[w++] = b[i++];
    }
    return w;
}

size_t pit100_drain(pit100_t *d, uint8_t *buf, size_t cap)
{
    uint32_t idle_ms = (4 * d->char_us) / 1000;
    if (idle_ms < 30)
        idle_ms = 30;                 /* tools/pit100.py:462 */

    uart_wait_tx_done(d->port, pdMS_TO_TICKS(1000));

    /* The firmware waits a fixed 3.9 ms before replying (ROM 0x1ABD, 40 delay
     * units) and then sends the whole reply as one burst, so any gap longer
     * than a few character times means it has finished. One second covers the
     * slowest case: five bytes at 300 baud is 183 ms. */
    int64_t deadline = esp_timer_get_time() + 1000000;
    size_t n = 0;
    bool got = false;

    while (n < cap && esp_timer_get_time() < deadline) {
        uint8_t b;
        int r = uart_read_bytes(d->port, &b, 1, pdMS_TO_TICKS(idle_ms));
        if (r == 1) {
            buf[n++] = b;
            got = true;
        } else if (got) {
            break;                    /* idle gap after data: the reply is done */
        }
    }

    n = strip_status_frames(buf, n);

    d->rx_n = n < sizeof d->rx ? n : sizeof d->rx;
    memcpy(d->rx, buf, d->rx_n);
    return n;
}

/* Parse <ACK|NAK> <addr> [errcode [p1 p2]]. tools/pit100.py:328-343. */
static esp_err_t read_ack(pit100_t *d, const char **err)
{
    static char detail[64];
    uint8_t raw[32];
    size_t n = pit100_drain(d, raw, sizeof raw);

    if (!n) {
        if (err) *err = "no reply: check wiring, line settings and address";
        return ESP_ERR_TIMEOUT;
    }
    if (raw[0] == PIT_ACK)
        return ESP_OK;
    if (raw[0] == PIT_NAK) {
        if (n < 3) {
            if (err) *err = "NAK, no error code";
            return ESP_FAIL;
        }
        uint8_t code = raw[2];
        if (!bare_error(code) && n >= 5)
            snprintf(detail, sizeof detail, "NAK 0x%02X %s (params 0x%02X 0x%02X)",
                     code, nak_text(code), raw[3], raw[4]);
        else
            snprintf(detail, sizeof detail, "NAK 0x%02X %s", code, nak_text(code));
        if (err) *err = detail;
        return ESP_FAIL;
    }
    snprintf(detail, sizeof detail, "expected ACK or NAK, got 0x%02X", raw[0]);
    if (err) *err = detail;
    return ESP_FAIL;
}

/* ----------------------------------------------------------------- writing */

esp_err_t pit100_message(pit100_t *d, const char *text, const char **err)
{
    uint8_t payload[BUFFER_COLS];
    const char *why = NULL;
    int n = cp437_encode(text, payload, sizeof payload, d->bits == 7, &why);
    if (n < 0) {
        if (err) *err = why;
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t frame[2 + BUFFER_COLS * 2];
    size_t w = 0;
    frame[w++] = PIT_STX;             /* also resets the write pointer to 0xE001 */
    w += cp437_escape_controls(payload, (size_t)n, frame + w, sizeof frame - w - 1);
    frame[w++] = PIT_ETX;
    emit(d, frame, w);

    return read_ack(d, err);
}

/* ----------------------------------------------------------------- queries */

esp_err_t pit100_identify(pit100_t *d, char *version, size_t cap)
{
    emit1(d, PIT_SUB);
    uint8_t raw[64];
    size_t n = pit100_drain(d, raw, sizeof raw);

    size_t i = 0;
    while (i < n && raw[i] == PIT_SYN)      /* strip the SYN preamble */
        i++;
    if (i >= n)
        return ESP_ERR_TIMEOUT;

    size_t k = 0;
    while (i < n && k + 1 < cap && k < 12) {
        uint8_t c = raw[i++];
        version[k++] = (c >= 0x20 && c < 0x7F) ? (char)c : '?';
    }
    version[k] = '\0';
    return k ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t pit100_cursor(pit100_t *d, int *row, int *col)
{
    emit1(d, PIT_TAB);
    uint8_t raw[16];
    size_t n = pit100_drain(d, raw, sizeof raw);
    if (n < 2)
        return ESP_ERR_TIMEOUT;
    *row = raw[0] - 0x30;
    *col = raw[1] - 0x30;
    return ESP_OK;
}

int pit100_read_screen(pit100_t *d, uint8_t *out, size_t cap)
{
    /* The firmware plants a 0xFF sentinel at 0xE083 to bound the reply, so the
     * last shadow byte is clobbered and character 63 is repainted on the next
     * pass. Harmless, but SI is not perfectly side-effect free.
     * tools/pit100.py:291-304. */
    emit1(d, PIT_SI);
    return (int)pit100_drain(d, out, cap);
}

void pit100_hex(const uint8_t *b, size_t n, char *out, size_t cap)
{
    size_t w = 0;
    out[0] = '\0';
    for (size_t i = 0; i < n && w + 4 < cap; i++)
        w += (size_t)snprintf(out + w, cap - w, i ? " %02X" : "%02X", b[i]);
}
