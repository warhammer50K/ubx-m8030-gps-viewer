# ubx-m8030-gps-viewer

![Ubuntu 22.04](https://img.shields.io/badge/Tested_on-Ubuntu_22.04-E95420?logo=ubuntu&logoColor=white)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-3.16%2B-064F8C?logo=cmake&logoColor=white)

A lightweight terminal-based GPS viewer that parses NMEA data from the u-blox UBX-M8030 module in real time.

## GPS Module

- **Chip**: u-blox UBX-M8030-KT
- **Protocol**: NMEA 0183 (GGA, RMC)
- **Interface**: USB-UART (ttyUSB / ttyACM)
- **Purchase**: [AliExpress](https://ko.aliexpress.com/item/1005006862529537.html)

## Features

- Auto-detect serial port (`/dev/ttyUSB*`, `/dev/ttyACM*`)
- Auto-detect baud rate (9600, 38400, 115200, 4800)
- NMEA checksum verification
- Real-time display of position, altitude, satellites, HDOP, speed, and heading

## Tested Environment

| Item | Detail |
|------|--------|
| OS | Ubuntu 22.04.5 LTS (Jammy Jellyfish) |
| Kernel | 6.8.0-90-generic |
| Compiler | g++ (C++17) |
| Build | CMake 3.16+ |
| Hardware | ASUS ROG Zephyrus G14 GA403UU |

## Build

```bash
mkdir -p build && cd build
cmake ..
make
```

## Usage

```bash
# Auto-detect port and baud rate
sudo ./gps_test

# Specify port
sudo ./gps_test /dev/ttyUSB0

# Specify port and baud rate
sudo ./gps_test /dev/ttyUSB0 9600
```

## Output Example

```
[*] port: /dev/ttyUSB0
[*] baud: 9600
[*] listening... (Ctrl+C to quit)

[#42] fix=1 sats=8  lat=37.12345678  lon=127.12345678  alt=45.2m  hdop=1.2  spd=0.3m/s  hdg=180.5
```

## License

MIT
