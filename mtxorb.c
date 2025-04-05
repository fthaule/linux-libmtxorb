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

#include "mtxorb.h"

#define MAX_WIDTH       40
#define MAX_HEIGHT      4
#define MAX_CELLWIDTH   5
#define MAX_CELLHEIGHT  8
#define MAX_CC          8   // Max. number of custom characters supported

//---------------------------------------------------------------------
// Command definitions
//---------------------------------------------------------------------
#define CMD_PREFIX                  '\xFE'

/* Communication */
// #define CMD_CHANGE_BAUDRATE         '\x39'
// #define CMD_CHANGE_I2C_ADDR         '\x33'
// #define CMD_XMIT_PROTOCOL_SELECT    '\xA0'
// #define CMD_SET_NON_STD_BAUDRATE    '\xA4'

/* Text */
#define CMD_CLEAR_SCREEN            '\x58'
// #define CMD_CHANGE_STARTUP_SCREEN   '\x40'
#define CMD_AUTO_SCROLL_ON          '\x51'
#define CMD_AUTO_SCROLL_OFF         '\x52'
#define CMD_AUTO_LINE_WRAP_ON       '\x43'
#define CMD_AUTO_LINE_WRAP_OFF      '\x44'
#define CMD_SET_CURSOR_POS          '\x47'
#define CMD_GO_HOME                 '\x48'
#define CMD_CURSOR_MOVE_BACK        '\x4C'
#define CMD_CURSOR_MOVE_FWD         '\x4D'
#define CMD_CURSOR_ULINE_ON         '\x4A'
#define CMD_CURSOR_ULINE_OFF        '\x4B'
#define CMD_CURSOR_BLOCK_ON         '\x53'
#define CMD_CURSOR_BLOCK_OFF        '\x54'

/* Special Characters */
#define CMD_CREATE_CC               '\x4E'
// #define CMD_LOAD_CC                 '\xC0'
// #define CMD_SAVE_CC                 '\xC1'
// #define CMD_SAVE_STARTUP_SCR_CC     '\xC2'
#define CMD_BIGNUM_MEDIUM           '\x6D'
#define CMD_BIGNUM_MEDIUM_PLACE     '\x6F'
#define CMD_BIGNUM_LARGE            '\x6E'
#define CMD_BIGNUM_LARGE_PLACE      '\x23'
#define CMD_HBAR_INIT               '\x68'
#define CMD_HBAR_PLACE              '\x7C'
#define CMD_VBAR_NARROW_INIT        '\x73'
#define CMD_VBAR_WIDE_INIT          '\x76'
#define CMD_VBAR_PLACE              '\x3D'

/* General Purpose Output */
#define CMD_GPO_OFF                 '\x56'
#define CMD_GPO_ON                  '\x57'
// #define CMD_GPO_SET_STARTUP_STATE   '\xC3'

/* Dallas One-Wire */
// #define CMD_OW_SEARCH               "\xC8\x02"
// #define CMD_OW_TRANSACTION          "\xC8\x01"

/* Keypad */
#define CMD_KEY_AUTOTX_ON           '\x41'
#define CMD_KEY_AUTOTX_OFF          '\x4F'
// #define CMD_KEY_POLL                '\x26'
// #define CMD_KEY_CLEAR_BUFFER        '\x45'
#define CMD_KEY_DEBOUNCE_TIME       '\x55'
#define CMD_KEY_AUTOREPEAT_MODE     '\x7E'
#define CMD_KEY_AUTOREPEAT_OFF      '\x60'
// #define CMD_KEYPAD_ASSIGN_CODES     '\xD5'
#define CMD_KEYPAD_BACKLIGHT_OFF    '\x9B' // LK only
#define CMD_KEYPAD_BRIGHTNESS       '\x9C' // LK only
// #define CMD_KEYPAD_BACKLIGHT_AUTO   '\x9D' // LK only

/* Display Functions */
#define CMD_BACKLIGHT_ON            '\x42'
#define CMD_BACKLIGHT_OFF           '\x46'
#define CMD_BRIGHTNESS_LCD          '\x99'
#define CMD_BRIGHTNESS_VFD          '\x59'
// #define CMD_BRIGHTNESS_SET_AND_SAVE '\x98'
#define CMD_BACKLIGHT_COLOR         '\x82' // LCD/LK only
#define CMD_CONTRAST                '\x50'
// #define CMD_CONTRAST_SET_AND_SAVE   '\x91'

