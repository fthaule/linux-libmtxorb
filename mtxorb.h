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

#ifndef MTXORB_H_
#define MTXORB_H_

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MTXORB_ENONE = 0,       // No error
    MTXORB_ENODEV,          // No such device
    MTXORB_ENOLCK,          // No locks available
    MTXORB_ENOMEM,          // Not enough space/cannot allocate memory
    MTXORB_ETERM,           // Terminal error
    MTXORB_EINVALBAUD,      // Invalid baudrate
    MTXORB_EINVALTYPE,      // Invalid module type
    MTXORB_EINVALSIZE,      // Invalid display size
    MTXORB_EINVALCSIZE,     // Invalid cell size
    MTXORB_EMAX_
} mtxorb_errorcode_t;

typedef enum {
    MTXORB_OFF,
    MTXORB_ON
} mtxorb_onoff_t;

typedef enum {
    MTXORB_TYPEMATIC,
    MTXORB_HOLD
} mtxorb_autorepeat_mode_t;

typedef enum {
    MTXORB_RIGHT,
    MTXORB_LEFT
} mtxorb_dir_t;

typedef enum {
    MTXORB_NARROW,
    MTXORB_WIDE
} mtxorb_vbar_style_t;

typedef enum {
    MTXORB_MEDIUM,
    MTXORB_LARGE
} mtxorb_bignum_style_t;

typedef enum {
    MTXORB_GPO1 = (1 << 0),
    MTXORB_GPO2 = (1 << 1),
    MTXORB_GPO3 = (1 << 2),
    MTXORB_GPO4 = (1 << 3),
    MTXORB_GPO5 = (1 << 4),
    MTXORB_GPO6 = (1 << 5)
} mtxorb_gpo_flags_t;

typedef enum {
    MTXORB_LCD,     // Standard LCD
    MTXORB_LKD,     // LCD w/keypad
    MTXORB_VFD,     // Vacuum fluorescent type
    MTXORB_VKD      // Vacuum fluorescent w/keypad
} mtxorb_module_type_t;

typedef struct {
    mtxorb_module_type_t type;  // Module type
    int width;                  // Number of columns
    int height;                 // Number of rows
    int cellwidth;              // Number of horizontal pixels in a cell (default: 5)
    int cellheight;             // Number of vertical pixels in a cell (default: 8)
    const char *portname;       // Device portname to use, e.g. /dev/ttySx or /dev/ttyUSBx
    int baudrate;               // Allowed rates are: 9600, 19200, 38400 and 57600
} mtxorb_info_t;

typedef void MTXORB; // Handler alias

/**
 * Open a session for controlling a display.
 * @portname:   device port name, e.g. '/dev/ttyS0', '/dev/ttyUSB0', '/dev/serial0'
 * @baudrate:   communication speed. Valid values: 9600, 19200, 38400 and 57600.
 * @info:       pointer to display device info
 * @return valid handle or NULL in case of error
 */
extern MTXORB *mtxorb_open(const mtxorb_info_t *info);

/**
 * Close a session.
 */
extern void mtxorb_close(MTXORB *handle);

/* ----- Text related functions ----- */

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
 * Write raw data to the display.
 * @buf:    pointer to buffer
 * @nbytes: number of bytes to write
 * @return number of bytes written, or -1 if error
 */
extern int mtxorb_write(MTXORB *handle, const void *buf, size_t nbytes);

/**
 * Read data from the display.
 * @buf:        pointer to buffer
 * @nbytes:     max number of bytes to read
 * @timeout:    number of milliseconds to wait for data, 0 = non-blocking
 * @return number of bytes read, or -1 if error
 */
extern int mtxorb_read(MTXORB *handle, void *buf, size_t nbytes, int timeout);

/**
 * Move cursor to the specified position.
 * @x: column position, 0-based
 * @y: row position, 0-based
 */
extern void mtxorb_set_cursor(MTXORB *handle, int x, int y);

/**
 * Move cursor to home position.
 */
extern void mtxorb_home(MTXORB *handle);

