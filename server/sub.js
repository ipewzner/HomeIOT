// MQTT subscriber
import * as mqtt from "mqtt"
const client = mqtt.connect('http://mqttServer.local:1883',{clientId:"mqttjs044"});

const topic = '/iot/table';

client.on('message', (topic, message)=>{
    message = message.toString()
    console.log(message)
})

 
client.on('connect', ()=>{
    console.log('pub connect');
    client.subscribe(topic)
})

 