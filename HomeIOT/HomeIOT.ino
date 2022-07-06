#include <LEAmDNS_Priv.h>
#include <LEAmDNS_lwIPdefs.h>
#include <LEAmDNS.h>
#include <ESP8266mDNS.h>
#include <Arduino.h>
#define RESET_BUTTON  2
#define LED      0
#define WORKS

#ifdef WIFI_MANAGER
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#ifdef ESP32
#include <SPIFFS.h>
#endif


//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40];
char mqtt_topic[100] = "";
char mqtt_port[6] = "8080";
char api_token[34] = "YOUR_API_TOKEN";
bool shouldSaveConfig = false;    //flag for saving data

void savingConfig() {
    Serial.println("saving config");

    DynamicJsonDocument json(1024);

    json["mqtt_server"] = mqtt_server;
    json["mqtt_topic"] = mqtt_topic;
    json["mqtt_port"] = mqtt_port;
    json["api_token"] = api_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) { Serial.println("failed to open config file for writing"); }

    serializeJson(json, Serial);
    serializeJson(json, configFile);

    configFile.close();
}

void readConfig() {
    //read configuration from FS json
    Serial.println("mounting FS...");

    if (SPIFFS.begin()) {
        Serial.println("mounted file system");
        if (SPIFFS.exists("/config.json")) {
            //file exists, reading and loading
            Serial.println("reading config file");
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile) {
                Serial.println("opened config file");
                size_t size = configFile.size();
                // Allocate a buffer to store contents of the file.
                std::unique_ptr<char[]> buf(new char[size]);

                configFile.readBytes(buf.get(), size);

                DynamicJsonDocument json(1024);
                auto deserializeError = deserializeJson(json, buf.get());
                serializeJson(json, Serial);
                if (!deserializeError) {
                    Serial.println("\nparsed json");
                    strcpy(mqtt_server, json["mqtt_server"]);
                    strcpy(mqtt_topic, json["mqtt_topic"]);
                    strcpy(mqtt_port, json["mqtt_port"]);
                    strcpy(api_token, json["api_token"]);
                }
                else { Serial.println("failed to load json config"); }
                configFile.close();
            }
        }
    }
    else { Serial.println("failed to mount FS"); }
}

//callback notifying us of the need to save config
void saveConfigCallback() {
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

void setup() {

    Serial.begin(115200);

    //SPIFFS.format();     //clean FS, for testing

    readConfig();

    // The extra parameters to be configured (can be either global or just in the setup)
    // After connecting, parameter.getValue() will get you the configured value
    // id/name placeholder/prompt default length
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
    WiFiManagerParameter custom_mqtt_topic("topic", "mqtt topic", mqtt_topic, 100);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
    WiFiManagerParameter custom_api_token("apikey", "API token", api_token, 32);

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    //set static ip
    wifiManager.setSTAStaticIPConfig(IPAddress(10, 0, 1, 99), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));

    //add all your parameters here
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_topic);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_api_token);

    //reset settings - for testing
    //wifiManager.resetSettings();

    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
        Serial.println("failed to connect and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.restart();
        delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

    //read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_topic, custom_mqtt_topic.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(api_token, custom_api_token.getValue());
    Serial.println("The values in the file are: ");
    Serial.println("\tmqtt_server : " + String(mqtt_server));
    Serial.println("\tmqtt_topic : " + String(mqtt_topic));
    Serial.println("\tmqtt_port : " + String(mqtt_port));
    Serial.println("\tapi_token : " + String(api_token));

    //save the custom parameters to FS
    if (shouldSaveConfig) { savingConfig(); }

    Serial.println("local ip");
    Serial.println(WiFi.localIP());
}

void loop() {
    // put your main code here, to run repeatedly:

}




#endif // WIFI_MANAGER

#ifdef ADAFRUIT_IO       
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#define WLAN_SSID       "ip1"
#define WLAN_PASS       "85208520"
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    ""
#define AIO_KEY         ""
#define NAME            "/topics/bed1"

// Create an ESP8266 WiFiClient class to connect to the MQTT server. or... use WiFiClientSecure for SSL
WiFiClient client;
//WiFiClientSecure client;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Setup a topic called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME NAME);


void MQTT_connect();

void setup() {
    Serial.begin(115200);
    pinMode(2, OUTPUT);

    WiFi.begin(WLAN_SSID, WLAN_PASS);
    while (WiFi.status() != WL_CONNECTED) {

        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Setup MQTT subscription for onoff topic.
    mqtt.subscribe(&onoffbutton);
}

uint32_t x = 0;

void loop() {
    MQTT_connect();

    // this is our 'wait for incoming subscription packets' busy subloop
    // try to spend your time here

    Adafruit_MQTT_Subscribe* subscription;
    while ((subscription = mqtt.readSubscription(1000))) {
        if (subscription == &onoffbutton) {
            Serial.print(F("Got: "));
            Serial.println((char*)onoffbutton.lastread);
            Serial.println(atoi((char*)onoffbutton.lastread));
            analogWrite(0, atoi((char*)onoffbutton.lastread)); // analogRead values go from 0 to 1023, analogWrite values from 0 to 255

        }
    }


    // ping the server to keep the mqtt connection alive
    // NOT required if you are publishing once every KEEPALIVE seconds
    /*
    if(! mqtt.ping()) {
      mqtt.disconnect();
    }
    */
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
    int8_t ret;

    // Stop if already connected.
    if (mqtt.connected()) { return; }

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
            while (1);
        }
    }
    Serial.println("MQTT Connected!");
}

