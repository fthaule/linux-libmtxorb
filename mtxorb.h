/**
 * Matrix Orbital character display user-space driver for Linux
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

#ifndef _MTXORB_H
#define _MTXORB_H

#ifdef __cplusplus
extern "C" {
#endif

enum mtxorb_onoff {
    MTXORB_OFF,
    MTXORB_ON
};

enum mtxorb_dir {
    MTXORB_RIGHT,
    MTXORB_LEFT
};

enum mtxorb_vbar_style {
    MTXORB_NARROW,
    MTXORB_WIDE
};

enum mtxorb_bignum_style {
    MTXORB_MEDIUM,
    MTXORB_LARGE
};

enum mtxorb_gpo_flags {
    MTXORB_GPO1 = (1<<0),
    MTXORB_GPO2 = (1<<1),
    MTXORB_GPO3 = (1<<2),
    MTXORB_GPO4 = (1<<3),
    MTXORB_GPO5 = (1<<4),
    MTXORB_GPO6 = (1<<5)
};

enum mtxorb_type {
    MTXORB_LCD,     /* standard lcd */
    MTXORB_LKD,     /* lcd w/keypad */
    MTXORB_VFD,     /* vacuum fluorescent type */
    MTXORB_VKD      /* vacuum fluorescent w/keypad */
};

struct mtxorb_device_info {
    int width;              /* number of columns */
    int height;             /* number of rows */
    int cellwidth;          /* number of horizontal pixels in a cell (default: 5) */
    int cellheight;         /* number of vertical pixels in a cell (default: 8) */
    enum mtxorb_type type;  /* display model type */
};

typedef void MTXORB;


/**
 * Open a session for controlling a display.
 * @portname:   device port name, e.g. '/dev/ttyS0' or '/dev/ttyUSB0'
 * @baudrate:   communication speed. Valid values: 9600, 19200, 38400 and 57600.
 * @info:       pointer to display device info
 * @return valid handle or NULL in case of error
 */
extern MTXORB *mtxorb_open(const char *portname, int baudrate, const struct mtxorb_device_info *info);

/**
 * Close a session.
 */
extern void mtxorb_close(MTXORB *handle);

/* ----- Text related functions ----- */

/**
 * Move the cursor to the home position (0, 0).
 */
extern void mtxorb_home(MTXORB *handle);

/**
 * Clear the display.
 */
extern void mtxorb_clear(MTXORB *handle);

/**
 * Put a single character on the display.
 * @c: character to put
 */
extern void mtxorb_putc(MTXORB *handle, char c);

/**
 * Put a string on the display.
 * @s: pointer to null-terminated string
 */
extern void mtxorb_puts(MTXORB *handle, const char *s);

/**
 * Write raw data to the display
 * @buf:    pointer to buffer
 * @nbytes: number of bytes to write
 */
extern void mtxorb_write(MTXORB *handle, const void *buf, size_t nbytes);

/**
 * Read data from the display.
 * @buf:        pointer to buffer
 * @nbytes:     max number of bytes to read
 * @timeout:    number of milliseconds to wait for data, 0 = non-blocking
 * @return number of bytes read, or -1 if error
 */
extern ssize_t mtxorb_read(MTXORB *handle, void *buf, size_t nbytes, int timeout);

/**
 * Move the cursor to the specified position.
 * @x: column position, 0-based
 * @y: row position, 0-based
 */
extern void mtxorb_gotoxy(MTXORB *handle, int x, int y);

/**
 * Set blinking block cursor on/off.
 */
extern void mtxorb_set_cursor_block(MTXORB *handle, enum mtxorb_onoff on);

/**
 * Set underline cursor on/off.
 */
extern void mtxorb_set_cursor_uline(MTXORB *handle, enum mtxorb_onoff on);

/**
 * Set display auto scroll on/off.
 * @on: on: auto scroll (default), off: no scrolling
 */
extern void mtxorb_set_auto_scroll(MTXORB *handle, enum mtxorb_onoff on);

/**
 * Set line wrapping on/off.
 * @on: on: wrap to next line (default), off: no wrapping
 */
extern void mtxorb_set_auto_line_wrap(MTXORB *handle, enum mtxorb_onoff on);

/* ----- Special Characters related functions ----- */

/**
 * Set a custom character at the specified id in display's memory.
 * @id:    id of the custom character, 0-7
 * @data:  pointer to data
 */
extern void mtxorb_set_custom_char(MTXORB *handle, int id, const char *data);

/**
 * Place a horizontal bar on the screen. This will replace all custom characters
 * currently present in memory.
 * @x:     column start position, 0-based
 * @y:     row position, 0-based
 * @len:   length of the bar, 0-100
 * @dir:   direction of the bar
 */
extern void mtxorb_hbar(MTXORB *handle, int x, int y, int len, enum mtxorb_dir dir);

/**
 * Place a vertical bar on the screen. This will replace all custom characters
 * currently present in memory.
 * @x:     column position, 0-based
 * @len:   length of the bar, 0-32
 * @style: bar style
 */
extern void mtxorb_vbar(MTXORB *handle, int x, int len, enum mtxorb_vbar_style style);

/**
 * Place a big number on the screen. This will replace all custom characters
 * currently present in memory.
 * @x:     column position, 0-based
 * @y:     row position, 0-based
 * @digit: digit to place, 0-9
 * @style: digit style, medium or large
 */
extern void mtxorb_bignum(MTXORB *handle, int x, int y, int digit, enum mtxorb_bignum_style style);

/* ----- Display related functions ----- */

/**
 * Turn the display's backlight off.
 */
extern void mtxorb_backlight_off(MTXORB *handle);

/**
 * Set the display's contrast.
 * @value: 0-255 (default: 128)
 */
extern void mtxorb_set_contrast(MTXORB *handle, int value);

/**
 * Set the display's brightness.
 * @value: 0(dim)-255(bright) for LCD/LKD, 3(dim)-0(bright) for VFD/VKD
 */
extern void mtxorb_set_brightness(MTXORB *handle, int value);

/**
 * Set the display's background color for models that supports it.
 * @r: red channel (0-255)
 * @g: green channel (0-255)
 * @b: blue channel (0-255)
 */
extern void mtxorb_set_bg_color(MTXORB *handle, int r, int g, int b);

/* ----- General Purpose Output related functions ----- */

/**
 * Set general purpose output states.
 * Note: Check with your display the number of outputs.
 * @flags: e.g. MTXORB_GPO1 | MTXORB_GPO2
 */
extern void mtxorb_set_output(MTXORB *handle, enum mtxorb_gpo_flags flags);

/* ----- Keypad related functions ----- */

/**
 * Turn the keypad's backlight off.
 */
extern void mtxorb_keypad_backlight_off(MTXORB *handle);

/**
 * Set the keypad's brightness.
 * @value: 0(dim)-255(bright)
 */
extern void mtxorb_set_keypad_brightness(MTXORB *handle, int value);

/**
 * Set key press auto-repeat on/off.
 * @on: on: hold mode, off: typematic mode (default)
 */
extern void mtxorb_set_key_auto_repeat(MTXORB *handle, enum mtxorb_onoff on);

/**
 * Set key press debounce time.
 * @time: debounce time = value * 6.554ms, (default: 8)
 */
extern void mtxorb_set_key_debounce_time(MTXORB *handle, int value);

#ifdef __cplusplus
}
#endif

#endif /* _MTXORB_H */
