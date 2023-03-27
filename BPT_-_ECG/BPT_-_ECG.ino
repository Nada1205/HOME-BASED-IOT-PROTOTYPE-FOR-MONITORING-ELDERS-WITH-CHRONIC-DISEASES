#include <Wire.h>
#include <LiquidCrystal.h>
#include "max32664.h"

#define RESET_PIN 04
#define MFIO_PIN 02
#define RAWDATA_BUFFLEN 250
#define PB1 45 
#define PB2 47
#define PB3 49
#define PB4 51
#define Buzzer 53

char BP_Flag = 0, ECG_Flag = 0;

long int BP_Count_i,BP_Count_f, BP_Count, ECG_Count = 0, ECG_Timer_i, ECG_Timer;

max32664 MAX32664(RESET_PIN, MFIO_PIN, RAWDATA_BUFFLEN);

extern void(* resetFunc) (void) = 0;

void mfioInterruptHndlr(){
  //Serial.println("i");
}

void enableInterruptPin(){

  //pinMode(mfioPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(MAX32664.mfioPin), mfioInterruptHndlr, FALLING);

}



void loadAlgomodeParameters(){

  algomodeInitialiser algoParameters;
  /*  Replace the predefined values with the calibration values taken with a reference spo2 device in a controlled environt.
      Please have a look here for more information, https://pdfserv.maximintegrated.com/en/an/an6921-measuring-blood-pressure-MAX32664D.pdf
      https://github.com/Protocentral/protocentral-pulse-express/blob/master/docs/SpO2-Measurement-Maxim-MAX32664-Sensor-Hub.pdf
  */

  algoParameters.calibValSys[0] = 120;
  algoParameters.calibValSys[1] = 122;
  algoParameters.calibValSys[2] = 125;

  algoParameters.calibValDia[0] = 80;
  algoParameters.calibValDia[1] = 81;
  algoParameters.calibValDia[2] = 82;

  algoParameters.spo2CalibCoefA = 1.5958422;
  algoParameters.spo2CalibCoefB = -34.659664;
  algoParameters.spo2CalibCoefC = 112.68987;

  MAX32664.loadAlgorithmParameters(&algoParameters);
}

float reading;
int coding;

LiquidCrystal lcd(36,37,40,41,42,43);

void setup() {
  // put your setup code here, to run once:

  lcd.begin(20,4);

  lcd.print("Hi NADA");

  delay(3000);
  
  digitalWrite(30, HIGH);
  pinMode(30, OUTPUT);
  
  pinMode(45, INPUT_PULLUP);
  pinMode(47, INPUT_PULLUP);
  pinMode(49, INPUT_PULLUP);
  pinMode(51, INPUT_PULLUP);

  pinMode(Buzzer, OUTPUT);
  digitalWrite(Buzzer, 1);
  
  Serial3.begin(9600);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Press B1 for BP");
  lcd.setCursor(0, 1);
  lcd.print("Press B2 for ECG");
  lcd.setCursor(0, 2);
  lcd.print("Press B3 to Reset");
  lcd.setCursor(0, 3);
  lcd.print("Press B4 for BP&ECG");
  
  
  char serial_input;
  
  while(digitalRead(PB1) && digitalRead(PB2) && digitalRead(PB3) && digitalRead(PB4));

  Serial3.print("1");
  
  if(digitalRead(PB1) == 0) coding = 1;
  if(digitalRead(PB2) == 0) coding = 2;
  if(digitalRead(PB3) == 0) coding = 3;
  if(digitalRead(PB4) == 0) coding = 4;



  if(coding == 1 || coding == 4){
    
    lcd.clear();
    
    lcd.setCursor(0, 0);
    lcd.print("Please Wait");
    lcd.setCursor(0, 1);
    lcd.print("And Keep your Index");
    lcd.setCursor(0, 2);
    lcd.print("Stable");
    
    Wire.begin();

    loadAlgomodeParameters();
  
    int result = MAX32664.hubBegin();
    if (result == CMD_SUCCESS){
      Serial3.println("Sensorhub begin!");
    }else{
      //stay here.
      while(1){
        Serial3.println("Could not communicate with the sensor! please make proper connections");
        delay(5000);
      }
    }
  
    bool ret = MAX32664.startBPTcalibration();
    while(!ret){
  
      delay(10000);
      Serial.println("failed calib, please retsart");
      //ret = MAX32664.startBPTcalibration();
    }
  
    delay(1000);
  
    //Serial.println("start in estimation mode");
    ret = MAX32664.configAlgoInEstimationMode();
    while(!ret){
  
      //Serial.println("failed est mode");
      ret = MAX32664.configAlgoInEstimationMode();
      delay(10000);
    }
  
    //MAX32664.enableInterruptPin();
    Serial3.println("Getting the device ready..");
    delay(1000);
  }


    pinMode(10, INPUT); // Setup for leads off detection LO +
    pinMode(11, INPUT); // Setup for leads off detection LO - 
  
}

