/**
 * Matrix Orbital character display userspace driver for Linux
 *
 * Copyright (c) 2021 Frank Thaule
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>
#include <errno.h>
#include <sys/file.h>

#include "mtxorb.h"

#define MAX_WIDTH 40
#define MAX_HEIGHT 4
#define MAX_CELLWIDTH 5
#define MAX_CELLHEIGHT 8
#define MAX_CC 8 /* max. number of custom characters */

#define IS_LCD_TYPE (p->info->type == MTXORB_LCD)
#define IS_LKD_TYPE (p->info->type == MTXORB_LKD)
#define IS_VFD_TYPE (p->info->type == MTXORB_VFD)
#define IS_VKD_TYPE (p->info->type == MTXORB_VKD)

/* Alias for ignoring the warning -Wunused-result for write() */
#define Write(fd, buf, n) ((void)!write(fd, buf, n))

enum mtxorb_cc_mode_e
{
    cc_hbar,
    cc_vbar,
    cc_bignum,
    cc_custom
};

struct mtxorb_priv_s
{
    int fd; /* File descriptor */

    struct termios oldtio;

    /* Keep track of hbar, vbar, num, custom chars
     * mode, so we don't re-initialize on subsequent
     * calls.
    */
    int current_cc_mode;

    const struct mtxorb_info_s *info;
};

static int mtxorb_validate_info(const struct mtxorb_info_s *info);

MTXORB *mtxorb_open(const struct mtxorb_info_s *info)
{
    struct mtxorb_priv_s *p;
    struct termios oldtio, newtio;
    int fd;
    int speed;

    if ((info == NULL) || (mtxorb_validate_info(info) != 0)) {
        errno = EINVAL;
        return NULL;
    }

    switch (info->baudrate)
    {
    case 9600:
        speed = B9600;
        break;
    case 19200:
        speed = B19200;
        break;
    case 38400:
        speed = B38400;
        break;
    case 57600:
        speed = B57600;
        break;
    default:
        errno = EINVAL;
        return NULL;
    }

    if ((fd = open(info->portname, O_RDWR | O_NOCTTY)) == -1)
        return NULL;

    if (flock(fd, LOCK_EX | LOCK_NB) == -1)
        return NULL;

    /* Save current port settings */
    tcgetattr(fd, &oldtio);

    /* 8-N-1, blocking read by default unless using poll or select */
    memset(&newtio, 0, sizeof(struct termios));
    newtio.c_cflag = speed | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR | ICRNL;
    newtio.c_cc[VMIN] = 1;
    newtio.c_cc[VTIME] = 0;

    /* Flush input buffer and apply new settings */
    tcflush(fd, TCIFLUSH);
    
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
        return NULL;

    /* Allocate memory for the new handler */
    p = malloc(sizeof(struct mtxorb_priv_s));
    if (p == NULL)
        return NULL;

    p->fd = fd;
    p->info = (struct mtxorb_info_s *)info;
    memcpy(&p->oldtio, &oldtio, sizeof(struct termios));

    mtxorb_clear(p);
    mtxorb_set_cursor(p, 0, 0);

    return p;
}

void mtxorb_close(MTXORB *handle)
{
    struct mtxorb_priv_s *p = handle;

    if (p == NULL)
        return;

    if (p->fd) {
        mtxorb_clear(p);
        mtxorb_set_cursor_block(p, MTXORB_OFF);
        mtxorb_backlight_off(p);
        mtxorb_keypad_backlight_off(p);
        mtxorb_set_output(p, 0);
        
        /* Wait for pending output to be written */
        tcdrain(p->fd);
        /* Release lock */
        flock(p->fd, LOCK_UN);
        /* Restore old port settings */
        tcsetattr(p->fd, TCSANOW, &p->oldtio);
        /* Close the port */
        close(p->fd);
    }

    free(p);
}

/* ----- Text functions ----- */

void mtxorb_clear(MTXORB *handle)
{
    struct mtxorb_priv_s *p = handle;

    Write(p->fd, "\xFE"
        "X",
        2);
}

void mtxorb_putc(MTXORB *handle, char c)
{
    struct mtxorb_priv_s *p = handle;

    if (c == '\xFE') c = ' ';

    Write(p->fd, &c, 1);
}

void mtxorb_puts(MTXORB *handle, const char *s)
{
    struct mtxorb_priv_s *p = handle;
    char *c;

    /* Avoid creating an intermediate buffer,
     * so check and send bytes one by one */
    c = (char *)s;

    for (; *c != '\0'; ++c) {
        /* Replace command prefix char with space */
        if (*c == '\xFE')
            Write(p->fd, " ", 1);
        else
            Write(p->fd, c, 1);
    }
}

void mtxorb_write(MTXORB *handle, const void *buf, size_t nbytes)
{
    struct mtxorb_priv_s *p = handle;

    Write(p->fd, buf, nbytes);
}

ssize_t mtxorb_read(MTXORB *handle, void *buf, size_t nbytes, int timeout)
{
    struct mtxorb_priv_s *p = handle;
    struct pollfd fds[1];

    fds[0].fd = p->fd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    poll(fds, 1, timeout);
    if (fds[0].revents == 0)
        return 0;

    return read(fds[0].fd, buf, nbytes);
}

