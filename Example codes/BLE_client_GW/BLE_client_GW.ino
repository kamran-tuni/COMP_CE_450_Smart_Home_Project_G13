#include <WiFi.h>
#include <PubSubClient.h>
#include "BLEDevice.h"
#include "esp_wpa2.h"

/************************** WiFi and MQTT Configurations **************************/
#define EAP_IDENTITY "@tuni.fi"     // add tuni username
#define EAP_PASSWORD ""             // add tuni password
#define PUBLISH_TIMEOUT 5000        // publish in 5s
const char* g_ssid = "eduroam";     // eduroam SSID

const char* g_mqtt_server = "broker.mqttdashboard.com"; // MQTT Broker IP address
const int g_mqtt_port = 1883;                           // MQTT port number
const char* g_mqtt_clientid = "esp32-gateway-01";       // MQTT client ID
const char* g_uplink_topic = "v1/devices/uplink";       // MQTT topic to send the data
const char* g_downlink_topic = "v1/devices/downlink";   // MQTT topic to receive the data

WiFiClient espClient;
PubSubClient client(espClient);
char g_uplink_data[512];  // char array to store SENSOR_DATA
long lastMsg = 0;         // send ble nano data to mqtt server evert 5s
void setup_wifi();        // setup wifi connection
void reconnect();         // reconnect if connection to mqtt broker is lost
void setupWifiMQTTConnection();   // connect to wifi and mqtt broker
void publishBLENanoData();
void MQTTcallback(char* topic, byte* message, unsigned int length); // check the topic and payload recieved from MQTT server

/************************ Sense Node Serial Configurations ************************/
// Define the pins for the ESP32-Arduino Serial communication
#define RX_PIN 16
#define TX_PIN 17
#define CommSerial Serial2

const char* unlock_response = "\x55\x02\x0a\x5e";         // Door successfully unlocked
const char* senseNode_notification = "\x55\x02\xa0\x5e";  // Presence detected by senseNode

void checkSensNodeSerial();       // Check if data is recieved from SenseNode

/************************ BLE data configurations ************************/
#define THLDeviceID     0xAA
#define THLPacketSize   9

// sesnor data packet structure
typedef struct SENSOR_DATA {
  float temperature;
  float humidity;
  float light;
  float pressure; 
}__attribute__((aligned))sensor_data_t;

sensor_data_t g_sensor_data;

/************************ BLE connection configurations ************************/
// The remote service we wish to connect to.
static BLEUUID serviceUUID("cfc1cd76-cb65-11ed-afa1-0242ac120002");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("cfc1c916-cb65-11ed-afa1-0242ac120002");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

/************************ BLE functions and callbacks ************************/

void initBLE();   // scan for BLE server if required
bool connectToBLEServer();       // returns true if connection to server succeeds
bool isBLEServerConnected();     // returns true if server is connected
void startBLEScanifRequired();   // scan for BLE server if required

// BLE notification recieved from BLE Nano
static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    //Serial.println("onDisconnect");
  }
};

// Scan for BLE servers and find the first one that advertises the service we are looking for.
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  // Called for each advertising BLE server.
  void onResult(BLEAdvertisedDevice advertisedDevice) {
      //Serial.print("BLE Advertised Device found: ");
      //Serial.println(advertisedDevice.toString().c_str());
  
      // We have found a device, let us now see if it contains the service we are looking for.
      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        doScan = true;
      } 
    } 
}; 

/************************ ESP32 Gateway Setup and Loop ************************/

void setup() {
  // setup UART 2 for serial communication with senseNode
  CommSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  // setup Serial Communication for Debugging
  Serial.begin(115200);
  //Serial.println("Starting Arduino BLE Client application...");

  // setup wifi connection and connection to mqtt server
  setup_wifi();
  client.setServer(g_mqtt_server, g_mqtt_port);
  client.setCallback(MQTTcallback);
  
  // initialize BLE connection parameters
  initBLE();
}

void loop() {
  // Wifi stuff
  setupWifiMQTTConnection();
  publishBLENanoData();
  
  // BLE stuff
  if(isBLEServerConnected()){     // checks if BLE server is advertising, if yes then attempt to connect to server
    //Serial.println("We are now connected to the BLE Server.");
  }
  startBLEScanifRequired();  
  delay(2000);                  // Delay a second between loops.

  // Serial stuff
  checkSensNodeSerial();
}



/************************ Function Definitions ************************/
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    if(length == THLPacketSize && pData[0] == THLDeviceID){
      g_sensor_data.temperature = (((uint16_t)pData[1] << 8) | pData[2])/100.0;
      g_sensor_data.humidity = (((uint16_t)pData[3] << 8) | pData[4])/100.0;
      g_sensor_data.light = (((uint16_t)pData[5] << 8) | pData[6])/100.0;
      g_sensor_data.pressure = (((uint16_t)pData[7] << 8) | pData[8])/100.0;
//      //Serial.print("Temperature: ");
//      //Serial.println(g_sensor_data.temperature);
//      //Serial.print("Humidity: ");
//      //Serial.println(g_sensor_data.humidity);
//      //Serial.print("Light: ");
//      //Serial.println(g_sensor_data.light);
//      //Serial.print("Pressure: ");
//      //Serial.println(g_sensor_data.pressure);
    }
}

