
#define NextTime2Analog 10000
#define touchDHT  1350
#define NextTime2DHT    90000
#define NextTime2Wifi   60000



//#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <ESP.h>
// #include "clickButton.h"
// #include "RCSwitch.h"
#include "ESP32WebServer.h"

#include <ArduinoOTA.h>

#define RX433   27

#define NextTimeAnalog  20000
#define SUB_DELAY 1000
#define BUTTON_MILLIS 20

#define Button1 18
#define Button2 5
#define Button3 17
#define Button4 16

#define ADC1  34
#define ADC2  35
#define ADC_BASE  4096

#define Switch1 26
#define Switch2 25
#define Switch10A1  33
#define Switch10A2  32

#define DHTPIN 19     // what digital pin we're connected to



#include "DHT.h"



// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)


unsigned long nextTimeAnalog = 0;
unsigned long nextTime2DHT = 0;
unsigned long nextTouchDHT = 0;
unsigned long nextTime2Wifi = 0;


unsigned int chipid = ESP.getEfuseMac();
String chipID = String(chipid,10);
String rootTopic = "/sqowe/RFbrocker/" + chipID;
String subTopic = rootTopic + "/commands/#";

bool update = false;

WiFiClientSecure espClient;
PubSubClient mqttClient(espClient);
ESP32WebServer server(80);

// Replace with your network credentials
char ssid[16] = "dacha11-outdoor";
char password[16] = "paradox1234567";
char mqttServer[16] = "iot.eclipse.org";
char mqttUser[16] = "";
char mqttPassword[16] = "";

int nWIFI = 0;
bool connected = false;

char strStatus[2][4];
char strStatusNC[2][4];


void loadCredentials() {
  int addr=0;
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
  char ok[2+1];
  EEPROM.get(addr, ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    ssid[0] = 0;
    password[0] = 0;
  }
  Serial.println("Recovered credentials:");
  Serial.println(ssid);
  Serial.println(strlen(password)>0?"********":"<no password>");
}


void saveCredentials() {
  int addr=0;

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
  char ok[2+1] = "OK";
  EEPROM.put(addr, ok);
  EEPROM.commit();
  EEPROM.end();
}


void mqttReconnect() {
  // Loop until we're reconnected
  unsigned int retry = 20;
  while (!mqttClient.connected() && retry > 0) {
    Serial.print("Attempting MQTT connection to ");
    Serial.print(mqttServer);
    Serial.print(" ...");
    // Create a random client ID
    String clientId = "ESP32Client-" + chipID;
    String LWT = rootTopic + "/lwt";

    // Attempt to connect
    if (mqttClient.connect(clientId.c_str(), mqttUser, mqttPassword, LWT.c_str(), 0, false, "OFF")) {
      Serial.print("connected ");
      Serial.print("rc=");
      Serial.println(mqttClient.state());
      // Once connected, publish an announcement...
      mqttClient.publish(LWT.c_str(), "ON");
      // ... and resubscribe
//      client.subscribe("inTopic");
        mqttClient.subscribe(subTopic.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      retry --;
      delay(5000);
    }
  }

  if (retry == 0) {WiFi.disconnect(); delay(500);}
}



void mqttGetTopic(char* topic, byte* payload, unsigned int length) {
//  char strPayload[8];
  String strCommand;



  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
//    strPayload[i] = (char)payload[i];
  }
//  strPayload[length] = 0;
  Serial.println();
  strCommand = String(topic);
//  strCommand.remove(0,rootTopic.length());


  if(strCommand == rootTopic + "/commands/powerSW1")
  {
    if(payload[0] == 'O' && payload[1] == 'N' ) { digitalWrite( Switch1, HIGH); }
    else { digitalWrite( Switch1, LOW); }
  }
  if(strCommand == rootTopic + "/commands/powerSW2")
  {
    if(payload[0] == 'O' && payload[1] == 'N') { digitalWrite( Switch2, HIGH); }
    else { digitalWrite( Switch2, LOW); }
  }
  if(strCommand == rootTopic + "/commands/powerSW10A1")
  {
    if(payload[0] == 'O' && payload[1] == 'N') { digitalWrite( Switch10A1, LOW); }
    else { digitalWrite( Switch10A1, HIGH); }
  }
  if(strCommand == rootTopic + "/commands/powerSW10A2")
  {
    if(payload[0] == 'O' && payload[1] == 'N') { digitalWrite( Switch10A2, LOW); }
    else { digitalWrite( Switch10A2, HIGH); }
  }

  publish_switches();

}

