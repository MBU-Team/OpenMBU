var dgram = require('dgram');
var net = require('net');

var clientName = process.argv[3];
var remoteName = process.argv[4];

var rendezvous = {
  address: process.argv[2],
  port: 6312
};

var client = {
  ack: false,
  connection: {}
};

var udp_in = dgram.createSocket('udp4');

var getNetworkIP = function(callback) {
  var socket = net.createConnection(80, rendezvous.address);
  socket.on('connect', function() {
    callback(undefined, socket.address().address);
      socket.end();
  });
  socket.on('error', function(e) {
    callback(e, 'error');
  });
}

var send = function(connection, msg, cb) {
  var data = new Buffer(JSON.stringify(msg));

  udp_in.send(data, 0, data.length, connection.port, connection.address, function(err, bytes) {
    if (err) {
      udp_in.close();
      console.log('# stopped due to error: %s', err);
    } else {
      console.log('# sent %s to %s:%s', msg.type, connection.address, connection.port);
      if (cb) cb();
    }
  });
}

udp_in.on("listening", function() {
  var linfo = { port: udp_in.address().port };
  getNetworkIP(function(error, ip) {
    if (error) return console.log("! Unable to obtain connection information!");
    linfo.address = ip;
    console.log('# listening as %s@%s:%s', clientName, linfo.address, linfo.port);
    send(rendezvous, { type: 'register', name: clientName, linfo: linfo }, function() {
      if (remoteName) {
        send(rendezvous, { type: 'connect', from: clientName, to: remoteName });
      }
    });
  });
});

udp_in.on('message', function(data, rinfo) {
  try {
    data = JSON.parse(data);
  } catch (e) {
    console.log('! Couldn\'t parse data(%s):\n%s', e, data);
    return;
  }
  if (data.type == 'connection') {
    console.log('# connecting with %s@[%s:%s | %s:%s]', data.client.name,
      data.client.connections.local.address, data.client.connections.local.port, data.client.connections.public.address, data.client.connections.public.port);
    remoteName = data.client.name;
    var punch = { type: 'punch', from: clientName, to: remoteName };
    for (var con in data.client.connections) {
      doUntilAck(1000, function() {
        send(data.client.connections[con], punch);
      });
    }
  } else if (data.type == 'punch' && data.to == clientName) {
    var ack = { type: 'ack', from: clientName };  
    console.log("# got punch, sending ACK");
    send(rinfo, ack);
  } else if (data.type == 'ack' && !client.ack) {
    client.ack = true;
    client.connection = rinfo;
    console.log("# got ACK, sending MSG");
    send(client.connection, {
      type: 'message',
      from: clientName,
      msg: 'Hello World, '+remoteName+'!' 
    });
  } else if (data.type == 'message') {
    console.log('> %s [from %s@%s:%s]', data.msg, data.from, rinfo.address, rinfo.port)
  } 
});


var doUntilAck = function(interval, fn) {
  if (client.ack) return;
  fn();
  setTimeout(function() {
    doUntilAck(interval, fn);
  }, interval);  
}

udp_in.bind();
