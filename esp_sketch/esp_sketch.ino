#include <Arduino.h>
#include <WiFi.h>
#include <ezTime.h>
#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>

#include "creds_settings.h"

// Globals
BLEAdvertisedDevice *pDiscoveredDevice = NULL;
BLERemoteService *pBodyCompositionService = NULL;
BLERemoteService *pHuamiConfigurationService = NULL;
BLERemoteCharacteristic *pBodyCompositionHistoryCharacteristic = NULL;
BLERemoteCharacteristic *pCurrentTimeCharacteristic = NULL;
BLERemoteCharacteristic *pScaleConfigurationCharacteristic = NULL;
bool bleConnected = false;
bool sendAckNeeded = false;
Timezone localTime;
size_t size;

// Convenience
#define SUCCESS true
#define FAILURE false

void setup()
{
  Serial.begin(115200);

  digitalWrite(ONBOARD_LED, HIGH);
  pinMode(ONBOARD_LED, OUTPUT);

  // TODO: what if no WiFi?

  // Set up time
  Serial.println("Connecting to WiFi");
  wifiConnect();
  Serial.println("WiFi connected");
  waitForSync(); // ezTime setup
  localTime.setLocation(TIMEZONE);
  Serial.println("Local time: " + localTime.dateTime());
  wifiDisconnect();
  Serial.println("WiFi disconnected");

  // Look for scale and enable notifications
  Serial.println("Scanning BLE");
  if (scanBle())
    Serial.println("Scale found, now initialising services");
  else
  {
    Serial.println("Cannot find scale, going to sleep");
    blinkThenSleep(FAILURE);
  }
  connectToScale();
  configureScale();
}

void loop()
{
  askForMeasurement();
  delay(3000);
}

class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pclient)
  {
    Serial.println("MyClientCallback: BLE client connected");
  }
  void onDisconnect(BLEClient *pclient)
  {
    Serial.println("MyClientCallback: BLE client disconnected");
    bleConnected = false;
    pDiscoveredDevice = NULL;
  }
};

void configureScale()
{
  // Set scale units
  Serial.println("Setting scale units");
  uint8_t setUnitCmd[] = {0x06, 0x04, 0x00, SCALE_UNIT};
  size = 4;
  pScaleConfigurationCharacteristic->writeValue(setUnitCmd, size, true);

  // Set time
  Serial.println("Setting scale time");
  uint16_t year = localTime.year();
  uint8_t month = localTime.month();
  uint8_t day = localTime.day();
  uint8_t hour = localTime.hour();
  uint8_t minute = localTime.minute();
  uint8_t second = localTime.second();
  uint8_t yearLeft = (uint8_t)(year >> 8);
  uint8_t yearRight = (uint8_t)year;

  uint8_t dateTimeByte[] = {yearRight, yearLeft, month, day, hour, minute, second, 0x03, 0x00, 0x00};
  size = 10;
  pCurrentTimeCharacteristic->writeValue(dateTimeByte, size, true);

  // Enable notifications. Still need an extra step to actually receive them.
  Serial.println("Register for notifications");
  pBodyCompositionHistoryCharacteristic->registerForNotify(notifyCallback);

  // Configure scale to get only last measurement
  Serial.println("Asking for last measurement only");
  uint8_t userIdentifier[5] = {0x01, 0xff, 0xff, USER_ID >> 8, USER_ID};
  size = 5;
  pBodyCompositionHistoryCharacteristic->writeValue(userIdentifier, size, true);
}

void askForMeasurement()
{
  // Ask for measurements
  Serial.println("Requesting measurement");
  pBodyCompositionHistoryCharacteristic->writeValue(uint8_t{0x02}, true);
}

int16_t stoi(String input, uint16_t index1)
{
  return (int16_t)(strtol(input.substring(index1, index1 + 2).c_str(), NULL, 16));
}

int16_t stoi2(String input, uint16_t index1)
{
  return (int16_t)(strtol((input.substring(index1 + 2, index1 + 4) + input.substring(index1, index1 + 2)).c_str(), NULL, 16));
}

void notifyCallback(
    BLERemoteCharacteristic *pBLERemoteCharacteristic,
    uint8_t *pData,
    size_t length,
    bool isNotify)
{
  // Nothing to do here
  if (pData == NULL || length == 0)
  {
    Serial.println("Empty notification received, ignoring");
    return;
  }

  // Stop notification received
  if (pData[0] == 0x03)
  {
    // Ack
    Serial.println("Scheduling to send ack at next loop iteration");
    sendAckNeeded = true;
  }

  if (length == 13)
  {
    String rawData = BLEUtils::buildHexData(nullptr, pData, length);

    float weight = stoi2(rawData, 22) * 0.01f / 2; // TODO: that probably depends on scale settings? ok for kg
    float impedance = stoi2(rawData, 18) * 0.01f;
    int user = stoi(rawData, 6);
    int units = stoi(rawData, 0);
    String time = String(String(stoi2(rawData, 4)) + " " + String(stoi(rawData, 8)) + " " + String(stoi(rawData, 10)) + " " + String(stoi(rawData, 12)) + " " + String(stoi(rawData, 14)) + " " + String(stoi(rawData, 16)));
    String output = String(weight) + " " + impedance + " " + user + " " + units + " " + time;
    Serial.println(output);
    return;
  }

  Serial.println("Received something else");
}

