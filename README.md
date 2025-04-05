# linux-libmtxorb

A userspace driver for controlling Matrix Orbital character type displays (LCD/VFD w/wo keypad) in Linux through serial interface.

All commands that could alter the content of the non-volatile part of the display's memory is left out. This is done intentionally as a precaution to eliminate the risk of memory-wear. That said, the function **mtxorb_write()** allows for sending raw data/custom commands to the display.

Documentation for the various displays can be found here: https://www.matrixorbital.com/manuals.

## Using the driver

Copy the source and header files into your project.

## Contributing

Contributions to improving the driver in any aspect are most welcome! Make a pull request on https://github.com/fthaule/linux-libmtxorb/pulls with your changes. All changes gets reviewed, tested and iterated on before applied.

## Development

The driver has been developed on a computer running Linux and is written in C using GNU C99. Compiled and tested with GCC v13.3.0.

## License

Copyright (c) 2021 Frank Thaule (MIT). You can find a copy of the license in the LICENSE file.
