# STM32 Firmware Updater

A desktop application and custom bootloader firmware for updating STM32 microcontrollers over USB CDC and UART communication.

The project provides a graphical interface for firmware management, flash programming, memory operations, and firmware integrity verification.

## Features

- USB CDC and UART communication through serial COM ports
- Firmware upload using `.bin` files
- CRC32-based firmware integrity verification
- Automatic firmware image and vector table validation
- Flash memory erase and programming
- Bootloader version, status, and chip identification
- Flash memory reading and CRC calculation
- Firmware upload progress monitoring
- Application jump and microcontroller reset commands
- Real-time operation logs
- Dark-themed Windows desktop interface

## Supported Microcontrollers

- STM32F407
- STM32F746

The firmware uses STM32 HAL/CMSIS and a custom communication protocol.

## Communication Protocol

The bootloader uses a binary packet protocol with the following structure:

`Header (AA 55) | Length | Command | Sequence | Payload | CRC32`

Supported commands include:

| Command | Description |
|---|---|
| GET_VERSION | Read bootloader version |
| GET_HELP | List supported commands |
| GET_CHIP_ID | Read MCU identification |
| GET_STATUS | Read bootloader status |
| START_UPDATE | Initialize firmware update |
| WRITE_CHUNK | Program firmware data |
| FINISH_UPDATE | Finalize firmware update |
| ABORT_UPDATE | Cancel firmware update |
| JUMP_TO_APP | Start user application |
| GET_RDP_STATUS | Read protection status |
| FLASH_ERASE | Erase application flash |
| MEM_READ | Read memory |
| GET_FLASH_CRC | Calculate flash CRC32 |
| RESET | Reset microcontroller |

## Flash Memory Layout

The current firmware configuration uses:

| Region | Start Address |
|---|---|
| Bootloader | `0x08000000` |
| User Application | `0x08020000` |

The application occupies the remaining configured flash region.

Flash memory settings must match the target microcontroller and linker configuration.

## Installation

### Windows Executable

Download `FirmwareUpdater.exe` from the GitHub Releases section when a Windows build is available.

### Python Version

Requirements:

- Python 3.11 or newer (tested development range: 3.11–3.13)
- PySerial
- Windows operating system

Install the dependencies:

`py -3 -m pip install -r requirements.txt`

Run the application:

`py -3 firmware_updater.py`

Alternatively, launch `run_updater.bat`.

To generate a Windows executable, run `build_windows.bat`. The executable will be created in the `dist` directory.

## How to Use

1. Connect the STM32 board to your computer.
2. Enter the bootloader mode.
3. Open STM32 Firmware Updater.
4. Select USB CDC or UART and the appropriate COM port.
5. Establish the connection.
6. Select a compatible `.bin` firmware file.
7. Start the firmware update.
8. Monitor programming progress and CRC32 verification.
9. Start the user application after a successful update.

## Project Structure

- `firmware_updater.py` — Main desktop application
- `run_updater.bat` — Python application launcher
- `build_windows.bat` — Windows executable build script
- `requirements.txt` — Python dependencies
- `firmware/` — STM32 bootloader firmware source code
- `tests/` — Automated protocol tests

## Development Environment

- Visual Studio Code
- STM32Cube tools
- STM32 HAL / CMSIS
- CMake / Ninja
- ARM GNU Toolchain
- Python / Tkinter / PySerial

## Testing

Protocol parsing, packet generation, CRC32 calculations, and simulated firmware upload operations are covered by software tests.

Physical hardware compatibility and Windows executable functionality should be verified on the target system before deployment.

## Important Notes

The target STM32 board must already contain a compatible bootloader.

Firmware binaries must use the correct application start address and memory configuration. Flash programming modifies the application region and may invalidate an existing application if the update is interrupted.

## Version

**v1.0.0 — Initial Release**

Initial standalone release featuring STM32 firmware management, serial communication, flash operations, and firmware verification.
