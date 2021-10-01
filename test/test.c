#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/termios.h>
#include <sys/poll.h>

#include "mtxorb.h"


static const struct mtxorb_device_info mtxorb_info = {
    20,         /* Columns */
    4,          /* Rows */
    5,          /* Cellwidth */
    8,          /* Cellheight */
    MTXORB_LKD  /* Device type */
};

static int is_running = 1;

static void handle_signal(int sig);


int main(void)
{
    MTXORB *mtxorb;
    char key;

    const char some_buffer[] = { 0x43, 0x6f, 0x66, 0x66, 0x65, 0x65 };
    const char custom_char[] = { 0x00, 0x0A, 0x1F, 0x1F, 0x0E, 0x04, 0x00, 0x00 };
    /* Bit assignment, i.e. 0b00000 is not allowed in c89, so we use hex representation */

    /* Capture Ctrl-C and kill signals */
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    mtxorb = mtxorb_open("/dev/ttyS3", 19200, &mtxorb_info);
    if (mtxorb == NULL) {
        perror("mtxorb_open");
        return EXIT_FAILURE;
    }

    printf("Connection opened. Press Ctrl-C to exit.\n");

/*******************************************************************/

    mtxorb_set_custom_char(mtxorb, 0, custom_char);

    mtxorb_set_brightness(mtxorb, 120);
    mtxorb_set_keypad_brightness(mtxorb, 20);

    mtxorb_gotoxy(mtxorb, 7, 3);

    /* Using this method of writing to the display could provide
     * useful when implementig a framebuffer. */
    mtxorb_write(mtxorb, some_buffer, sizeof(some_buffer));

    mtxorb_gotoxy(mtxorb, 5, 3);
    mtxorb_putc(mtxorb, 0);
    mtxorb_gotoxy(mtxorb, 14, 3);
    mtxorb_putc(mtxorb, 0);

    mtxorb_gotoxy(mtxorb, 3, 1);
    mtxorb_puts(mtxorb, "System Failure");

    mtxorb_set_cursor_block(mtxorb, MTXORB_ON);

    /* mtxorb_set_bg_color(mtxorb, 0, 255, 200); */

    /* mtxorb_set_contrast(mtxorb, 128); */
    /* mtxorb_set_bg_color(mtxorb, 0, 255, 150); */

    /* mtxorb_hbar(mtxorb, 0, 0, 50, MTXORB_RIGHT); */
    /* mtxorb_vbar(mtxorb, 0, 32, MTXORB_WIDE); */
    /* mtxorb_bignum(mtxorb, 0, 0, 5, MTXORB_LARGE); */

    /* mtxorb_set_output(mtxorb, MTXORB_GPO1 | MTXORB_GPO3 | MTXORB_GPO5); */

    /* mtxorb_set_key_auto_repeat(mtxorb, 1); */
    /* mtxorb_set_key_debounce_time(mtxorb, 8); */

/*******************************************************************/

    while (is_running) {
        /* Check for avail. input data with a timeout of 100ms */
        if (mtxorb_read(mtxorb, &key, 1, 100) > 0)
            printf("mtxorb: key pressed '%c' (0x%02X)\n", key, key);
    }

    mtxorb_backlight_off(mtxorb);
    mtxorb_keypad_backlight_off(mtxorb);
    mtxorb_set_cursor_block(mtxorb, MTXORB_OFF);
    /* mtxorb_set_output(mtxorb, 0); */

    mtxorb_close(mtxorb);
    printf("Connection closed\n");

    return EXIT_SUCCESS;
}

static void handle_signal(int signo)
{
    (void)signo;

    is_running = 0;
}
