// AP_Lab6.c
// Runs on either MSP432 or TM4C123
// see GPIO.c file for hardware connections 

// Daniel Valvano and Jonathan Valvano
// November 20, 2016
// CC2650 booster or CC2650 LaunchPad, CC2650 needs to be running SimpleNP 2.2 (POWERSAVE)

#include <stdint.h>
#include "../inc/UART0.h"
#include "../inc/UART1.h"
#include "../inc/AP.h"
#include "AP_Lab6.h"
//**debug macros**APDEBUG defined in AP.h********
#ifdef APDEBUG
#define OutString(STRING) UART0_OutString(STRING)
#define OutUHex(NUM) UART0_OutUHex(NUM)
#define OutUHex2(NUM) UART0_OutUHex2(NUM)
#define OutChar(N) UART0_OutChar(N)
#else
#define OutString(STRING)
#define OutUHex(NUM)
#define OutUHex2(NUM)
#define OutChar(N)
#endif

//****links into AP.c**************
extern const uint32_t RECVSIZE;
extern uint8_t RecvBuf[];
typedef struct characteristics{
  uint16_t theHandle;          // each object has an ID
  uint16_t size;               // number of bytes in user data (1,2,4,8)
  uint8_t *pt;                 // pointer to user data, stored little endian
  void (*callBackRead)(void);  // action if SNP Characteristic Read Indication
  void (*callBackWrite)(void); // action if SNP Characteristic Write Indication
}characteristic_t;
extern const uint32_t MAXCHARACTERISTICS;
extern uint32_t CharacteristicCount;
extern characteristic_t CharacteristicList[];
typedef struct NotifyCharacteristics{
  uint16_t uuid;               // user defined 
  uint16_t theHandle;          // each object has an ID (used to notify)
  uint16_t CCCDhandle;         // generated/assigned by SNP
  uint16_t CCCDvalue;          // sent by phone to this object
  uint16_t size;               // number of bytes in user data (1,2,4,8)
  uint8_t *pt;                 // pointer to user data array, stored little endian
  void (*callBackCCCD)(void);  // action if SNP CCCD Updated Indication
}NotifyCharacteristic_t;
extern const uint32_t NOTIFYMAXCHARACTERISTICS;
extern uint32_t NotifyCharacteristicCount;
extern NotifyCharacteristic_t NotifyCharacteristicList[];
//**************Lab 6 routines*******************
// **********SetFCS**************
// helper function, add check byte to message
// assumes every byte in the message has been set except the FCS
// used the length field, assumes less than 256 bytes
// FCS = 8-bit EOR(all bytes except SOF and the FCS itself)
// Inputs: pointer to message
//         stores the FCS into message itself
// Outputs: none
void SetFCS(uint8_t *msg){
//****You implement this function as part of Lab 6*****
  uint8_t length, fcs;
  fcs = 0;
  msg++;  // point to the length of the message
  length = *msg; // set length
  fcs = length; 
  msg += 2; // point to first command 
  fcs ^= *msg; // exclusive or fcs with first command value
  msg++; // point to second command
  fcs ^= *msg; // exclusive or fcs with second command value
  for(int i = 0; i < length; i++){
    msg++;
    fcs ^= *msg;  // exclusive or fcs with all values in payload
  }
  msg++;
  *msg = fcs; // set last byte in msg equal to fcs
}
//*************BuildGetStatusMsg**************
// Create a Get Status message, used in Lab 6
// Inputs pointer to empty buffer of at least 6 bytes
// Output none
// build the necessary NPI message that will Get Status
void BuildGetStatusMsg(uint8_t *msg){
// hint: see NPI_GetStatus in AP.c
//****You implement this function as part of Lab 6*****
  *(msg++) = SOF;
  *(msg++) = 0;    // set length to 0
  *(msg++) = 0x00;
  *(msg++) = 0x55; // set first command
  *(msg++) = 0x06; // set second command
  *msg = 0x53; // set fcs
}
//*************Lab6_GetStatus**************
// Get status of connection, used in Lab 6
// Input:  none
// Output: status 0xAABBCCDD
// AA is GAPRole Status
// BB is Advertising Status
// CC is ATT Status
// DD is ATT method in progress
uint32_t Lab6_GetStatus(void){volatile int r; uint8_t sendMsg[8];
  OutString("\n\rGet Status");
  BuildGetStatusMsg(sendMsg);
  r = AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  return (RecvBuf[4]<<24)+(RecvBuf[5]<<16)+(RecvBuf[6]<<8)+(RecvBuf[7]);
}

