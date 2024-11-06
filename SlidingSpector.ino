



// ***************************************
// ********** Global Variables ***********
// ***************************************


//Globals for Wifi Setup and OTA
#include <credentials.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//WiFi Credentials
#ifndef STASSID
#define STASSID "your_ssid"
#endif
#ifndef STAPSK
#define STAPSK  "your_password"
#endif
const char* ssid = STASSID;
const char* password = STAPSK;

//MQTT
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#ifndef AIO_SERVER
#define AIO_SERVER      "your_MQTT_server_address"
#endif
#ifndef AIO_SERVERPORT
#define AIO_SERVERPORT  0000 //Your MQTT port
#endif
#ifndef AIO_USERNAME
#define AIO_USERNAME    "your_MQTT_username"
#endif
#ifndef AIO_KEY
#define AIO_KEY         "your_MQTT_key"
#endif
#define MQTT_KEEP_ALIVE 150
unsigned long previousTime;

//Initialize and Subscribe to MQTT
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Subscribe spector = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/PerilousPassage.SlidingSpector");

//Globals for Relays
#define r_spool D5  //1
#define r_setPos D6 //2
#define r_setNeg D7 //3
#define r_Lock D3   //4

//Input/Output





// ***************************************
// *************** Setup *****************
// ***************************************


void setup() {

  //Initialize Serial
  Serial.begin(115200);
  Serial.println("Booting");

  //WiFi Initialization
  wifiSetup();

  //Initialize MQTT
  mqtt.subscribe(&spector);
  MQTT_connect();

  //Initialize Relays
  delay(1000);
  pinMode(r_spool, OUTPUT);
  pinMode(r_setPos, OUTPUT);
  pinMode(r_setNeg, OUTPUT);
  pinMode(r_Lock, OUTPUT);
  digitalWrite(r_spool, HIGH); //HIGH is off
  digitalWrite(r_setPos, HIGH);
  digitalWrite(r_setNeg, HIGH);
  digitalWrite(r_Lock, HIGH);
  delay(100);
}




// ***************************************
// ************* Da Loop *****************
// ***************************************


void loop() {

  //Network Housekeeping
  ArduinoOTA.handle();
  MQTT_connect();

  //State Manager
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(10))) {
    Serial.println("Subscription Recieved");
    uint16_t value = atoi((char *)spector.lastread);

    // [Standby]
    if (value == 0) {
      Serial.println("Standby");
      digitalWrite(r_spool, HIGH); //HIGH is off
      digitalWrite(r_setPos, HIGH);
      digitalWrite(r_setNeg, HIGH);
      digitalWrite(r_Lock, HIGH);
    }

    // [Drop]
    if (value == 1) {
      //Pin raises and Dementor Drops
      Serial.println("DROP IT LIKE IT'S HOT");

      digitalWrite(r_Power1, LOW);
      digitalWrite(r_Power2, LOW);
      digitalWrite(r_Out1, LOW);
      digitalWrite(r_Out2, LOW);
      delay(120);
      digitalWrite(r_Power1, HIGH);
      digitalWrite(r_Power2, HIGH);
      digitalWrite(r_Out1, HIGH);
      digitalWrite(r_Out2, HIGH);
      value = 0;
    }

    // [Climb]
    if (value == 2) {
      //Wheel begins turning and pin latches in place
      Serial.println("Raising");

      digitalWrite(r_Power1, HIGH);
      digitalWrite(r_Power2, HIGH);
      digitalWrite(r_wheel, LOW);
      delay(5000);
      digitalWrite(r_Out1, LOW);
      digitalWrite(r_Out2, LOW);
      delay(160);
      digitalWrite(r_Out1, HIGH);
      digitalWrite(r_Out2, HIGH);
      delay(1);
      digitalWrite(r_wheel, HIGH);
      value = 0;
    }
  }

  delay(100);
}




// ***************************************
// ********** Backbone Methods ***********
// ***************************************


void wifiSetup() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.setHostname("SlidingSpector");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
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
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    //Serial.println("Connected");
    return;
  }
  Serial.print("Connecting to MQTT... ");
  uint8_t retries = 3;

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      //while (1);
      Serial.println("Wait 10 min to reconnect");
      delay(600000);
    }
  }
  Serial.println("MQTT Connected!");
}