/* Data Security */
// #define CMD_SET_REMEMBER            '\x93'
// #define CMD_DATA_LOCK               "\xCA\xF5\xA0"
// #define CMD_DATA_LOCK_SET_AND_SAVE  "\xCB\xF5\xA0"

/* Miscellaneous */
// #define CMD_CUSTOMER_DATA_WRITE     '\x34'
// #define CMD_CUSTOMER_DATA_READ      '\x35'
// #define CMD_VERSION_NUM_READ        '\x36'
// #define CMD_MODULE_TYPE_READ        '\x37'

//---------------------------------------------------------------------
// Macros
//---------------------------------------------------------------------
#define IS_LCD_TYPE (p->info->type == MTXORB_LCD)
#define IS_LKD_TYPE (p->info->type == MTXORB_LKD)
#define IS_VFD_TYPE (p->info->type == MTXORB_VFD)
#define IS_VKD_TYPE (p->info->type == MTXORB_VKD)

//---------------------------------------------------------------------
// Types
//---------------------------------------------------------------------

typedef enum {
    CC_HBAR,
    CC_VBAR,
    CC_BIGNUM,
    CC_CUSTOM
} mtxorb_cc_mode_t;

typedef struct {
    // File descriptor
    int fd;

    // Copy of original terminal settings
    struct termios oldtio;

    // Keep track of hbar, vbar, num, custom character mode,
    // so we don't re-initialize on subsequent calls.
    int current_cc_mode;

    // Display information
    const mtxorb_info_t *info;
} mtxorb_private_data_t;

//---------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------

static const char * const mtxorb_errors[] = {
    "No error",
    "No such device",
    "No locks available",
    "Not enough space/cannot allocate memory",
    "Terminal error",
    "Invalid baudrate",
    "Invalid module type",
    "Invalid display size",
    "Invalid cell size"
};

//---------------------------------------------------------------------
// Variables
//---------------------------------------------------------------------

static mtxorb_errorcode_t mtxorb_err = MTXORB_ENONE;

//---------------------------------------------------------------------
// Function prototypes
//---------------------------------------------------------------------
static int mtxorb_validate_info(const mtxorb_info_t *info);

//---------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------

MTXORB *mtxorb_open(const mtxorb_info_t *info)
{
    mtxorb_private_data_t *p;
    struct termios oldtio, newtio;
    int fd;
    int speed;

    if (info == NULL || mtxorb_validate_info(info) < 0)
        return NULL;

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
        mtxorb_err = MTXORB_EINVALBAUD;
        return NULL;
    }

    if ((fd = open(info->portname, O_RDWR | O_NOCTTY)) == -1) {
        mtxorb_err = MTXORB_ENODEV;
        return NULL;
    }

    /* Save original port settings */
    tcgetattr(fd, &oldtio);

    /* 8-N-1, blocking read by default unless using poll or select */
    memset(&newtio, 0, sizeof(struct termios));
    newtio.c_cflag = speed | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR | ICRNL;
    newtio.c_cc[VMIN] = 1;
    newtio.c_cc[VTIME] = 0;

    /* Flush input buffer and apply new settings */
    tcflush(fd, TCIFLUSH);
    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        mtxorb_err = MTXORB_ETERM;
        return NULL;
    }

    /* Allocate memory for the new handler */
    if ((p = malloc(sizeof(mtxorb_private_data_t))) == NULL) {
        mtxorb_err = MTXORB_ENOMEM;
        return NULL;
    }

    p->fd = fd;
    p->info = (mtxorb_info_t *)info;
    memcpy(&p->oldtio, &oldtio, sizeof(struct termios));

    mtxorb_clear(p);
    mtxorb_home(p);

    return p;
}

void mtxorb_close(MTXORB *handle)
{
    mtxorb_private_data_t *p = handle;

    if (p != NULL) {
        if (p->fd >= 0) {
            mtxorb_backlight_off(p);
            mtxorb_clear(p);
            mtxorb_set_cursor_block(p, MTXORB_OFF);
            mtxorb_keypad_backlight_off(p);
            mtxorb_set_output(p, 0);
            
            // Wait for pending output to be written
            tcdrain(p->fd);
            // Restore old terminal settings
            tcsetattr(p->fd, TCSANOW, &p->oldtio);
            // Close the port
            close(p->fd);
        }

        free(p);
    }
}

