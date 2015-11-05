
#include <SFE_BMP180.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <RTClib.h>
#include <RTC_DS3231.h>
#include <SPI.h>
#include <SoftwareSerial.h>

RTC_DS3231 RTC;

Adafruit_ADS1115 ads1;
Adafruit_ADS1115 ads2(B1001001);

#define SHT2x_address 64
const byte mask = B11111100;
const byte temp_command = B11100011;
const byte hum_command = B11100101;
byte TEMP_byte1, TEMP_byte2, TEMP_byte3;
byte HUM_byte1, HUM_byte2, HUM_byte3;
byte check1, check2;
unsigned int temperature_board, humidity_board;
float temperature_SHT, humidity_SHT;

//SHT25 address  B01100100
//BMP180 address B11101111
//DS3231 address B01101000
//
SFE_BMP180 BMP;
double T, P, p0, a;
char status;

SoftwareSerial ss(8,9);

//holders for the ads ADCs
int ADC1[4];
int ADC2[4];

float V1[4];
float V2[4];

int wind_count = 0;


void setup() {
  attachInterrupt(4, anemometercount, FALLING); //anemometer reed switch on pin 7--> interrupt# 4
  pinMode(11, OUTPUT);
  pinMode(10, OUTPUT);
  //pinMode(9,OUTPUT);
  Serial.begin(9600);
  ads1.begin();
  ads2.begin();

  BMP.begin();
  //if (BMP.begin())
  //    Serial.println("BMP180 init success");
  //  else
  //  {
  //    while(1)
  //    {
  //      Serial.println("BMP180 init fail\n\n");
  //      delay(1000);
  //    }
  //  }

  RTC.begin();
  ss.begin(4800);
  ss.println("$PTNLSNM,0021,02");
  delay(500);
  ss.println("$PTNLSNM,0021,02"); //repeat set GPS command b/c noticed some poor GPS reads in past
  //this sets the Copernicus to output both GGA and ZTG strings (NMEA standard)



}

void loop() {
  wind_count = 0;
  int  wind_dir = analogRead(A0); //wind direction being read by A0 on Yun
  DateTime now = RTC.now();

  //Get SHT data
  get_SHT2x();

  float CO2 = getS300CO2();

  //Get BMP data
  status = BMP.startTemperature();
  if (status != 0)
  {
    Serial.println(status);
    delay(status);
    status = BMP.getTemperature(T);
    status = BMP.startPressure(3);
    if (status != 0)
    {
      delay(status);
      status = BMP.getPressure(P, T);
    }
    else //if good temp; but can't compute P
    {
      P = -99;
    }
  }
  else //if bad temp; then can't compute temp or pressure
  {
    T = -99;
    P = -99;
  }

  //    Serial.print("Temp/Press:");
  //    Serial.print(T);
  //    Serial.print(",");
  //    Serial.println(P);
  //    Serial.print("Temp/Rh:");
  //    Serial.print(temperature_SHT);
  //    Serial.print(",");
  //    Serial.print(humidity_SHT);
  //    Serial.println("ADS voltages:");
  for (int i = 0; i < 4; i++)
  {
    ADC1[i] = ads1.readADC_SingleEnded(i);
    V1[i] = ADC1[i] * 0.1875;
    Serial.print(V1[i]);
    Serial.print(",");
  }

  for (int i = 0; i < 4; i++)
  {
    ADC2[i] = ads2.readADC_SingleEnded(i);
    V2[i] = ADC2[i] * 0.1875;
    Serial.print(V2[i]);
    Serial.print(",");
  }
  Serial.print(temperature_SHT);
  Serial.print(",");
  Serial.print(humidity_SHT);
  Serial.print(",");
  Serial.print(T);
    Serial.print(",");
  Serial.print(P);  
  //
  //  Serial.println();
  Serial.print(", ");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  //  Serial.println();
  //
  //  while(ss.available())
  //  {
  //   char GPSin=ss.read();
  //   Serial.print(GPSin);
  //  }
  //  Serial.println();
  //
  //  Serial.print("CO2: ");
  //  Serial.println(CO2);

  Serial.print(",");
  Serial.print(wind_count);
  Serial.print(",");
  Serial.print(wind_dir);
  Serial.print(",");
  float co2 = getS300CO2();
  Serial.println(co2);

  while (ss.available())
  {
    char GPSin = ss.read();
    Serial.print(GPSin);
  }
  Serial.println();




  // put your main code here, to run repeatedly:
  digitalWrite(11, HIGH);
  digitalWrite(10, HIGH);
  // digitalWrite(9,HIGH);
  delay(2000);
  digitalWrite(11, LOW);
  digitalWrite(10, LOW);
  //  digitalWrite(9,LOW);
  delay(2000);
}

void get_SHT2x()
{
  Wire.beginTransmission(SHT2x_address);
  Wire.write(temp_command);
  check1 = Wire.endTransmission();

  Wire.requestFrom(SHT2x_address, 3);

  TEMP_byte1 = Wire.read();
  TEMP_byte2 = Wire.read();
  TEMP_byte3 = Wire.read();

  Wire.beginTransmission(SHT2x_address);
  Wire.write(hum_command);
  check2 = Wire.endTransmission();

  Wire.requestFrom(SHT2x_address, 3);
  HUM_byte1 = Wire.read();
  HUM_byte2 = Wire.read();
  HUM_byte3 = Wire.read();

  humidity_board = ( (HUM_byte1 << 8) | (HUM_byte2) & mask );
  temperature_board = ( (TEMP_byte1 << 8) | (TEMP_byte2) & mask );

  humidity_SHT = ((125 * (float)humidity_board) / (65536)) - 6.00;
  temperature_SHT = ((175.72 * (float)temperature_board) / (65536)) - 46.85;
}

float getS300CO2()
{
  int i = 1;
  long reading;
  float CO2val;
  Wire.beginTransmission(0x31);
  Wire.write(0x52);
  Wire.endTransmission();
  Wire.requestFrom(0x31, 7);
  while (Wire.available())
  {
    byte val = Wire.read();
    if (i == 2)
    {
      reading = val;
      reading = reading << 8;
    }
    if (i == 3)
    {
      reading = reading | val;
    }
    i = i + 1;
  }
  CO2val = reading / 4095.0 * 5000.0;
  CO2val = reading;
  return CO2val;
}

void anemometercount()
{
  wind_count = wind_count + 1;
}

