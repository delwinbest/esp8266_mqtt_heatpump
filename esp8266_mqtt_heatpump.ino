#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "HeatPump.h"
#include "heatpump_html.h"

// WiFi includes
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
//#include <NTPClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>

// OTA Includes
#include <ArduinoOTA.h>

// Custom Includes
#include "config.h"

ESP8266WebServer server(80);
/* Soft AP network parameters */
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

// DNS server
DNSServer dnsServer;

// wifi, mqtt and heatpump client instances
WiFiClient espClient;
PubSubClient mqtt_client(espClient);
HeatPump hp;
unsigned long lastTempSend;

// debug mode, when true, will send all packets received from the heatpump to topic heatpump_debug_topic
// this can also be set by sending "on" to heatpump_debug_set_topic
bool _debugMode = false;


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
#ifdef SERIALDEBUG
  Serial.begin(115200);
#endif
  // connect to the heatpump. Callbacks first so that the hpPacketDebug callback is available for connect()
  hp.setSettingsChangedCallback(hpSettingsChanged);
  hp.setStatusChangedCallback(hpStatusChanged);
  hp.setPacketCallback(hpPacketDebug);

#ifndef SERIALDEBUG
  hp.connect(&Serial);
#endif

  lastTempSend = millis();

  WiFi.disconnect(true);
#ifdef SERIALDEBUG
  Serial.println("Wifi Disconnect");
#endif
  WiFi.softAPdisconnect(true);
  WiFi.persistent(false);
  delay(500);
  WiFi.mode(WIFI_AP_STA);
#ifdef SERIALDEBUG
  Serial.println("Wifi Setmode AP_STA");
#endif
  delay(500);
  setupCore();

  // Set Hostname.
  String hostname(myHostname);
  WiFi.hostname(myHostname);

  // startup mqtt connection
  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  //mqttConnect();

}

void hpSettingsChanged() {
  const size_t bufferSize = JSON_OBJECT_SIZE(6);
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonObject& root = jsonBuffer.createObject();

  heatpumpSettings currentSettings = hp.getSettings();

  root["power"]       = currentSettings.power;
  if (currentSettings.power == "off") {
    root["mode"]        = "off";
  } else {
    root["mode"]        = currentSettings.mode;
  }
  root["temperature"] = currentSettings.temperature;
  root["fan"]         = currentSettings.fan;
  root["vane"]        = currentSettings.vane;
  root["wideVane"]    = currentSettings.wideVane;
  //root["iSee"]        = currentSettings.iSee;


  char buffer[512];
  root.printTo(buffer, sizeof(buffer));



  bool retain = true;
  if (!mqtt_client.publish(client_id, buffer, retain)) {
    mqtt_client.publish(heatpump_debug_topic, "failed to publish to heatpump topic");
  }
}

void hpStatusChanged(heatpumpStatus currentStatus) {
  // send room temp and operating info
  const size_t bufferSizeInfo = JSON_OBJECT_SIZE(2);
  DynamicJsonBuffer jsonBufferInfo(bufferSizeInfo);

  JsonObject& rootInfo = jsonBufferInfo.createObject();
  rootInfo["roomTemperature"] = currentStatus.roomTemperature;
  rootInfo["operating"]       = currentStatus.operating;

  char bufferInfo[512];
  rootInfo.printTo(bufferInfo, sizeof(bufferInfo));

  //mqtt_client.publish(heatpump_target_current_temp, rootInfo["roomTemperature"]);

  if (!mqtt_client.publish(heatpump_status_topic, bufferInfo, true)) {
    mqtt_client.publish(heatpump_debug_topic, "failed to publish to room temp and operation status to heatpump/status topic");
  }

  // send the timer info
  const size_t bufferSizeTimers = JSON_OBJECT_SIZE(5);
  DynamicJsonBuffer jsonBufferTimers(bufferSizeTimers);

  JsonObject& rootTimers = jsonBufferTimers.createObject();
  rootTimers["mode"]          = currentStatus.timers.mode;
  rootTimers["onMins"]        = currentStatus.timers.onMinutesSet;
  rootTimers["onRemainMins"]  = currentStatus.timers.onMinutesRemaining;
  rootTimers["offMins"]       = currentStatus.timers.offMinutesSet;
  rootTimers["offRemainMins"] = currentStatus.timers.offMinutesRemaining;

  char bufferTimers[512];
  rootTimers.printTo(bufferTimers, sizeof(bufferTimers));

  if (!mqtt_client.publish(heatpump_timers_topic, bufferTimers, true)) {
    mqtt_client.publish(heatpump_debug_topic, "failed to publish timer info to heatpump/status topic");
  }
}

