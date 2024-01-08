// MQTT publisher for testing
import * as mqtt from "mqtt";
const client = mqtt.connect('mqtt://localhost:1883',{clientId:"mqttjs02"});
const topic = '/iot/table';
const message = "255";
console.log('koko');

client.on('connect', ()=>{ 
    client.publish(topic, message);

  
});  