
#define OTA
//#define SERIALDEBUG
//#define CLIENTID "hvac_6789"
#define CLIENTID "hvac_upstairs"

// Hostname
const char *myHostname = CLIENTID;

/* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
char ssid[32] = "";
char password[32] = "";
const unsigned int UPDATE_INTERVAL[sizeof(uint)] = { 30000 }; // default update interval

/* Set these to your desired softAP credentials. They are not configurable at runtime */
const char *softAP_ssid = CLIENTID;
const char *softAP_password = "12345678";

// DNS server
const byte DNS_PORT = 53;

// mqtt server settings
const char *mqtt_server   = "192.168.10.4";
const int mqtt_port       = 1883;
const char *mqtt_username = "hvac";
const char *mqtt_password = "hvacPassword";

// mqtt client settings
// Note PubSubClient.h has a MQTT_MAX_PACKET_SIZE of 128 defined, so either raise it to 256 or use short topics
const char* client_id                   = CLIENTID;
const char* heatpump_set_topic          = CLIENTID"/set";
const char* heatpump_set_mode_topic     = CLIENTID"/set/mode";
const char* heatpump_set_temp_topic     = CLIENTID"/set/temp";
const char* heatpump_set_fan_topic      = CLIENTID"/set/fan";
const char* heatpump_set_vane_topic      = CLIENTID"/set/vane";

const char* heatpump_status_topic       = CLIENTID"/status";
const char* heatpump_timers_topic       = CLIENTID"/timers";

const char* heatpump_debug_topic        = CLIENTID"/debug";
const char* heatpump_debug_set_topic    = CLIENTID"/debug/set";

// sketch settings
const unsigned int SEND_ROOM_TEMP_INTERVAL_MS = 30000;
