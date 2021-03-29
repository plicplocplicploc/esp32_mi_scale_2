# Xiaomi Mi Scale V2 ESP32 Bridge

## Credits
* Initially forked from [this project](https://github.com/rando-calrissian/esp32_xiaomi_mi_2_hass).
* Some of the code was translated from [openScale](https://github.com/oliexdev/openScale).
* This [summary](https://github.com/wiecosystem/Bluetooth/blob/master/doc/devices/huami.health.scale2.md#advertisement) was useful too.

## What's new, in a nutshell
* Massive code re-write from forked repo, better organised.
* Now support for scale config.
* Listener that uploads data to Garmin Connect.

## Known problems and to-do
* Adjust config to prevent incomplete measurements.
* Right now the config (units and time) is re-written every time. Instead retrieve instructions from an MQTT queue at start-up.
* NTP sometimes times out... add a watchdog.
* Occasional crash?
* Add a user weight range in user config and ignore values outside of that range (python side).
* Units: kg and lbs are okay, setting catty shows a different value on screen (not really what it should be?) and still reports kg via BLE.
* I haven't touched the `appdaemon` side of things (related to Home Assistant). The `garmin_upload` and `appdaemon` should be merged where possible.

## How to
* Power on the ESP32 and weigh yourself.
* If you know you are changing units or date/time (e.g. switch to DST), briefly touch the scale shortly before turning on the ESP32. Weigh yourself after the initial waiting time (10s), otherwise values will be reported in mixture of old and new settings

## Improvements over the forked repo
* Integrate 2 issues reported by other users: [this](https://github.com/rando-calrissian/esp32_xiaomi_mi_2_hass/issues/3) and [that](https://github.com/rando-calrissian/esp32_xiaomi_mi_2_hass/pull/2/commits/02b5ce7a416f39f3d03ec222934be112e28b3e7d).
* Slight change in the way detected devices are computed. The newer BLE lib breaks compatibility with the way the original sketch worked, see [this](https://github.com/espressif/arduino-esp32/issues/4627#issuecomment-751400018).
* ESP32 onboard LED blinking to notify status.
* Uses new lib as suggested by [that comment](https://github.com/rando-calrissian/esp32_xiaomi_mi_2_hass/issues/1).
* Better organisation and split user config file, general settings, and code.
