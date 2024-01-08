
#include <LEAmDNS_Priv.h>
#include <LEAmDNS_lwIPdefs.h>
#include <LEAmDNS.h>
#include <ESP8266mDNS.h>
//#include <Arduino.h>
#define RESET_BUTTON 2
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

    SPIFFS.format();     //clean FS, for testing

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

#ifdef WORK4S
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <ESP8266WiFi.h>
#include <ArduinoMqttClient.h>

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
IPAddress mqtt_serverIP;// (192, 168, 150, 138);

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40]="server1.local";
char mqtt_topic[100] = "/arduino/simple";
char mqtt_port[6] = "1883";
char api_token[34] = "YOUR_API_TOKEN";
bool shouldSaveConfig = false;    //flag for saving data

//topics
char availabilityTopic[100] = "homeassistant";
char discoveryTopic[100] = "homeassistant";
char state_topic[100] = "homeassistant";
char command_topic[100] = "homeassistant";

char discoveryMsg[1024];
DynamicJsonDocument json(1024);
char returnStateMsg[1024];

char statusValue[3] = "ON";
size_t brightnessValue = 0;

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

void sendMsg(char *topic,char* msg) {
    mqttClient.beginMessage(topic);
	//convert from msg to uint8_t*
	uint8_t* msgUint8 = (uint8_t*)msg;
    
	mqttClient.write(msgUint8,strlen(msg));
	
   // mqttClient.print(msg);
    mqttClient.endMessage();
}

void MQTTConnect() {
    mqttClient.setUsernamePassword("mqttBroker", "85208520");
    mqttClient.setId("esp01");

#pragma region connect
    if (
        (mqtt_server[0] == '(') ||
        (mqtt_server[0] == ':') ||
        (mqtt_server[0] == '.') ||
        (mqtt_server[1] == '.') ||
        (mqtt_server[2] == '.') ||
        (mqtt_server[3] == '.')
        )
    {     //is ip addras
        mqtt_serverIP.fromString(mqtt_server);
        Serial.print(mqtt_serverIP.toString());
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
    Serial.println("\nYou're connected to the MQTT broker!");
#pragma endregion
                              
    Serial.println("subscribe");      
    mqttClient.subscribe(command_topic);
    mqttClient.subscribe(discoveryTopic);
         
	sendMsg(discoveryTopic, discoveryMsg);
    sendMsg(availabilityTopic, "ON");

    //mqttClient.unsubscribe(mqtt_topic);

    Serial.println("Waiting for messages on topic: "); Serial.println(String(command_topic));
}

void setup() {                 
    pinMode(LED, OUTPUT);
    pinMode(RESET_BUTTON, INPUT); 
    digitalWrite(RESET_BUTTON, HIGH);   //for reset buton to work properly
    analogWrite(LED,10);                //turn light a little bit

    Serial.begin(115200);
    while (!Serial) { 
        analogWrite(LED,100);
        delay(1000);
        analogWrite(LED, 0);
        delay(1000);
    }
   
#pragma region WiFi Manager
    
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
#pragma endregion
  
    #pragma region HomeAssistantTopic
    strcat(availabilityTopic, mqtt_topic);
	strcat(availabilityTopic, "/avty_t");
                            
	strcat(discoveryTopic, mqtt_topic);
	strcat(discoveryTopic, "/config");
	
    strcat(state_topic, mqtt_topic);
	strcat(state_topic, "/state");
                                            
	strcat(command_topic, mqtt_topic);
	strcat(command_topic, "/set");
#pragma endregion

#pragma region discoveryMsg
    DynamicJsonDocument json(1024);
	json["~"] = "homeassistant/arduino/simple";
    json["name"] = "Shelf LED";
    json["unique_id"] = "Shelf_01";
    json["avty_t"] = "~/avty_t"; //  availabilityTopic
    json["pl_avail"] = "ON"; //         payload_available
    json["pl_not_avail"] = "OFF";          //payload_not_available
    json["stat_t"] = "~/state";
    json["cmd_t"] = "~/set";
    json["brightness"] = true;
    json["bri_scl"] = 100;        //brightness_scale
    json["schema"] = "json";
    JsonObject device = json.createNestedObject("device");
    device["ids"] = "light_01"; //identifiers
   // device["name"] = "esp8266";
   // device["model"] = "01";
   // device["mf"] = "Esp"; //manufacturer
   // json["platform"] = "mqtt";
    serializeJson(json, discoveryMsg);
#pragma endregion
    
    Serial.println("local ip");
    Serial.println(WiFi.localIP());
    MQTTConnect();
    
}

void loop() {              
     
    //Press the button for 3 sec to enter  ConfigPortal
   // checkIfResetAndConigButtonIsPress();
  
    // avoids being disconnected by the broker
   mqttClient.poll();
    
          
    //ToDo:fix this and add Wifi reconnect
    if(mqttClient.connected() == 0) { MQTTConnect();}
    
	Serial.print(mqttClient.messageTopic());
    int messageSize = mqttClient.parseMessage();
    if (messageSize) {
        Serial.print("\nReceived a message with topic '" + mqttClient.messageTopic() + "', length " + String(messageSize) + " bytes:");

        char incuming[1024];
        int i = 0;
        // use the Stream interface to print the contents
        while (mqttClient.available()) {
            incuming[i] = (char)mqttClient.read();
            i++;
        }
        i = 0;
        Serial.print("incuming: "+String(incuming));
        //analogWrite(LED, constrain(atoi(incuming), 0, 255));
                                             
        json["state"] = statusValue;
        json["brightness"] = brightnessValue;
        serializeJson(json, returnStateMsg);
        
		sendMsg(state_topic, returnStateMsg);
        sendMsg(availabilityTopic, "ON");
        Serial.print("\nreturnStateMsg: " + String(returnStateMsg));
    }
}
#endif // WORKS1


#ifdef WORK0S
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <ESP8266WiFi.h>
#include <ArduinoMqttClient.h>

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

IPAddress mqtt_serverIP;// (192, 168, 150, 138);

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40] = "server1.local";
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
    mqttClient.setUsernamePassword("mqttBroker", "85208520");
    mqttClient.setId("esp01");

    if (
        (mqtt_server[0] == '(') ||
        (mqtt_server[0] == ':') ||
        (mqtt_server[0] == '.') ||
        (mqtt_server[1] == '.') ||
        (mqtt_server[2] == '.') ||
        (mqtt_server[3] == '.')
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
    Serial.println("Subscribing to topic: " + String(mqtt_topic));

    mqttClient.subscribe(mqtt_topic);
    //mqttClient.unsubscribe(mqtt_topic);

    Serial.println("Waiting for messages on topic: " + String(mqtt_topic));

   }