void mtxorb_set_cursor(MTXORB *handle, int x, int y)
{
    struct mtxorb_priv_s *p = handle;
    unsigned char out[] = {'\xFE', 'G', 0, 0};

    if ((x >= 0) && (x < p->info->width))
        out[2] = x + 1;
    if ((y >= 0) && (y < p->info->height))
        out[3] = y + 1;

    Write(p->fd, out, 4);
}

void mtxorb_set_cursor_block(MTXORB *handle, enum mtxorb_onoff_e on)
{
    struct mtxorb_priv_s *p = handle;
    unsigned char out[] = {'\xFE', 0};

    out[1] = (on == MTXORB_ON) ? 'S' : 'T';
    Write(p->fd, out, 2);
}

void mtxorb_set_cursor_uline(MTXORB *handle, enum mtxorb_onoff_e on)
{
    struct mtxorb_priv_s *p = handle;
    unsigned char out[] = {'\xFE', 0};

    out[1] = (on == MTXORB_ON) ? 'J' : 'K';
    Write(p->fd, out, 2);
}

void mtxorb_set_auto_scroll(MTXORB *handle, enum mtxorb_onoff_e on)
{
    struct mtxorb_priv_s *p = handle;
    unsigned char out[] = {'\xFE', 0};

    out[1] = (on == MTXORB_ON) ? 'Q' : 'R';
    Write(p->fd, out, 2);
}

void mtxorb_set_auto_line_wrap(MTXORB *handle, enum mtxorb_onoff_e on)
{
    struct mtxorb_priv_s *p = handle;
    unsigned char out[] = {'\xFE', 0};

    out[1] = (on == MTXORB_ON) ? 'C' : 'D';
    Write(p->fd, out, 2);
}

/* ----- Special characters functions ----- */

void mtxorb_set_custom_char(MTXORB *handle, int id, const char *data)
{
    struct mtxorb_priv_s *p = handle;
    unsigned char out[] = {'\xFE', 'N', 0, 0, 0, 0, 0, 0, 0, 0, 0};
    unsigned char mask = (1 << p->info->cellwidth) - 1;
    int i;

    if ((data == NULL) || (id < 0) || (id >= MAX_CC))
        return;

    out[2] = id;

    for (i = 0; i < p->info->cellheight; i++)
        out[i + 3] = data[i] & mask;

    Write(p->fd, out, 11);

    p->current_cc_mode = cc_custom;
}

void mtxorb_hbar(MTXORB *handle, int x, int y, int len, enum mtxorb_dir_e dir)
{
    struct mtxorb_priv_s *p = handle;
    unsigned char out[] = {'\xFE', 0, 0, 0, 0, 0};

    if ((x < 0) || (x >= p->info->width) ||
        (y < 0) || (y >= p->info->height) ||
        (len < 0) || (len > 100))
        return;

    /* Initialize the bar, replacing custom characters
     * currently present in memory bank 0.
     */
    if (p->current_cc_mode != cc_hbar) {
        out[1] = 'h';
        Write(p->fd, out, 2);

        p->current_cc_mode = cc_hbar;
    }

    /* Place the bar */
    out[1] = '\x7C';
    out[2] = x + 1;
    out[3] = y + 1;
    out[4] = (dir == MTXORB_LEFT) ? 1 : 0;
    out[5] = len;
    Write(p->fd, out, 6);
}

void mtxorb_vbar(MTXORB *handle, int x, int len, enum mtxorb_vbar_style_e style)
{
    struct mtxorb_priv_s *p = handle;
    unsigned char out[] = {'\xFE', 0, 0, 0};

    if ((x < 0) || (x >= p->info->width) ||
        (len < 0) || (len > 32))
        return;

    /* Initialize the bar, replacing custom characters currently present
     * in memory. */
    if (p->current_cc_mode != cc_vbar) {
        out[1] = (style == MTXORB_WIDE) ? 'v' : 'h';
        Write(p->fd, out, 2);

        p->current_cc_mode = cc_vbar;
    }

    /* Place the bar */
    out[1] = '=';
    out[2] = x + 1;
    out[3] = len;
    Write(p->fd, out, 4);
}

void mtxorb_bignum(MTXORB *handle, int x, int y, int digit, enum mtxorb_bignum_style_e style)
{
    struct mtxorb_priv_s *p = handle;
    unsigned char out[] = {'\xFE', 0, 0, 0, 0};

    if ((x < 0) || (x >= p->info->width) ||
        (digit < 0) || (digit > 9))
        return;

    /* Initialize the bar, replacing all custom characters
     * currently present. */
    if (p->current_cc_mode != cc_bignum) {
        out[1] = (style == MTXORB_LARGE) ? 'n' : 'm';
        Write(p->fd, out, 2);

        p->current_cc_mode = cc_bignum;
    }

    /* Place the digit */
    if (style == MTXORB_LARGE) {
        out[1] = '#';
        out[2] = x + 1;
        out[3] = digit;
        Write(p->fd, out, 4);
    }
    else {
        if ((y < 0) || (y >= p->info->height))
            return;
        /* Medium sized digit */
        out[1] = 'o';
        out[2] = y + 1;
        out[3] = x + 1;
        out[4] = digit;
        Write(p->fd, out, 5);
    }
}