void hpPacketDebug(byte* packet, unsigned int length, char* packetDirection) {
  if (_debugMode) {
    String message;
    for (int idx = 0; idx < length; idx++) {
      if (packet[idx] < 16) {
        message += "0"; // pad single hex digits with a 0
      }
      message += String(packet[idx], HEX) + " ";
    }

    const size_t bufferSize = JSON_OBJECT_SIZE(1);
    DynamicJsonBuffer jsonBuffer(bufferSize);

    JsonObject& root = jsonBuffer.createObject();

    root[packetDirection] = message;

    char buffer[512];
    root.printTo(buffer, sizeof(buffer));

    if (!mqtt_client.publish(heatpump_debug_topic, buffer)) {
      mqtt_client.publish(heatpump_debug_topic, "failed to publish to heatpump/debug topic");
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // Copy payload into message buffer
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';

  heatpumpSettings settings = hp.getSettings();
  if (strcmp(topic, heatpump_set_mode_topic) == 0) { // Handle Home Assistant Queues
    if (strcmp(message, "off") == 0) {
      settings.power = "off";
      hp.setSettings(settings);
      hp.update();
    } else {
      settings.power = "on";
      settings.mode = message;
      hp.setSettings(settings);
      hp.update();
    }
    return;
  }

  if (strcmp(topic, heatpump_set_temp_topic) == 0) { // Handle Home Assistant Queues
    settings.power = "on";
    settings.temperature = atoi(message);
    hp.setSettings(settings);
    hp.update();
    return;
  }

  if (strcmp(topic, heatpump_set_vane_topic) == 0) { // Handle Home Assistant Queues
    settings.vane = message;
    hp.setSettings(settings);
    hp.update();
    return;
  }

  if (strcmp(topic, heatpump_set_fan_topic) == 0) { // Handle Home Assistant Queues
    settings.power = "on";
    settings.fan = message;
    hp.setSettings(settings);
    hp.update();
    return;
  }
}

void mqttConnect() {
  // Loop until we're reconnected
#ifdef SERIALDEBUG
  Serial.println("Connecting to MQTT");
#endif
  //while (!mqtt_client.connected() && ((long) (millis() - 1000) < 0)) {
  // Attempt to connect
  if (mqtt_client.connect(client_id, mqtt_username, mqtt_password)) {
    mqtt_client.subscribe(heatpump_set_topic);
    mqtt_client.subscribe(heatpump_set_mode_topic);
    mqtt_client.subscribe(heatpump_set_temp_topic);
    mqtt_client.subscribe(heatpump_set_fan_topic);
    mqtt_client.subscribe(heatpump_set_vane_topic);
    mqtt_client.subscribe(heatpump_debug_set_topic);
#ifdef SERIALDEBUG
    Serial.println("CONNECTED TO MQTT");
#endif
  }
  //}
}

void loop() {
  if (!mqtt_client.connected()) {
    mqttConnect();
  } else {
    mqtt_client.loop();
  }

#ifndef SERIALDEBUG
  hp.sync();
  if (millis() > (lastTempSend + SEND_ROOM_TEMP_INTERVAL_MS)) { // only send the temperature every 60s
    hpStatusChanged(hp.getStatus());
    lastTempSend = millis();
  }
#endif

#ifdef SERIALDEBUG
  Serial.println(".");
  delay(500);
#endif

  //DNS
  dnsServer.processNextRequest();
  //HTTP
  server.handleClient();
  //#ifdef OTA
  ArduinoOTA.handle();
  //#endif
}





heatpumpSettings change_states(heatpumpSettings settings) {
  if (server.hasArg("CONNECT")) {
    hp.connect(&Serial);
  }
  else {
    bool update = false;
    if (server.hasArg("PWRCHK")) {
      settings.power = server.hasArg("POWER") ? "on" : "off";
      update = true;
    }
    if (server.hasArg("MODE")) {
      settings.mode = server.arg("MODE").c_str();
      update = true;
    }
    if (server.hasArg("TEMP")) {
      settings.temperature = server.arg("TEMP").toInt();
      update = true;
    }
    if (server.hasArg("FAN")) {
      settings.fan = server.arg("FAN").c_str();
      update = true;
    }
    if (server.hasArg("VANE")) {
      settings.vane = server.arg("VANE").c_str();
      update = true;
    }
    if (server.hasArg("WIDEVANE")) {
      settings.wideVane = server.arg("WIDEVANE").c_str();
      update = true;
    }
    if (update) {
      hp.setSettings(settings);
      hp.update();
    }
  }
  return settings;
}



void configureOTA() {
  //#ifdef OTA

  // Port defaults to 8266
  //ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    Serial.end();
  });


  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  });
  ArduinoOTA.setHostname(myHostname);
  //ArduinoOTA.setRebootOnSuccess(false);
  ArduinoOTA.begin();
