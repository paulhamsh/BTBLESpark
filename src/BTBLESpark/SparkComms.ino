#include "Spark.h"
#include "SparkComms.h"
#include "BluetoothSerial.h"

// Updated with changes from David Thompson github: https://github.com/happyhappysundays/SparkBoxHeltec - most of the BLE callbacks

//BLE callbacks


// Callbacks for client events
class ClientCallbacks : public BLEClientCallbacks {
  void onConnect(BLEClient* pClient) {
    isBTConnected = true;
  }

  void onDisconnect(BLEClient* pClient) {
    isBTConnected = false;
  }
};


// Callbacks for advertisment events
class AdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if(advertisedDevice.isAdvertisingService(BLEUUID("ffc0")))
    {
      BLEDevice::getScan()->stop();
    }
  }
};



// Callback function to write to ring buffer
void notifyCB(BLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify){
  int i;

  Serial.print("INCOMING: ");
  
  for (i = 0; i < length; i++) {
    ble_in.add(pData[i]);
    Serial.print(pData[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  ble_in.commit();
}




// Non callback functions
void start_ser() {
  uint8_t b;
  
  ser = new BluetoothSerial(); 
 
  if (!ser->begin("Spark 40 Audio")) {
    DEBUG("Bluetooth Serial init fail");
    while (true);
  }  
  while (ser->available())
    b = ser->read(); 
  DEBUG("Serial BT started");  
}

void start_bt() {
  BLEDevice::init("");
}

void connect_to_spark() {
  uint8_t b;
  int i;
  bool connected;

  connected = false;
  while (!connected) {
    BLEDevice::init("");
  
    BLEScan *pScan = BLEDevice::getScan();
      
    // Create a callback that gets called when advertisers are found
    pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
    BLEScanResults results = pScan->start(4);

    BLEUUID serviceUuid("ffc0");               // service ffc0 for Spark

    for(i = 0; i < results.getCount() && !connected; i++) {
      device = results.getDevice(i);
    
      if (device.isAdvertisingService(serviceUuid)) {
        pClient = BLEDevice::createClient();
        pClient->setClientCallbacks(new ClientCallbacks());
        
        if(pClient->connect(&device)) {
          connected = true;
          DEBUG("BLE Connected");
        }
      }
    }

    // Get the services
  
    if (connected) {
      pService = pClient->getService(serviceUuid);
                
      if (pService != nullptr) {
        pSender   = pService->getCharacteristic("ffc1");
        pReceiver = pService->getCharacteristic("ffc2");
        if (pReceiver && pReceiver->canNotify()) {
           pReceiver->registerForNotify(notifyCB); 
        }
      }
            
      if (!connected) {
        DEBUG ("Not connected - trying again");
        delay(200); // Idea from https://github.com/espressif/esp-idf/issues/5105
      }
    }
  }
}



bool ser_available() {
  return ser->available();
}

bool bt_available() {
  return (!(ble_in.is_empty()));
}

uint8_t ser_read() {
  return ser->read();
}

uint8_t bt_read() {
  uint8_t b;
  ble_in.get(&b);
  return b;
}

void ser_write(byte *buf, int len) {
  ser->write(buf, len);
}

void bt_write(byte *buf, int len) {
  pSender->writeValue(buf, len, false);
}

int ble_getRSSI() {
  return pClient->getRssi();
}