void sendMsg(char* topic, char* msg) {
    mqttClient.beginMessage(topic);
    mqttClient.print("test");
    //convert from msg to uint8_t*
    uint8_t* msgUint8 = (uint8_t*)msg;

    mqttClient.write(msgUint8, strlen(msg));

    // mqttClient.print(msg);
    mqttClient.endMessage();
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }
    pinMode(LED, OUTPUT);
    analogWrite(LED, 10);

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
    //checkIfResetAndConigButtonIsPress();

    //ToDo:fix this and add Wifi reconnect
    if (mqttClient.connected() == 0) {
       
        MQTTConnect();
        //Serial.print("\nWiFi.status: " + String(WiFi.status()));
    }
    else {
    }    int messageSize = mqttClient.parseMessage();
    if (messageSize) {
        // we received a message, print out the topic and contents
        Serial.print("Received a message with topic '");
        Serial.print(mqttClient.messageTopic());
        Serial.print("', length ");
        Serial.print(messageSize);
        Serial.println(" bytes:");

        // use the Stream interface to print the contents
        while (mqttClient.available()) {
            Serial.print((char)mqttClient.read());
        }
		sendMsg("test", "test2");
      
    }    
    delay(100);

        }
#endif // WORKS

#ifdef WORKS_13_04_2023 
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
       
#define MODEL "01"
#define MANUFACTURER "IPewzner"
#define PAYLOAD_NOT_AVAILABLE "OFF"
#define PAYLOAD_AVAILABLE "ON"
#define PLATFORM "mqtt"
#define BRIGHTNESS true
#define MQTT_TOPIC "/light/Bed"   //any Entitie in Home Assistant must have the uniq topic
#define ENTITY_NAME "leds2"       //any Entitie in Home Assistant must have the uniq name
#define DEVICE_NAME "myesp8266"   //any Entitie in Home Assistant must have the uniq device name
#define MAX_BRIGHTNESS 255
#define BUFFER_SIZE 1025

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
IPAddress mqtt_serverIP;                // (192, 168, 150, 138);
       
