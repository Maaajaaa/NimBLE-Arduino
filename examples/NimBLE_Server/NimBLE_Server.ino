
/** NimBLE_Server Demo:
 *
 *  Demonstrates many of the available features of the NimBLE server library.
 *  
 *  Created: on March 22 2020
 *      Author: H2zero
 * 
*/

#include <NimBLEDevice.h>
#include <NimBLE2904.h>
#include <NimBLE2902.h>


/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */  
class ServerCallbacks: public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer) {
    Serial.println("Client connected");
    Serial.println("Multi-connect support: start advertising");
    NimBLEDevice::startAdvertising();
  };
 /** Alternative onConnect() method to extract details of the connection. 
  *  See: src/ble_gap.h for the details of the ble_gap_conn_desc struct.
  */  
  void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
    Serial.print("Client address: ");
    Serial.println(NimBLEAddress(desc->peer_ota_addr).toString().c_str());
  /** We can use the connection handle here to ask for different connection parameters.
   *  Args: connection handle, min connection interval, max connection interval
   *  latency, supervision timeout.
   */
    pServer->updateConnParams(desc->conn_handle, 24, 48, 0, 800);
  };
  void onDisconnect(NimBLEServer* pServer) {
    Serial.println("Client disconnected - starting advertising");
    NimBLEDevice::startAdvertising();
  };
    
/********************* Security handled here **********************
****** Note: these are the same return values as defaults ********/
  uint32_t onPassKeyRequest(){
    Serial.println("Server Passkey Request");
    /** This should return a random 6 digit number for security 
     *  or make your own static passkey as done here.
     */
    return 123456; 
  };

  bool onConfirmPIN(uint32_t pass_key){
    Serial.print("The passkey YES/NO number: ");Serial.println(pass_key);
    /** Return false if passkeys don't match. */
    return true; 
  };

  void onAuthenticationComplete(ble_gap_conn_desc* desc){
    /** Check that encryption was successful, if not we disconnect the client */  
    if(!desc->sec_state.encrypted) {
      // createServer returns the current server reference unless one is not already created
      NimBLEDevice::createServer()->disconnect(desc->conn_handle);
      Serial.println("Encrypt connection failed - disconnecting client");
      return;
    }
    Serial.println("Starting BLE work!");
  }
};


class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
  void onRead(NimBLECharacteristic* pCharacteristic){
    Serial.print(pCharacteristic->getUUID().toString().c_str());
    Serial.print(": onRead(), value: ");
    Serial.println(pCharacteristic->getValue().c_str());
  };
  
  void onWrite(NimBLECharacteristic* pCharacteristic){
    Serial.print(pCharacteristic->getUUID().toString().c_str());
    Serial.print(": onWrite(), value: ");
    Serial.println(pCharacteristic->getValue().c_str());
  };
  
  void onNotify(NimBLECharacteristic* pCharacteristic){
    Serial.println("Sending notification to client");
  };
  
  
  /** The status returned in s is defined in NimBLECharacteristic.h.
   *  The value returned in code is the NimBLE host return code.
   */
  void onStatus(NimBLECharacteristic* pCharacteristic, Status s, int code){
    Serial.print("Notification/Indication status code: ");
    Serial.print(s);
    Serial.print(", return code: ");
    Serial.println(NimBLEUtils::returnCodeToString(code));
  };
};
    
    
class DescriptorCallbacks : public NimBLEDescriptorCallbacks
{
  void onWrite(NimBLEDescriptor* pDescriptor)
  {
    if(pDescriptor->getUUID().equals(NimBLEUUID("2902"))){
      /** Cast to NimBLE2902 to use the class specific functions. **/
      NimBLE2902* p2902 = (NimBLE2902*)pDescriptor;
	  if(p2902->getNotifications())
	  {
	    Serial.println("Client Subscribed to notfications");
	  }
	  else
	  {
	    Serial.println("Client Unubscribed to notfications");
	  }
    } 
    else {
      std::string dscVal((char*)pDescriptor->getValue(), pDescriptor->getLength());
      Serial.print("Descriptor witten value:");
      Serial.println(dscVal.c_str());
    }
  };
  
  void onRead(NimBLEDescriptor* pDescriptor)
  {
    Serial.print(pDescriptor->getUUID().toString().c_str());
    Serial.println(" Descriptor read");
  };
};


/** Define callback instances globally to use for multiple Charateristics \ Descriptors **/ 
static DescriptorCallbacks dscCallbacks;
static CharacteristicCallbacks chrCallbacks;

