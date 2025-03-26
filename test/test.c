#include <stdio.h>
#include <signal.h>

#include "mtxorb.h"

static const struct mtxorb_info_s lcd_info = {
    MTXORB_LCD,     /* Display type */
    20,             /* Display columns */
    4,              /* Display rows */
    5,              /* Display cell-width */
    8,              /* Display cell-height */
    "dev/ttyUSB0",  /* Connection portname */
    19200           /* Connection baudrate */
};

static int is_running = 1;

static void handle_signal(int sig);


int main(void)
{
    MTXORB *lcd;
    char key;

    const char some_buffer[] = { 0x43, 0x6f, 0x66, 0x66, 0x65, 0x65 };
    const char custom_char[] = { 0x00, 0x0A, 0x1F, 0x1F, 0x0E, 0x04, 0x00, 0x00 };
    /* Bit assignment, i.e. 0b00000 is not allowed in c89, so we use hex representation */

    /* Capture Ctrl-C and kill signals */
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    lcd = mtxorb_open(&lcd_info);
    if (lcd == NULL) {
        perror("Error opening connection");
        return EXIT_FAILURE;
    }

    printf("Connection established. Press Ctrl-C to exit.\n");

/*******************************************************************/

    mtxorb_set_custom_char(lcd, 0, custom_char);

    mtxorb_set_brightness(lcd, 120);
    mtxorb_set_keypad_brightness(lcd, 20);

    mtxorb_gotoxy(lcd, 7, 3);

    /* Using this method of writing to the display could provide
     * useful when implementig a framebuffer. */
    mtxorb_write(lcd, some_buffer, sizeof(some_buffer));

    mtxorb_gotoxy(lcd, 5, 3);
    mtxorb_putc(lcd, 0);
    mtxorb_gotoxy(lcd, 14, 3);
    mtxorb_putc(lcd, 0);

    mtxorb_gotoxy(lcd, 3, 1);
    mtxorb_puts(lcd, "System Failure");

    mtxorb_set_cursor_block(lcd, MTXORB_ON);

    /* mtxorb_set_bg_color(lcd, 0, 255, 200); */

    /* mtxorb_set_contrast(lcd, 128); */
    /* mtxorb_set_bg_color(lcd, 0, 255, 150); */

    /* mtxorb_hbar(lcd, 0, 0, 50, MTXORB_RIGHT); */
    /* mtxorb_vbar(lcd, 0, 32, MTXORB_WIDE); */
    /* mtxorb_bignum(lcd, 0, 0, 5, MTXORB_LARGE); */

    /* mtxorb_set_output(lcd, MTXORB_GPO1 | MTXORB_GPO3 | MTXORB_GPO5); */

    /* mtxorb_set_key_auto_repeat(lcd, 1); */
    /* mtxorb_set_key_debounce_time(lcd, 8); */

/*******************************************************************/

    while (is_running) {
        /* Check for avail. input data with a timeout of 100ms */
        if (mtxorb_read(lcd, &key, 1, 100) > 0)
            printf("lcd: key pressed '%c' (0x%02X)\n", key, key);
    }

    mtxorb_backlight_off(lcd);
    mtxorb_keypad_backlight_off(lcd);
    mtxorb_set_cursor_block(lcd, MTXORB_OFF);
    /* mtxorb_set_output(lcd, 0); */

    mtxorb_close(lcd);
    printf("Connection closed\n");

    return EXIT_SUCCESS;
}

static void handle_signal(int signo)
{
    (void)signo;

    is_running = 0;
}