/**
 * Move cursor back.
 */
extern void mtxorb_move_cursor_back(MTXORB *handle);

/**
 * Move cursor forward.
 */
extern void mtxorb_move_cursor_forward(MTXORB *handle);

/**
 * Set blinking block cursor on/off.
 */
extern void mtxorb_set_cursor_block(MTXORB *handle, mtxorb_onoff_t on);

/**
 * Set underline cursor on/off.
 */
extern void mtxorb_set_cursor_uline(MTXORB *handle, mtxorb_onoff_t on);

/**
 * Set display auto scroll on/off.
 * @on: on: auto scroll (default), off: no scrolling
 */
extern void mtxorb_set_auto_scroll(MTXORB *handle, mtxorb_onoff_t on);

/**
 * Set line wrapping on/off.
 * @on: on: wrap to next line (default), off: no wrapping
 */
extern void mtxorb_set_auto_line_wrap(MTXORB *handle, mtxorb_onoff_t on);

/* ----- Special characters related functions ----- */

/**
 * Set a custom character at the specified id in display's memory.
 * @id:    id of the custom character, 0-7
 * @data:  pointer to data
 */
extern void mtxorb_create_custom_char(MTXORB *handle, int id, const char *data);

/**
 * Place a horizontal bar on the screen. This will replace all custom characters
 * currently present in memory.
 * @x:     column start position, 0-based
 * @y:     row position, 0-based
 * @len:   length of the bar, 0-100
 * @dir:   direction of the bar
 */
extern void mtxorb_hbar(MTXORB *handle, int x, int y, int len, mtxorb_dir_t dir);

/**
 * Place a vertical bar on the screen. This will replace all custom characters
 * currently present in memory.
 * @x:     column position, 0-based
 * @len:   length of the bar, 0-32
 * @style: bar style
 */
extern void mtxorb_vbar(MTXORB *handle, int x, int len, mtxorb_vbar_style_t style);

/**
 * Place a big number on the screen. This will replace all custom characters
 * currently present in memory.
 * @x:     column position, 0-based
 * @y:     row position, 0-based
 * @digit: digit to place, 0-9
 * @style: digit style, medium or large
 */
extern void mtxorb_bignum(MTXORB *handle, int x, int y, int digit, mtxorb_bignum_style_t style);

/* ----- Display related functions ----- */

/**
 * Turn the display's backlight on.
 * @minutes: Number of minutes to leave backlight on,
 * a value of 0 leaves the display on indefinitely.
 */
extern void mtxorb_backlight_on(MTXORB *handle, int minutes);

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

/* ----- General purpose output related functions ----- */

/**
 * Set general purpose output states.
 * Note: Check with your display the number of outputs.
 * @flags: e.g. MTXORB_GPO1 | MTXORB_GPO2
 */
extern void mtxorb_set_output(MTXORB *handle, mtxorb_gpo_flags_t flags);

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
 * Set if keypresses should be sent immediately or to use polling mode.
 * @value: on: send immediately, off: polling mode (default)
 */
extern void mtxorb_set_key_auto_tx(MTXORB *handle, mtxorb_onoff_t on);

/**
 * Set key press auto-repeat on/off.
 * @mode: hold or typematic (default)
 */
extern void mtxorb_set_key_autorepeat_mode(MTXORB *handle, mtxorb_autorepeat_mode_t mode);

/**
 * Turn off autorepeat mode. Default is on (typematic).
 */
extern void mtxorb_set_key_autorepeat_off(MTXORB *handle);

/**
 * Set key press debounce time.
 * @time: debounce time = value * 6.554ms, (default: 8)
 */
extern void mtxorb_set_key_debounce_time(MTXORB *handle, int value);

/**
 * Get current errorcode.
 * @return current errorcode
 */
extern int mtxorb_get_errorcode(void);

/**
 * Get current error as a string.
 * @return error in string format
 */
extern const char *mtxorb_get_error_str(void);

#ifdef __cplusplus
}
#endif

#endif /* MTXORB_H_ */