/* ----- Text functions ----- */

void mtxorb_clear(MTXORB *handle)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, CMD_CLEAR_SCREEN };

    mtxorb_write(p, out, 2);
}

void mtxorb_putc(MTXORB *handle, char c)
{
    mtxorb_private_data_t *p = handle;

    /* Replace command prefix with space */
    if (c == CMD_PREFIX)
        c = ' ';

    mtxorb_write(p, &c, 1);
}

void mtxorb_puts(MTXORB *handle, const char *s)
{
    mtxorb_private_data_t *p = handle;
    char *c;

    // Avoid creating an intermediate buffer,
    // so check and send bytes one by one
    c = (char *)s;

    for (; *c != '\0'; ++c) {
        // Replace command prefix with space
        if (*c == CMD_PREFIX)
            mtxorb_write(p, " ", 1);
        else
            mtxorb_write(p, c, 1);
    }
}

int mtxorb_write(MTXORB *handle, const void *buf, size_t nbytes)
{
    mtxorb_private_data_t *p = handle;

    return (int)write(p->fd, buf, nbytes);
}

int mtxorb_read(MTXORB *handle, void *buf, size_t nbytes, int timeout)
{
    mtxorb_private_data_t *p = handle;
    struct pollfd fds[1];

    fds[0].fd = p->fd;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    poll(fds, 1, timeout);
    if (fds[0].revents == 0)
        return 0;

    return (int)read(fds[0].fd, buf, nbytes);
}

void mtxorb_set_cursor(MTXORB *handle, int x, int y)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, CMD_SET_CURSOR_POS, 0, 0 };

    if ((x >= 0 && x < p->info->width) &&
        (y >= 0 && y < p->info->height))
    {
        out[2] = x + 1;
        out[3] = y + 1;
        mtxorb_write(p, out, 4);
    }
}

void mtxorb_home(MTXORB *handle)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, CMD_GO_HOME };

    mtxorb_write(p, out, 2);
}

void mtxorb_move_cursor_back(MTXORB *handle)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, CMD_CURSOR_MOVE_BACK };

    mtxorb_write(p, out, 2);
}

void mtxorb_move_cursor_forward(MTXORB *handle)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, CMD_CURSOR_MOVE_FWD };

    mtxorb_write(p, out, 2);
}

void mtxorb_set_cursor_block(MTXORB *handle, mtxorb_onoff_t on)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, 0 };

    out[1] = on ? CMD_CURSOR_BLOCK_ON : CMD_CURSOR_BLOCK_OFF;
    mtxorb_write(p, out, 2);
}

void mtxorb_set_cursor_uline(MTXORB *handle, mtxorb_onoff_t on)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, 0 };

    out[1] = on ? CMD_CURSOR_ULINE_ON : CMD_CURSOR_ULINE_OFF;
    mtxorb_write(p, out, 2);
}

void mtxorb_set_auto_scroll(MTXORB *handle, mtxorb_onoff_t on)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, 0 };

    out[1] = on ? CMD_AUTO_SCROLL_ON : CMD_AUTO_SCROLL_OFF;
    mtxorb_write(p, out, 2);
}

void mtxorb_set_auto_line_wrap(MTXORB *handle, mtxorb_onoff_t on)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, 0 };

    out[1] = on ? CMD_AUTO_LINE_WRAP_ON : CMD_AUTO_LINE_WRAP_OFF;
    mtxorb_write(p, out, 2);
}

/* ----- Special characters functions ----- */

void mtxorb_create_custom_char(MTXORB *handle, int id, const char *data)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, CMD_CREATE_CC, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    unsigned char mask = (1 << p->info->cellwidth) - 1;
    int i;

    if (data && (id >= 0 && id < MAX_CC)) {
        out[2] = id;

        for (i = 0; i < p->info->cellheight; i++)
            out[i + 3] = data[i] & mask;

        mtxorb_write(p, out, 11);

        p->current_cc_mode = CC_CUSTOM;
    }
}