//*************BuildGetVersionMsg**************
// Create a Get Version message, used in Lab 6
// Inputs pointer to empty buffer of at least 6 bytes
// Output none
// build the necessary NPI message that will Get Status
void BuildGetVersionMsg(uint8_t *msg){
// hint: see NPI_GetVersion in AP.c
//****You implement this function as part of Lab 6*****
  *(msg++) = SOF;
  *(msg++) = 0;    // set length to 0
  *(msg++) = 0x00;
  *(msg++) = 0x35; // set first command
  *(msg++) = 0x03; // set second command
  *msg = 0x36; // set fcs
}
//*************Lab6_GetVersion**************
// Get version of the SNP application running on the CC2650, used in Lab 6
// Input:  none
// Output: version
uint32_t Lab6_GetVersion(void){volatile int r;uint8_t sendMsg[8];
  OutString("\n\rGet Version");
  BuildGetVersionMsg(sendMsg);
  r = AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE); 
  return (RecvBuf[5]<<8)+(RecvBuf[6]);
}

//*************BuildAddServiceMsg**************
// Create an Add service message, used in Lab 6
// Inputs uuid is 0xFFF0, 0xFFF1, ...
//        pointer to empty buffer of at least 9 bytes
// Output none
// build the necessary NPI message that will add a service
void BuildAddServiceMsg(uint16_t uuid, uint8_t *msg){
//****You implement this function as part of Lab 6*****
  uint8_t *ptr = msg; // set ptr to beginning of message
  *(msg++) = SOF;
  *(msg++) = 3;    // set length to 3
  *(msg++) = 0x00;
  *(msg++) = 0x35; // set fist command
  *(msg++) = 0x81; // set second command
  *(msg++) = 0x01; // primary service
  *(msg++) = uuid; // set first btyte of uuid 
  *(msg++) = uuid >> 8; //set second byte of uuid
  SetFCS(ptr);
}
//*************Lab6_AddService**************
// Add a service, used in Lab 6
// Inputs uuid is 0xFFF0, 0xFFF1, ...
// Output APOK if successful,
//        APFAIL if SNP failure
int Lab6_AddService(uint16_t uuid){ int r; uint8_t sendMsg[12];
  OutString("\n\rAdd service");
  BuildAddServiceMsg(uuid,sendMsg);
  r = AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);  
  return r;
}
//*************AP_BuildRegisterServiceMsg**************
// Create a register service message, used in Lab 6
// Inputs pointer to empty buffer of at least 6 bytes
// Output none
// build the necessary NPI message that will register a service
void BuildRegisterServiceMsg(uint8_t *msg){
//****You implement this function as part of Lab 6*****
  uint8_t *ptr = msg; // set ptr to beginning of message
  *(msg++) = SOF;
  *(msg++) = 0;    // set length to 0
  *(msg++) = 0x00; 
  *(msg++) = 0x35; // set fist command
  *(msg++) = 0x84; // set second command
  SetFCS(ptr); // set fcs
  
}
//*************Lab6_RegisterService**************
// Register a service, used in Lab 6
// Inputs none
// Output APOK if successful,
//        APFAIL if SNP failure
int Lab6_RegisterService(void){ int r; uint8_t sendMsg[8];
  OutString("\n\rRegister service");
  BuildRegisterServiceMsg(sendMsg);
  r = AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  return r;
}

