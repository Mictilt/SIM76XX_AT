# ESP32 Modem Communication Project

This project sets up serial communication between an ESP32 microcontroller and a SIMCOM modem over UART, enabling AT command control of the modem to manage SIM card status, network registration, signal quality, and IP address retrieval.

## Features

- Initializes UART communication on the ESP32 for serial data exchange with the modem.
- Powers on, resets, and initializes the modem, ensuring it's ready to receive commands.
- Sends AT commands to:
  - Check SIM card status.
  - Verify network registration status.
  - Query signal quality and operator information.
  - Retrieve the device's IP address.
  - Get network time.
  - Check the modem's firmware version.
  - Connect to the network.
  - Connect to GPRS.
  - Send data over TCP/UDP.
  - ETC.

## Prerequisites

### Hardware Requirements

- **ESP32** microcontroller.
- **SIMCOM Modem** (only SIMA76XX is accepted) connected to the ESP32 via UART.
- Required **wiring** connections:
  - `TX` and `RX` pins for UART communication.
  - `GPIO` pins for modem control (e.g., `PWRKEY`, `RESET`).

### Software Requirements

- **ESP-IDF (Espressif IoT Development Framework)** installed and configured on your development environment.
- **Serial terminal** software (e.g., `minicom`, `Putty`) to monitor the ESP32's console output.

## Wiring Configuration

Connect the ESP32 GPIO pins to the modem as follows for the UART and control signals:

| ESP32 Pin       | Function          | Description                              |
|-----------------|-------------------|------------------------------------------|
| GPIO 25         | `MODEM_DTR_PIN`   | Data Terminal Ready                      |
| GPIO 26         | `MODEM_TX_PIN`    | UART TX (ESP32 to Modem)                 |
| GPIO 27         | `MODEM_RX_PIN`    | UART RX (Modem to ESP32)                 |
| GPIO 4          | `BOARD_PWRKEY_PIN`| Power Key to power on/off the modem      |
| GPIO 12         | `BOARD_POWERON_PIN` | Power ON pin to enable modem power     |
| GPIO 5          | `MODEM_RESET_PIN` | Reset pin for the modem                  |

## Project Structure
```
esp32-modem-project/
├── main/
|   ├── tests/
│   |   └── test_main.c    # Unit tests for the main
|   ├── simA76XX.c        # Modem communication 
│   ├── simA76XX.h        # Header file for modem
|   ├── utilities.c      # Utility functions
│   ├── utilities.h      # Header file for utilities
|   ├── Kconfig.projbuild # Project config (dog)
│   ├── main.c           # Main application code
│   └── CMakeLists.txt   # Build configuration local
├── CMakeLists.txt        # app build configuration
|── create_release_zip.sh # Script create a release
├── LICENSE              # Project license
├── README.md            # Project documentation
└── sdkconfig            # ESP-IDF configuration file
```

### Code Overview
- **main.c**: Initializes the UART, configures GPIO pins for modem control, and sends/receives AT commands to communicate with the modem.
- **UART Driver**: The project uses the UART driver provided by ESP-IDF for serial communication.

## Installation and Setup

### 1. Clone the Repository
```bash
git clone https://github.com/mictilt/SIM76XX_AT.git
cd esp32-modem-project
```

### 2. Configure ESP32 and Modem Parameters
- Update the GPIO pin definitions in `simA76XX.h` to match your specific hardware setup if necessary.
- Optionally, adjust the modem baud rate and buffer size for larger data transfers.

### 3. Build and Flash the Code
```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash
```
*Note: Replace `/dev/ttyUSB0` with the correct port for your device.*

or you can use the following command to build:
```bash
sh create_release_zip.sh
```

### 4. Monitor Output
To monitor the ESP32 output on a serial console, use:
```bash
idf.py -p /dev/ttyUSB0 monitor
```
*Note: Replace `/dev/ttyUSB0` with the correct port for your device.*

## License

This project is open-source and available under the MIT License. See the LICENSE file for more details.
