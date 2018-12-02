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

Via serial console at `115200` baudios or please connect to GATT server via Bluetoot with [nRF Connect for Mobile](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp&hl=en) software for now.

## Output
``` bash
==>[DHT22 ESP32]<==
-->[BLE] GATT server ready. (Waiting a client to notify)
-->[DHT] initiated
-->[DHT] temp task loop started
-->[DHT] T:25.50 H:44.20 I:25.26 D:12.44 Comfort_OK
-->[BLE] stop advertising
-->[ESP] enter deep sleep..
...

-->[BLE] onConnect
-->[DHT] initiated
-->[DHT] temp task loop started
-->[DHT] T:25.50 H:43.90 I:25.25 D:12.34 Comfort_OK
-->[DHT] T:25.40 H:43.80 I:25.14 D:12.21 Comfort_OK

```