/* ----- Display functions ----- */

void mtxorb_backlight_off(MTXORB *handle)
{
    struct mtxorb_priv_s *p = handle;

    if (IS_LCD_TYPE || IS_LKD_TYPE) {
        Write(p->fd, "\xFE" "F", 2);
    }
}

void mtxorb_set_contrast(MTXORB *handle, int value)
{
    struct mtxorb_priv_s *p = handle;
    unsigned char out[] = {'\xFE', 'P', 0};

    if ((value < 0) || (value > 255))
        return;

    if (IS_LCD_TYPE || IS_LKD_TYPE) {
        out[2] = value;
        Write(p->fd, out, 3);
    }
}

void mtxorb_set_brightness(MTXORB *handle, int value)
{
    struct mtxorb_priv_s *p = handle;
    unsigned char out[] = {'\xFE', 0, 0};

    if ((value < 0) || (value > 255))
        return;

    if (IS_VFD_TYPE || IS_VKD_TYPE) {
        if (value > 3) value = 3;
        out[1] = 'Y';
    }
    else out[1] = '\x99';

    out[2] = value;
    Write(p->fd, out, 3);
}

void mtxorb_set_bg_color(MTXORB *handle, int r, int g, int b)
{
    struct mtxorb_priv_s *p = handle;
    unsigned char out[] = {'\xFE', '\x82', 0, 0, 0};

    if (IS_LCD_TYPE || IS_LKD_TYPE) {
        out[2] = r & 0xFF;
        out[3] = g & 0xFF;
        out[4] = b & 0xFF;
        Write(p->fd, out, 5);
    }
}

/* ----- General Purpose Output functions ----- */

void mtxorb_set_output(MTXORB *handle, enum mtxorb_gpo_flags_e flags)
{
    struct mtxorb_priv_s *p = handle;
    unsigned char out[] = {'\xFE', 0, 0};
    int i;

    if (IS_LKD_TYPE || IS_VKD_TYPE) {
        for (i = 0; i < 6; i++) {
            out[1] = (flags & (1 << i)) ? 'W' : 'V';
            out[2] = i + 1;
            Write(p->fd, out, 3);
        }
    } else {
        /* Only one output on LCD/VFD displays */
        out[1] = (flags) ? 'W' : 'V';
        Write(p->fd, out, 2);
    }
}

/* ----- Keypad functions ----- */

void mtxorb_keypad_backlight_off(MTXORB *handle)
{
    struct mtxorb_priv_s *p = handle;

    if (IS_LKD_TYPE)
        Write(p->fd, "\xFE" "\x9B", 2);
}

void mtxorb_set_keypad_brightness(MTXORB *handle, int value)
{
    struct mtxorb_priv_s *p = handle;
    unsigned char out[] = {'\xFE', '\x9C', 0};

    if (IS_LKD_TYPE) {
        if ((value < 0) || (value > 255))
            return;

        out[2] = value;
        Write(p->fd, out, 3);
    }
}

void mtxorb_set_key_auto_tx(MTXORB *handle, enum mtxorb_onoff_e on)
{
    struct mtxorb_priv_s *p = handle;
    unsigned char out[] = {'\xFE', 0};

    if (IS_LKD_TYPE || IS_VKD_TYPE) {
        out[1] = (on == MTXORB_ON) ? 'A' : 'O';
        Write(p->fd, out, 2);
    }
}

void mtxorb_set_key_auto_repeat(MTXORB *handle, enum mtxorb_onoff_e on)
{
    struct mtxorb_priv_s *p = handle;
    unsigned char out[] = {'\xFE', '\x7E', 0};

    if (IS_LKD_TYPE || IS_VKD_TYPE) {
        out[2] = (on == MTXORB_ON) ? 1 : 0;
        Write(p->fd, out, 3);
    }
}

void mtxorb_set_key_debounce_time(MTXORB *handle, int value)
{
    struct mtxorb_priv_s *p = handle;
    unsigned char out[] = {'\xFE', 'U', 0};

    if ((value < 0) || (value > 255))
        return;

    if (IS_LKD_TYPE || IS_VKD_TYPE) {
        out[2] = value;
        Write(p->fd, out, 3);
    }
}

/* ------ Internal functions ----- */

static int mtxorb_validate_info(const struct mtxorb_info_s *info)
{
    if ((info->width < 0) || (info->width > MAX_WIDTH) ||
        (info->height < 0) || (info->height > MAX_HEIGHT))
        return -1;
    else if ((info->cellwidth < 0) || (info->cellwidth > MAX_CELLWIDTH) ||
             (info->cellheight < 0) || (info->cellheight > MAX_CELLHEIGHT))
        return -1;
    else if ((info->type != MTXORB_LCD) && (info->type != MTXORB_LKD) &&
             (info->type != MTXORB_VFD) && (info->type != MTXORB_VKD))
        return -1;

    return 0;
}
