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
