#define ESP8266     1
#define REPORT      600000
#define DELAYMQTT   1000
#define SETUPBLINK  200
#define NextTime2Wifi 60000


#define BUTTON      0
#define PWM0        12
#define PWM1        13

#include <dummy.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>

#include <EEPROM.h>

#include "clickButton.h"


/*
  Simple example for receiving

  https://github.com/sui77/rc-switch/
*/

//#include "RCSwitch.h"
#include "ArduinoOTA.h"



int prevValueSent = 0;
unsigned long nextTime2RF = 0;
unsigned long nextTime2Report = 0;
unsigned long nextTimeMQTTReconnect = 0;
unsigned long nextTime2Blink = 0;
unsigned long nextTime2Wifi = 0;
bool need2Blink = false;


// Replace with your network credentials
char ssid[16] = "NetByNet-FR";
char password[16] = "";
char mqttServer[16] = "iot.eclipse.org";
char mqttUser[16] = "";
char mqttPassword[16] = "";

const char cStructure[128] = "{\
  \"state\": {\
    \"powerSW1\": \"Switch\"\
  },\
  \"commands\": {\
    \"powerSW1\": \"Switch\"\
  },\
  \"lwt\": \"lwt\"\
}";

char strStatus[2][4];

String chipID = String(ESP.getChipId());
String rootTopic = "/sqowe/sOnOff-Wall/" + chipID;
String subTopic = rootTopic + "/commands/#";


int nWIFI = 0;
bool connected = false;


//WiFiClient espClient;
WiFiClientSecure espClient;

PubSubClient mqttClient(espClient);
ESP8266WebServer server(80);