void mtxorb_hbar(MTXORB *handle, int x, int y, int len, mtxorb_dir_t dir)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, 0, 0, 0, 0, 0 };

    if ((x >= 0 && x < p->info->width) &&
        (y >= 0 && y < p->info->height) &&
        (len >= 0 && len <= 100))
    {
        /* Initialize the bar, replacing custom characters
        * currently present in memory bank 0.
        */
        if (p->current_cc_mode != CC_HBAR) {
            out[1] = CMD_HBAR_INIT;
            mtxorb_write(p, out, 2);

            p->current_cc_mode = CC_HBAR;
        }

        /* Place the bar */
        out[1] = CMD_HBAR_PLACE;
        out[2] = x + 1;
        out[3] = y + 1;
        out[4] = (dir == MTXORB_LEFT) ? 1 : 0;
        out[5] = len;
        mtxorb_write(p, out, 6);
    }
}

void mtxorb_vbar(MTXORB *handle, int x, int len, mtxorb_vbar_style_t style)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, 0, 0, 0 };

    if ((x >= 0 && x < p->info->width) &&
        (len >= 0 && len <= 32))
    {
        /* Initialize the bar, replacing custom characters currently present
        * in memory. */
        if (p->current_cc_mode != CC_VBAR) {
            out[1] = (style == MTXORB_WIDE) ? CMD_VBAR_WIDE_INIT : CMD_VBAR_NARROW_INIT;
            mtxorb_write(p, out, 2);

            p->current_cc_mode = CC_VBAR;
        }

        /* Place the bar */
        out[1] = CMD_VBAR_PLACE;
        out[2] = x + 1;
        out[3] = len;
        mtxorb_write(p, out, 4);
    }
}

void mtxorb_bignum(MTXORB *handle, int x, int y, int digit, mtxorb_bignum_style_t style)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, 0, 0, 0, 0 };

    if ((x >= 0 && x < p->info->width) &&
        (digit >= 0 && digit < 10))
    {
        /* Initialize the bar, replacing all custom characters
        * currently present. */
        if (p->current_cc_mode != CC_BIGNUM) {
            out[1] = (style == MTXORB_LARGE) ? CMD_BIGNUM_LARGE : CMD_BIGNUM_MEDIUM;
            mtxorb_write(p, out, 2);

            p->current_cc_mode = CC_BIGNUM;
        }

        /* Place the digit */
        if (style == MTXORB_LARGE) {
            out[1] = CMD_BIGNUM_LARGE_PLACE;
            out[2] = x + 1;
            out[3] = digit;
            mtxorb_write(p, out, 4);
        } else {
            if (y >= 0 && y < p->info->height) {
                /* Medium sized digit */
                out[1] = CMD_BIGNUM_MEDIUM_PLACE;
                out[2] = y + 1;
                out[3] = x + 1;
                out[4] = digit;
                mtxorb_write(p, out, 5);
            }
        }
    }
}

/* ----- Display functions ----- */

void mtxorb_backlight_on(MTXORB *handle, int minutes)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, CMD_BACKLIGHT_ON, 0 };

    if (IS_LCD_TYPE || IS_LKD_TYPE) {
        if (minutes > 0 && minutes < 256)
            out[2] = (unsigned char)minutes;
        
        mtxorb_write(p, out, 3);
    }
}

void mtxorb_backlight_off(MTXORB *handle)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, CMD_BACKLIGHT_OFF };

    if (IS_LCD_TYPE || IS_LKD_TYPE) {
        mtxorb_write(p, out, 2);
    }
}

void mtxorb_set_contrast(MTXORB *handle, int value)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, CMD_CONTRAST, 0 };

    if ((IS_LCD_TYPE || IS_LKD_TYPE) &&
        (value >= 0 && value < 256))
    {
        out[2] = value;
        mtxorb_write(p, out, 3);    
    }
}

void mtxorb_set_brightness(MTXORB *handle, int value)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, 0, 0 };

    if (value >= 0 && value < 256) {
        if (IS_VFD_TYPE || IS_VKD_TYPE) {
            if (value > 3) value = 3;
            out[1] = CMD_BRIGHTNESS_VFD;
        } else
            out[1] = CMD_BRIGHTNESS_LCD;
    
        out[2] = value;
        mtxorb_write(p, out, 3);
    }
}

