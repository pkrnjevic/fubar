// -----
// Resource-Service -- Caller Code v0.0.1
// (client, PORT, NAME, TYPE, ENV) :: start_express_server(), deregister_express_server(), healthData => resources
// -----


'use strict';

var resources = {};
var healthData = {};
var registrationService;

var getIPAddr = function() {
    try {
		var networkInterfaces = require('os').networkInterfaces();
		var eth = networkInterfaces["eth0"] || networkInterfaces["eth1"];
		for( var x=0; x < eth.length; x++ ) {
			if( !eth[x]['internal'] ) {
				return eth[x]['address'];
			}
		}
	} catch ( e ) {
		return "0.0.0.0";
	}
};

var encodeHash = function( ip, port, time ) { return ip.replace(/\./gi, 'N') + "P" + port + "T" + time;	};
var	hash = 0;
var ping = null;
//var port = null;
	
// Register this service
var register = function(client, PORT, NAME, TYPE, ENV, cb) {
	
	if( ENV == "PROD" || ENV == "STAGE" )
		registrationService = "http://192.168.50.211:8300";
	else
		registrationService = "http://192.168.34.161:8300";
	
	hash = encodeHash(getIPAddr(), PORT, Math.floor(Date.now() / 1000) );
	
	//port = PORT;
	
	//console.log("CALL----", registrationService + "/register/" + ENV + "/" + hash + "?name=" + NAME + "&type=" + TYPE);
	
	var reg_req = client.get( registrationService + "/register/" + ENV + "/" + hash + "?name=" + NAME + "&type=" + TYPE, function(data, response) {
		
		console.log('Registered OK!, resources=', data );
		
		// NPM Module version bug string / json return type
		if( typeof data === "string" ) data = JSON.parse(data);
		
		resources = data.config;
		
		cb(resources);
		
		// Ping interval
		ping = setInterval(function() {
						
			var reg_req2 = client.get( registrationService + "/ping/" + hash + "?health=" + encodeURIComponent(JSON.stringify(healthData)), function(data, response) {
				
				//data = JSON.parse( data );
				// NPM Module version bug string / json return type
				if( typeof data === "string" ) data = JSON.parse(data);
				
				if( data.status === 200 ) {
					
					resources = data.config;
				
				} else {
					
					console.log("Bad Ping! Re-Registering...");
					var reg_req3 = client.get( registrationService + "/register/" + ENV + "/" + hash + "?name=" + NAME + "&type=" + TYPE, function(data, response) {
						console.log("Re-Registred ( " + registrationService + ")! Using new resources!");
						//data = JSON.parse( data );
						// NPM Module version bug string / json return type
						if( typeof data === "string" ) data = JSON.parse(data);
						
						resources = data.config;
					});
					
					reg_req3.on('error', function (err) {
						console.log("Registry Service ( " + registrationService + " ) Problem! Cant re-register, using existing [resources]!");
					});
				}
			});
				
			reg_req2.on('error', function(err) {
				console.log("Registration Service ( " + registrationService + " ) Down, PING Failed! Trying Again Later, using Existing [resources] ***", err);
			});
			
		}, 10000 * 6 ); // Ping every 60 sec
	
	});

	reg_req.on('error', function(err) {
		console.log("Registry Service ( " + registrationService + " ) Down! Cant start without [resources] *** err=", JSON.stringify(err) );
		process.exit(1);		
	});

};
	
var deregister = function(client) {
	client.get( registrationService + "/deregister/" + hash, function(data, response) {
		console.log("deregister_express_server()=", JSON.stringify(data) );
		process.exit(1);
		return true;
	});
	clearInterval(ping);
};

var hello = function(){
	console.log("caller-code says Hello!");
};

var updateHealthData = function(data) {
	healthData = data;
}

module.exports = { register : register, deregister : deregister, updateHealthData : updateHealthData, healthData : healthData, hello : hello };

/* jshint ignore:end */
