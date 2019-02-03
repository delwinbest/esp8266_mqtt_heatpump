/** Handle root or redirect to captive portal */
void handleRoot() {
  heatpumpSettings settings = hp.getSettings();
  settings = change_states(settings);
  String toSend = html;
  toSend.replace("_RATE_", "60");
  toSend.replace("_ROOMTEMP_", String(hp.getRoomTemperature()));
  toSend.replace("_POWER_",settings.power == "on" ? "checked" : "");
  if(settings.mode == "HEAT") {
    toSend.replace("_MODE_H_","checked");
  }
  else if(settings.mode == "dry") {
    toSend.replace("_MODE_D_","checked");
  }
  else if(settings.mode == "cool") {
    toSend.replace("_MODE_C_","checked");
  }
  else if(settings.mode == "fan_only") {
    toSend.replace("_MODE_F_","checked");
  }
  else if(settings.mode == "auto") {
    toSend.replace("_MODE_A_","checked");
  }
  if(settings.fan == "auto") {
    toSend.replace("_FAN_A_","checked");
  }
  else if(settings.fan == "low") {
    toSend.replace("_FAN_Q_","checked");
  }
  else if(settings.fan == "1") {
    toSend.replace("_FAN_1_","checked");
  }
  else if(settings.fan == "medium") {
    toSend.replace("_FAN_2_","checked");
  }
  else if(settings.fan == "3") {
    toSend.replace("_FAN_3_","checked");
  }
  else if(settings.fan == "high") {
    toSend.replace("_FAN_4_","checked");
  }
 
  toSend.replace("_VANE_V_",settings.vane);
  if(settings.vane == "AUTO") {
    toSend.replace("_VANE_C_","rotate0");
    toSend.replace("_VANE_T_","AUTO");
  }
  else if(settings.vane == "1") {
    toSend.replace("_VANE_C_","rotate0");
    toSend.replace("_VANE_T_","&#10143;");
  }
  else if(settings.vane == "2") {
    toSend.replace("_VANE_C_","rotate22");
    toSend.replace("_VANE_T_","&#10143;");
  }
  else if(settings.vane == "3") {
    toSend.replace("_VANE_C_","rotate45");
    toSend.replace("_VANE_T_","&#10143;");
  }
  else if(settings.vane == "4") {
    toSend.replace("_VANE_C_","rotate67");
    toSend.replace("_VANE_T_","&#10143;");
  }
  else if(settings.vane == "5") {
    toSend.replace("_VANE_C_","rotate90");
    toSend.replace("_VANE_T_","&#10143;");
  }
  else if(settings.vane == "SWING") {
    toSend.replace("_VANE_C_","rotateV");
    toSend.replace("_VANE_T_","&#10143;");
  }
  toSend.replace("_WIDEVANE_V_",settings.wideVane);
  if(settings.wideVane == "<<") {
    toSend.replace("_WIDEVANE_C_","rotate157");
    toSend.replace("_WIDEVANE_T_","&#10143;");
  }
  else if(settings.wideVane == "<") {
    toSend.replace("_WIDEVANE_C_","rotate124");
    toSend.replace("_WIDEVANE_T_","&#10143;");
  }
  else if(settings.wideVane == "|") {
    toSend.replace("_WIDEVANE_C_","rotate90");
    toSend.replace("_WIDEVANE_T_","&#10143;");
  }
  else if(settings.wideVane == ">") {
    toSend.replace("_WIDEVANE_C_","rotate57");
    toSend.replace("_WIDEVANE_T_","&#10143;");
  }
  else if(settings.wideVane == ">>") {
    toSend.replace("_WIDEVANE_C_","rotate22");
    toSend.replace("_WIDEVANE_T_","&#10143;");
  }
  else if(settings.wideVane == "<>") {
    toSend.replace("_WIDEVANE_C_","");
    toSend.replace("_WIDEVANE_T_","<div class='rotate124'>&#10143;</div>&nbsp;<div class='rotate57'>&#10143;</div>");
  }
  else if(settings.wideVane == "SWING") {
    toSend.replace("_WIDEVANE_C_","rotateH");
    toSend.replace("_WIDEVANE_T_","&#10143;");
  }
  toSend.replace("_TEMP_", String(hp.getTemperature()));

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  //server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.


  server.send(200, "text/html", toSend);

  server.sendContent("");
  server.client().stop(); // Stop is needed because we sent no content length

  delay(100);
}