#endif // ADAFRUIT_IO

#ifdef LOCAL_MQTT
#include <ESP8266WiFi.h>

#include <ArduinoMqttClient.h>
#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_AVR_UNO_WIFI_REV2)
#include <WiFiNINA.h>
#elif defined(ARDUINO_SAMD_MKR1000)
#include <WiFi101.h>
#elif defined(ARDUINO_ESP8266_ESP12)
#include <ESP8266WiFi.h>
#endif

char ssid[] = "ip1";
char pass[] = "85208520";
//----------
const long interval = 1000;
unsigned long previousMillis = 0;
int count = 0;
//---------
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

IPAddress mqtt_server(192, 168, 150, 138);
const char topic[] = "/arduino/simple";


void WifiConnect() {
    Serial.println("Attempting to connect to WPA SSID: " + String(ssid));
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("You're connected to the network");
}

void MQTTConnect() {

    mqttClient.setUsernamePassword("username", "password");
    mqttClient.setId("esp01");
    Serial.print("Try connecting to the MQTT broker: " + String(mqtt_server.toString()));
    while (!mqttClient.connect(mqtt_server)) {
        Serial.print("\nMQTT connection failed! Error code = " + String(mqttClient.connectError()));
        delay(5000);
    }

    Serial.println("You're connected to the MQTT broker!");
    Serial.print("Subscribing to topic: "); Serial.print(String(topic));

    mqttClient.subscribe(topic);
    //mqttClient.unsubscribe(topic);

    Serial.println("Waiting for messages on topic: "); Serial.println(String(topic));
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }
    WifiConnect();
    MQTTConnect();
}

void loop() {
    int messageSize = mqttClient.parseMessage();
    if (messageSize) {
        Serial.print("\nReceived a message with topic '" + mqttClient.messageTopic() + "', length " + String(messageSize) + " bytes:");

        // use the Stream interface to print the contents
        while (mqttClient.available()) {
            Serial.print((char)mqttClient.read());
        }
    }


    /*  //-----------
         // call poll() regularly to allow the library to send MQTT keep alives which
      // avoids being disconnected by the broker
      mqttClient.poll();

      // avoid having delays in loop, we'll use the strategy from BlinkWithoutDelay
      // see: File -> Examples -> 02.Digital -> BlinkWithoutDelay for more info
      unsigned long currentMillis = millis();

      if (currentMillis - previousMillis >= interval) {
          // save the last time a message was sent
          previousMillis = currentMillis;

          Serial.print("\nSending message to topic: " + String(topic2) + "hello " + String(count));

          // send message, the Print interface can be used to set the message contents
          mqttClient.beginMessage(topic2);
          mqttClient.print("hello ");
          mqttClient.print(count);
          mqttClient.endMessage();
          count++;
      }    */
}
#endif // LOCAL_MQTT

#ifdef WORKS
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <ESP8266WiFi.h>
#include <ArduinoMqttClient.h>

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

IPAddress mqtt_serverIP;// (192, 168, 150, 138);

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40] = "";
char mqtt_topic[100] = "/arduino/simple";
char mqtt_port[6] = "1883";
char api_token[34] = "YOUR_API_TOKEN";
bool shouldSaveConfig = false;    //flag for saving data

void savingConfig() {
    Serial.println("saving config");

    DynamicJsonDocument json(1024);

    json["mqtt_server"] = mqtt_server;
    json["mqtt_topic"] = mqtt_topic;
    json["mqtt_port"] = mqtt_port;
    json["api_token"] = api_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) { Serial.println("failed to open config file for writing"); }

    serializeJson(json, Serial);
    serializeJson(json, configFile);

    configFile.close();
}

void readConfig() {
    //read configuration from FS json
    Serial.println("mounting FS...");

    if (SPIFFS.begin()) {
        Serial.println("mounted file system");
        if (SPIFFS.exists("/config.json")) {
            //file exists, reading and loading
            Serial.println("reading config file");
            File configFile = SPIFFS.open("/config.json", "r");
            if (configFile) {
                Serial.println("opened config file");
                size_t size = configFile.size();
                // Allocate a buffer to store contents of the file.
                std::unique_ptr<char[]> buf(new char[size]);

                configFile.readBytes(buf.get(), size);

                DynamicJsonDocument json(1024);
                auto deserializeError = deserializeJson(json, buf.get());
                serializeJson(json, Serial);
                if (!deserializeError) {
                    Serial.println("\nparsed json");
                    strcpy(mqtt_server, json["mqtt_server"]);
                    strcpy(mqtt_topic, json["mqtt_topic"]);
                    strcpy(mqtt_port, json["mqtt_port"]);
                    strcpy(api_token, json["api_token"]);
                }
                else { Serial.println("failed to load json config"); }
                configFile.close();
            }
        }
    }
    else { Serial.println("failed to mount FS"); }
}

