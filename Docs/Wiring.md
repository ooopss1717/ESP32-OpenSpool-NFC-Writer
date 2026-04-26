
# Wiring Diagram

## ESP32-S2 Connections

| Device | GPIO |
|--------|------|
| TFT CS | 12 |
| TFT DC | 16 |
| TFT RST | 17 |
| TFT MOSI | 11 |
| TFT SCLK | 10 |
| Backlight | 9 |
| Encoder A | 2 |
| Encoder B | 3 |
| Button OK | 7 |
| Button Back | 5 |
| PN532 SDA | 33 |
| PN532 SCL | 34 |

## Notes
- PN532 must be set to I2C mode
- Use NTAG215 or NTAG216 tags
