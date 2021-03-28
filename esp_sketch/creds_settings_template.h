// Scale discovery method: "mac" for MAC address, anything else for first
// found scale with relevant given BLE UUIDs
#define DISCOVERY_METHOD "mac"

// User ID value, can be any integer up to 65535 with virtually no consequence
// unless plenty of clients are polling the scale at the same time. Just sort
// of random here, leaving as is is fine
#define USER_ID 32109

// Scale unit: 0 for metric, 1 for imperial, 2 for catty
#define SCALE_UNIT 0

// Scale Mac Address. Letters should be in lower case.
#define SCALE_MAC_ADDRESS "aa:bb:cc:dd:ee:ff"

// For ESP32 development board LED feedback. On mine the LED is on GPIO 22
#define ONBOARD_LED 22

// Timezone (https://en.wikipedia.org/wiki/List_of_tz_database_time_zones)
#define TIMEZONE "Europe/Paris"

// network details
const char *WIFI_SSID = "ssid";
const char *WIFI_PASSWORD = "password";

// MQTT Details
const char *MQTT_SERVER = "192.168.15.82";
const int MQTT_PORT = 8884;
String MQTT_TOPIC = "scale";
const char *MQTT_USERNAME = "username";
const char *MQTT_PASSWORD = "password";
const char *MQTT_CLIENTID = "scaleEsp";

// Event durations (milliseconds)
#define MAX_BLE_SCAN_DURATION 10000
#define MAX_WIFI_ATTEMPT_DURATION 10000
#define DELAY_BEFORE_REREAD_STALE 5000
#define DELAY_BEFORE_REREAD_IMPEDANCE 5000

// Other constants
#define MAX_WEIGHT_READINGS_STALE 5
#define MAX_WEIGHT_READINGS_IMPEDANCE 5

// Blinking durations (milliseconds)
#define BLINK_FOR 8000
#define SUCCESS_BLINK_ON 800
#define SUCCESS_BLINK_OFF 200
#define FAILURE_BLINK_ON 100
#define FAILURE_BLINK_OFF 100

// BLE UUIDs
BLEUUID BODY_COMPOSITION_SERVICE("0000181b-0000-1000-8000-00805f9b34fb");
BLEUUID BODY_COMPOSITION_HISTORY_CHARACTERISTIC("00002a2f-0000-3512-2118-0009af100700");
BLEUUID CURRENT_TIME_CHARACTERISTIC("00002a2b-0000-1000-8000-00805f9b34fb");

BLEUUID HUAMI_CONFIGURATION_SERVICE("00001530-0000-3512-2118-0009af100700");
BLEUUID SCALE_CONFIGURATION_CHARACTERISTIC("00001542-0000-3512-2118-0009af100700");
