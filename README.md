# Arduino / ESP32 Wired Game Controller

> A DIY USB game controller built on an Arduino Uno (or ESP32), with host-side
> receivers that turn its serial output into a virtual Xbox gamepad or keyboard on
> Linux and Windows.

## Overview

This project implements a complete homemade game-controller stack:

1. **Firmware** running on an Arduino Uno or ESP32 reads an analog joystick,
   several push buttons and drives status LEDs, then streams the input state over
   the USB serial link as a simple comma-separated line at 115200 baud.
2. A **host-side receiver** on the PC reads that serial stream and injects it into
   the operating system as either a virtual Xbox 360 gamepad or a virtual
   keyboard, so any game can be played with the physical controller.
3. A **connection heartbeat**: the PC periodically sends `CONNECTED` back to the
   board, which lights a green LED while the link is alive and a red LED when it is
   not.

The controller was built and 3D-printed as a hardware/software integration
project; the STL files for the printed enclosure are included.

## Key features

- Analog joystick (X/Y + click) with configurable dead-zones
- Five push buttons mapped to gamepad / keyboard actions
- Connection-status LEDs driven by a serial heartbeat (green = connected, red = idle)
- Two firmware targets: **Arduino Uno** (10-bit ADC) and **ESP32** (12-bit ADC)
- Three host receivers:
  - Virtual **Xbox 360 gamepad** on Linux (`evdev`/`uinput`)
  - Virtual **keyboard** on Linux, written in C (`uinput`), that maps controller inputs to keyboard keys for any game
  - Virtual **Xbox gamepad** on Windows (`vgamepad` + ViGEmBus)
- 3D-printable controller enclosure (STL)

## Serial protocol

The firmware prints one line per poll (~50 Hz):

```
joyX,joyY,joySW,btn1,btn2,btn3,btn4
```

- `joyX`, `joyY` — raw ADC readings (Uno: 0–1023, ESP32: 0–4095)
- `joySW` — joystick push (1 = pressed)
- `btn1..btn4` — push buttons (1 = pressed, active-low inputs inverted in firmware)

The PC sends `CONNECTED\n` roughly every 250 ms as a heartbeat.

## Hardware

| Component | Arduino Uno pins | ESP32 pins |
|-----------|------------------|------------|
| Joystick X (VRx) | A0 | GPIO34 |
| Joystick Y (VRy) | A1 | GPIO35 |
| Joystick switch  | D2 (`INPUT_PULLUP`) | GPIO32 |
| Button 1 | D3 | GPIO13 |
| Button 2 | D4 | GPIO12 |
| Button 3 | D5 | GPIO14 |
| Button 4 | D6 | GPIO27 |
| Button 5 (ESP32 only) | — | GPIO26 |
| Green "connected" LED | D7 | GPIO33 |
| Red "idle" LED | D8 | GPIO25 |

Pin assignments are taken directly from the firmware source — adjust there if your
wiring differs. Buttons use the internal pull-ups and are wired to ground; LEDs use
series resistors (~220 Ω).

## Project structure

```
.
├── firmware/
│   ├── arduino-uno/arduino_uno_controller.ino   # Uno firmware (10-bit ADC)
│   └── esp32/esp32_controller.ino               # ESP32 firmware (12-bit ADC)
├── receiver/
│   ├── linux/
│   │   ├── gamepad_receiver.py    # virtual Xbox 360 gamepad (evdev/uinput)
│   │   ├── keyboard_receiver.c    # virtual keyboard in C (uinput), controller -> keyboard keys
│   │   └── setup_and_run.sh       # compile + run the C keyboard receiver
│   └── windows/
│       └── windows_receiver.py    # virtual Xbox gamepad via vgamepad + ViGEmBus
└── hardware/                      # 3D-printable enclosure (STL)
```

## Getting started

### 1. Flash the firmware

Open the matching sketch in the Arduino IDE and upload it to your board:

- Arduino Uno: `firmware/arduino-uno/arduino_uno_controller.ino`
- ESP32: `firmware/esp32/esp32_controller.ino` (install the ESP32 board package first)

Both run at 115200 baud. Note the serial device (e.g. `/dev/ttyACM0`, `/dev/ttyUSB0`,
or `COM3`) — you may need to set it at the top of the receiver you use.

### 2. Run a receiver on the PC

**Linux — virtual Xbox gamepad:**

```bash
sudo apt install python3-evdev python3-serial
sudo python3 receiver/linux/gamepad_receiver.py
```

**Linux — virtual keyboard (for games that only accept keyboard input):**

```bash
cd receiver/linux
./setup_and_run.sh        # installs gcc, loads uinput, compiles and runs keyboard_receiver.c
```

**Windows — virtual Xbox gamepad:**

```bash
pip install vgamepad pyserial
# install the ViGEmBus driver: https://github.com/nefarius/ViGEmBus/releases
python receiver/windows/windows_receiver.py
```

Edit `SERIAL_PORT` at the top of the chosen receiver to match your board's port.
`uinput` access on Linux requires `sudo` or membership of the `input` group.

### Keyboard mapping

The C keyboard receiver translates the controller state into key presses:

| Controller input | Keyboard key |
|------------------|--------------|
| Joystick left / right / up / down | Arrow keys |
| Button 1 | Enter |
| Button 2 | Left Ctrl |
| Button 3 | Esc |
| Button 4 | J |
| Joystick click | Space |

Change the `KEY_*` codes in `receiver/linux/keyboard_receiver.c` to suit a particular game.

## Usage

With the firmware flashed and a receiver running, the board's green LED lights once
the heartbeat is received and the controller acts as a standard gamepad or keyboard,
usable in any game that supports either input (e.g. SuperTuxKart via the gamepad
receiver).

## Academic context

Developed as an embedded-systems / C programming integration project at **TEKUP
University** (Tunisia).

## Author

**Nasrallah Hadhri** — hardware, firmware and host-side receivers — [github.com/Nasrallah-Hadhri](https://github.com/Nasrallah-Hadhri)

## License

Released under the [MIT License](LICENSE).
