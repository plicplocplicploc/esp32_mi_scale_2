# Xiaomi Mi Scale V2 ESP32 Bridge

## Credits
* Initially forked from [this project](https://github.com/rando-calrissian/esp32_xiaomi_mi_2_hass).
* Some of the code was translated from [openScale](https://github.com/oliexdev/openScale).
* This [summary](https://github.com/wiecosystem/Bluetooth/blob/master/doc/devices/huami.health.scale2.md#advertisement) and [that other one](https://github.com/wiecosystem/Bluetooth/blob/master/doc/devices/huami.health.scale2.md#advertisement) were useful too.

## What's new, in a nutshell
* Code re-write from forked repo, better organised.
* Support for scale config.

## Known problems and to-do
* Monitor: MQTT messages somehow didn't always go through. Now using a check mechanism to make sure the MQTT payload was actually sent. Can probably be improved.

## How to
* Have a look at `usersettings.h`; some things can also be tweaked in `settings.h` but that shouldn't be necessary.
* Power on the ESP32 and weigh yourself should just work. Just make sure you end up with a stabilised weigh-in including impedance (blinking weight + blinking impedance signal on weighing scale).
* If the scale needs to be reconfigured (units and time), send a `CONFIG_TRIGGER_STR` message to the `MQTT_SETTINGS_TOPIC` MQTT topic. Make sure it is sent as a retained message for the ESP to catch it. Step on the scale, power on the ESP, and the scale will be reconfigured. No reading will be acquired this time. Make sure the scale returns to zero before new weigh-ins.
```
mosquitto_pub -h <host> -p 8884 -t 'scaleSettings' -u <mqtt user> -P '<mqtt password>' -m '1' -r
```
* Spits out scale results in a log file + possibility to pass data to a `post_weighin` script. There is `historical` code that was updating data into Garmin Connect, some of the code will be reusable but it isn't maintained.

## Improvements over the forked repo
* Integrate 2 issues reported by other users: [this](https://github.com/rando-calrissian/esp32_xiaomi_mi_2_hass/issues/3) and [that](https://github.com/rando-calrissian/esp32_xiaomi_mi_2_hass/pull/2/commits/02b5ce7a416f39f3d03ec222934be112e28b3e7d).
* Slight change in the way detected devices are computed. The newer BLE lib breaks compatibility with the way the original sketch worked, see [this](https://github.com/espressif/arduino-esp32/issues/4627#issuecomment-751400018).
* Uses new lib as suggested by [that comment](https://github.com/rando-calrissian/esp32_xiaomi_mi_2_hass/issues/1).
* Better organisation and split user config file, general settings, and code.
* A number of various improvements.
