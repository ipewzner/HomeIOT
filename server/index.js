//temporary client 
import express from "express";
import bodyParser from "body-parser";
import cors from "cors";
import mongoose from "mongoose";
import path from "path";
import fs from "fs";
import os from "os";
import * as mqtt from "mqtt"

import { fileURLToPath } from 'url';
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const app = express();
app.use(express.static(path.join(__dirname, 'public')));

//import html file to use in the server from the current directory

let html=fs.readFileSync("./public/web.html");
/*
//function that return ocal ip address
const getLocalIp = () => {

  const ifaces = os.networkInterfaces();
  let ip = "";
  Object.keys(ifaces).forEach(function (ifname) {
    let alias = 0;
    ifaces[ifname].forEach(function (iface) {
      if ("IPv4" !== iface.family || iface.internal !== false) {
        // skip over internal (i.e.)
        //console.log(ifname, alias, iface.address);
        ip = iface.address;
        // return;
      }
      if (alias >= 1) { ip = iface.address; }
      ++alias;
    });
  });
  return ip;
};

console.log("Local IP: " + getLocalIp());
*/

app.use(bodyParser.json({ limit: "50mb", extends: true }));
app.use(bodyParser.urlencoded({ limit: "50mb", extends: true }));
app.use(cors());

const PORT = process.env.PORT || 5000;
const server = app.listen(PORT, () => console.log(`Server running on port: ${PORT}`));
app.get("/home", function (req, res) {
  //return web.html file thet in the current directory
  console.log("/home")
  res.setHeader("Content-Type", "text/html");
  res.writeHead(200);
  res.end(html);
});

//-----------------------------------------------

const client = mqtt.connect('http://mqttServer.local:1883',{clientId:"webSite"});
const topic = '/iot/table';

client.on('connect', function () {
  client.subscribe(topic, function (err) {
    if (!err) {
     // client.publish(topic, 'Hello mqtt')
    }
  })
})

client.on('message', function (topic, message) {
  // message is Buffer
  console.log(message.toString())
  //client.end()
})

app.post('/volume', function (req, res) {
  console.log(req.body.val);
  client.publish(topic, req.body.val)
  //client.publish(topic, "koko")
});




/*
import * as net from "net";
import aedes from 'aedes';
const port = 1883;
const aedes2 = aedes({ id: 'broker' });
const server3 = net.createServer(aedes2.handle)

server3.listen(port, function () {
  console.log('Aedes listening on port:', port)
  aedes2.publish({ topic: 'aedes/hello', payload: "I'm broker " + aedes2.id })
})

app.get("/home", function (req, res) {
  //return web.html file thet in the current directory
  res.setHeader("Content-Type", "text/html");
  res.writeHead(200);
  res.end(html);
});
app.post('/volume', function (req, res) {
  console.log(req.body.val);
  aedes2.publish({ topic: '/arduino/simple', payload: req.body.val })

});

aedes2.on('subscribe', function (subscriptions, client) {
  console.log('MQTT client \x1b[32m' + (client ? client.id : client) +
    '\x1b[0m subscribed to topics: ' + subscriptions.map(s => s.topic).join('\n'), 'from broker', aedes2.id)
})

aedes2.on('unsubscribe', function (subscriptions, client) {
  console.log('MQTT client \x1b[32m' + (client ? client.id : client) +
    '\x1b[0m unsubscribed to topics: ' + subscriptions.join('\n'), 'from broker', aedes2.id)
})

// fired when a client connects
aedes2.on('client', function (client) {
  console.log('Client Connected: \x1b[33m' + (client ? client.id : client) + '\x1b[0m', 'to broker', aedes2.id)
})

// fired when a client disconnects
aedes2.on('clientDisconnect', function (client) {
  console.log('Client Disconnected: \x1b[31m' + (client ? client.id : client) + '\x1b[0m', 'to broker', aedes2.id)
})
*/
/*
// fired when a message is published
aedes2.on('publish', async function (packet, client) {
  console.log('Client \x1b[31m' + (client ? client.id : 'BROKER_' + aedes2.id) + '\x1b[0m has published', packet.payload.toString(), 'on', packet.topic, 'to broker', aedes2.id)
})

*/