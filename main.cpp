#include <Arduino.h>
#include <Adafruit_BusIO_Register.h>
#include <SparkFun_BNO08x_Arduino_Library.h>
#include <Wire.h>

BNO08x myIMU;

#define BNO08X_INT  A4
#define BNO08X_RST  A5
#define BNO08X_ADDR 0x4B
// put function declarations here:


void setup() {
Serial.begin(115200);
while(!Serial) delay(10);

Wire.begin();

if(myIMU.begin(BNO08X_ADDR, Wire, BNO08X_INT, BNO08X_RST) == false){
  Serial.println(F("Yeah....... It's not working"));
  Serial.println(F("Freezing Now."));
  while(1)
    ;
}
  Serial.println("All Good. Sensor found!");

}
void setSetings()
{
  if(myIMU.enableRotationVector() == true){
    Serial.println(F("Rotation vector enabled"));
    Serial.println(F("Output in form i, j, k, real, accuracy"));
  }else{
    Serial.println(F("Could not enable rotation vector."));
  }
}

void loop() {
  delay(10);
  if(myIMU.wasReset()){
    Serial.println(F("Sensor Reset"));
    setSetings();
  }

  if(myIMU.getSensorEvent() == true)
    {
      if(myIMU.getSensorEventID() == SENSOR_REPORTID_ROTATION_VECTOR)
      {
        float quatI = myIMU.getQuatI();
        float quatJ = myIMU.getQuatJ();
        float quatK = myIMU.getQuatK();
        float quatReal = myIMU.getQuatReal();
        float quatRadianAccuracy = myIMU.getQuatRadianAccuracy();

        Serial.print(quatI, 2);
        Serial.print(F(","));
        Serial.print(quatJ, 2);
        Serial.print(F(","));
        Serial.print(quatK, 2);
        Serial.print(F(","));
        Serial.print(quatReal, 2);
        Serial.print(F(","));
        Serial.print(quatRadianAccuracy, 2);
        
        Serial.println();
      }

    }
}