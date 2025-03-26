# linux-libmtxorb

A userspace driver for controlling Matrix Orbital character type displays (LCD/VFD w/wo keypad) in Linux through serial (UART) interface.

The driver is designed in a modular way to be able to control multiple displays.

All commands that could alter the content of the non-volatile part of the display's memory is left out. This is done intentionally as a precaution to eliminate the risk of memory-wear. That said, the function **mtxorb_write()** allows for sending raw data/custom commands to the display. Use with caution as you could potentially brick your display!

Documentation for the displays can be found here: http://www.matrixorbital.com/manuals.

## Using the driver

Copy the source and header files directly into your project.

## Simple Implementation Example

```
#include <stdio.h>
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

int main(void)
{
    MTXORB *lcd1;

    lcd1 = mtxorb_open(&lcd_info1);
    if (lcd1 == NULL)
        return -1;

    mtxorb_set_brightness(lcd1, 120);

    mtxorb_gotoxy(lcd1, 3, 1);
    mtxorb_puts(lcd1, "System Failure");

    mtxorb_close(lcd1);

    return 0;
}
```

Have a look at the code in the test directory for a more advanced use-case.

## Contributing

Contributions to improving the driver in any aspect are most welcome! Make a pull request on https://github.com/fratha/linux-libmtxorb/pulls with your changes. All changes gets reviewed, tested and iterated on before applied.

## Development

The driver has been developed on a computer running Linux and is written in C using the ISO C89/C90 standard with portability in mind. Tested with GCC v13.3.0.

## License

Copyright (c) 2021 Frank Thaule (MIT). You can find a copy of the license in the LICENSE file.
