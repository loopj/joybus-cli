# Joybus CLI

Command-line Joybus tester for scripting/automation of Joybus targets (N64/GCN controllers, N64 cartridges, etc), based on [libjoybus](https://github.com/loopj/libjoybus).

Response bytes are written to stdout as hex on success:

```
$ joybus-cli.py transfer 3 0x00
00 05 00
```

Failures are written to stderr with a non-zero exit code:

```
$ joybus-cli.py transfer 3 0x00
Timeout error
```

## Firmware

### Building

```bash
cmake -Bbuild . && cmake --build build
```

Requires `PICO_SDK_PATH` to point at the [pico-sdk](https://github.com/raspberrypi/pico-sdk).

### Flashing

Hold BOOTSEL while plugging in the Pico, then copy `build/joybus-cli.uf2` to the
`RPI-RP2` drive, or use `picotool`:

```bash
picotool load -f build/joybus-cli.uf2
```

## Host CLI

```bash
pip install -r requirements.txt
./joybus-cli.py transfer 3 0x00
```

The port is auto-detected from the Pico's USB vendor ID, you can override with `--port`.
