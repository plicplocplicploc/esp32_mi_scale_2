int16_t stoi(String input, uint16_t index1)
{
  return (int16_t)(strtol(input.substring(index1, index1 + 2).c_str(), NULL, 16));
}

int16_t stoi2(String input, uint16_t index1)
{
  return (int16_t)(strtol((input.substring(index1 + 2, index1 + 4) + input.substring(index1, index1 + 2)).c_str(), NULL, 16));
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

String processScaleData(String rawDataFromScale)
{
  float weight = stoi2(rawDataFromScale, 22) * 0.01f; // there's a trick
  float impedance = stoi2(rawDataFromScale, 18) * 0.01f;
  int user = stoi(rawDataFromScale, 6); // pointless on this scale?
  int units = stoi(rawDataFromScale, 0);
  String strUnits;
  if (units == 1)
    strUnits = "jin";
  else if (units == 3)
    strUnits = "lbs";
  else // if (units == 2) // it appears that the scale defaults to kg?
  {
    strUnits = "kg";
    weight /= 2;
  }

  // Prepare return string
  String time = String(String(stoi2(rawDataFromScale, 4)) + " " + String(stoi(rawDataFromScale, 8)) + " " + String(stoi(rawDataFromScale, 10)) + " " + String(stoi(rawDataFromScale, 12)) + " " + String(stoi(rawDataFromScale, 14)) + " " + String(stoi(rawDataFromScale, 16)));
  String parsedData =
      String("{\"Weight\":\"") +
      String(weight) +
      String("\", \"Impedance\":\"") +
      String(impedance) +
      String("\", \"Units\":\"") +
      String(strUnits) +
      String("\", \"User\":\"") +
      String(user) +
      String("\", \"Timestamp\":\"") +
      time +
      String("\"}");

  return parsedData;
}
