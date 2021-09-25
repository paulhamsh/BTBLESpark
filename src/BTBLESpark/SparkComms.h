#ifndef SparkComms_h
#define SparkComms_h


#include "BLEDevice.h"
#include "BLEUtils.h"


#include "BluetoothSerial.h"
#include "RingBuffer.h"

#define BLE_BUFSIZE 5000

// Bluetooth vars
#define  SPARK_NAME  "Spark 40 Audio"

void start_ser();
void start_bt();
void connect_to_spark();

bool ser_available();
bool bt_available();

uint8_t ser_read();
uint8_t bt_read();

void ser_write(byte *buf, int len);
void bt_write(byte *buf, int len);

int ble_getRSSI();

// bluetooth communications

BluetoothSerial *ser;
boolean isBTConnected;  

// BLE 
BLEAdvertisedDevice device;
BLEClient *pClient;
BLERemoteService *pService;
BLERemoteCharacteristic *pSender;
BLERemoteCharacteristic *pReceiver;

RingBuffer ble_in;

void notifyCB(BLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

#endif
