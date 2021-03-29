#include <Arduino.h>
#include <WiFi.h>
#include <ezTime.h>
#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <PubSubClient.h>

#include "usersettings.h"
#include "settings.h"

// Globals
BLEClient *pClient = NULL;
BLEScan *pBLEScan = NULL;
BLEAdvertisedDevice *pDiscoveredDevice = NULL;
BLERemoteService *pBodyCompositionService = NULL;
BLERemoteService *pHuamiConfigurationService = NULL;
BLERemoteCharacteristic *pBodyCompositionHistoryCharacteristic = NULL;
BLERemoteCharacteristic *pCurrentTimeCharacteristic = NULL;
BLERemoteCharacteristic *pScaleConfigurationCharacteristic = NULL;
Timezone localTime;
size_t size;
WiFiClient espClient;
PubSubClient mqtt_client(espClient);

// Convenience
#define SUCCESS true
#define FAILURE false

// These callbacks need to be defined here
class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pclient)
  {
    Serial.println("BLE client connected");
  }
  void onDisconnect(BLEClient *pclient)
  {
    Serial.println("BLE client disconnected");
  }
};

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

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting program");

  digitalWrite(ONBOARD_LED, HIGH);
  pinMode(ONBOARD_LED, OUTPUT);

  // Get time from the internet and test MQTT at the same time
  Serial.println("Connecting to WiFi");
  if (!wifiConnect())
  {
    Serial.println("Cannot connect to WiFi, stopping everything");
    blinkThenSleep(FAILURE);
  }
  else
    Serial.println("WiFi connected");
  waitForSync(); // ezTime setup
  localTime.setLocation(TIMEZONE);
  Serial.println("Local time: " + localTime.dateTime());
  mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
  if (!mqtt_client.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD))
  {
    Serial.println("Cannot connect to MQTT, stopping everything");
    blinkThenSleep(FAILURE);
  }
  else
    Serial.println("MQTT reachable");
  mqtt_client.disconnect();
  WiFi.disconnect();
  Serial.println("WiFi disconnected");

  // Configure BLE callbacks
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());

  // Look for scale
  Serial.println("Scanning BLE");
  if (scanBle())
    Serial.println("Scale found, now initialising services");
  else
  {
    Serial.println("Cannot find scale, going to sleep");
    blinkThenSleep(FAILURE);
  }

  // Create BLE client and create service/characteristics objects
  connectScale();

  // Send configuration to scale. Units and time are re-configured every time,
  // not sure if that's the best but... why not. That won't prevent daylight
  // saving time issues though, best would be to use UTC for the scale and
  // process elsewhere but that's not how Xiaomi works... To retain
  // compatibility on scales that might concurrently be used with the Mi Fit
  // app, we stick to local time instead of UTC.
  configureScale();
}

void loop()
{
  // At this point the scale has already been reconfigured with correct time
  // and units. User should be weighing themselves, so we start by waiting.
  delay(TIME_BETWEEN_CONFIG_AND_POLL);

  String scaleReading;

  for (int i = 0; i < POLL_ATTEMPTS; i++)
  {
    // Reconnect scale. It seems silly to have to re-scan BLE but unless I do
    // that, I never get a new weigh-in value. Disconnecting/reconnecting client
    // isn't enough.
    reconnectScale();

    String currentReading = processScaleData(readScaleData());
    Serial.print("Reading: ");
    Serial.println(currentReading.c_str());
    if (currentReading == scaleReading) // no fresher data from scale
      break;
    else
    {
      scaleReading = currentReading;
      if (i == POLL_ATTEMPTS - 1)
        break;
      else
        delay(TIME_BETWEEN_POLLS);
    }
  }

  // Ready to transmit data. WiFi and MQTT should succeed, they have already
  // been tested in setup.
  Serial.println("Connecting to WiFi and MQTT");
  wifiConnect();
  mqtt_client.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);
  Serial.print("Publishing data to ");
  Serial.println(MQTT_TOPIC.c_str());
  if (mqtt_client.publish(MQTT_TOPIC.c_str(), scaleReading.c_str(), false))
  {
    Serial.println("Success");
    blinkThenSleep(SUCCESS);
  }
  else
  {
    Serial.println("Failure");
    blinkThenSleep(FAILURE);
  }
}
