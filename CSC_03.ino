#include <CurieBLE.h>

BLEPeripheral blePeripheral;
BLEService cscService("1816");

BLECharacteristic cscChar("2A5B", BLENotify, 11);
BLEUnsignedShortCharacteristic cscFtr("2A5C", BLERead);
BLEUnsignedCharCharacteristic cscSL("2A5D", BLERead);
//BLECharacteristic cscCP("2A55", BLEIndicate | BLEWrite, 20);

volatile unsigned long previousMillis = 0;
volatile unsigned long lastMillis = 0;
volatile unsigned long currentMillis = 0;

volatile unsigned long time_prev_wheel = 0, time_now_wheel;
volatile unsigned long time_prev_crank = 0, time_now_crank;
volatile unsigned long time_chat = 100;

volatile unsigned int wheelRev = 0;
volatile unsigned int oldWheelRev = 0;
volatile unsigned long oldWheelMillis = 0;
volatile unsigned long lastWheeltime = 0;
volatile unsigned int crankRev = 0;
volatile unsigned int oldCrankRev = 0;
volatile unsigned long lastCranktime = 0;
volatile unsigned long oldCrankMillis = 0;
char packet[20];

boolean centralConnected = false;

void setup() {
  Serial.begin(9600);       // initialize serial communication
/*
  while (!Serial) {
    ; // Wait for ready
  }
*/
  Serial.println("App Start");
  pinMode(13, OUTPUT);      // initialize the LED on pin 13 to indicate when a central is connected
  blePeripheral.setLocalName("CSCSketch");
  blePeripheral.setAdvertisedServiceUuid(cscService.uuid());  // add the service UUID
  blePeripheral.addAttribute(cscService);   // Add the BLE service
  blePeripheral.addAttribute(cscChar); // add characteristic
  blePeripheral.addAttribute(cscFtr);
  blePeripheral.addAttribute(cscSL);
//  blePeripheral.addAttribute(cscCP);
  
  lastWheeltime = millis();
  lastCranktime = millis();

  const unsigned char charArray[11] = {
    3,                
    (unsigned char)wheelRev, (unsigned char)(wheelRev >> 8), (unsigned char)(wheelRev >> 16), (unsigned char)(wheelRev >> 24), 
    (unsigned char)lastWheeltime, (unsigned char)(lastWheeltime >> 8), 
    (unsigned char)crankRev, (unsigned char)(crankRev >> 8), 
    (unsigned char)lastCranktime, (unsigned char)(lastCranktime >> 8) };
  cscChar.setValue(charArray, 11);   // initial value for this characteristic
  cscFtr.setValue(3);
  cscSL.setValue(11);

  blePeripheral.begin();
  Serial.println("Bluetooth device active, waiting for connections...");

  attachInterrupt(digitalPinToInterrupt(11), wheelAdd, FALLING); // RISING FALLING CHANGE LOW 
  attachInterrupt(digitalPinToInterrupt(10), crankAdd, FALLING);
}

void wheelAdd() {
  time_now_wheel = millis();
  if( time_now_wheel > time_prev_wheel + time_chat){
    wheelRev = wheelRev + 1;
    time_prev_wheel = time_now_wheel;
    lastWheeltime = millis();
  }
}

void crankAdd() {
  time_now_crank = millis();
  if( time_now_crank > time_prev_crank + time_chat){
    crankRev = crankRev + 1;
    time_prev_crank = time_now_crank;
    lastCranktime = millis();
  }
}

void loop() {
  // listen for BLE peripherals to connect:
  BLECentral central = blePeripheral.central();

  // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());
    // turn on the LED to indicate the connection:
    digitalWrite(13, HIGH);

    // check the csc mesurement every 200ms
    while (central.connected()) {
      centralConnected = true;
      currentMillis = millis();
      // if 200ms have passed, check the blood pressure mesurement:
      if (oldWheelRev < wheelRev && currentMillis - oldWheelMillis >= 500) {
        updateCSC("wheel");
      }
      else if (oldCrankRev < crankRev && currentMillis - oldCrankMillis >= 500) {
        updateCSC("crank");
      }
      else if (currentMillis - previousMillis >= 1000) {
        updateCSC("timer");
      }
    }
    // when the central disconnects, turn off the LED:
    digitalWrite(13, LOW);
    centralConnected = false;
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}

void updateCSC(String sType) {
  oldWheelRev = wheelRev;
  oldCrankRev = crankRev;
  oldWheelMillis = currentMillis;
  previousMillis = currentMillis;

  const unsigned char cscCharArray[11] = {
    3,                
    (unsigned char)wheelRev, (unsigned char)(wheelRev >> 8), (unsigned char)(wheelRev >> 16), (unsigned char)(wheelRev >> 24), 
    (unsigned char)lastWheeltime, (unsigned char)(lastWheeltime >> 8), 
    (unsigned char)crankRev, (unsigned char)(crankRev >> 8), 
    (unsigned char)lastCranktime, (unsigned char)(lastCranktime >> 8) };
  cscChar.setValue(cscCharArray, 11);
  
  Serial.print("Wheel Rev.: ");
  Serial.print(wheelRev);
  Serial.print(" WheelTime : ");
  Serial.print(lastWheeltime);
  Serial.print(" Crank Rev.: ");
  Serial.print(crankRev);
  Serial.print(" CrankTime : ");
  Serial.print(lastCranktime);
  Serial.print("  ");
  Serial.println(sType);
}

