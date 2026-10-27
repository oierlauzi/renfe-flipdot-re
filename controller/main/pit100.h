/* The PI-T100-M command protocol (receive handler B), over an ESP32 UART.
 *
 * A port of tools/pit100.py. The command set is derived in
 * docs/3-the-commands.md; every non-obvious step below cites the Python line
 * or the ROM address it came from.
 *
 * This layer is pure protocol. It does not decide when to repaint, does not
 * rate-limit, and is not thread safe -- panel.c owns one instance and is the
 * only caller.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/uart.h"
#include "esp_err.h"

#include "cp437.h"

/* Command bytes. The C0 control set, used as the command space. */
enum {
    PIT_NUL = 0x00, PIT_SOH = 0x01, PIT_STX = 0x02, PIT_ETX = 0x03,
    PIT_EOT = 0x04, PIT_ENQ = 0x05, PIT_ACK = 0x06, PIT_BEL = 0x07,
    PIT_BS  = 0x08, PIT_TAB = 0x09, PIT_LF  = 0x0A, PIT_VT  = 0x0B,
    PIT_FF  = 0x0C, PIT_CR  = 0x0D, PIT_SO  = 0x0E, PIT_SI  = 0x0F,
    PIT_DLE = 0x10, PIT_DC1 = 0x11, PIT_DC2 = 0x12, PIT_DC3 = 0x13,
    PIT_DC4 = 0x14, PIT_NAK = 0x15, PIT_SYN = 0x16, PIT_ETB = 0x17,
    PIT_CAN = 0x18, PIT_EM  = 0x19, PIT_SUB = 0x1A, PIT_ESC = 0x1B,
    PIT_FS  = 0x1C, PIT_GS  = 0x1D, PIT_RS  = 0x1E, PIT_US  = 0x1F,
};

#define PIT_BROADCAST 0x7F           /* the address every unit answers to */

typedef struct {
    uart_port_t port;
    int      address;                /* 0-31, or PIT_BROADCAST */
    int      baud;
    int      bits;                   /* 7 or 8 */
    char     parity;                 /* 'E', 'O' or 'N' */
    uint32_t char_us;                /* one character time on the wire */

    /* Diagnostics for the status page. Reset at the start of a transaction. */
    uint8_t  tx[96];
    size_t   tx_n;
    uint8_t  rx[96];
    size_t   rx_n;
} pit100_t;

/* Install the UART driver. Call once. */
esp_err_t pit100_init(pit100_t *d, uart_port_t port, int tx_gpio, int rx_gpio);

/* Change line settings on the fly, from the web UI. */
esp_err_t pit100_set_line(pit100_t *d, int baud, int bits, char parity);

/* Encode a DIP-switch address the way the firmware does at ROM 0x0F69/0x1078:
 * bits 4:0 become printable ASCII, 0x30-0x3F or 0x40-0x4F.
 * tools/pit100.py:125-135. */
uint8_t pit100_address_byte(int address);

/* --- framing ----------------------------------------------------------- */

void pit100_select(pit100_t *d);     /* GS <addr> */
void pit100_deselect(pit100_t *d);   /* RS -- releases the repaint interlock */
void pit100_clear(pit100_t *d);      /* CAN -- buffer to spaces */
void pit100_fill(pit100_t *d);       /* FF  -- buffer to 0x23, every dot on */
void pit100_reset(pit100_t *d);      /* DLE -- remote reboot */
void pit100_goto(pit100_t *d, int row, int col);   /* VT <row+0x30> <col+0x30> */

/* Begin a transaction: RS, then discard anything the last one left on the line.
 * Both halves matter -- see the comment in the implementation. */
void pit100_begin(pit100_t *d);

/* STX <escaped bytes> ETX. `text` is UTF-8 and is encoded for the panel.
 * Returns ESP_OK when the unit ACKs. On a NAK or a silent line, `err` is set to
 * a one-line explanation. */
esp_err_t pit100_message(pit100_t *d, const char *text, const char **err);

/* --- queries ----------------------------------------------------------- */

/* SUB: the version string and config block from ROM 0x0064. `version` receives
 * up to 13 bytes ("AK0052.139U1" plus a NUL) -- the known-answer health check. */
esp_err_t pit100_identify(pit100_t *d, char *version, size_t cap);

/* TAB: the cursor position. Not two ASCII digits -- the firmware adds 0x30 to
 * the raw value, so column 10 comes back as ':' and 63 as 'o'. */
esp_err_t pit100_cursor(pit100_t *d, int *row, int *col);

/* SI: read back what is DISPLAYED, from the cursor onward -- the shadow buffer,
 * not the message buffer, so it reports the panel rather than the last write. */
int pit100_read_screen(pit100_t *d, uint8_t *out, size_t cap);

/* Drain the line until it goes quiet. Never reads a fixed count: reply lengths
 * vary, and counting leaves the remainder in the buffer so every later read
 * falls one reply behind. tools/pit100.py:448-473. */
size_t pit100_drain(pit100_t *d, uint8_t *buf, size_t cap);

/* Hex-format the last transaction's bytes, for the status page. */
void pit100_hex(const uint8_t *b, size_t n, char *out, size_t cap);