void mtxorb_set_bg_color(MTXORB *handle, int r, int g, int b)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, CMD_BACKLIGHT_COLOR, 0, 0, 0 };

    if (IS_LCD_TYPE || IS_LKD_TYPE) {
        out[2] = (r & 0xFF);
        out[3] = (g & 0xFF);
        out[4] = (b & 0xFF);
        mtxorb_write(p, out, 5);
    }
}

/* ----- General Purpose Output functions ----- */

void mtxorb_set_output(MTXORB *handle, mtxorb_gpo_flags_t flags)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, 0, 0 };
    int i;

    if (IS_LKD_TYPE || IS_VKD_TYPE) {
        for (i = 0; i < 6; i++) {
            out[1] = (flags & (1 << i)) ? CMD_GPO_ON : CMD_GPO_OFF;
            out[2] = i + 1;
            mtxorb_write(p, out, 3);
        }
    } else {
        /* Only one output on LCD/VFD displays */
        out[1] = flags ? CMD_GPO_ON : CMD_GPO_OFF;
        mtxorb_write(p, out, 2);
    }
}

/* ----- Keypad functions ----- */

void mtxorb_keypad_backlight_off(MTXORB *handle)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, CMD_KEYPAD_BACKLIGHT_OFF };

    if (IS_LKD_TYPE)
        mtxorb_write(p, out, 2);
}

void mtxorb_set_keypad_brightness(MTXORB *handle, int value)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, CMD_KEYPAD_BRIGHTNESS, 0 };

    if (IS_LKD_TYPE && (value >= 0 && value < 256)) {
        out[2] = value;
        mtxorb_write(p, out, 3);
    }
}

void mtxorb_set_key_auto_tx(MTXORB *handle, mtxorb_onoff_t on)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, 0 };

    if (IS_LKD_TYPE || IS_VKD_TYPE) {
        out[1] = on ? CMD_KEY_AUTOTX_ON : CMD_KEY_AUTOTX_OFF;
        mtxorb_write(p, out, 2);
    }
}

void mtxorb_set_key_autorepeat_mode(MTXORB *handle, mtxorb_autorepeat_mode_t mode)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, CMD_KEY_AUTOREPEAT_MODE, 0 };

    if (IS_LKD_TYPE || IS_VKD_TYPE) {
        out[2] = mode ? 1 : 0;
        mtxorb_write(p, out, 3);
    }
}

void mtxorb_set_key_autorepeat_off(MTXORB *handle)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, CMD_KEY_AUTOREPEAT_OFF };

    if (IS_LKD_TYPE || IS_VKD_TYPE)
        mtxorb_write(p, out, 2);
}

void mtxorb_set_key_debounce_time(MTXORB *handle, int value)
{
    mtxorb_private_data_t *p = handle;
    unsigned char out[] = { CMD_PREFIX, CMD_KEY_DEBOUNCE_TIME, 0 };

    if ((IS_LKD_TYPE || IS_VKD_TYPE) &&
        (value >= 0 && value < 256))
    {
        out[2] = value;
        mtxorb_write(p, out, 3);
    }
}

int mtxorb_get_errorcode(void)
{
    return mtxorb_err;
}

const char *mtxorb_get_error_str(void)
{
    if (mtxorb_err >= 0 && mtxorb_err < MTXORB_EMAX_)
        return mtxorb_errors[mtxorb_err];
    
    return "Unknown error";
}

//---------------------------------------------------------------------
// Private functions
//---------------------------------------------------------------------

static int mtxorb_validate_info(const mtxorb_info_t *info)
{
    if (info->type < 0 || info->type > MTXORB_VKD) {
        mtxorb_err = MTXORB_EINVALTYPE;
        return -1;
    }
    if ((info->width < 0 || info->width > MAX_WIDTH) ||
        (info->height < 0 || info->height > MAX_HEIGHT)) {
        mtxorb_err = MTXORB_EINVALSIZE;
        return -1;
    }
    if ((info->cellwidth < 0 || info->cellwidth > MAX_CELLWIDTH) ||
        (info->cellheight < 0 || info->cellheight > MAX_CELLHEIGHT)) {
        mtxorb_err = MTXORB_EINVALCSIZE;
        return -1;
    }

    return 0;
}
