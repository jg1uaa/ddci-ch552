# DitDahChat/VBand Interface, CH552 implementation

## Description

Telegraph key interface for PC, using WinChipHead's CH552.

## Prerequisities

- CH552 processor board ([WeActStudio CH552CoreBoard](https://github.com/WeActStudio/WeActStudio.CH552CoreBoard) recommended)
- Arduino IDE with [CH55xduino](https://github.com/DeqingSun/ch55xduino)
- Your favorite telegraph key / paddle
- Following circuit:

```
CH552 Board
------+                      100
   P33|------+--------------\/\/\/------> Dit
      |      |               100
   P34|-------------+-------\/\/\/------> Dah
      |      |      |
   P35|--------------------+
      |      |      |      |
      |      |      |      o JP
      |  10n =  10n =         open:  []
      |      |      |      o  close: L/R ctrl
      |      |      |      |
   GND|------+------+------+------------> GND
------+
```

## Usage

1. Configure your Arduino IDE with CH55xduino to support CH552.
2. Flash ddci-ch552 sketch to your CH552 board (modify pin definition if needed).
3. Connect the board to a USB port and hook up your key/paddle. It will work as a standard USB HID keyboard (`[]` or `Left/Right Ctrl` depending on the jumper).
4. Enjoy communication via virtual CW service such as [DitDahChat](https://ditdah.jp/ditdah-chat/) and [VBand](https://hamradio.solutions/vband/).

## License

- ddci-ch552.ino: GPL v3.0 or later
- codes from CH55xduino: LGPL v2.1
