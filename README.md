# SteelSeries Rival 300 configuration utility

Command line utility for GNU/linux to configure SteelSeries Rival 300 mice.

## Dependencies:

- libusb-1.0

## Compile & install

```shell
$ make
$ sudo make install INSTALL_DIR=/usr/local/bin
```

### NOTES:

- make install set SUID bit to executable for non-root users to
  access USB device

## Usage

```shell
$ rival300
invalid arguments
usage: ./rival300 [OPTION=VALUE]...

OPTION             VALUE
  --pollrate       1 (1000 Hz)
                   2 (500 Hz)
                   3 (250 Hz)
                   4 (125 Hz)
  --sens1          50 - 6500 (step 50)
  --sens2          50 - 6500 (step 50)
  --wheel_effect   1 (static), 2 - 4 (pulse, slow->fast)
  --logo_effect    1 (static), 2 - 4 (pulse, slow->fast)
  --wheel_color    r,g,b (0-255,0-255,0-255)
  --logo_color     r,g,b (0-255,0-255,0-255)
```

## Examples

- set poll rate to 500 Hz, logo led to static blue
```shell
rival300 --pollrate 2 --logo_effect 1 --logo_color 0,0,255
```

- Switch off logo led
```shell
rival300 --wheel_color 0,0,0
```

## TODO

- validate/sanitize user input for commands
- do error checking (i.e. execute_commands)
