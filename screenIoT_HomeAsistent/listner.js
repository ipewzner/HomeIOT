
import * as mqtt from "mqtt"

const brokeraddress = "10.0.0.1"
const brokerName = "mqttBroker"
const brokerPassword = "85208520"
const brokerPort = 1883


const client = mqtt.connect(`mqtt://${brokeraddress}:${brokerPort}`, {
    clientId: "localSub01",
    username: brokerName,
    password: brokerPassword,
}
)

client.on("connect", () => {
    console.log("connected to broker")
    client.subscribe("#")

 /*
    client.subscribe("homeassistant/arduino/simple/availability")
    client.subscribe("homeassistant/arduino/simple/config")
    client.subscribe("homeassistant/arduino/simple/state")
    client.subscribe("homeassistant/arduino/simple/set")
    */

})

client.on("message", (topic, message) => {
    console.log("message received")
    console.log("topic: "+topic+". message: "+message.toString())
})