/** Wifi config page handler */
void handleSettings() {
  uint32_t realSize = ESP.getFlashChipRealSize();
  uint32_t ideSize = ESP.getFlashChipSize();
  FlashMode_t ideMode = ESP.getFlashChipMode();

  
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent(
    "<html><head></head><body>"
    "<h1>Wifi config</h1>"
  );
  if (server.client().localIP() == apIP) {
    server.sendContent(String("<p>You are connected through the soft AP: ") + softAP_ssid + "</p>");
  } else {
    server.sendContent(String("<p>You are connected through the wifi network: ") + ssid + "</p>");
  }
  
  server.sendContent(String() + "<p>Flash real size: " + realSize + "</p>");
  server.sendContent(String() + "<p>Flash ide size: " + ideSize + "</p>");
  server.sendContent(String() + "<p>Flash ide mode: " + ideMode + "</p>");


  server.sendContent(
    "\r\n<br />"
    "<table><tr><th align='left'>SoftAP config</th></tr>"
  );
  server.sendContent(String() + "<tr><td>SSID " + String(softAP_ssid) + "</td></tr>");
  server.sendContent(String() + "<tr><td>IP " + toStringIp(WiFi.softAPIP()) + "</td></tr>");
  server.sendContent(
    "</table>"
    "\r\n<br />"
    "<table><tr><th align='left'>WLAN config</th></tr>"
  );
  server.sendContent(String() + "<tr><td>SSID " + String(ssid) + "</td></tr>");
  server.sendContent(String() + "<tr><td>IP " + toStringIp(WiFi.localIP()) + "</td></tr>");
  server.sendContent(
    "</table>"
    "\r\n<br />"
    "<table><tr><th align='left'>WLAN list (refresh if any missing)</th></tr>"
  );
  int n = WiFi.scanNetworks();
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      server.sendContent(String() + "\r\n<tr><td>SSID " + WiFi.SSID(i) + String((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : " *") + " (" + WiFi.RSSI(i) + ")</td></tr>");
    }
  } else {
    server.sendContent(String() + "<tr><td>No WLAN found</td></tr>");
  }
  server.sendContent(
    "</table>"
    "\r\n<br /><form method='POST' action='settingssave'><h4>Connect to network:</h4>"
    "<input type='text' placeholder='network' name='n'/>"
    "<br /><input type='password' placeholder='password' name='p'/>"
    "<br /><input type='submit' value='Connect/Disconnect'/></form>"
    "<p><a href='/reboot'>Reboot</a></p>"
    "<p>You may want to <a href='/'>return to the home page</a>.</p>"
    "</body></html>"
  );
  server.sendContent("");
  server.client().stop(); // Stop is needed because we sent no content length
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void handleSettingsSave() {
  server.arg("n").toCharArray(ssid, sizeof(ssid) - 1);
  server.arg("p").toCharArray(password, sizeof(password) - 1);
  server.sendHeader("Location", "wifi", true);
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(302, "text/plain", "");    // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.client().stop(); // Stop is needed because we sent no content length
  saveCredentials();
  //connect = strlen(ssid) > 0; // Request WLAN connect with new credentials if there is a SSID
  ESP.restart();
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void handleReboot() {
  ESP.restart();
}

void handleNotFound() {
  //if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
  //  return;
  //}
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(404, "text/plain", message);
}
