# ubx-m8030-gps-viewer

u-blox UBX-M8030 GPS 모듈의 NMEA 데이터를 실시간으로 파싱하고 터미널에 표시하는 뷰어입니다.

## GPS Module

- **Chip**: u-blox UBX-M8030-KT
- **Protocol**: NMEA 0183 (GGA, RMC)
- **Interface**: USB-UART (ttyUSB / ttyACM)
- **구매처**: [AliExpress](https://ko.aliexpress.com/item/1005006862529537.html)

## Features

- 시리얼 포트 자동 감지 (`/dev/ttyUSB*`, `/dev/ttyACM*`)
- Baud rate 자동 감지 (9600, 38400, 115200, 4800)
- NMEA checksum 검증
- 실시간 위치, 고도, 위성 수, HDOP, 속도, 방위각 출력

## Environment

| 항목 | 내용 |
|------|------|
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
# 자동 감지 (포트 + baud rate)
sudo ./gps_test

# 포트 지정
sudo ./gps_test /dev/ttyUSB0

# 포트 + baud rate 지정
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
