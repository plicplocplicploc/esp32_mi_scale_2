// Scale Mac Address
// If you don't know it, you can scan with serial debugging
// enabled and uncomment the lines to print out everything you find
// You should use the scale while this is running
// Letters should be in lower case
#define scale_mac_addr "aa:bb:cc:dd:ee:ff"

// For ESP32 development board LED feedback. On mine the LED is on GPIO 22
#define ONBOARD_LED 22

// network details
const char *ssid = "your SSID";
const char *password = "your password";

// This ESP32's IP
// Use a static IP so we shave off a bunch of time
// connecting to wifi
IPAddress ip(192, 168, 1, 200);
IPAddress gateway(192, 168, 1, 254);
IPAddress subnet(255, 255, 255, 0);

// MQTT Details
const char *mqtt_server = "192.168.1.56";
const int mqtt_port = 8884;
const char *mqtt_userName = "MQTT username";
const char *mqtt_userPass = "MQTT password";
const char *clientId = "esp32_";

// If you change these, you should also update the corresponding topics in the appdaemon app
String base_topic = "scale";
const char *mqtt_command = "cmnd/";
const char *mqtt_stat = "stat/";
const char *mqtt_attributes = "/attributes";
const char *mqtt_telemetry = "tele/";
const char *mqtt_tele_status = "/status";

// Event durations (all milliseconds)
#define waitBeforeRetryBLE 500
#define maxScanDuration 8000
#define waitForImpedance 1500
#define waitBeforeRetryWiFi 100
#define waitBeforeRetryMQTT 100
#define statusBlink 8000
#define successBlinkOn 800
#define successBlinkOff 200
#define failureBlinkOn 50
#define failureBlinkOff 50
#define mainLoopDelay 200

// Other constants
#define maxNoImpedanceCount 5
#define maxWiFiAttempts 20
#define maxMQTTAttempts 20
