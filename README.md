# ESP32 Bluetooth Weather Station

**W A R N I N G :** Project in development

Arduino - Platformio Weather Station project with a Bluetooth GATT server on a ESP32 for publish temperature and humidity from DHT22 sensor to Android app (in development too)

## Software Dependencies

Please install first [PlatformIO](http://platformio.org/) open source ecosystem for IoT development compatible with **Arduino** IDE and its command line tools (Windows, MacOs and Linux). Also, you may need to install [git](http://git-scm.com/) in your system.

## Compiling and installing

For `TTGO` D1Mini v2 board, please first connect it via USB, then clone and upload firmware like this:

``` bash
git clone https://github.com/hpsaturn/esp32_weather_station
cd esp32_weather_station
pio run --target upload
```
## Debugging

Via serial console at `115200` baudios or please connect to GATT server via Bluetoot with [nRF Connect for Mobile](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp&hl=en) software.