//*************BuildAddCharValueMsg**************
// Create a Add Characteristic Value Declaration message, used in Lab 6
// Inputs uuid is 0xFFF0, 0xFFF1, ...
//        permission is GATT Permission, 0=none,1=read,2=write, 3=Read+write 
//        properties is GATT Properties, 2=read,8=write,0x0A=read+write, 0x10=notify
//        pointer to empty buffer of at least 14 bytes
// Output none
// build the necessary NPI message that will add a characteristic value
void BuildAddCharValueMsg(uint16_t uuid,  
  uint8_t permission, uint8_t properties, uint8_t *msg){
// set RFU to 0 and
// set the maximum length of the attribute value=512
// for a hint see NPI_AddCharValue in AP.c
// for a hint see first half of AP_AddCharacteristic and first half of AP_AddNotifyCharacteristic
//****You implement this function as part of Lab 6*****
  uint8_t *ptr = msg; // set ptr to beginning of message
  *(msg++) = SOF;
  *(msg++) = 8; // set length to 8
  *(msg++) = 0x00;
  *(msg++) = 0x35; // set fist command
  *(msg++) = 0x82; // set second command
  *(msg++) = permission; // set permission
  *(msg++) = properties; // set properties
  *(msg++) = 0x00; 
  *(msg++) = 0x00; // set RFU
  *(msg++) = 0x00;
  *(msg++) = 0x02; // set max length 
  *(msg++) = uuid; // set first btyte of uuid 
  *(msg++) = uuid >> 8; //set second byte of uuid    
  SetFCS(ptr);
    
}

//*************BuildAddCharDescriptorMsg**************
// Create a Add Characteristic Descriptor Declaration message, used in Lab 6
// Inputs name is a null-terminated string, maximum length of name is 20 bytes
//        pointer to empty buffer of at least 32 bytes
// Output none
// build the necessary NPI message that will add a Descriptor Declaration
void BuildAddCharDescriptorMsg(char name[], uint8_t *msg){
// set length and maxlength to the string length
// set the permissions on the string to read
// for a hint see NPI_AddCharDescriptor in AP.c
// for a hint see second half of AP_AddCharacteristic
//****You implement this function as part of Lab 6*****
  uint16_t size;
  uint8_t *ptr; // set ptr to beginning of message
  size = 0;
  ptr = msg; // set ptr to beginning of message
  for(char *n = name; *n != '\0'; n++){
    size++; // find the size of the array
  }
  size++; // include for the last null character
  *(msg++) = SOF;
  *(msg++) = 6 + size; // set length to 6 + string length
  *(msg++) = 0x00;
  *(msg++) = 0x35; // set fist command
  *(msg++) = 0x83; // set second command
  *(msg++) = 0x80; // user description string
  *(msg++) = 0x01; // GATT read permission
  *(msg++) = (uint8_t)size; // maximum possible length
  *(msg++) = (uint8_t)size>>4;
  *(msg++) = (uint8_t)size; // initial length
  *(msg++) = (uint8_t)size>>4;
  for(char *n = name; *n != '\0'; n++){
    *(msg++) = *n;
  }
  *(msg++) = 0; // add null character
  SetFCS(ptr);
  
}

//*************Lab6_AddCharacteristic**************
// Add a read, write, or read/write characteristic, used in Lab 6
//        for notify properties, call AP_AddNotifyCharacteristic 
// Inputs uuid is 0xFFF0, 0xFFF1, ...
//        thesize is the number of bytes in the user data 1,2,4, or 8 
//        pt is a pointer to the user data, stored little endian
//        permission is GATT Permission, 0=none,1=read,2=write, 3=Read+write 
//        properties is GATT Properties, 2=read,8=write,0x0A=read+write
//        name is a null-terminated string, maximum length of name is 20 bytes
//        (*ReadFunc) called before it responses with data from internal structure
//        (*WriteFunc) called after it accepts data into internal structure
// Output APOK if successful,
//        APFAIL if name is empty, more than 8 characteristics, or if SNP failure
int Lab6_AddCharacteristic(uint16_t uuid, uint16_t thesize, void *pt, uint8_t permission,
  uint8_t properties, char name[], void(*ReadFunc)(void), void(*WriteFunc)(void)){
  int r; uint16_t handle; 
  uint8_t sendMsg[32];  
  if(thesize>8) return APFAIL;
  if(name[0]==0) return APFAIL;       // empty name
  if(CharacteristicCount>=MAXCHARACTERISTICS) return APFAIL; // error
  BuildAddCharValueMsg(uuid,permission,properties,sendMsg);
  OutString("\n\rAdd CharValue");
  r=AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  if(r == APFAIL) return APFAIL;
  handle = (RecvBuf[7]<<8)+RecvBuf[6]; // handle for this characteristic
  OutString("\n\rAdd CharDescriptor");
  BuildAddCharDescriptorMsg(name,sendMsg);
  r=AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  if(r == APFAIL) return APFAIL;
  CharacteristicList[CharacteristicCount].theHandle = handle;
  CharacteristicList[CharacteristicCount].size = thesize;
  CharacteristicList[CharacteristicCount].pt = (uint8_t *) pt;
  CharacteristicList[CharacteristicCount].callBackRead = ReadFunc;
  CharacteristicList[CharacteristicCount].callBackWrite = WriteFunc;
  CharacteristicCount++;
  return APOK; // OK
} 
  

