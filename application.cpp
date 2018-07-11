#include <SmingCore/SmingCore.h>
#include <user_config.h>
#include "user_interface.h"


// Device-specific network settings
#define myHost "Darkwater" // Host name
#define myAddress "192.168.0.106" // Receiver IP
#define rAddress "192.168.0.1" // Gateway IP
#define netMask "255.255.255.0" // Net mask

#ifndef WIFI_SSID
	#define WIFI_SSID "xxxxxxxxxxxxx"
	#define WIFI_PWD "xxxxxxxxxxxxxxx"
#endif

// Application-specific settings
#define ioPin 4 // GPIO4
#define indLED 16 // Onboard NodeMCU LED
uint32_t cycleTime = 50; // Cycle delay period in milliseconds

String buf;
int ioState;
bool serverStarted = false;
HttpServer server;

// Setup initial configuration for Station
void initialWifiConfig()
{
	// Set up Station and WiFi client
	WifiStation.setHostname(myHost);
	WifiStation.setIP(IPAddress(myAddress),IPAddress(netMask),IPAddress(rAddress));
	WifiStation.config(WIFI_SSID, WIFI_PWD);
	WifiStation.enable(true);
	WifiStation.connect();
}

void STAGotIP(IPAddress ip, IPAddress mask, IPAddress gateway)
// Will be called when WiFi station was connected to AP and got IP from it
{
	Serial.printf("GOTIP - IP: %s, MASK: %s, GW: %s\n", ip.toString().c_str(), mask.toString().c_str(), gateway.toString().c_str());
	// TODO: Add commands to be executed after successfully connecting to AP and got IP from it
}
void STADisconnect(String ssid, uint8_t ssid_len, uint8_t bssid[6], uint8_t reason)
// Will be called when disconnected from AP or not connected to last known AP (ssid/password)
{
	Serial.printf("DISCONNECT - SSID: %s, REASON: %d\n", ssid.c_str(), reason);
	// TODO: Add commands to be executed after falling off the AP
}

void setOP(int oState)
// sets the output state
{
	digitalWrite(ioPin, ioState);
	delay(cycleTime);
	digitalWrite(indLED, !ioState);
	delay(cycleTime);
	system_rtc_mem_write(64, &ioState, 4);
}

void ready()
// Will be called when WiFi hardware and software initialization was finished
{
	Serial.printf("%s is READY!\n", myHost);
	pinMode(ioPin, OUTPUT);
	pinMode(indLED, OUTPUT);

	// Reload from RTC memory (to get around watchdog restarts)
	system_rtc_mem_read(64, &ioState, 4);

	//Clean up in case power cycled and random data
	if (ioState < 0 || ioState > 1){
		ioState=0;
	}

	setOP(ioState);
}

void onStatus(HttpRequest &request, HttpResponse &response)
{
	//Send a status response
	buf = "Status OK";

	// And send it
	response.setCache(86400, true);
	response.sendString(buf);
}

void onIndex(HttpRequest &request, HttpResponse &response)
{
	// Set the IO pin according to the requested state
	ioState = request.getQueryParameter("state", "0").toInt();
	setOP(ioState);

	//Build a status response
	if (ioState > 0) {
		buf = "State set to ON";
	}
	else {
		buf = "State set to OFF";
	}

	// And send it
	response.setCache(86400, true); // It's important to use cache for better performance.
	response.sendString(buf);
}

void startWebServer()
{
	if (serverStarted) return;

	server.listen(80);
	server.addPath("/", onIndex);
	server.addPath("/status", onStatus);
	server.setDefaultHandler(onIndex);
	serverStarted = true;
}

void connectOk(IPAddress ip, IPAddress mask, IPAddress gateway)
// Will be called when WiFi station was connected to AP
{
	Serial.println("I'm CONNECTED");
	Serial.println(ip.toString());
}

void connectFail(String ssid, uint8_t ssidLength, uint8_t *bssid, uint8_t reason)
// Will be called when WiFi station was disconnected
{
	// The different reason codes can be found in user_interface.h. in your SDK.
	Serial.printf("Disconnected from %s. Reason: %d", ssid.c_str(), reason);
	// TODO: Add commands to be executed to reconnect
	serverStarted = false;
	initialWifiConfig();
	startWebServer();
}


void init()
{
	//Debug output settings
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(false); // Allow debug print to serial if true

	initialWifiConfig();

	// Attach Wifi events handlers
	WifiEvents.onStationDisconnect(STADisconnect);
	WifiEvents.onStationGotIP(STAGotIP);

	// Set system ready callback method
	System.onReady(ready);

	startWebServer();
}




