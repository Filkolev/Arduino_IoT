// WiFi access point details
#define WLAN_SSID "SoftUni"
#define WLAN_PASS ""

// Adafruit IO access details
#define AIO_KEY "4c48fa6fbd0d471f99ed1fa786500128"

// Requested temperature range
#define REQ_TEMPERATURE_MIN 0
#define REQ_TEMPERATURE_MAX 31
#define REQ_TEMPERATURE_DEFAULT 25

// Message types definitions
#define REGISTER 0
#define CONFIRM 1
#define READING 2
#define INFO 3
#define COMMAND 4

// Communication protocol
#define BAUD_RATE 9600
#define MAX_CONNECTED_DEVICES 8
#define MSG_BUFFER_LEN 128
#define MAX_DATA_LEN 25
#define START_SYMBOL '%'
#define END_SYMBOL '%'

// Serial communication
#define TX 2
#define RX 3
