import * as net from "net";
import aedes from 'aedes';
const port = 1883;
const broker = aedes({ id: 'broker' });
const server = net.createServer(broker.handle)

server.listen(port, function () {
    console.log('Aedes listening on port:', port)
    broker.publish({ topic: 'aedes/hello', payload: "I'm broker " + broker.id })
})


broker.on('subscribe', function (subscriptions, client) {
    console.log('MQTT client \x1b[32m' + (client ? client.id : client) +
        '\x1b[0m subscribed to topics: ' + subscriptions.map(s => s.topic).join('\n'), 'from broker', broker.id)
})

broker.on('unsubscribe', function (subscriptions, client) {
    console.log('MQTT client \x1b[32m' + (client ? client.id : client) +
        '\x1b[0m unsubscribed to topics: ' + subscriptions.join('\n'), 'from broker', broker.id)
})

// fired when a client connects
broker.on('client', function (client) {
    console.log('Client Connected: \x1b[33m' + (client ? client.id : client) + '\x1b[0m', 'to broker', broker.id)
})

// fired when a client disconnects
broker.on('clientDisconnect', function (client) {
    console.log('Client Disconnected: \x1b[31m' + (client ? client.id : client) + '\x1b[0m', 'to broker', broker.id)
})

/*
// fired when a message is published
broker.on('publish', async function (packet, client) {
    console.log('Client \x1b[31m' + (client ? client.id : 'BROKER_' + broker.id) + '\x1b[0m has published', packet.payload.toString(), 'on', packet.topic, 'to broker', broker.id)
})
*/