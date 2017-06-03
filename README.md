# usb-oscilloscope
atmega328p usb oscilloscope
```
                 ------------
                | ATmega328p |
                |            |
                |RESET   ADC5|-----> Measured voltage 0..3.3V
                |            |
                |            |
  D+   <--------|INT0        |
  3.3V <--------|VCC         |
  0V   <--------|GND         |
            ----|PB6         |
           |    |            |
    1k5  |---|  |            |
         |___|  |            |
           |    |            |
  D-   <--------|PD7         |
  LED  <--------|PB0         |
                 ------------
```