static NimBLEServer* pServer;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting NimBLE Server");

  NimBLEDevice::init("NimBLE-Arduino"); // sets device name
  
  /** Uncomment this if you encounter watchdog timer resets after pairing with the device.
   *  Known bug in the NimBLE host that still needs to be resolved.
   */
  //ble_store_clear();
  
  /** Set the IO capabilities of the device, each option will trigger a different pairing method.
   *  BLE_HS_IO_DISPLAY_ONLY    - Passkey pairing
   *  BLE_HS_IO_DISPLAY_YESNO   - Numeric comparison pairing
   *  BLE_HS_IO_NO_INPUT_OUTPUT - DEFAULT setting - just works pairing
   */
  //NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); // use passkey
  //NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO); //use numeric comparison
  
  /** 2 different ways to set security - both calls achieve the same result.
   *  no bonding, no man in the middle protection, secure connections. 
   *  These are the default values, only shown here for demonstration.   
   */ 
  //NimBLEDevice::setSecuityAuth(false, false, true); 
  NimBLEDevice::setSecuityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);
  
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
    
  NimBLEService* pDeadService = pServer->createService("DEAD");
  NimBLECharacteristic* pBeefCharacteristic = pDeadService->createCharacteristic(
                                               "BEEF",
                                               NIMBLE_PROPERTY::READ |
                                               NIMBLE_PROPERTY::WRITE |
                          /** Require a secure connection for read and write access **/
                                               NIMBLE_PROPERTY::READ_ENC |  // only allow reading if paired / encrypted
                                               NIMBLE_PROPERTY::WRITE_ENC   // only allow writing if paired / encrypted
                                              );
  
  pBeefCharacteristic->setValue("Burger");
  pBeefCharacteristic->setCallbacks(&chrCallbacks);
    
  /** 2902 and 2904 descriptors are a special case, when createDescriptor is called with
   *  either of those uuid's it will create the associated class with the correct properties
   *  and sizes. However we must cast the returned reference to the correct type as the method
   *  only returns a pointer to the base NimBLEDescriptor class.
   */
  NimBLE2904* pBeef2904 = (NimBLE2904*)pBeefCharacteristic->createDescriptor("2904"); 
  pBeef2904->setFormat(NimBLE2904::FORMAT_UTF8);
  pBeef2904->setCallbacks(&dscCallbacks);
  

  NimBLEService* pBaadService = pServer->createService("BAAD");
  NimBLECharacteristic* pFoodCharacteristic = pBaadService->createCharacteristic(
                                               "F00D",
                                               NIMBLE_PROPERTY::READ |
                                               NIMBLE_PROPERTY::WRITE |
                                               NIMBLE_PROPERTY::NOTIFY
                                              );

  pFoodCharacteristic->setValue("Fries");
  pFoodCharacteristic->setCallbacks(&chrCallbacks);
  
  /** Custom descriptor: Arguments are UUID, Properties, max length in bytes of the value **/
  NimBLEDescriptor* pC01Ddsc = pFoodCharacteristic->createDescriptor(
                                               "C01D",
                                               NIMBLE_PROPERTY::READ | 
                                               NIMBLE_PROPERTY::WRITE|
                                               NIMBLE_PROPERTY::WRITE_ENC, // only allow writing if paired / encrypted
                                               20
                                              );
  pC01Ddsc->setValue("No tip!");
  pC01Ddsc->setCallbacks(&dscCallbacks);
  
  /** Note a 2902 descriptor does NOT need to be created as any chactateristic with 
   *  notification or indication properties will have one created autmatically.
   *  Manually creating it is only useful if you wish to handle callback functions
   *  as shown here. Otherwise this can be removed without loss of functionality.
   */
  NimBLE2902* pFood2902 = (NimBLE2902*)pFoodCharacteristic->createDescriptor("2902"); 
  pFood2902->setCallbacks(&dscCallbacks);
  
  /** Start the services when finished creating all Characteristics and Descriptors **/  
  pDeadService->start();
  pBaadService->start();
  
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  /** Add the services to the advertisment data **/
  pAdvertising->addServiceUUID(pDeadService->getUUID());
  pAdvertising->addServiceUUID(pBaadService->getUUID());
  /** If your device is battery powered you may consider setting scan response
   *  to false as it will extend battery life at the expense of less data sent.
   */
  pAdvertising->setScanResponse(true);
  pAdvertising->start();
  
  Serial.println("Advertising Started");
}


void loop() {
  /** Do your thing here, this just spams notifications to all connected clients */
  if(pServer->getConnectedCount()) {
    NimBLEService* pSvc = pServer->getServiceByUUID("BAAD");
    if(pSvc) {
      NimBLECharacteristic* pChr = pSvc->getCharacteristic("F00D");
      if(pChr) {
        pChr->notify(true);
      }
    }
  }
        
  delay(2000);
}