# linux-libmtxorb

A user-space driver for controlling Matrix Orbital character type displays (LCD/VFD w/wo keypad) in Linux through serial (UART) interface.

The driver is designed in a modular way to be able to control multiple display instances. It is also made ready to be included directly in a C++ project.
At this point in time, I2C and OneWire support are not implemented. Maybe it'll be realized in a future release if there is a interest for it.

In addition, all commands that could alter the content of the non-volatile part of the display's memory is left out. This is done intentionally as a precaution to eliminate the risk of memory-wear. That said, the function **mtxorb_write()** allows for sending raw data/custom commands to the display. Use with caution as you could potentially brick your display!

Documentation for the displays can be found here: http://www.matrixorbital.com/manuals.

## Using the driver

Option 1: Copy the source and header files as is, directly into your project.

Option 2: Build as a static library:
```
$ make lib-static
```
Option 3: Build as a shared library, ready for system wide use (not recommended):
```
$ make lib-shared
```

## Simple Implementation Example

```
#include <stdio.h>
#include "mtxorb.h"

/* Display specifications */
#define LCD_WIDTH        20
#define LCD_HEIGHT       4
#define LCD_CELLWIDTH    5
#define LCD_CELLHEIGHT   8

/* Connection details */
#define LCD_PORTNAME     "/dev/ttyUSB0"
#define LCD_BAUDRATE     19200


static const struct mtxorb_device_info lcd_dev_info = {
    LCD_WIDTH, LCD_HEIGHT,
    LCD_CELLWIDTH, LCD_CELLHEIGHT,
    MTXORB_LCD /* Display type: MTXORB_LCD, MTXORB_LKD, MTXORB_VFD or MTXORB_VKD */
};

int main(void)
{
    MTXORB *lcd;

    lcd = mtxorb_open(LCD_PORTNAME, LCD_BAUDRATE, &lcd_dev_info);
    if (lcd == NULL)
        return -1;

    mtxorb_set_brightness(lcd, 120);

    mtxorb_gotoxy(lcd, 3, 1);
    mtxorb_puts(lcd, "System Failure");

    mtxorb_close(lcd);

    return (0);
}
```
Have a look at the code in the test directory for a more advanced use case.

## Contributing

Contributions to improving the driver in any aspect are most welcome! Make a pull request on https://github.com/fratha/linux-libmtxorb/pulls with your changes. All changes gets reviewed, tested and iterated on before applied.

## Development

The driver has been developed on a computer running Linux and is written in C using the ISO C89/C90 standard with portability in mind. Compiled with GCC v9.3.0.

## License

Copyright (c) 2021 Frank Thaule (MIT). You can find a copy of the license in the LICENSE file.
