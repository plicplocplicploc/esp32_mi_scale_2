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
}

void askForMeasurement()
{
  // Ask for measurements
  Serial.println("Requesting measurement");
  pBodyCompositionHistoryCharacteristic->writeValue(uint8_t{0x02}, true);
}

String readScaleData()
{
  // We're only interested in the latest entry, not all entries.
  int lastEntry = pDiscoveredDevice->getServiceDataCount() - 1;
  std::string md = pDiscoveredDevice->getServiceData(lastEntry);
  uint8_t *mdp = (uint8_t *)pDiscoveredDevice->getServiceData(lastEntry).data();

  return BLEUtils::buildHexData(nullptr, mdp, md.length());
}
