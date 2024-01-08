// MQTT subscriber for testing

//https://www.npmjs.com/package/brightness

import * as mqtt from "mqtt"
import brightness from 'brightness';

const brokeraddress = "192.168.100.198"//"10.0.0.1"
const brokerName = "core-mosquitto"//"mqttBroker"
const brokerPassword = "85208520"
const brokerPort = 1883

//home assistant 

const availabilityTopic = 'homeassistant/light/Bedroom/availability'
const discoveryTopic = 'homeassistant/light/Bedroom/config'
const state_topic = 'homeassistant/light/Bedroom/state'
const command_topic = 'homeassistant/light/Bedroom/set'
const defultBrightness = 100
let brightnessValue = 0
let statusValue = 'ON'

//const discovery Message for device with 2 entitys brightness and state
const discoveryMessage ={
    "name": "Screen Brightness",
    "unique_id": "pc1_light",
    "availability_topic": availabilityTopic,
    "payload_available": 'ON',
    "payload_not_available": 'OFF',
    "state_topic": state_topic,
    "command_topic": command_topic,
    "brightness": true,
    "brightness_scale": 100,
    //"brightness_state_topic": state_topic+"1",
    //"brightness_command_topic": command_topic+"1",
    "schema": "json",
    device: {
        "identifiers": ["pc_01"],
        "name": "my_pc",
        "model": "Latitude_E5590",
        "manufacturer": "Dell",
    },
    "platform": "mqtt"
}

const client = mqtt.connect(`mqtt://${brokeraddress}:${brokerPort}`, {
    clientId: "localSub",
    username: brokerName,
    password: brokerPassword,
}
    /* ,
         {
                 will: {
                     topic: availabilityTopic,
                     payload: 'ON',
                     retain: true
                 }
              
          }*/
);

client.on('connect', () => {
    console.log('pub connect');
    client.subscribe(command_topic)
   // client.subscribe('homeassistant/status')
    client.subscribe(discoveryTopic)
    client.publish(discoveryTopic, JSON.stringify(discoveryMessage))
})

client.on('message', (topic, message) => {
    message = message.toString()
    client.publish(availabilityTopic, 'ON')

    console.log('--------------------')
    console.log(new Date())
    console.log('topic: ' + topic + " , message: " + message)


    let msg = JSON.parse(message) 
    msg.state == 'ON' ? statusValue = 'ON' : statusValue = 'OFF'
    
    if (msg.brightness) brightnessValue = parseFloat(JSON.parse(message).brightness / 100)

    if (msg.state == 'ON') { brightness.set(brightnessValue).then(() => { console.log('Changed brightness to ' + brightnessValue * 100 + '%'); });}
    else { brightness.set(0).then(() => { console.log('Changed brightness to 0%'); });}
   
    client.publish(state_topic, JSON.stringify({ state: statusValue, brightness: brightnessValue }))

    /*  
    if (!isJson(message)) {
        console.log('message: ' + message);
        if (message == 'online') {
            console.log('online')
           // client.publish(discoveryTopic, JSON.stringify(discoveryMessage))
            //client.publish(availabilityTopic, 'online')
            //client.publish(state_topic, 'ON')
            //client.publish(state_topic, JSON.stringify({ state: 'ON', brightness: 255 }))
        }
    }
    else {
        let messageAsJson = JSON.parse(message)
        message
        let val = parseFloat(messageAsJson.brightness / 255)
        messageAsJson.state == 'OFF' ? val = 0 : val = val;
        brightness.set(val).then(() => { console.log('Changed brightness to ' + val + '%'); });
        console.log('121')
        client.publish(availabilityTopic, 'online')

    }
    */
})



/*
brightness.get().then(level => { console.log('level: ' + level); });
brightness.set(0.8).then(() => { console.log('Changed brightness to 80%'); });
*/

function isJson(str) {
    try { JSON.parse(str); }
    catch (e) { return false; }
    return true;
}