//*************BuildAddNotifyCharDescriptorMsg**************
// Create a Add Notify Characteristic Descriptor Declaration message, used in Lab 6
// Inputs name is a null-terminated string, maximum length of name is 20 bytes
//        pointer to empty buffer of at least bytes
// Output none
// build the necessary NPI message that will add a Descriptor Declaration
void BuildAddNotifyCharDescriptorMsg(char name[], uint8_t *msg){
// set length and maxlength to the string length
// set the permissions on the string to read
// set User Description String
// set CCCD parameters read+write
// for a hint see NPI_AddCharDescriptor4 in VerySimpleApplicationProcessor.c
// for a hint see second half of AP_AddNotifyCharacteristic
//****You implement this function as part of Lab 6*****
  uint16_t size;
  uint8_t *ptr;
  size = 0;
  ptr = msg; // set ptr to beginning of message
  for(char *n = name; *n != '\0'; n++){
    size++; // find the size of the array
  }
  size++; // include for the last nul character
  *(msg++) = SOF;
  *(msg++) = 7 + size; // set length to 6 + string length
  *(msg++) = 0x00;
  *(msg++) = 0x35; // set fist command
  *(msg++) = 0x83; // set second command
  *(msg++) = 0x84; // CCCD + user description string
  *(msg++) = 0x03; // CCCD read + write permision
  *(msg++) = 0x01; // GATT read permission
  *(msg++) = size; // maximum possible length
  *(msg++) = size>>8;
  *(msg++) = size; // initial length
  *(msg++) = size>>8;
  for(char *n = name; *n != '\0'; n++){
    *(msg++) = *n;
  }
  *(msg++) = '\0'; // add null character
  SetFCS(ptr);
  
}
  
//*************Lab6_AddNotifyCharacteristic**************
// Add a notify characteristic, used in Lab 6
//        for read, write, or read/write characteristic, call AP_AddCharacteristic 
// Inputs uuid is 0xFFF0, 0xFFF1, ...
//        thesize is the number of bytes in the user data 1,2,4, or 8 
//        pt is a pointer to the user data, stored little endian
//        name is a null-terminated string, maximum length of name is 20 bytes
//        (*CCCDfunc) called after it accepts , changing CCCDvalue
// Output APOK if successful,
//        APFAIL if name is empty, more than 4 notify characteristics, or if SNP failure
int Lab6_AddNotifyCharacteristic(uint16_t uuid, uint16_t thesize, void *pt,   
  char name[], void(*CCCDfunc)(void)){
  int r; uint16_t handle; 
  uint8_t sendMsg[36];  
  if(thesize>8) return APFAIL;
  if(NotifyCharacteristicCount>=NOTIFYMAXCHARACTERISTICS) return APFAIL; // error
  BuildAddCharValueMsg(uuid,0,0x10,sendMsg);
  OutString("\n\rAdd Notify CharValue");
  r=AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  if(r == APFAIL) return APFAIL;
  handle = (RecvBuf[7]<<8)+RecvBuf[6]; // handle for this characteristic
  OutString("\n\rAdd CharDescriptor");
  BuildAddNotifyCharDescriptorMsg(name,sendMsg);
  r=AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  if(r == APFAIL) return APFAIL;
  NotifyCharacteristicList[NotifyCharacteristicCount].uuid = uuid;
  NotifyCharacteristicList[NotifyCharacteristicCount].theHandle = handle;
  NotifyCharacteristicList[NotifyCharacteristicCount].CCCDhandle = (RecvBuf[8]<<8)+RecvBuf[7]; // handle for this CCCD
  NotifyCharacteristicList[NotifyCharacteristicCount].CCCDvalue = 0; // notify initially off
  NotifyCharacteristicList[NotifyCharacteristicCount].size = thesize;
  NotifyCharacteristicList[NotifyCharacteristicCount].pt = (uint8_t *) pt;
  NotifyCharacteristicList[NotifyCharacteristicCount].callBackCCCD = CCCDfunc;
  NotifyCharacteristicCount++;
  return APOK; // OK
}

