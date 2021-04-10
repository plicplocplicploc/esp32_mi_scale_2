// The program should work fine without modifying these

// Event durations (milliseconds)
#define MAX_BLE_SCAN_DURATION 10000
#define MAX_WIFI_ATTEMPT_DURATION 8000
#define TIME_BETWEEN_POLLS 5000 // more will be spent due to BLE re-scan
#define MQTT_POLL_TIME 1000
#define DEFAULT_DELAY 100

// Number of attempts
#define POLL_ATTEMPTS 3
#define MAX_WIFI_CONNECT_ATTEMPTS 4

// Blinking durations (milliseconds)
#define BLINK_FOR 8000
#define SUCCESS_BLINK_ON 900
#define SUCCESS_BLINK_OFF 100
#define FAILURE_BLINK_ON 100
#define FAILURE_BLINK_OFF 100

// Message to be sent to MQTT_SETTINGS_TOPIC to trigger scale reconfig
#define CONFIG_TRIGGER_STR "1"

// BLE UUIDs
BLEUUID BODY_COMPOSITION_SERVICE("0000181b-0000-1000-8000-00805f9b34fb");
BLEUUID BODY_COMPOSITION_HISTORY_CHARACTERISTIC("00002a2f-0000-3512-2118-0009af100700");
BLEUUID CURRENT_TIME_CHARACTERISTIC("00002a2b-0000-1000-8000-00805f9b34fb");

BLEUUID HUAMI_CONFIGURATION_SERVICE("00001530-0000-3512-2118-0009af100700");
BLEUUID SCALE_CONFIGURATION_CHARACTERISTIC("00001542-0000-3512-2118-0009af100700");