void publish_switches()
{
  String strTopic;
  int value;
  strTopic = rootTopic + "/powerSW1";
  value = digitalRead(Switch1);
  mqttClient.publish(strTopic.c_str(), strStatus[value]);

    strTopic = rootTopic + "/powerSW2";
  value = digitalRead(Switch2);
  mqttClient.publish(strTopic.c_str(), strStatus[value]);

    strTopic = rootTopic + "/powerSW10A1";
  value = digitalRead(Switch10A1);
  mqttClient.publish(strTopic.c_str(), strStatusNC[value]);

    strTopic = rootTopic + "/powerSW10A2";
  value = digitalRead(Switch10A2);
  mqttClient.publish(strTopic.c_str(), strStatusNC[value]);
}

void startServer() {

  String ssidAP = "sqowe-" + chipID;

  if(connected){
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  } else {
  Serial.println("");
  Serial.print("NOT Connected to ");
  Serial.println(ssid);

  // Set WiFi to station mode and disconnect from an AP if it was previously connected

  WiFi.disconnect();
  delay(100);

  int n = WiFi.scanNetworks();
  nWIFI = n;

  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
//      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
      delay(10);
    }
  }
  Serial.println("");

  }




  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssidAP.c_str());


  Serial.println("");
  Serial.print("Connect to ");
  Serial.println(ssidAP.c_str());
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on ( "/", handleWeb );
  server.onNotFound ( handleNotFound );
  server.begin();



}

void handleNotFound() {

  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
     message += "" + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );

}




void handleWeb() {

  String buf = "<!DOCTYPE html>\r\n\r\n";
      buf += "<html>\r\n\r\n";

      buf += "<head>";
      buf += "<meta charset=\"utf-8\">\r\n\r\n";
      buf += "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\r\n\r\n";
      buf += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no\" />\r\n\r\n";
      buf += "<meta name=\"HandheldFriendly\" content=\"true\" />\r\n\r\n";

      buf += "<title>Sqowe</title></head><body>";

  switch (server.args()) {
  case 5:
      if ( server.argName(0) == "ap" && server.argName(2) == "mqttServer" ){
        buf += "<title>Sqowe</title></head><body>";
        buf += "<h2 style=\"text-align: center;\">Sqowe Settings</h2>";
        buf += "<h3 style=\"text-align: center;\">Sensor ID: <span style=\"color: #ff0000;\">" + chipID + "</span></h3>";
        buf += "<div><form action=\"/\" method=\"post\">";
        buf += "<div style=\"text-align: center;\">";
        buf += "<div><strong><label for=\"ap\">WiFi Network</label></strong></div>";
        buf += "<div style=\"text-align: center;\"><span style=\"color: #3366ff;\"><strong><span style=\"font-family: -apple-system;\"><span style=\"white-space: pre;\">" + server.arg(0) + "</span></span></strong></span></div>";
        buf += "<div><span style=\"color: #3366ff;\">&nbsp;</span></div>";
        buf += "<div>&nbsp;</div>";
        buf += "<div><strong><label for=\"mqttServer\">Controller Address</label> </strong></div>";
        buf += "<div style=\"text-align: center;\"><strong><span style=\"color: #3366ff;\"><span style=\"font-family: -apple-system;\">" + server.arg(2) + "</span></span></strong></div>";
        buf += "<div><strong><label for=\"mqttUser\">Controller Username</label> </strong></div>";
        buf += "<div style=\"text-align: center;\"><span style=\"color: #3366ff;\"><strong><span style=\"font-family: -apple-system;\">" + server.arg(3) + "</span></strong></span></div>";
        buf += "<input type='hidden' name='reboot' value='1'></input>";
        buf += "<div>&nbsp;</div><div>&nbsp;</div></div>";
        buf += "<div style=\"text-align: center;\"><button type=\"submit\">Apply and reboot</button></div>";
        buf += "</form></div></body></html>";
        server.send ( 200, "text/html", buf );

        server.arg(0).toCharArray(ssid, server.arg(0).length()+1);
        server.arg(1).toCharArray(password, server.arg(1).length()+1);
        server.arg(2).toCharArray(mqttServer, server.arg(2).length()+1);
        server.arg(3).toCharArray(mqttUser, server.arg(3).length()+1);
        server.arg(4).toCharArray(mqttPassword, server.arg(4).length()+1);


        saveCredentials();

        break;
      }
      handleNotFound();
      break;
  case 1:
      if (server.argName(0) == "reboot" && server.arg(0) == "1" ){
        buf += "<title>Sqowe</title></head><body>";
        buf += "<h2 style=\"text-align: center;\">Sqowe</h2>";
        buf += "<h3 style=\"text-align: center;\">Sensor ID: <span style=\"color: #ff0000;\">" + chipID + "</span></h3>";
        buf += "<h3 style=\"text-align: center;\">Reboot!!!</h3>";
        buf += "</body></html>";
        server.send ( 200, "text/html", buf );

        delay(1000);
        ESP.restart();
      }
      handleNotFound();
      break;
  case 0:
      buf += "<h2 style=\"text-align: center;\">Sqowe</h2>";
      buf += "<div><form action=\"/\" method=\"post\">";
      buf += "<div>";
      buf += "<div style=\"text-align: center;\">";
      buf += "<div><strong><label for=\"ap\">Available APs</label></strong></div>";
      buf += "<div><select name=\"ap\">";

      for ( uint8_t i = 0; i < nWIFI; i++ ) {
        buf += "<option>" + WiFi.SSID(i)+ "</option>\n";
      }

      buf += "</select></div>";
      buf += "<div><strong><label for=\"psw\">Wi-Fi Password</label> </strong></div>";
      buf += "<div><input name=\"psw\"  type=\"password\" placeholder=\"Wi-Fi Password\" /></div>";
      buf += "<div>&nbsp;</div>";
      buf += "<div>&nbsp;</div>";
      buf += "<div><strong><label for=\"mqttServer\">Controller Address</label> </strong></div>";
      buf += "<div><input name=\"mqttServer\" required=\"\" type=\"text\" placeholder=\"IP Address\" /></div>";
      buf += "<div><strong><label for=\"mqttUser\">Controller Username</label> </strong></div>";
      buf += "<div><input name=\"mqttUser\"  type=\"text\" placeholder=\"Username\" /></div>";
      buf += "<div><strong><label for=\"mqttPsw\">Controller Password</label></strong></div>";
      buf += "<div><input name=\"mqttPsw\" type=\"password\" placeholder=\"Password\" /></div>";
      buf += "<div>&nbsp;</div>";
      buf += "</div>";
      buf += "<div style=\"text-align: center;\"><button type=\"submit\">Setup</button></div>";
      buf += "</div>";
      buf += "</form></div></body>";

      server.send ( 200, "text/html", buf );
      break;
   default:
      handleNotFound();
      break;
  }
}

