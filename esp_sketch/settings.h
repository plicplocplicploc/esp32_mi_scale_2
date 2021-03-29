// The program should work fine without modifying these

// User ID value, can be any integer up to 65535 with virtually no consequence
// unless plenty of clients are polling the scale at the same time. Just sort
// of random here, leaving as is is fine
#define USER_ID 32109

// Event durations (milliseconds)
#define MAX_BLE_SCAN_DURATION 10000
#define MAX_WIFI_ATTEMPT_DURATION 10000
#define TIME_BETWEEN_CONFIG_AND_POLL 10000
#define TIME_BETWEEN_POLLS 4000 // more will be spent due to BLE re-scan

// Number of attempts
#define POLL_ATTEMPTS 3

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
