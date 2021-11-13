#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/termios.h>
#include <sys/poll.h>

#include "mtxorb.h"

static const struct mtxorb_info lcd1_info = {
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
    MTXORB *lcd1;
    char key;

    const char some_buffer[] = { 0x43, 0x6f, 0x66, 0x66, 0x65, 0x65 };
    const char custom_char[] = { 0x00, 0x0A, 0x1F, 0x1F, 0x0E, 0x04, 0x00, 0x00 };
    /* Bit assignment, i.e. 0b00000 is not allowed in c89, so we use hex representation */

    /* Capture Ctrl-C and kill signals */
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    lcd1 = mtxorb_open(&lcd1_info);
    if (lcd1 == NULL) {
        perror("lcd1_open");
        return EXIT_FAILURE;
    }

    printf("Connection established. Press Ctrl-C to exit.\n");

/*******************************************************************/

    mtxorb_set_custom_char(lcd1, 0, custom_char);

    mtxorb_set_brightness(lcd1, 120);
    mtxorb_set_keypad_brightness(lcd1, 20);

    mtxorb_gotoxy(lcd1, 7, 3);

    /* Using this method of writing to the display could provide
     * useful when implementig a framebuffer. */
    mtxorb_write(lcd1, some_buffer, sizeof(some_buffer));

    mtxorb_gotoxy(lcd1, 5, 3);
    mtxorb_putc(lcd1, 0);
    mtxorb_gotoxy(lcd1, 14, 3);
    mtxorb_putc(lcd1, 0);

    mtxorb_gotoxy(lcd1, 3, 1);
    mtxorb_puts(lcd1, "System Failure");

    mtxorb_set_cursor_block(lcd1, MTXORB_ON);

    /* mtxorb_set_bg_color(lcd1, 0, 255, 200); */

    /* mtxorb_set_contrast(lcd1, 128); */
    /* mtxorb_set_bg_color(lcd1, 0, 255, 150); */

    /* mtxorb_hbar(lcd1, 0, 0, 50, MTXORB_RIGHT); */
    /* mtxorb_vbar(lcd1, 0, 32, MTXORB_WIDE); */
    /* mtxorb_bignum(lcd1, 0, 0, 5, MTXORB_LARGE); */

    /* mtxorb_set_output(lcd1, MTXORB_GPO1 | MTXORB_GPO3 | MTXORB_GPO5); */

    /* mtxorb_set_key_auto_repeat(lcd1, 1); */
    /* mtxorb_set_key_debounce_time(lcd1, 8); */

/*******************************************************************/

    while (is_running) {
        /* Check for avail. input data with a timeout of 100ms */
        if (mtxorb_read(lcd1, &key, 1, 100) > 0)
            printf("lcd1: key pressed '%c' (0x%02X)\n", key, key);
    }

    mtxorb_backlight_off(lcd1);
    mtxorb_keypad_backlight_off(lcd1);
    mtxorb_set_cursor_block(lcd1, MTXORB_OFF);
    /* mtxorb_set_output(lcd1, 0); */

    mtxorb_close(lcd1);
    printf("Connection closed\n");

    return EXIT_SUCCESS;
}

static void handle_signal(int signo)
{
    (void)signo;

    is_running = 0;
}