void setup() {
  Serial.begin(115200);


  int iRetry = 0;

      delay(10);

    strcpy(strStatus[0],"OFF");
    strcpy(strStatus[1],"ON");
    strcpy(strStatusNC[0],"ON");
    strcpy(strStatusNC[1],"OFF");


//    adcAttachPin(ADC1);
//    adcAttachPin(ADC2);


    loadCredentials();


    // We start by connecting to a WiFi network

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED && iRetry < 60) {
        delay(500);
        iRetry ++;
        Serial.print(".");
    }

      if (WiFi.status() == WL_CONNECTED){
  connected = true;
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  mqttClient.setServer(mqttServer, 8883);
  mqttClient.setCallback(mqttGetTopic);



  Serial.print("I am start\n");

  }
  else {
    connected = false;
    startServer();
  }




   // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname(String("ESP32Client-" + chipID).c_str());

  // No authentication by default
  ArduinoOTA.setPassword("zxcvbnm");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
    update = true;
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();


}

void loop() {

 String strTopic;

  if(connected) {


        delay(10); // <- fixes some issues with WiFi stability

    unsigned long now = millis();


        

          if (!mqttClient.connected() and !update) {
              if ( WiFi.status() == WL_CONNECTED ) { mqttReconnect(); }
              else
              {

                    if (now > nextTime2Wifi) {
                          Serial.println();
                          Serial.println("Lost Wifi Connection");
                          Serial.print("Trying to reConnecting to ");
                          Serial.println(ssid);
                          WiFi.disconnect();
                          delay(500);
                          WiFi.begin(ssid, password);

                          nextTime2Wifi = now + NextTime2Wifi;
                    }

              }
          }
          mqttClient.loop();




/*
    if (now > nextTouchDHT) {
      dht.readTemperature();
      nextTouchDHT = now + NextTouchDHT;
    }
*/
    if (now > nextTime2DHT) {
        strTopic = rootTopic + "/test";
        mqttClient.publish(strTopic.c_str(), "test");
        Serial.println("I'm here");
      nextTime2DHT = now + NextTime2DHT;
    }


  }
  else
  {
    server.handleClient();
  }


  ArduinoOTA.handle();

}