char mqtt_server[100] = "server1.local";
char mqtt_topic[100] = MQTT_TOPIC;
char mqtt_port[10] = "1883";
char server_usrname[100] = "mqttBroker";
char server_pass[100] = "85208520";
char api_token[100] = "YOUR_API_TOKEN";

bool shouldSaveConfig = false;    //flag for saving data

//topics
char topic_Availability[100] = "homeassistant";
char topic_Discovery[100] = "homeassistant";
char topic_State[100] = "homeassistant";
char topic_Command[100] = "homeassistant";

char stateValue[3] = "ON";

char discoveryMsg[1024];
char returnStateMsg[1024];
DynamicJsonDocument json(1024);

#ifdef BRIGHTNESS
size_t brightnessValue = 0;
#endif // !BRIGHTNESS


void savingConfig() {
    Serial.println("saving config");

    DynamicJsonDocument json(1024);

    json["mqtt_server"] = mqtt_server;
    json["mqtt_topic"] = mqtt_topic;
    json["mqtt_port"] = mqtt_port;
    json["api_token"] = api_token;
    json["server_usrname"] = server_usrname;
    json["server_pass"] = server_pass;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) { Serial.println("failed to open config file for writing"); }

    serializeJson(json, Serial);
    serializeJson(json, configFile);

    configFile.close();
}

void readConfig() {
    //read configuration from FS json
    Serial.println("mounting FS...");

    if (!SPIFFS.begin()) {
        Serial.println("ERR: Failed to mount FS");
        return;
    }
 
    Serial.println("mounted file system");
    
    if (!SPIFFS.exists("/config.json")) {
        Serial.println("ERR: File not exist!");
        return;
    }
    
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
            strcpy(server_usrname, json["server_usrname"]);
            strcpy(server_pass, json["server_pass"]);
        }
        else { Serial.println("failed to load json config"); }
        configFile.close();
    }
}