//*************BuildSetDeviceNameMsg**************
// Create a Set GATT Parameter message, used in Lab 6
// Inputs name is a null-terminated string, maximum length of name is 24 bytes
//        pointer to empty buffer of at least 36 bytes
// Output none
// build the necessary NPI message to set Device name
void BuildSetDeviceNameMsg(char name[], uint8_t *msg){
// for a hint see NPI_GATTSetDeviceNameMsg in VerySimpleApplicationProcessor.c
// for a hint see NPI_GATTSetDeviceName in AP.c
//****You implement this function as part of Lab 6*****
  uint16_t size;
  uint8_t *ptr;
  size = 0;
  ptr = msg; // set ptr to beginning of message
  for(char *n = name; *n != '\0'; n++){
    size++; // find the size of the array
  }
  *(msg++) = SOF;
  *(msg++) = 3 + size;     // set length to 0
  *(msg++) = 0x00; 
  *(msg++) = 0x35; // set fist command
  *(msg++) = 0x8C; // set second command
  *(msg++) = 0x01; // generic access service
  *(msg++) = 0x00; // device name
  *(msg++) = 0x00;
  for(char *n = name; *n != '\0'; n++){
    *(msg++) = *n;
   }
  //*(msg++) = '\0'; // add null character
  SetFCS(ptr);
  
}
//*************BuildSetAdvertisementData1Msg**************
// Create a Set Advertisement Data message, used in Lab 6
// Inputs pointer to empty buffer of at least 16 bytes
// Output none
// build the necessary NPI message for Non-connectable Advertisement Data
void BuildSetAdvertisementData1Msg(uint8_t *msg){
// for a hint see NPI_SetAdvertisementMsg in VerySimpleApplicationProcessor.c
// for a hint see NPI_SetAdvertisement1 in AP.c
// Non-connectable Advertisement Data
// GAP_ADTYPE_FLAGS,DISCOVERABLE | no BREDR  
// Texas Instruments Company ID 0x000D
// TI_ST_DEVICE_ID = 3
// TI_ST_KEY_DATA_ID
// Key state=0
//****You implement this function as part of Lab 6*****
  *(msg++) = SOF;
  *(msg++) = 11; // set length to 11
  *(msg++) = 0x00; 
  *(msg++) = 0x55; // set fist command
  *(msg++) = 0x43; // set second command
  *(msg++) = 0x01; // not connected advertismeent data
  *(msg++) = 0x02; // GAP_ADTYPE_FLAGS,DISCOVERABLE | no BREDR
  *(msg++) = 0x01;
  *(msg++) = 0x06;
  *(msg++) = 0x06; // length, manufacturer specific
  *(msg++) = 0xFF;
  *(msg++) = 0x0D; // Texas Instruments Company ID
  *(msg++) = 0x00;
  *(msg++) = 0x03; // TI_ST_DEVICE_ID  
  *(msg++) = 0x00; // TI_ST_KEY_DATA_ID
  *(msg++) = 0x00; // Key state
  *msg = 0xEE;     // fcs
}