void loop() {
  // put your main code here, to run repeatedly:
  if(coding == 1 || coding == 4){

    if(digitalRead(PB3) == 0) resetFunc();
    
    uint8_t num_samples = MAX32664.readSamples();
    
    if(BP_Flag == 0){
      BP_Flag = 1;
      BP_Count_i = millis()/1000;
    }    

    BP_Count = (millis()/1000) - BP_Count_i;

    Serial3.println(BP_Count);
    
    if(BP_Count >= 60){
      if(num_samples){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("sys = ");
        lcd.print(MAX32664.max32664Output.sys);
//        lcd.setCursor(0, 1);
        lcd.print(" dia = ");
        lcd.print(MAX32664.max32664Output.dia);
        lcd.setCursor(0, 1);
        lcd.print("hr = ");
        lcd.print(MAX32664.max32664Output.hr);
//        lcd.setCursor(0, 3);
        lcd.print(" spo2 = ");
        lcd.print(MAX32664.max32664Output.spo2);
        lcd.setCursor(0, 3);
        lcd.print("Remove your index");
        dash();
      }

      if(BP_Count == 62){
        digitalWrite(Buzzer, 1);
        if (coding == 4){
          delay(4000);
          coding = 5;
          goto ECG;
        }
        coding = 0;
        while(digitalRead(PB3));
        resetFunc();
      }
    }
    
    delay(100);
  }



  if(coding == 2 || coding == 5){
    ECG:
    // put your main code here, to run repeatedly:
    if((digitalRead(10) == 1)||(digitalRead(11) == 1)){
      Serial.println('!');
      ECG_Count = 0;
    }

    if(digitalRead(PB3) == 0) resetFunc();
     
    else{
      // send the value of analog input 0:
        reading = Get_Reading();
        Serial3.println(reading);
        ECG_Count++;
    }
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Please, Keep the");
    lcd.setCursor(0, 1);
    lcd.print("Electrodes Stable");
    lcd.setCursor(0, 2);
    lcd.print("on your body");

    
    if(ECG_Flag == 0){
      ECG_Timer_i = millis()/1000;
      ECG_Flag = 1;
    }

    if(((millis()/1000) - ECG_Timer_i) == 60){
      dash();
      dash();
      dash();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Remove Electrodes");
      coding = 0;
      while(digitalRead(PB3));
      resetFunc();
    }
  }

  if(coding == 3){
    
  }
//  lcd.setCursor(0, 1);
//  lcd.print(millis() / 1000);
}

float Get_Reading(void){
  float AverageTemperature = 0;
  int MeasurementsToAverage = 20;
  for(int i = 0; i < MeasurementsToAverage; ++i)
  {
    AverageTemperature += analogRead(A0);
    delay(1);
  }
  AverageTemperature /= MeasurementsToAverage;
  return AverageTemperature;
}


void dash(){
  digitalWrite(Buzzer, LOW); // Tone ON
  delay(100); // Tone length
  digitalWrite(Buzzer, HIGH); // Tone OFF
  delay(100); // Symbol pause
  return;
}