void sendAck()
{
  Serial.println("Sending ack");
  pBodyCompositionHistoryCharacteristic->writeValue(uint8_t{0x03}, true);
  uint8_t userIdentifier[] = {0x04, 0xff, 0xff, USER_ID >> 8, USER_ID};
  size = 5;
  pBodyCompositionHistoryCharacteristic->writeValue(userIdentifier, size, true);
}

void blinkThenSleep(bool successStatus)
{
  int blinkOn = successStatus ? SUCCESS_BLINK_ON : FAILURE_BLINK_ON;
  int blinkOff = successStatus ? SUCCESS_BLINK_OFF : FAILURE_BLINK_OFF;
  uint64_t untilTime = millis() + BLINK_FOR;
  while (millis() < untilTime)
  {
    digitalWrite(ONBOARD_LED, LOW); // that means pull-up low --> LED ON
    delay(blinkOn);
    digitalWrite(ONBOARD_LED, HIGH); // that means pull-up high --> LED OFF
    delay(blinkOff);
  }
  esp_deep_sleep_start();
}

bool connectToScale()
{
  BLEClient *pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  pClient->connect(pDiscoveredDevice);

  // TODO: so much code duplication, there has to be a better way to do this

  // Characteristics under BODY_COMPOSITION_SERVICE
  pBodyCompositionService = pClient->getService(BODY_COMPOSITION_SERVICE);
  if (pBodyCompositionService == nullptr)
  {
    Serial.print("BODY_COMPOSITION_SERVICE failure");
    pClient->disconnect();
    Serial.println("Going to sleep"); // ?
    blinkThenSleep(FAILURE);          // ?
  }
  else
  {
    Serial.println("BODY_COMPOSITION_SERVICE ok");
  }

  pBodyCompositionHistoryCharacteristic = pBodyCompositionService->getCharacteristic(BODY_COMPOSITION_HISTORY_CHARACTERISTIC);
  if (pBodyCompositionHistoryCharacteristic == nullptr)
  {
    Serial.print("BODY_COMPOSITION_HISTORY_CHARACTERISTIC failure");
    pClient->disconnect();
    Serial.println("Going to sleep"); // ?
    blinkThenSleep(FAILURE);          // ?
  }
  else
  {
    Serial.println("BODY_COMPOSITION_HISTORY_CHARACTERISTIC ok");
  }

  pCurrentTimeCharacteristic = pBodyCompositionService->getCharacteristic(CURRENT_TIME_CHARACTERISTIC);
  if (pCurrentTimeCharacteristic == nullptr)
  {
    Serial.print("CURRENT_TIME_CHARACTERISTIC failure");
    pClient->disconnect();
    Serial.println("Going to sleep"); // ?
    blinkThenSleep(FAILURE);          // ?
  }
  else
  {
    Serial.println("CURRENT_TIME_CHARACTERISTIC ok");
  }

  // Characteristics under HUAMI_CONFIGURATION_SERVICE
  pHuamiConfigurationService = pClient->getService(HUAMI_CONFIGURATION_SERVICE);
  if (pHuamiConfigurationService == nullptr)
  {
    Serial.print("HUAMI_CONFIGURATION_SERVICE failure");
    pClient->disconnect();
    Serial.println("Going to sleep"); // ?
    blinkThenSleep(FAILURE);          // ?
  }
  else
  {
    Serial.println("HUAMI_CONFIGURATION_SERVICE ok");
  }

  pScaleConfigurationCharacteristic = pHuamiConfigurationService->getCharacteristic(SCALE_CONFIGURATION_CHARACTERISTIC);
  if (pScaleConfigurationCharacteristic == nullptr)
  {
    Serial.print("SCALE_CONFIGURATION_CHARACTERISTIC failure");
    pClient->disconnect();
    Serial.println("Going to sleep"); // ?
    blinkThenSleep(FAILURE);          // ?
  }
  else
  {
    Serial.println("SCALE_CONFIGURATION_CHARACTERISTIC ok");
  }

  bleConnected = true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Serial.print(".");
    if (DISCOVERY_METHOD == "mac")
    {
      if (advertisedDevice.getAddress().toString() != SCALE_MAC_ADDRESS)
        return;
    }
    else
    {
      if (!advertisedDevice.haveServiceUUID() && !advertisedDevice.isAdvertisingService(BODY_COMPOSITION_SERVICE))
        return;
    }

    // Reach this point only if the advertisedDevice corresponds to the MAC or has the right service UUID
    pDiscoveredDevice = new BLEAdvertisedDevice(advertisedDevice);
    BLEScan *pBLEScan = BLEDevice::getScan();
    pBLEScan->stop();
  }
};

bool scanBle()
{
  BLEDevice::init("");
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(0x50);
  pBLEScan->setWindow(0x30);
  pBLEScan->start(MAX_BLE_SCAN_DURATION / 1000);

  Serial.println("");

  if (pDiscoveredDevice)
    return true;
  else
    return false;
}

bool wifiDisconnect()
{
  WiFi.disconnect();
  return false;
}

bool wifiConnect()
{
  float startTime = millis();
  float untilTime = startTime + MAX_WIFI_ATTEMPT_DURATION;
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // DHCP is just fine
  while (millis() < untilTime)
  {
    if (WiFi.status() == WL_CONNECTED)
      return true;
    else
      delay(100);
  }

  if (!WiFi.status() == WL_CONNECTED)
  {
    return false;
  }
}