//*************BuildSetAdvertisementDataMsg**************
// Create a Set Advertisement Data message, used in Lab 6
// Inputs name is a null-terminated string, maximum length of name is 24 bytes
//        pointer to empty buffer of at least 36 bytes
// Output none
// build the necessary NPI message for Scan Response Data
void BuildSetAdvertisementDataMsg(char name[], uint8_t *msg){
// for a hint see NPI_SetAdvertisementDataMsg in VerySimpleApplicationProcessor.c
// for a hint see NPI_SetAdvertisementData in AP.c
//****You implement this function as part of Lab 6*****
  uint16_t size;
  uint8_t *ptr;
  size = 0;
  ptr = msg; // set ptr to beginning of message
  for(char *n = name; *n != '\0'; n++){
    size++; // find the size of the array
  }
  size++; // include null character in size
  *(msg++) = SOF;
  *(msg++) = 11 + size; // set length to 16 + size
  *(msg++) = 0x00; 
  *(msg++) = 0x55; // set fist command
  *(msg++) = 0x43; // set second command
  *(msg++) = 0x00; // scan response data
  *(msg++) = size; 
  *(msg++) = 0x09; // type=LOCAL_NAME_COMPLETE
  for(char *n = name; *n != '\0'; n++){
    *(msg++) = *n;
  }
  // connection interval range
  *(msg++) = 0x05; // length of this data
  *(msg++) = 0x12; // GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE
  *(msg++) = 0x50; // DEFAULT_DESIRED_MIN_CONN_INTERVAL
  *(msg++) = 0x00;
  *(msg++) = 0x20; // DEFAULT_DESIRED_MAX_CONN_INTERVAL
  *(msg++) = 0x03;
  // Tx power level
  *(msg++) = 0x02; // length of this data
  *(msg++) = 0x0A; // GAP_ADTYPE_POWER_LEVEL
  *(msg++) = 0x00; // 0dBm
  SetFCS(ptr);
  
}
//*************BuildStartAdvertisementMsg**************
// Create a Start Advertisement Data message, used in Lab 6
// Inputs advertising interval
//        pointer to empty buffer of at least 20 bytes
// Output none
// build the necessary NPI message to start advertisement
void BuildStartAdvertisementMsg(uint16_t interval, uint8_t *msg){
// for a hint see NPI_StartAdvertisementMsg in VerySimpleApplicationProcessor.c
// for a hint see NPI_StartAdvertisement in AP.c
//****You implement this function as part of Lab 6*****
  uint8_t *ptr = msg; // set ptr to beginning of message
  *(msg++) = SOF;
  *(msg++) = 14; // set length to 14
  *(msg++) = 0x00; 
  *(msg++) = 0x55; // set fist command
  *(msg++) = 0x42; // set second command
  *(msg++) = 0x00; // connectable undirected advertisement
  *(msg++) = 0x00; // advertise infinitely
  *(msg++) = 0x00;
  *(msg++) = interval; // interval
  *(msg++) = interval >> 8; 
  *(msg++) = 0x00; // filter policy
  *(msg++) = 0x00; // initiator type
  *(msg++) = 0x00; // initiator address
  *(msg++) = 0x01;
  *(msg++) = 0x00;
  *(msg++) = 0x00;
  *(msg++) = 0x00;
  *(msg++) = 0xC5;
  *(msg++) = 0x02; // advertising will restart with connectable advertising when a connection is terminated
  SetFCS(ptr);     // fcs
}

//*************Lab6_StartAdvertisement**************
// Start advertisement, used in Lab 6
// Input:  none
// Output: APOK if successful,
//         APFAIL if notification not configured, or if SNP failure
int Lab6_StartAdvertisement(void){volatile int r; uint8_t sendMsg[40];
  OutString("\n\rSet Device name");
  BuildSetDeviceNameMsg("Shape the World",sendMsg);
  r =AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  OutString("\n\rSetAdvertisement1");
  BuildSetAdvertisementData1Msg(sendMsg);
  r =AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  OutString("\n\rSetAdvertisement Data");
  BuildSetAdvertisementDataMsg("Shape the World",sendMsg);
  r =AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  OutString("\n\rStartAdvertisement");
  BuildStartAdvertisementMsg(100,sendMsg);
  r =AP_SendMessageResponse(sendMsg,RecvBuf,RECVSIZE);
  return r;
}

