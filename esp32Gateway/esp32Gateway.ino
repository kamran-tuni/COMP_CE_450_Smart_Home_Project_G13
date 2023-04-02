#include "BLEDevice.h"
#define temperatureCelsius
#define BLE_server "BLENano_Server"
  
static BLEUUID dhtServiceUUID("cfc1cd76-cb65-11ed-afa1-0242ac120002");
static BLEUUID temperatureCharacteristicUUID("cfc1c916-cb65-11ed-afa1-0242ac120002");
   
static boolean doConnect = false;
static boolean connected = false;

static BLEAddress *pServerAddress;
static BLERemoteCharacteristic* temperatureCharacteristic;
    
const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};

bool connectToServer(BLEAddress pAddress) {
  BLEClient* pClient = BLEDevice::createClient();
  
  pClient->connect(pAddress);
  Serial.println(" - Connected successfully to server");
  
  BLERemoteService* pRemoteService = pClient->getService(dhtServiceUUID);
  
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(dhtServiceUUID.toString().c_str());
    return (false);
  }
    
  temperatureCharacteristic = pRemoteService->getCharacteristic(temperatureCharacteristicUUID);
  if (temperatureCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID");
    return false;
  }
  Serial.println(" Characteristics Found!");
  
  temperatureCharacteristic->registerForNotify(temperatureNotifyCallback);
}
    
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) 
  {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(dhtServiceUUID)) { 
      advertisedDevice.getScan()->stop();
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
      Serial.println("Device found. Connecting!");
    }
  }
};
    
static void temperatureNotifyCallback(BLERemoteCharacteristic*pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify)
{
  Serial.print("Temperature: ");
  Serial.print((char*)pData);  
}
    
void setup() 
{
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");
  
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30,false);
}

void loop() {
  if (doConnect == true) {
    if (connectToServer(*pServerAddress)) {
      Serial.println("We are now connected to the BLE Server.");
      temperatureCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      connected = true;
    } 
    else {
      Serial.println("Failed to connect to server!");
    }
      doConnect = false;
  }
  
  delay(1000);
}