//callback notifying us of the need to save config
void saveConfigCallback() {
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

void checkIfResetAndConigButtonIsPress() {
    if (digitalRead(RESET_BUTTON) < 1) {
        for (size_t i = 0; i < 6; i++) {
            analogWrite(LED, 100 * (i % 2));
            delay(500);
        }
        if (digitalRead(RESET_BUTTON) < 1) {
            ESP.restart();
        }
    }
}

void MQTTConnect() {
    mqttClient.setUsernamePassword("username", "password");
    mqttClient.setId("esp01");

    if (
        (mqtt_server[0] = '(') ||
        (mqtt_server[0] = ':') ||
        (mqtt_server[0] = '.') ||
        (mqtt_server[1] = '.') ||
        (mqtt_server[2] = '.') ||
        (mqtt_server[3] = '.')
        )
    {     //is ip addras
        mqtt_serverIP.fromString(mqtt_server);
        Serial.print("Try connecting to the MQTT broker: " + String(mqtt_serverIP.toString()));
        while (!mqttClient.connect(mqtt_serverIP)) {
            Serial.print("\nMQTT connection failed! Error code = " + String(mqttClient.connectError()));
            delay(5000);

            //Press the button for 3 sec to enter  ConfigPortal
            checkIfResetAndConigButtonIsPress();
        }
    }
    else {   //is path addras
        Serial.print("Try connecting to the MQTT broker: " + String(mqtt_server));
        while (!mqttClient.connect(mqtt_server, atoi(mqtt_port))) {
            Serial.print("\nMQTT connection failed! Error code = " + String(mqttClient.connectError()));
            delay(5000);

            //Press the button for 3 sec to enter  ConfigPortal
            checkIfResetAndConigButtonIsPress();
        }
    }
    Serial.println("You're connected to the MQTT broker!");
    Serial.print("Subscribing to topic: "); Serial.print(String(mqtt_topic));

    mqttClient.subscribe(mqtt_topic);
    //mqttClient.unsubscribe(mqtt_topic);

    Serial.println("Waiting for messages on topic: "); Serial.println(String(mqtt_topic));
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }
    pinMode(LED, OUTPUT);
    analogWrite(LED,10);

    pinMode(RESET_BUTTON, INPUT);
    digitalWrite(RESET_BUTTON, HIGH);
    delay(100);

    //SPIFFS.format();     //clean FS, for testing
    readConfig();

    // The extra parameters to be configured (can be either global or just in the setup)
    // After connecting, parameter.getValue() will get you the configured value
    // id/name placeholder/prompt default length
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
    WiFiManagerParameter custom_mqtt_topic("topic", "mqtt topic", mqtt_topic, 100);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
    WiFiManagerParameter custom_api_token("apikey", "API token", api_token, 32);

    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    

    //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    //set static ip
    //wifiManager.setSTAStaticIPConfig(IPAddress(10, 0, 1, 99), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));

    //add all your parameters here
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_topic);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_api_token);

    //reset settings - for testing
    //wifiManager.resetSettings();

    //Press button to enter  ConfigPortal
    if (digitalRead(RESET_BUTTON) < 1) {
        if (!wifiManager.startConfigPortal("ESP_AutoConnectAP"/*, "password"*/)) {
            Serial.println("failed to connect and hit timeout");
            delay(3 * 1000);
            //reset and try again, or maybe put it to deep sleep
            ESP.restart();
            delay(5 * 1000);
        }
    }

    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect("ESP_AutoConnectAP"/*, "password"*/)) {
        Serial.println("failed to connect and hit timeout");
        delay(3 * 1000);
        //reset and try again, or maybe put it to deep sleep
        ESP.restart();
        delay(5 * 1000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

    //read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_topic, custom_mqtt_topic.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(api_token, custom_api_token.getValue());

    Serial.println("The values in the file are: ");
    Serial.println("\tmqtt_server : " + String(mqtt_server));
    Serial.println("\tmqtt_topic : " + String(mqtt_topic));
    Serial.println("\tmqtt_port : " + String(mqtt_port));
    Serial.println("\tapi_token : " + String(api_token));

    //save the custom parameters to FS
    if (shouldSaveConfig) { savingConfig(); }

    Serial.println("local ip");
    Serial.println(WiFi.localIP());
    MQTTConnect();
}

void loop() {
    //Press the button for 3 sec to enter  ConfigPortal
    checkIfResetAndConigButtonIsPress();
     
    int messageSize = mqttClient.parseMessage();
    if (messageSize) {
        Serial.print("\nReceived a message with topic '" + mqttClient.messageTopic() + "', length " + String(messageSize) + " bytes:");

        char incuming[100];
        int i = 0;
        // use the Stream interface to print the contents
        while (mqttClient.available()) {
            incuming[i] = (char)mqttClient.read();
            i++;
        }
        i = 0;
        Serial.print(String(incuming));
        analogWrite(LED, constrain(atoi(incuming), 0, 255));
    }
}
#endif // WORKS