//callback notifying us of the need to save config
void saveConfigCallback() {
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

void checkIfResetAndConigButtonIsPress() {
    
    if (digitalRead(RESET_BUTTON) >= 1) { return; }
    for (size_t i = 0; i < 6; i++) {
#ifdef BRIGHTNESS                   
        analogWrite(LED, MAX_BRIGHTNESS * (i % 2));
#endif // !BRIGHTNESS
#ifndef BRIGHTNESS     
        digitalWrite(LED, ((i % 2)==0)?LOW:HIGH);
#endif // !BRIGHTNESS
        delay(500);
    }
    if (digitalRead(RESET_BUTTON) < 1) { ESP.restart(); } 
}

void stateValueOn(size_t brightnessValue, String stateValue) {
    Serial.println("stateValueOn");
    Serial.println(stateValue);
#ifdef BRIGHTNESS        
    Serial.println("brightnessValue"+String(brightnessValue));
    analogWrite(LED, constrain(brightnessValue, 0, MAX_BRIGHTNESS));
#endif // !BRIGHTNESS
#ifndef BRIGHTNESS
    digitalWrite(LED, HIGH);
#endif // !BRIGHTNESS
}

void stateValueOff(size_t brightnessValue, String stateValue) {
	Serial.println("stateValueOff");
    Serial.println(stateValue);
#ifdef BRIGHTNESS     
    analogWrite(LED, 0);
    Serial.println("brightnessValue"+String(brightnessValue));
#endif // !BRIGHTNESS
#ifndef BRIGHTNESS
    digitalWrite(LED, LOW);
#endif  // !BRIGHTNESS
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.println("\ncallback topic: " + String(topic));

    //insert msg to json
    DynamicJsonDocument recivedJson(1024);
	deserializeJson(recivedJson, payload, length);
	
    if (recivedJson.containsKey("state")) {
		strcpy(stateValue, recivedJson["state"]);
		Serial.println("json[state]: " + String(stateValue));
	}
    
	if (strcmp(topic, "homeassistant/status") == 0) {
		Serial.println("recived status msg");
		mqttClient.publish(topic_Discovery, discoveryMsg); //may be need one time reset by hand
		//ESP.restart();
    }
    
#ifdef BRIGHTNESS 
    if (recivedJson.containsKey("brightness")) {
		brightnessValue = recivedJson["brightness"];
		Serial.println("json[brightness]: " + String(brightnessValue));
	}
#endif // !BRIGHTNESS
	
    if (strcmp(stateValue, "ON") == 0) {
        stateValueOn(brightnessValue, String(stateValue));
    }
    else {stateValueOff(brightnessValue, String(stateValue));}
    
    //----- return -----

    DynamicJsonDocument returnJson(1024);
    char returnStateMsg[1024];
	returnJson["state"] = stateValue;

#ifdef BRIGHTNESS      
    returnJson["brightness"] = brightnessValue;
#endif // !BRIGHTNESS

    serializeJson(returnJson, returnStateMsg);
    mqttClient.publish(topic_State, returnStateMsg);
    mqttClient.publish(topic_Availability, "ON");
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {;}

    pinMode(LED, OUTPUT);
    pinMode(RESET_BUTTON, INPUT);
    digitalWrite(RESET_BUTTON, HIGH);   //for reset buton to work properly

#ifdef BRIGHTNESS  
    analogWrite(LED, 10);                //turn light a little bit
    delay(100);
#endif // !BRIGHTNESS
   
    checkIfResetAndConigButtonIsPress();

#pragma region WiFi Manager

    //SPIFFS.format();     //clean FS, for testing
    readConfig();

    // The extra parameters to be configured (can be either global or just in the setup)
    // After connecting, parameter.getValue() will get you the configured value
    // id/name placeholder/prompt default length
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
    WiFiManagerParameter custom_mqtt_topic("topic", "mqtt topic", mqtt_topic, 100);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
    WiFiManagerParameter custom_server_usrname("usrname", "server usrname", server_usrname, 20);
    WiFiManagerParameter custom_server_pass("password", "server password", server_pass, 20);
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
    wifiManager.addParameter(&custom_server_usrname);
    wifiManager.addParameter(&custom_server_pass);
    wifiManager.addParameter(&custom_api_token);

    //reset settings - for testing
    //wifiManager.resetSettings();

    //Press button to enter  ConfigPortal
    if (digitalRead(RESET_BUTTON) < 1) {
        if (!wifiManager.startConfigPortal("ESP_AutoConnectAP"/*, "password"*/)) {
            Serial.println("ERR: Failed to connect and hit timeout");
            delay(3 * 1000);
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
		delay(5 * 1000);  //maybe not needed becuse of restart
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

    //read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_topic, custom_mqtt_topic.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(server_usrname, custom_server_usrname.getValue());
    strcpy(server_pass, custom_server_pass.getValue());
    strcpy(api_token, custom_api_token.getValue());
    

    Serial.println("The values in the file are: ");
    Serial.println("\tmqtt_server : " + String(mqtt_server));
    Serial.println("\tmqtt_topic : " + String(mqtt_topic));
    Serial.println("\tmqtt_port : " + String(mqtt_port));
    Serial.println("\tserver_usrname : " + String(server_usrname));
    Serial.println("\tserver_pass : " + String(server_pass));
    Serial.println("\tapi_token : " + String(api_token));

    //save the custom parameters to FS
    if (shouldSaveConfig) { savingConfig(); }
#pragma endregion

#pragma region HomeAssistantTopic
    strcat(topic_Availability, mqtt_topic);
	strcat(topic_Availability, "/availability");

    strcat(topic_Discovery, mqtt_topic);
    strcat(topic_Discovery, "/config");

    strcat(topic_State, mqtt_topic);
    strcat(topic_State, "/state");

    strcat(topic_Command, mqtt_topic);
    strcat(topic_Command, "/set");
#pragma endregion HomeAssistantTopic
    
#pragma region discoveryMsg
    DynamicJsonDocument json(1024);
    json["name"] = ENTITY_NAME;
    json["unique_id"] = ENTITY_NAME + String(ESP.getChipId());
    json["availability_topic"] = topic_Availability;
	json["state_topic"] = topic_State;
	json["command_topic"] = topic_Command;
    json["payload_available"] = PAYLOAD_AVAILABLE;
    json["payload_not_available"] = PAYLOAD_NOT_AVAILABLE; 

#ifdef BRIGHTNESS
    json["brightness"] = true;
    json["brightness_scale"] = MAX_BRIGHTNESS;
 #endif // !BRIGHTNESS
    
    json["schema"] = "json";
    JsonObject device = json.createNestedObject("device");
	device["identifiers"] = DEVICE_NAME + String(ESP.getChipId());
    device["name"] = DEVICE_NAME;
    device["model"] = MODEL; 
    device["manufacturer"] = MANUFACTURER;
    json["platform"] = PLATFORM;
    
    serializeJson(json, discoveryMsg);

    Serial.print("\nDiscoveryMsg: " + String(discoveryMsg));

#pragma endregion discoveryMsg
    
    Serial.println("local ip: " + WiFi.localIP().toString());
    Serial.println("Connecting to MQTT...");
    
    mqttClient.setBufferSize(BUFFER_SIZE);
    mqttClient.setServer(mqtt_server, atoi(mqtt_port));
    mqttClient.setCallback(callback);
    checkIfResetAndConigButtonIsPress();
}

void reconnect() {

    while (!mqttClient.connected()) {
        checkIfResetAndConigButtonIsPress();

        Serial.print("Attempting MQTT connection...");
        if (!mqttClient.connect(String(ESP.getChipId()).c_str(), server_usrname, server_pass)) {
                Serial.println("failed, rc="+String(mqttClient.state())+" try again in 5 seconds");
            delay(1000);
        }

        Serial.println("connected");

        mqttClient.publish(topic_Discovery, discoveryMsg);
        mqttClient.publish(topic_Availability, "ON");

        if (!mqttClient.subscribe(topic_Command)) { Serial.print("fail to "); }
        Serial.println("subscribe to: " + String(topic_Command));
        if (!mqttClient.subscribe("homeassistant/status")) { Serial.print("fail to "); }
        Serial.println("subscribe to: homeassistant/status");

    }
}

void loop() {
    checkIfResetAndConigButtonIsPress();
    if (!mqttClient.connected()) { reconnect();}
    mqttClient.loop();
}

#endif // WORKS_13_04_2023


#ifdef WORKS
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define MODEL "01"
#define MANUFACTURER "IPewzner"
#define PAYLOAD_NOT_AVAILABLE "OFF"
#define PAYLOAD_AVAILABLE "ON"
#define PLATFORM "mqtt"
#define BRIGHTNESS true
#define MQTT_TOPIC "/light/bed"   //any Entitie in Home Assistant must have the uniq topic
#define ENTITY_NAME "led"       //any Entitie in Home Assistant must have the uniq name
#define DEVICE_NAME "esp8266"   //any Entitie in Home Assistant must have the uniq device name
#define MAX_BRIGHTNESS 255
#define BUFFER_SIZE 1025

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
IPAddress mqtt_serverIP;                // (192, 168, 150, 138);

char mqtt_server[100] = "server1.local";
char mqtt_topic[100] = MQTT_TOPIC;
char mqtt_port[10] = "1883";
char server_usrname[100] = "mqttBroker";
char server_pass[100] = "85208520";
char api_token[100] = "YOUR_API_TOKEN";

bool shouldSaveConfig = false;    //flag for saving data

//topics
char topic_Availability[100] = "homeassistant";
char topic_Discovery[100] = "homeassistant";
char topic_State[100] = "homeassistant";
char topic_Command[100] = "homeassistant";

char stateValue[3] = "ON";

char discoveryMsg[1024];
char returnStateMsg[1024];
DynamicJsonDocument json(1024);

#ifdef BRIGHTNESS
size_t brightnessValue = 0;
#endif // !BRIGHTNESS

bool resetButtonPress() { return (digitalRead(RESET_BUTTON) < 1); }

void savingConfig() {
    Serial.println("saving config");

    DynamicJsonDocument json(1024);
    json["mqtt_server"] = mqtt_server;
    json["mqtt_topic"] = mqtt_topic;
    json["mqtt_port"] = mqtt_port;
    json["api_token"] = api_token;
    json["server_usrname"] = server_usrname;
    json["server_pass"] = server_pass;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) { Serial.println("failed to open config file for writing"); }

    serializeJson(json, Serial);
    serializeJson(json, configFile);

    configFile.close();
}

void readConfig() {
    //read configuration from FS json
    Serial.println("mounting FS...");

    if (!SPIFFS.begin()) {
        Serial.println("ERR: Failed to mount FS");
        return;
    }

    Serial.println("mounted file system");

    if (!SPIFFS.exists("/config.json")) {
        Serial.println("ERR: File not exist!");
        return;
    }

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
            strcpy(server_usrname, json["server_usrname"]);
            strcpy(server_pass, json["server_pass"]);
        }
        else { Serial.println("failed to load json config"); }
        configFile.close();
    }
}

