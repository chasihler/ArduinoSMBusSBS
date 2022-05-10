#include <Wire.h>

/***************************************************************
 * SMBus Battery Information 
 * chas@snocomakers.org
 * 
 * http://sbs-forum.org/specs/sbdat110.pdf
 * This is the making of an application for your next project. 
 * 
 * Note:
 * 0x03 Register - Note Capacity Mode (bit 15) and Condition Flag (bit 7)
 * 
 * 0x16
 * * * * * * * Alarm Bits * * * * *
 * 0x8000 OVER_CHARGED_ALARM
 * 0x4000 TERMINATE_CHARGE_ALARM
 * 0x2000 Reserved
 * 0x1000 OVER_TEMP_ALARM
 * 0x0800 TERMINATE_DISCHARGE_ALARM
 * 0x0400 Reserved
 * 0x0200 REMAINING_CAPACITY_ALARM
 * 0x0100 REMAINING_TIME_ALARM
 * * * * * * Status Bits * * * * *
 * 0x0080 INITIALIZED
 * 0x0040 DISCHARGING
 * 0x0020 FULLY_CHARGED
 * 0x0010 FULLY_DISCHARGED
 * 
 * 
 * 
 * *************************************************************
 */

#define BatteryAddress 11     //0x0B

//Register Definitions
#define BatteryMode 3         //0x03
#define AtRateToEmpty 6       //0x06 0-65534 min 65535 = Invalid, No discharge)
#define Temperature 8         //0x08 0 - 6553.5K
#define Voltage 9             //0x09 0 - 65,535mV
#define Current 10            //0x0A signed int -32K to 32K mA (neg discharge)
#define AverageCurrent 11     //0x0B -32K - +32K mA (neg = discharge)
#define StateOfCharge 13      //0x0D Relative State of Charge (0-100 (%) 
#define RemainingCapacity 15  //0x0F maH/C5 or 10mWh/P5 (Capacity Bit) 0-65k
#define RuntimeToEmpty 17     //0x11 0-65,534 mins left 
#define BatteryStatus 22      //0x16 See notes above
#define CycleCount 23         //0x17 Number of Cycles the battery has experienced. 
                              //"odometer" 0-65k   


const int wait_interval = 2000;  
unsigned long previousMillis = 0;   

void setup() {
  Serial.begin(115200);
  Wire.begin();
  delay(100);
  Serial.println("SMbus Battery Information");
}

 boolean isBitSetinByte (byte myVar, byte bitNumber) {
  bool bitvalue;
  bitvalue = myVar & (1 << bitNumber);
  return bitvalue;
}  
 boolean isBitSetinInt (int myVar, byte bitNumber) {
  bool bitvalue;
  bitvalue = myVar & (1 << bitNumber);
  return bitvalue;
}  


 bool non_blocking_wait (void) {
  //call in loop
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= wait_interval) {
    // time passed, save new time.
    previousMillis = currentMillis;
    return true;
  }
  return false;
}

int GetStatus (void) {
  Wire.beginTransmission(BatteryAddress);
  Wire.write(BatteryStatus);
  Wire.endTransmission();
  Wire.requestFrom(BatteryAddress, 2);
  Wire.available();
  int battery_status = Wire.read();

  return battery_status;
}


uint16_t GetTemp (void) {
  uint8_t vals[2] = {0,0};
  uint8_t count = 0;
  Wire.beginTransmission(BatteryAddress);
  Wire.write(Temperature);
  Wire.endTransmission();
  Wire.requestFrom(BatteryAddress, 2);
    while(Wire.available()){
      vals[count++] = Wire.read();
    }
    //Convert K to C
    uint16_t temp = (uint16_t)(vals[1]) << 8 | (uint16_t)(vals[0]);
    temp = temp -2730;
    temp = temp/10;
    //error trap bad values.
    return temp;
}

uint16_t GetRuntimeToEmpty (void) {
  uint8_t vals[2] = {0,0};
  uint8_t count = 0;
  Wire.beginTransmission(BatteryAddress);
  Wire.write(RuntimeToEmpty);
  Wire.endTransmission();
  Wire.requestFrom(BatteryAddress, 2);
  count = 0;    //Reset Counter
  while(Wire.available()){
      vals[count++] = Wire.read();
    }
    uint16_t runtime = (uint16_t)(vals[1]) << 8 | (uint16_t)(vals[0]);
  return runtime;
}

uint16_t GetVoltage (void) {
  uint8_t vals[2] = {0,0};
  uint8_t count = 0;
  Wire.beginTransmission(BatteryAddress);
  Wire.write(Voltage);
  Wire.endTransmission();
  Wire.requestFrom(BatteryAddress, 2);
  count = 0;    //Reset Counter
  while(Wire.available()){
      vals[count++] = Wire.read();
    }
    uint16_t volts = (uint16_t)(vals[1]) << 8 | (uint16_t)(vals[0]);
  return volts;
}

int GetSOC (void) {     
  Wire.beginTransmission(BatteryAddress); 
  Wire.write(StateOfCharge); 
  Wire.endTransmission();
  Wire.requestFrom(BatteryAddress, 1); 
  Wire.available(); 
  int soc = Wire.read();
  if (soc < 0) soc = 0; 
  return soc;
}

void loop() {
  int b_soc, b_status;
  uint16_t b_temp, b_runtime;  
  bool report = false; 
  
  if (non_blocking_wait()) {
    b_soc = GetSOC(); 
    b_status = GetStatus();
    b_temp = GetTemp();
    b_runtime = GetRuntimeToEmpty();
    report = true;
  }

  if (report) {
    Serial.println("---------------------------------------------------------------");
    Serial.print("Charge Status: "); Serial.print(b_soc); Serial.println("%");
    if (isBitSetinInt(b_status, 0x10)) {
      Serial.print("Alarm: Battery Fully Discharged. ");
    } else {
      Serial.print("Battery Discharge Alarm Not Present ");
    }
    Serial.print(" Code: 0x"); Serial.println(b_status, HEX);
    Serial.print("Pack Temperature: "); Serial.print(b_temp); Serial.println(" C"); 
    Serial.print("Runtime Remaining: "); Serial.print(b_runtime); Serial.println(" mins"); 
  }

}
