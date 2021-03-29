void reconnectScale()
{
  pClient->disconnect();
  while (pClient->isConnected())
    delay(100);
  delete (pDiscoveredDevice);
  scanBle();
  connectScale();
}

bool connectScale()
{
  pClient->connect(pDiscoveredDevice);

  // Characteristics under BODY_COMPOSITION_SERVICE
  pBodyCompositionService = pClient->getService(BODY_COMPOSITION_SERVICE);
  if (pBodyCompositionService == nullptr)
  {
    Serial.print("BODY_COMPOSITION_SERVICE failure, stopping everything");
    pClient->disconnect();
    blinkThenSleep(FAILURE);
  }

  pBodyCompositionHistoryCharacteristic = pBodyCompositionService->getCharacteristic(BODY_COMPOSITION_HISTORY_CHARACTERISTIC);
  if (pBodyCompositionHistoryCharacteristic == nullptr)
  {
    Serial.print("BODY_COMPOSITION_HISTORY_CHARACTERISTIC failure, stopping everything");
    pClient->disconnect();
    blinkThenSleep(FAILURE);
  }

  pCurrentTimeCharacteristic = pBodyCompositionService->getCharacteristic(CURRENT_TIME_CHARACTERISTIC);
  if (pCurrentTimeCharacteristic == nullptr)
  {
    Serial.print("CURRENT_TIME_CHARACTERISTIC failure, stopping everything");
    pClient->disconnect();
    blinkThenSleep(FAILURE);
  }

  // Characteristics under HUAMI_CONFIGURATION_SERVICE
  pHuamiConfigurationService = pClient->getService(HUAMI_CONFIGURATION_SERVICE);
  if (pHuamiConfigurationService == nullptr)
  {
    Serial.print("HUAMI_CONFIGURATION_SERVICE failure, stopping everything");
    pClient->disconnect();
    blinkThenSleep(FAILURE);
  }

  pScaleConfigurationCharacteristic = pHuamiConfigurationService->getCharacteristic(SCALE_CONFIGURATION_CHARACTERISTIC);
  if (pScaleConfigurationCharacteristic == nullptr)
  {
    Serial.print("SCALE_CONFIGURATION_CHARACTERISTIC failure, stopping everything");
    pClient->disconnect();
    blinkThenSleep(FAILURE);
  }
}

bool scanBle()
{
  BLEDevice::init("");
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