//callback notifying us of the need to save config
void saveConfigCallback() {
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

void wiFi_Manager(bool enterConfigPortal = false) {
    //SPIFFS.format();     //clean FS, for testing
    readConfig();

    // The extra parameters to be configured (can be either global or just in the setup)
    // After connecting, parameter.getValue() will get you the configured value
    // id/name placeholder/prompt default length
    WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
    WiFiManagerParameter custom_mqtt_topic("topic", "mqtt topic", mqtt_topic, 100);
    WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
    WiFiManagerParameter custom_server_usrname("usrname", "server usrname", server_usrname, 20);
    WiFiManagerParameter custom_server_pass("password", "server password", server_pass, 20);
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
    wifiManager.addParameter(&custom_server_usrname);
    wifiManager.addParameter(&custom_server_pass);
    wifiManager.addParameter(&custom_api_token);

    //reset settings - for testing
    //wifiManager.resetSettings();

    //Press button to enter  ConfigPortal
    if (resetButtonPress() || enterConfigPortal) {
        if (!wifiManager.startConfigPortal("ESP_AutoConnectAP"/*, "password"*/)) {
            Serial.println("ERR: Failed to connect and hit timeout");
            delay(3 * 1000);
            ESP.restart();
            // delay(5 * 1000);
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
        //   delay(5 * 1000);  //maybe not needed becuse of restart
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");

    //read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_topic, custom_mqtt_topic.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(server_usrname, custom_server_usrname.getValue());
    strcpy(server_pass, custom_server_pass.getValue());
    strcpy(api_token, custom_api_token.getValue());


    Serial.println("The values in the file are: ");
    Serial.println("\tmqtt_server : " + String(mqtt_server));
    Serial.println("\tmqtt_topic : " + String(mqtt_topic));
    Serial.println("\tmqtt_port : " + String(mqtt_port));
    Serial.println("\tserver_usrname : " + String(server_usrname));
    Serial.println("\tserver_pass : " + String(server_pass));
    Serial.println("\tapi_token : " + String(api_token));

    //save the custom parameters to FS
    if (shouldSaveConfig) { savingConfig(); }
}

void discoveryMessage() {
    DynamicJsonDocument json(1024);
    json["name"] = ENTITY_NAME;
    json["unique_id"] = ENTITY_NAME + String(ESP.getChipId());
    json["availability_topic"] = topic_Availability;
    json["state_topic"] = topic_State;
    json["command_topic"] = topic_Command;
    json["payload_available"] = PAYLOAD_AVAILABLE;
    json["payload_not_available"] = PAYLOAD_NOT_AVAILABLE;
    json["info:"] = "send msg to topic \"" + String(MQTT_TOPIC)+"/gotoconfigmode\" to enter config mode";
    
#ifdef BRIGHTNESS
    json["brightness"] = true;
    json["brightness_scale"] = MAX_BRIGHTNESS;
#endif // !BRIGHTNESS

    json["schema"] = "json";
    JsonObject device = json.createNestedObject("device");
    device["identifiers"] = DEVICE_NAME + String(ESP.getChipId());
    device["name"] = DEVICE_NAME;
    device["model"] = MODEL;
    device["manufacturer"] = MANUFACTURER;
    json["platform"] = PLATFORM;

    serializeJson(json, discoveryMsg);

    Serial.print("\nDiscoveryMsg: " + String(discoveryMsg));
}

void HomeAssistantTopic() {

    strcat(topic_Availability, mqtt_topic);
    strcat(topic_Availability, "/availability");

    strcat(topic_Discovery, mqtt_topic);
    strcat(topic_Discovery, "/config");

    strcat(topic_State, mqtt_topic);
    strcat(topic_State, "/state");

    strcat(topic_Command, mqtt_topic);
    strcat(topic_Command, "/set");
}

void checkIfResetAndConigButtonIsPress() {

    if (digitalRead(RESET_BUTTON) >= 1) { return; }
    for (size_t i = 0; i < 6; i++) {
#ifdef BRIGHTNESS                   
        analogWrite(LED, MAX_BRIGHTNESS * (i % 2));
#else    
        digitalWrite(LED, ((i % 2) == 0) ? LOW : HIGH);
#endif // !BRIGHTNESS
        delay(500);
    }
    if (resetButtonPress) { ESP.restart(); }
}

void stateValueOn(size_t brightnessValue, String stateValue) {
    Serial.println("stateValueOn");
    Serial.println(stateValue);
#ifdef BRIGHTNESS        
    Serial.println("brightnessValue" + String(brightnessValue));
    analogWrite(LED, constrain(brightnessValue, 0, MAX_BRIGHTNESS));
#else
    digitalWrite(LED, HIGH);
#endif // !BRIGHTNESS
}

void stateValueOff(size_t brightnessValue, String stateValue) {
    Serial.println("stateValueOff");
    Serial.println(stateValue);
#ifdef BRIGHTNESS     
    analogWrite(LED, 0);
    Serial.println("brightnessValue" + String(brightnessValue));
#else
    digitalWrite(LED, LOW);
#endif  // !BRIGHTNESS
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.println("\ncallback topic: " + String(topic));

    //insert msg to json
    DynamicJsonDocument recivedJson(1024);
    deserializeJson(recivedJson, payload, length);

    if (recivedJson.containsKey("state")) {
        strcpy(stateValue, recivedJson["state"]);
        Serial.println("json[state]: " + String(stateValue));
    }

    if (strcmp(topic, "homeassistant/status") == 0) {
        Serial.println("recived status msg");
        mqttClient.publish(topic_Discovery, discoveryMsg); //may be need one time reset by hand
        //ESP.restart();
    }
    
    if (strcmp(topic, MQTT_TOPIC"/gotoconfigmode") == 0) {wiFi_Manager(true);}

#ifdef BRIGHTNESS 
    if (recivedJson.containsKey("brightness")) {
        brightnessValue = recivedJson["brightness"];
        Serial.println("json[brightness]: " + String(brightnessValue));
    }
#endif // !BRIGHTNESS

    if (strcmp(stateValue, "ON") == 0) {
        stateValueOn(brightnessValue, String(stateValue));
    }
    else { stateValueOff(brightnessValue, String(stateValue)); }

    //----- return -----
    
    DynamicJsonDocument returnJson(1024);
    char returnStateMsg[1024];
    returnJson["state"] = stateValue;

#ifdef BRIGHTNESS      
    returnJson["brightness"] = brightnessValue;
#endif // !BRIGHTNESS

    serializeJson(returnJson, returnStateMsg);
    mqttClient.publish(topic_State, returnStateMsg);
    mqttClient.publish(topic_Availability, "ON");
}

void reconnect() {

    while (!mqttClient.connected()) {
        checkIfResetAndConigButtonIsPress();

        Serial.print("Attempting MQTT connection...");
        if (!mqttClient.connect(String(ESP.getChipId()).c_str(), server_usrname, server_pass)) {
            Serial.println("failed, rc=" + String(mqttClient.state()) + " try again in 5 seconds");
            delay(1000);
        }

        Serial.println("connected");

        mqttClient.publish(topic_Discovery, discoveryMsg);
        mqttClient.publish(topic_Availability, "ON");

        if (!mqttClient.subscribe(topic_Command)) { Serial.print("fail to "); }
        Serial.println("subscribe to: " + String(topic_Command));
        if (!mqttClient.subscribe("homeassistant/status")) { Serial.print("fail to "); }
        Serial.println("subscribe to: homeassistant/status");
        if (!mqttClient.subscribe(MQTT_TOPIC"/gotoconfigmode")) { Serial.print("fail to "); }
        Serial.println("subscribe to: gotoconfigmode");
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) { ; }

    pinMode(LED, OUTPUT);
    pinMode(RESET_BUTTON, INPUT);
    digitalWrite(RESET_BUTTON, HIGH);   //for reset buton to work properly

#ifdef BRIGHTNESS  
    analogWrite(LED, 10);                //turn light a little bit
#else
	digitalWrite(LED, HIGH);            //turn light on
#endif // !BRIGHTNESS

    checkIfResetAndConigButtonIsPress();
    wiFi_Manager();
    HomeAssistantTopic();
    discoveryMessage();
    
    Serial.println("local ip: " + WiFi.localIP().toString());
    Serial.println("Connecting to MQTT...");

    mqttClient.setBufferSize(BUFFER_SIZE);
    mqttClient.setServer(mqtt_server, atoi(mqtt_port));
    mqttClient.setCallback(callback);
    checkIfResetAndConigButtonIsPress();
}

void loop() {
    checkIfResetAndConigButtonIsPress();
    if (!mqttClient.connected()) { reconnect(); }
    mqttClient.loop();
}
#endif // WORKS