ClickButton button1(BUTTON, LOW, CLICKBTN_PULLUP);



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
      if ( server.argName(0) == "ap" && server.argName(2) == "mqttServer" ) {
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

        server.arg(0).toCharArray(ssid, server.arg(0).length() + 1);
        server.arg(1).toCharArray(password, server.arg(1).length() + 1);
        server.arg(2).toCharArray(mqttServer, server.arg(2).length() + 1);
        server.arg(3).toCharArray(mqttUser, server.arg(3).length() + 1);
        server.arg(4).toCharArray(mqttPassword, server.arg(4).length() + 1);


        saveCredentials();

        break;
      }
      handleNotFound();
      break;
    case 1:
      if (server.argName(0) == "reboot" && server.arg(0) == "1" ) {
        buf += "<title>Sqowe</title></head><body>";
        buf += "<h2 style=\"text-align: center;\">Sqowe</h2>";
        buf += "<h3 style=\"text-align: center;\">Sensor ID: <span style=\"color: #ff0000;\">" + chipID + "</span></h3>";
        buf += "<h3 style=\"text-align: center;\">Reboot!!!</h3>";
        buf += "</body></html>";
        server.send ( 200, "text/html", buf );

        delay(1000);
        ESP.reset();
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
        buf += "<option>" + WiFi.SSID(i) + "</option>\n";
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




void startServer() {

  String ssidAP = "sqowe" + chipID;

  if (connected) {
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
        Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
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



void mqttReconnect() {
  // Loop until we're reconnected
  unsigned int retry = 5;
  while (!mqttClient.connected() && retry > 0) {
    Serial.print("Attempting MQTT connection to ");
    Serial.print(mqttServer);
    Serial.print(" ...");
    // Create a random client ID
    String clientId = "ESP8266Client-" + chipID;
    String LWT = rootTopic + "/lwt";
    String strIP = rootTopic + "/ip";
    String strStructure = rootTopic + "/Structure";


    // Attempt to connect
    if (mqttClient.connect(clientId.c_str(), mqttUser, mqttPassword, LWT.c_str(), 0, false, "OFF")) {
      Serial.print("connected ");
      Serial.print("rc=");
      Serial.println(mqttClient.state());
      // Once connected, publish an announcement...
      mqttClient.publish(LWT.c_str(), "ON");
      mqttClient.publish(strStructure.c_str(), cStructure);
      mqttClient.publish(strIP.c_str(), WiFi.localIP().toString().c_str());

      // ... and resubscribe
      //      client.subscribe("inTopic");
      mqttClient.subscribe(subTopic.c_str());
    } else {

        Serial.print("failed, rc=");
        Serial.print(mqttClient.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        retry--;
        delay(5000);
    }
  }

  if (retry == 0) {WiFi.disconnect(); delay(500);}

}

void mqttGetTopic(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
    strCommand = String(topic);
//  strCommand.remove(0,rootTopic.length());


  if(strCommand == rootTopic + "/commands/powerSW1")
  {
    if(payload[0] == 'O' && payload[1] == 'N' ) { digitalWrite( PWM0, HIGH); digitalWrite( PWM1, HIGH); }
    else { digitalWrite( PWM0, LOW); digitalWrite( PWM1, LOW); }
  }
}

void publish_switches()
{
  String strTopic;
  int value;
  strTopic = rootTopic + "/powerSW1";
  value = digitalRead(PWM0);
  mqttClient.publish(strTopic.c_str(), strStatus[value]);
}


void setup() {


  pinMode(5, INPUT_PULLUP);
  pinMode(PWM0,OUTPUT);
  pinMode(PWM1,OUTPUT);

  digitalWrite(PWM0,LOW);

  strcpy(strStatus[0],"OFF");
  strcpy(strStatus[1],"ON");

  int count = 0;


  Serial.begin(115200);

  loadCredentials();


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);


  if (ssid[0] != 0){


  //  chipID = String(ESP.getChipId());

  delay(100);




  // Wait for connection
  while (WiFi.status() != WL_CONNECTED &&  count < 30) {
    delay(500);
    if(digitalRead(PWM1))
    {
      digitalWrite(PWM1, LOW);
    }
    else
    {
      digitalWrite(PWM1, HIGH);
    }
    Serial.print(".");
    count ++;

  }



  if (WiFi.status() == WL_CONNECTED && digitalRead(5) == 1) {
    connected = true;
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());



    mqttClient.setServer(mqttServer, 8883);
    mqttClient.setCallback(mqttGetTopic);



  }
  else {
    connected = false;
    need2Blink = true;
    startServer();
  }
  } else
  {
    connected = false;
    need2Blink = true;
    startServer();
  }

  pinMode(BUTTON, INPUT_PULLUP);


  // Setup button timers (all in milliseconds / ms)
  // (These are default if not set, but changeable for convenience)
  button1.debounceTime   = 10;   // Debounce timer in ms
  button1.multiclickTime = 250;  // Time limit for multi clicks
  button1.longClickTime  = 1000; // time until "held-down clicks" register


     // Port defaults to 3232
    // ArduinoOTA.setPort(3232);

    // Hostname defaults to esp3232-[MAC]
    ArduinoOTA.setHostname(String("Sqowe-" + chipID).c_str());

    // No authentication by default
    ArduinoOTA.setPassword("Sqowe$");

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

    digitalWrite(PWM1, LOW);

}

void loop() {

  String strTopic;
  unsigned long now = millis();


  button1.Update();

    switch (button1.clicks) {
      case -1:
          connected = false;
          startServer();
          break;
      case 1:
          if(digitalRead(PWM0))
          {
            digitalWrite(PWM0, LOW);
            digitalWrite(PWM1, LOW);
          }
          else
          {
            digitalWrite(PWM0, HIGH);
            digitalWrite(PWM1, HIGH);
          }
          if (connected){publish_switches();}
          break;
    }


  if (connected) {

    ArduinoOTA.handle();


//    delay(2); // <- fixes some issues with WiFi stability

      if (!mqttClient.connected()) {
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



    if (now > nextTime2Report)
    {
        strTopic = rootTopic + "/lwt";
        mqttClient.publish(strTopic.c_str(), "ON");
        strTopic = rootTopic + "/Structure";
        mqttClient.publish(strTopic.c_str(), cStructure);
        publish_switches();
        nextTime2Report = now + REPORT;
    }



  } else
  {
    server.handleClient();
  }


  if (now > nextTime2Blink && need2Blink)
  {
    if(digitalRead(PWM1))
    {
      digitalWrite(PWM1, LOW);
    }
    else
    {
      digitalWrite(PWM1, HIGH);
    }
      nextTime2Blink = now + SETUPBLINK;
  }


}
