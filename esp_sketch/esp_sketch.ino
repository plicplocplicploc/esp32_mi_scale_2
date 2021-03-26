/*********
 * ESP32 Xiaomi Body Scale to MQTT/AppDaemon bridge
 * The ESP32 scans BLE for the scale and posts it
 * to an MQTT topic when found.
 * AppDaemon processes the data and creates sensors
 * in Home Assistant
 * Uses the formulas and processing from
 * https://github.com/lolouk44/xiaomi_mi_scale
 * to calculate various metrics from the scale data
 *
 * https://github.com/wiecosystem/Bluetooth/blob/master/doc/devices/huami.health.scale2.md
*********/

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "creds_settings.h"

String mqtt_clientId = String(clientId + base_topic);                                 //esp32_scale
String mqtt_topic_subscribe = String(mqtt_command + base_topic);                      //cmnd/scale
String mqtt_topic_telemetry = String(mqtt_telemetry + base_topic + mqtt_tele_status); //tele/scale/status
String mqtt_topic_attributes = String(mqtt_stat + base_topic + mqtt_attributes);      //stat/scale/attributes

uint64_t unNextTime = 0;

String publish_data;

// The object where we'll store our found weighing scale (used in the MyAdvertisedDeviceCallbacks BLE scan callback)
BLEAdvertisedDevice discoveredDevice;

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

uint8_t unNoImpedanceCount = 0;

int16_t stoi(String input, uint16_t index1)
{
  return (int16_t)(strtol(input.substring(index1, index1 + 2).c_str(), NULL, 16));
}

int16_t stoi2(String input, uint16_t index1)
{
  // Serial.print("Substring : ");
  // Serial.println((input.substring(index1 + 2, index1 + 4) + input.substring(index1, index1 + 2)).c_str());
  return (int16_t)(strtol((input.substring(index1 + 2, index1 + 4) + input.substring(index1, index1 + 2)).c_str(), NULL, 16));
}

// WiFi and BLE work best when mutually exclusive.
// WiFi/MQTT needs to (re)connect each time we get new BLE scan data.
void connect()
{
  int nFailCount = 0;
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.config(ip, gateway, subnet);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
      delay(waitBeforeRetryWiFi);
      Serial.print(".");
      nFailCount++;
      if (nFailCount > maxWiFiAttempts)
        // Cannot connect to WiFi, maybe rebooting will help
        esp_restart();
    }
    Serial.println("WiFi connected");
  }
  // Here we are connected to WiFi (if not, we're rebooting)

  while (!mqtt_client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    mqtt_client.setServer(mqtt_server, mqtt_port);
    // Attempt to connect
    if (mqtt_client.connect(mqtt_clientId.c_str(), mqtt_userName, mqtt_userPass))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println("; try again after delay");
      delay(waitBeforeRetryMQTT);
      nFailCount++;
      if (nFailCount > maxMQTTAttempts)
        esp_restart(); // Cannot connect to MQTT, maybe rebooting will help
    }
  }
}

bool publish()
{
  if (!mqtt_client.connected())
  {
    Serial.println("Connecting to WiFi and MQTT...");
    connect();
  }

  Serial.print("Publishing... ");
  Serial.println(publish_data.c_str());
  Serial.print("to : ");
  Serial.println(mqtt_topic_attributes.c_str());

  // Change `false` to `true` to retain message on MQTT queue
  bool publishSuccess = mqtt_client.publish(mqtt_topic_attributes.c_str(), publish_data.c_str(), false);
  return publishSuccess;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Serial.print(".");
    if (advertisedDevice.getAddress().toString() == scale_mac_addr)
    {
      discoveredDevice = advertisedDevice;
      BLEScan *pBLEScan = BLEDevice::getScan(); // found what we want, stop now
      pBLEScan->stop();
    }
  }
};

void ScanBLE()
{
  Serial.println("Starting BLE Scan");
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Disconnecting from MQTT and WiFi first");
    mqtt_client.disconnect();
    WiFi.disconnect();
  }

  BLEDevice::init("");
  BLEScan *pBLEScan = BLEDevice::getScan(); // create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
  pBLEScan->setInterval(0x50);
  pBLEScan->setWindow(0x30);
  pBLEScan->start(maxScanDuration / 1000); // scan will be shorter if device is found (see callback)

  if (discoveredDevice.haveServiceData())
  {
    Serial.println(" scale found, will now handle data");
    Serial.println("");
    handleScaleData();
  }
  else
  {
    Serial.println(" didn't find our scale during this scan");
    Serial.println("");
    unNextTime = millis() + waitBeforeRetryBLE;
  }
}