#ifdef SERIALDEBUG
  Serial.println(" OTA ");
#endif
}

void setupAP() {
  digitalWrite(LED_BUILTIN, LOW);   // toggle LED

  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(softAP_ssid, softAP_password);
  delay(500); // Without delay I've seen the IP address blank
  digitalWrite(LED_BUILTIN, HIGH);   // toggle LED
}

void connectWifi() {
  /*
    typedef enum {
    WL_NO_SHIELD        = 255,   // for compatibility with WiFi Shield library
    WL_IDLE_STATUS      = 0,
    WL_NO_SSID_AVAIL    = 1,
    WL_SCAN_COMPLETED   = 2,
    WL_CONNECTED        = 3,
    WL_CONNECT_FAILED   = 4,
    WL_CONNECTION_LOST  = 5,
    WL_DISCONNECTED     = 6
    } wl_status_t;
  */

  WiFi.begin ( ssid, password );
  //if ( connRes == WL_CONNECTED ) {
  //} else {
  //}
#ifdef SERIALDEBUG
  Serial.print(" AP ");
#endif
}

void setupCore() {
#ifdef SERIALDEBUG
  Serial.println("Setup Core:");
#endif
  loadCredentials(); // Load WLAN credentials from network

  if (strlen(ssid) > 0) {
    connectWifi();
  }
  setupAP();

  // Setup MDNS responder
  int connRes = WiFi.waitForConnectResult();
  if (MDNS.begin(myHostname)) {
    /* Setup the DNS server redirecting all the domains to the apIP */
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    if ( connRes == WL_CONNECTED ) {
      dnsServer.start(DNS_PORT, "*", WiFi.localIP());
    } else {
      dnsServer.start(DNS_PORT, "*", apIP);
    }
    MDNS.addService("http", "tcp", 80);
#ifdef SERIALDEBUG
    Serial.print(" MDNS ");
#endif
  }
  startHTTP();
  configureOTA();
#ifdef SERIALDEBUG
  Serial.println("Setup Core Complete!");
#endif
}

void startHTTP() {
  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  server.on("/", handleRoot);
  server.on("/settings", handleSettings);
  server.on("/reboot", handleReboot);
  server.on("/settingssave", handleSettingsSave);
  server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.onNotFound ( handleNotFound );
  server.begin(); // Web server start
#ifdef SERIALDEBUG
  Serial.print(" HTTP ");
#endif
}
