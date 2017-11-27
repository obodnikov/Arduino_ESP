void loadCredentials() {
  int addr = 0;
  EEPROM.begin(512);
  EEPROM.get(addr, ssid);
  addr += sizeof(ssid);
  EEPROM.get(addr, password);
  addr += sizeof(password);
  EEPROM.get(addr, mqttServer);
  addr += sizeof(mqttServer);
  EEPROM.get(addr, mqttUser);
  addr += sizeof(mqttUser);
  EEPROM.get(addr, mqttPassword);
  addr += sizeof(mqttPassword);
  char ok[2 + 1];
  EEPROM.get(addr, ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    ssid[0] = 0;
    password[0] = 0;
  }
  Serial.println("Recovered credentials:");
  Serial.println(ssid);
  Serial.println(strlen(password) > 0 ? "********" : "<no password>");
}

/** Store WLAN credentials to EEPROM */
void saveCredentials() {
  int addr = 0;

  EEPROM.begin(512);
  EEPROM.put(addr, ssid);
  addr += sizeof(ssid);
  EEPROM.put(addr, password);
  addr += sizeof(password);
  EEPROM.put(addr, mqttServer);
  addr += sizeof(mqttServer);
  EEPROM.put(addr, mqttUser);
  addr += sizeof(mqttUser);
  EEPROM.put(addr, mqttPassword);
  addr += sizeof(mqttPassword);
  char ok[2 + 1] = "OK";
  EEPROM.put(addr, ok);
  EEPROM.commit();
  EEPROM.end();
}