void handleScaleData()
{
  Serial.printf("Service Data UUID : %s\n", discoveredDevice.getServiceDataUUID().toString().c_str());

  // We're only interested in the latest entry, not all entries.
  int lastEntry = discoveredDevice.getServiceDataCount() - 1;
  std::string md = discoveredDevice.getServiceData(lastEntry);
  uint8_t *mdp = (uint8_t *)discoveredDevice.getServiceData(lastEntry).data();

  char *pHex = BLEUtils::buildHexData(nullptr, mdp, md.length());
  String hex = pHex;
  // Serial.println(hex);
  // Serial.println(pHex);
  // Serial.println(stoi2(hex, 22));
  free(pHex);

  float weight = stoi2(hex, 22) * 0.01f / 2; // https://github.com/rando-calrissian/esp32_xiaomi_mi_2_hass/issues/3
  float impedance = stoi2(hex, 18) * 0.01f;

  // https://github.com/rando-calrissian/esp32_xiaomi_mi_2_hass/pull/2/commits/02b5ce7a416f39f3d03ec222934be112e28b3e7d
  if (unNoImpedanceCount < maxNoImpedanceCount && impedance <= 0)
  {
    // Got a reading, but it's missing the impedance value
    // We may have gotten the scan in between the weight measurement
    // and the impedance reading or there may just not be a reading...
    // We'll try a few more times to get a good read before we publish
    // what we've got.
    unNextTime = millis() + waitForImpedance;
    unNoImpedanceCount++;
    Serial.println("Reading incomplete, reattempting");
    return;
  }

  unNoImpedanceCount = 0;

  int user = stoi(hex, 6);
  int units = stoi(hex, 0);
  String strUnits;
  if (units == 1)
    strUnits = "jin";
  else if (units == 3)
    strUnits = "lbs";
  // it appears that kg is the default
  else // if (units == 2)
    strUnits = "kg";

  String time = String(String(stoi2(hex, 4)) + " " + String(stoi(hex, 8)) + " " + String(stoi(hex, 10)) + " " + String(stoi(hex, 12)) + " " + String(stoi(hex, 14)) + " " + String(stoi(hex, 16)));

  if (weight > 0)
  {
    publish_data = String("{\"Weight\":\"");
    publish_data += String(weight);
    publish_data += String("\", \"Impedance\":\"");
    publish_data += String(impedance);
    publish_data += String("\", \"Units\":\"");
    publish_data += String(strUnits);
    publish_data += String("\", \"User\":\"");
    publish_data += String(user);
    publish_data += String("\", \"Timestamp\":\"");
    publish_data += time;
    publish_data += String("\"}");

    bool publishStatus = publish();
    int blinkOn = publishStatus ? successBlinkOn : failureBlinkOn;
    int blinkOff = publishStatus ? successBlinkOff : failureBlinkOff;

    Serial.print("Weight published: ");
    if (publishStatus)
      Serial.println("success");
    else
      Serial.println("failure");
    Serial.print("Blinking status then going to sleep");

    // Blink a bit, then go to bed
    unNextTime = millis() + statusBlink;
    while (millis() < unNextTime)
    {
      digitalWrite(ONBOARD_LED, LOW); // that means pull-up low --> LED ON
      delay(blinkOn);
      digitalWrite(ONBOARD_LED, HIGH); // that means pull-up high --> LED OFF
      delay(blinkOff);
    }
    esp_deep_sleep_start();
  }
  Serial.println("Finished BLE Scan");
}

void setup()
{
  // Initializing serial port for debugging purposes
  Serial.begin(115200);

  // Set up onboard LED
  digitalWrite(ONBOARD_LED, HIGH);
  pinMode(ONBOARD_LED, OUTPUT);

  Serial.println("Setup Finished.");
  ScanBLE();
}

void loop()
{
  uint64_t time = millis();

  if (time > unNextTime)
  {
    ScanBLE();
    Serial.println("Waiting for next Scan");
  }

  delay(mainLoopDelay);
}
