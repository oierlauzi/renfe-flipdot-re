; hardware.inc - I/O port and memory equates for the PAGE Iberica PI-T100-M
;
; Kept in sync with ghidra/symbols.csv and docs/4-inside-the-firmware.md. Assembler: Macro
; Assembler AS (asl), CPU 8085.

; ---------------------------------------------------------------------------
; Memory map - docs/4-inside-the-firmware.md
; ---------------------------------------------------------------------------
ROM_BASE        equ     0000h           ; U4 M27256, 32 KiB
ROM_END         equ     7FFFh
ROM_USED_END    equ     3047h           ; last non-erased byte

RAM_BASE        equ     0A000h          ; U2 M81C55-5 static RAM, 256 bytes
RAM_END         equ     0A0FFh

FONT_5X7        equ     1C0Dh           ; 256 glyphs x 5 columns - docs/5-the-panel.md
FONT_GLYPH_SIZE equ     5
FW_VERSION_STR  equ     0064h           ; "AK0052.139U1"

; ---------------------------------------------------------------------------
; U3 M82C51A-2 USART - external serial link
; ---------------------------------------------------------------------------
UART_DATA       equ     80h             ; R: RX data   W: TX data
UART_CTRL       equ     81h             ; R: status    W: mode / command

; Mode word bits (8251A)
UART_MODE_8E1_X16 equ   07Eh            ; 1 stop, even parity, 8 data, /16

; Command word bits
UART_CMD_TXEN   equ     001h
UART_CMD_DTR    equ     002h
UART_CMD_RXEN   equ     004h
UART_CMD_SBRK   equ     008h
UART_CMD_ERRRST equ     010h
UART_CMD_RTS    equ     020h
UART_CMD_IRESET equ     040h
UART_CMD_HUNT   equ     080h

; Status register bits (8251A)
UART_ST_TXRDY   equ     001h
UART_ST_RXRDY   equ     002h
UART_ST_TXEMPTY equ     004h
UART_ST_PE      equ     008h
UART_ST_OE      equ     010h
UART_ST_FE      equ     020h
UART_ST_SYNDET  equ     040h
UART_ST_DSR     equ     080h

; ---------------------------------------------------------------------------
; U2 M81C55-5 I/O section - DIP switches + UART baud clock
; ---------------------------------------------------------------------------
SW_CMD          equ     0A0h            ; command/status register
SW_PORT_A       equ     0A1h            ; DIP switch bank (input)
SW_PORT_B       equ     0A2h            ; DIP switches; bits 7:5 = baud select
SW_PORT_C       equ     0A3h            ; DIP switches; bit 5 = startup mode
SW_TIMER_LO     equ     0A4h            ; timer count LSB
SW_TIMER_HI     equ     0A5h            ; timer count MSB + mode bits

SW_BAUD_MASK    equ     0E0h            ; SW_PORT_B bits 7:5
SW_MODE_BIT     equ     020h            ; SW_PORT_C bit 5

; ---------------------------------------------------------------------------
; Display interface, group 0 - flip-dot panel (81C55-like, docs/4-inside-the-firmware.md)
; ---------------------------------------------------------------------------
DISP0_CMD       equ     0E0h
DISP0_PORT_A    equ     0E1h
DISP0_PORT_B    equ     0E2h
DISP0_PORT_C    equ     0E3h
DISP0_TIMER_LO  equ     0E4h
DISP0_TIMER_HI  equ     0E5h
XRAM_PAGE       equ     0E7h            ; R: capability (0FFh = board absent)
                                        ; W: page for the 0E000h window
XRAM_WINDOW     equ     0E000h          ; paged window into expansion memory
XRAM_ABSENT     equ     0FFh

; ---------------------------------------------------------------------------
; Display interface, group 1 - second board (absent in this unit)
; ---------------------------------------------------------------------------
DISP1_CMD       equ     0E8h
DISP1_PORT_A    equ     0E9h
DISP1_PORT_B    equ     0EAh
DISP1_PORT_C    equ     0EBh
DISP1_TIMER_LO  equ     0ECh
DISP1_TIMER_HI  equ     0EDh

; ---------------------------------------------------------------------------
; Unidentified - probable LED matrix interface. See docs/open-questions.md
; ---------------------------------------------------------------------------
IO_C0           equ     0C0h
IO_C1           equ     0C1h
IO_C2           equ     0C2h
IO_C3           equ     0C3h
IO_C4           equ     0C4h
IO_C5           equ     0C5h
IO_CD           equ     0CDh            ; read by the RST7.5 handler
IO_CE           equ     0CEh
IO_CF           equ     0CFh
IO_DE           equ     0DEh            ; two reads in the whole firmware
