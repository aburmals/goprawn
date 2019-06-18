#include <TheThingsNetwork.h>
#include <Sodaq_RN2483.h>
#include <OneWire.h>
#include <DallasTemperature.h>


#define loraSerial Serial1
#define debugSerial SerialUSB
#define freqPlan TTN_FP_EU868
#define TdsSensorPin A2
#define SCOUNT  30 
#define VREF 5.0  
const char *appEui = "70B3D57ED001DF20";
const char *appKey = "7445C5C2FB29B02BD5545BCA6FB83270";

TheThingsNetwork ttn(loraSerial, debugSerial, freqPlan);
int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0;
float averageVoltage = 0,tdsValue = 0,temperature = 25;
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  loraSerial.begin(57600);
  debugSerial.begin(9600);
//  while (!debugSerial && millis() < 10000);
  //debugSerial.println("Test");
  pinMode(TdsSensorPin,INPUT);
  // put your setup code here, to run once:
  debugSerial.println("-- STATUS");
  ttn.showStatus();

  debugSerial.println("-- JOIN");
  ttn.join(appEui, appKey);
  

}

void loop() {
  debugSerial.println("-- LOOP");
  static unsigned long printTime = millis();
  static unsigned long analogSampleTimepoint = millis();
//  debugSerial.println("Test");
  if(millis()-analogSampleTimepoint > 40U)     //every 40 milliseconds,read the analog value from the ADC
   {
     analogSampleTimepoint = millis();
     analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
     analogBufferIndex++;
     if(analogBufferIndex == SCOUNT)
         analogBufferIndex = 0;
   }
   static unsigned long printTimepoint = millis();
   if(millis()-printTimepoint > 800U)
   {
      printTimepoint = millis();
      for(copyIndex=0;copyIndex<SCOUNT;copyIndex++)
        analogBufferTemp[copyIndex]= analogBuffer[copyIndex];
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 1024.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      float compensationCoefficient=1.0+0.02*(temperature-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
      float compensationVolatge=averageVoltage/compensationCoefficient;  //temperature compensation
      tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5; //convert voltage value to tds value
      //Serial.print("voltage:");
      //Serial.print(averageVoltage,2);
      //Serial.print("V   ");
      debugSerial.println("TDS Value:");
      debugSerial.print(tdsValue,0);
      debugSerial.print("ppm");
      delay(1500);
      uint32_t tdsSensor = tdsValue;
      byte payload[4];
//      payload[0] = tdsValue;
      payload[0] = highByte(tdsSensor);
      payload[1] = lowByte(tdsSensor);
      payload[2] = highByte(tdsSensor);
      payload[3] = lowByte(tdsSensor);
      ttn.sendBytes(payload,sizeof(payload));
   }

}
int getMedianNum(int bArray[], int iFilterLen)
{
      int bTab[iFilterLen];
      for (byte i = 0; i<iFilterLen; i++)
      bTab[i] = bArray[i];
      int i, j, bTemp;
      for (j = 0; j < iFilterLen - 1; j++)
      {
      for (i = 0; i < iFilterLen - j - 1; i++)
          {
        if (bTab[i] > bTab[i++])
            {
        bTemp = bTab[i];
            bTab[i] = bTab[i++];
        bTab[i+1] = bTemp;
         }
      }
      }
      if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
      else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
      return bTemp;
}