void initBLE(){
  // Initializa BLE connection parameters
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

bool connectToBLEServer() {
    //Serial.print("Forming a connection to ");
    //Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    //Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    //Serial.println(" - Connected to server");
    pClient->setMTU(517); //set client to request maximum MTU from server (default is 23 otherwise)
  
    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      //Serial.print("Failed to find our service UUID: ");
      //Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    //Serial.println(" - Found our service");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      //Serial.print("Failed to find our characteristic UUID: ");
      //Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    //Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      //Serial.print("The characteristic value was: ");
      //Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}

bool isBLEServerConnected(){
  if (doConnect == true)
    return connectToBLEServer()? true:false;
  return false;
}

void startBLEScanifRequired(){
  if(!connected && doScan)
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
}

void checkSensNodeSerial(){
  String msg = "";
  while(CommSerial.available()) {
    // todo uncomment this
    msg += char(CommSerial.read());
    // msg = CommSerial.readString();
    delay(50);
    // //Serial.println(msg);
  }

  // check if lock command is recieved
  if (msg == unlock_response){
    //Serial.println("Door unlocked");
  }

  // check if senseNode notification is recieved
  if (msg == senseNode_notification){
    //Serial.println("Someone is at the door!!!");
    // todo remove
    byte message[] = {0x55, 0x01, 0x0A, 0x5E};
    CommSerial.write(message, sizeof(message));
  }
}

void setup_wifi(){
  byte error = 0;
  //Serial.println("Connecting to: ");
  //Serial.println(g_ssid);
  WiFi.disconnect(true);  //disconnect from wifi to set new wifi connection
  error += esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
  error += esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
  error += esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD)); //Following times, it ran fine with just this line (connects very fast).
  if (error != 0) {
    //Serial.println("Error setting WPA properties.");
  }
  WiFi.enableSTA(true);
  
  // esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT();
  if (esp_wifi_sta_wpa2_ent_enable() != ESP_OK) {
    //Serial.println("WPA2 Settings not OK");
  }
  
  WiFi.begin(g_ssid);                 //  connect to Eduroam function
  WiFi.setHostname("RandomHostname"); //set Hostname for your device - not neccesary
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
  }
  //Serial.println("");
  //Serial.println("WiFi connected");
  //Serial.println("IP address set: ");
  //Serial.println(WiFi.localIP());     // print LAN IP
}

void MQTTcallback(char* topic, byte* message, unsigned int length) 
{
  int access_reply;
  const char* g_downlink_data = reinterpret_cast<const char*>(message);
  //Serial.print("Message arrived on topic: ");
  //Serial.println(topic);
  //Serial.print("Message: ");
  //Serial.println(g_downlink_data);
  sscanf(g_downlink_data,"{\"bn\": \"ThingsBoardIoT/\",\"e\": [{\"n\": \"access_reply\",\"vb\": %d}]}",access_reply);
  // If a message is received on the topic, you check if the message is either "true" or "false". 
  // Changes the output state according to the message
  if (topic == g_downlink_topic) 
  {
    if(access_reply == 1) 
    { 
      //Serial.println("on");
      // digitalWrite(ledPin, HIGH);
    }
    else if(access_reply == 0) 
    {
      //Serial.println("off");
      // digitalWrite(ledPin, LOW);
    }
  }
}

void reconnect() 
{
  // Loop until we're reconnected
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(g_mqtt_clientid)) {
      //Serial.println("connected");
      // Subscribe
      client.subscribe(g_downlink_topic);
    } 
    else {
      //Serial.print("failed, rc=");
      //Serial.print(client.state());
      //Serial.println("try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setupWifiMQTTConnection(){
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(g_ssid);
  }
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
  }
  
  if(!client.connected()) {
    reconnect();
  }
  client.loop();
}

void publishBLENanoData(){
  long now = millis();
  if (now - lastMsg > PUBLISH_TIMEOUT)
  {
    lastMsg = now;
    sprintf(g_uplink_data , "{\"bn\": \"SenseNode/\",\"e\":[{\"n\": \"temperature\",\"u\": \"Cel\",\"v\": %f},{\"n\": \"humidity\",\"u\": \"%\",\"v\": %f},{\"n\": \"light\",\"u\":\"lx\",\"v\": %f},{ \"n\": \"pressure\",\"u\": \"Pa\",\"v\": %f}]}", g_sensor_data.temperature, g_sensor_data.humidity, g_sensor_data.light, g_sensor_data.pressure);
    // sprintf(g_uplink_data , "{\"bn\": \"SenseNode/\",\"e\":[{\"n\": \"temperature\",\"u\": \"Cel\",\"v\": %f},{\"n\": \"humidity\",\"u\": \"%%\",\"v\": %f},{\"n\": \"light\",\"u\":\"lx\",\"v\": %f},{ \"n\": \"pressure\",\"u\": \"Pa\",\"v\": %f}]}", 23.4, 45.0, 90, 23.6);
    client.publish(g_uplink_topic, g_uplink_data);
    //Serial.println("Published Successful");
    //Serial.println(g_uplink_data);
  }
}
