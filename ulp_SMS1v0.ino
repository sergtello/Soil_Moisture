//Source code - Arduino version
//Written by Sergio Tello for Ecobridge SAC
/*Hardware Description:
  uC : ESP32 Firebeetle
  125080 Li-Po 3.7V Battery 
*/

//Required Arduino libraries
//SD Card module
#include "FS.h"
#include "SD.h"
#include <SPI.h>

//DBS18B20
#include <OneWire.h>
#include <DallasTemperature.h>

// ADC
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include "math.h"

#include "RTClib.h"
#include <WiFi.h>

//Reed switch - ULP
#include "esp_sleep.h"
#include "driver/rtc_io.h"


//Pin definition and initialization
//SD Card module
//Default VSPI pins for the ESP32 board were selected
/*
 * SD_MISO_PIN = 19
 * SD_MOSI_PIN = 23
 * SD_CLK_PIN  = 18
 */
#define SD_CS_PIN 16
//Select the file format for the data output type
#define OUTPUT_DATA_FORMAT 0                /* 1 = ".csv" file extension, 0 = ".txt" file extension */
const char* file_name =    "/data";         /* Name must start with the '/' character */
RTC_DATA_ATTR char file_name_type[30];

//Timestamp
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//DS18B20
#define ONE_WIRE_BUS 13
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18(&oneWire);


//ADC
 Adafruit_ADS1115 ads ;  /* Use this for the 16-bit version */
#define PWM1 17
#define PWR_A2 15
#define PWM3 14
#define PWR_A0 4
#define EN1 25
#define EN2 26
const float tempC = 24.0;

//Deep Sleep
#define PWR_PIN 27
#define PWR_SEN 5
#define LED_PIN 2
#define uS_TO_S_FACTOR 1000000ULL      /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  30              /* Time ESP32 will go to sleep (in seconds) */
RTC_DATA_ATTR int bootCount = 0;      /* Boot counter stored in RTC memory */
gpio_num_t pwr_pin= GPIO_NUM_27;
gpio_num_t pwr_sen= GPIO_NUM_5;
gpio_num_t led_pin= GPIO_NUM_2;


//Functions and methods required

//DBS18B20 /////////////////////////////////////
void get_temperature_measure(float* temperature_value1,float* temperature_value2,float* temperature_value3 ){
   ds18.begin();
// Call sensors.requestTemperatures() to issue a global temperature 
// Request to all devices on the bus
   Serial.print(F("Requesting temperatures..."));
   ds18.requestTemperatures(); // Send the command to get temperatures
   Serial.println(F("DONE"));
// After we got the temperatures, we can print them here.
// We use the function ByIndex in order to get the temperature from the first sensor only, in our case there is just one.
   *temperature_value1 = constrain(ds18.getTempCByIndex(0),0.0,100.0);
   Serial.print(F("Temperature for the sensor (index 0) is: "));
   Serial.print(*temperature_value1);  
   Serial.println(F("°C"));
    *temperature_value2 = constrain(ds18.getTempCByIndex(1),0.0,100.0);
   Serial.print(F("Temperature for the sensor (index 1) is: "));
   Serial.print(*temperature_value2);  
   Serial.println(F("°C"));
    *temperature_value3 = constrain(ds18.getTempCByIndex(2),0.0,100.0);
   Serial.print(F("Temperature for the sensor (index 2) is: "));
   Serial.print(*temperature_value3);  
   Serial.println(F("°C"));
}
/////////////////////////////////////////////////

//MOISTURE ///////////////////////////////////////
void get_moisture_measure(float *moisture_value1,float *moisture_value2,float *moisture_value3,float *moisture_value4){
    ads.begin();
  int16_t adc0[3], adc1[3], adc2[3], adc3[3];
  float multiplier = 0.1875F; /* ADS1115  @ +/- 6.144V gain (16-bit results) */
  uint8_t flag1,flag2,flag3,flag4;
  pinMode(PWR_A2,INPUT);
  pinMode(PWR_A0,INPUT);
   
  digitalWrite(PWM1,LOW);
  digitalWrite(PWM3,LOW);

  Serial.println("COMPARANDO MUX1 MUX3");
  for(int i=0; i<3; i++){
  adc0[i] = ads.readADC_SingleEnded(0);
  adc1[i] = ads.readADC_SingleEnded(1);
  adc2[i] = ads.readADC_SingleEnded(2);
  adc3[i] = ads.readADC_SingleEnded(3);
 // Serial.print("AIN0: "); Serial.println(adc0[i]*multiplier);
 // Serial.print("AIN1: "); Serial.println(adc1[i]*multiplier);
 // Serial.print("AIN2: "); Serial.println(adc2[i]*multiplier);
 // Serial.print("AIN3: "); Serial.println(adc3[i]*multiplier);
 // Serial.println(" ");
  delay(200);
  }
   if((adc1[0]+adc1[1]+adc1[2])*multiplier/3.0 > 3150.0 && (adc1[0]+adc1[1]+adc1[2])*multiplier/3.0 < 3400.0){
      Serial.print("******************************HUM1 = TEROS10\n");
      flag1 = 0;
      float adc_avg;
      adc_avg = (adc0[0]+adc0[1]+adc0[2])*multiplier/3.0;
      *moisture_value1= 0.4824*powf(adc_avg/1000.0,3) - 2.278*powf(adc_avg/1000.0,2) + 3.898*(adc_avg/1000.0)- 2.154;
  }
  else{
    flag1 = 1;
    pinMode(PWR_A0,OUTPUT);
  }
  
 if((adc3[0]+adc3[1]+adc3[2])*multiplier/3.0 > 3150.0 && (adc3[0]+adc3[1]+adc3[2])*multiplier/3.0 < 3400.0){
      Serial.print("******************************HUM3 = TEROS10\n");
      flag3 = 0;
      float adc_avg;
      adc_avg = (adc2[0]+adc2[1]+adc2[2])*multiplier/3.0;
      *moisture_value3= 00.4824*powf(adc_avg/1000.0,3) - 2.278*powf(adc_avg/1000.0,2) + 3.898*(adc_avg/1000.0)- 2.154;
 }
  else {
    flag3 = 1;
    pinMode(PWR_A2,OUTPUT);
  }

  if(flag1){
     digitalWrite(PWR_A0,HIGH);
     delayMicroseconds(100);
     adc0[0] = ads.readADC_SingleEnded(0);
     adc1[0] = ads.readADC_SingleEnded(1);
     digitalWrite(PWR_A0,LOW);
     delay(100);
     pinMode(PWR_A0,INPUT);
    if((adc1[0])*multiplier < 9.0 ){
      Serial.print("******************************Not sensor detected or properly implemented\n");
        *moisture_value1 = 0.0; 
    }
    else {
      Serial.println("CALCULANDO hum1");
      float resistance;
      resistance = (4700*(adc1[0]*1.0/adc0[0]))/(1- (adc1[0]*1.0/adc0[0]));
      Serial.print("la resistencia es"); Serial.println(resistance);
      if(resistance < 550) *moisture_value1 = 0.0;
      else{
        if(resistance <1000) *moisture_value1 = -20.00*((resistance/1000.0)*(1.0 + 0.018*(tempC - 24))-0.55); 
        else{
          if(resistance <8000) *moisture_value1 = (-3.213*(resistance/1000.0)-4.093)/(1-0.009733*(resistance/1000.0)-0.01205*(tempC));
          else{
            *moisture_value1 = -2.246-5.239*(resistance/1000.0)*(1+0.018*(tempC-24))-0.06756*(resistance/1000.0)*(resistance/1000.0)*((1+0.018*(tempC-24))*(1+0.018*(tempC-24)));
          }
        }
      }
    }
  }
  if(flag3){
     digitalWrite(PWR_A2,HIGH);
     delayMicroseconds(100);
     adc2[0] = ads.readADC_SingleEnded(2);
     adc3[0] = ads.readADC_SingleEnded(3);
     digitalWrite(PWR_A2,LOW);
     delay(100);
     pinMode(PWR_A2,INPUT);
    if((adc3[0])*multiplier < 9.0 ){
      Serial.print("******************************Not sensor detected or properly implemented\n");
        *moisture_value3 = 0.0; 
    }
    else {
      //CODE
      Serial.println("CALCULANDO hum3");
      float resistance;
      resistance = (4700*(adc3[0]*1.0/adc2[0]))/(1- (adc3[0]*1.0/adc2[0]));
      Serial.print("la resistencia es"); Serial.println(resistance);
      if(resistance < 550) *moisture_value3 = 0.0;
      else{
        if(resistance <1000) *moisture_value3 = -20.00*((resistance/1000.0)*(1.0 + 0.018*(tempC - 24))-0.55); 
        else{
          if(resistance <8000) *moisture_value3 = (-3.213*(resistance/1000.0)-4.093)/(1-0.009733*(resistance/1000.0)-0.01205*(tempC));
          else{
            *moisture_value3 = -2.246-5.239*(resistance/1000.0)*(1+0.018*(tempC-24))-0.06756*(resistance/1000.0)*(resistance/1000.0)*((1+0.018*(tempC-24))*(1+0.018*(tempC-24)));
          }
        }
      }
    }
 } 
 
  pinMode(PWR_A2,INPUT);
  pinMode(PWR_A0,INPUT);
  
  digitalWrite(PWM1,HIGH);
  digitalWrite(PWM3,HIGH);

Serial.println("COMPARANDO MUX2 Y MUX4");
  for(int i=0; i<3; i++){
  adc0[i] = ads.readADC_SingleEnded(0);
  adc1[i] = ads.readADC_SingleEnded(1);
  adc2[i] = ads.readADC_SingleEnded(2);
  adc3[i] = ads.readADC_SingleEnded(3);
//  Serial.print("AIN0: "); Serial.println(adc0[i]*multiplier);
//  Serial.print("AIN1: "); Serial.println(adc1[i]*multiplier);
//  Serial.print("AIN2: "); Serial.println(adc2[i]*multiplier);
//  Serial.print("AIN3: "); Serial.println(adc3[i]*multiplier);
//  Serial.println(" ");
  delay(200);
  }
 
  if((adc1[0]+adc1[1]+adc1[2])*multiplier/3.0 > 3150.0 && (adc1[0]+adc1[1]+adc1[2])*multiplier/3.0 < 3400.0){
      Serial.print("******************************HUM2 = TEROS10\n");
      flag2 = 0;
      float adc_avg;
      adc_avg = (adc0[0]+adc0[1]+adc0[2])*multiplier/3.0;
      *moisture_value2= 0.4824*powf(adc_avg/1000.0,3) - 2.278*powf(adc_avg/1000.0,2) + 3.898*(adc_avg/1000.0)- 2.154;
  }
  else{
    flag2 = 1;
    pinMode(PWR_A0,OUTPUT);
  }
  
 if((adc3[0]+adc3[1]+adc3[2])*multiplier/3.0 > 3150.0 && (adc3[0]+adc3[1]+adc3[2])*multiplier/3.0 < 3400.0){
      Serial.print("******************************HUM4 = TEROS10\n");
      flag4 = 0;
      float adc_avg;
      adc_avg = (adc2[0]+adc2[1]+adc2[2])*multiplier/3.0;
      *moisture_value4 = 0.4824*powf(adc_avg/1000.0,3) - 2.278*powf(adc_avg/1000.0,2) + 3.898*(adc_avg/1000.0)- 2.154;
 }
  else {
    flag4 = 1;
    pinMode(PWR_A2,OUTPUT);
  }


  if(flag2){
     digitalWrite(PWR_A0,HIGH);
     delayMicroseconds(100);
     adc0[0] = ads.readADC_SingleEnded(0);
     adc1[0] = ads.readADC_SingleEnded(1);
     digitalWrite(PWR_A0,LOW);
     delay(100);
     pinMode(PWR_A0,INPUT);
    if((adc1[0])*multiplier < 9.0 ){
      Serial.print("******************************Not sensor detected or properly implemented\n");
        *moisture_value2 = 0.0; 
    }
    else {
      Serial.println("CALCULANDO hum2");
      float resistance;
      resistance = (4700*(adc1[0]*1.0/adc0[0]))/(1- (adc1[0]*1.0/adc0[0]));
      Serial.print("la resistencia es"); Serial.println(resistance);
      if(resistance < 550) *moisture_value2 = 0.0;
      else{
        if(resistance <1000) *moisture_value2 = -20.00*((resistance/1000.0)*(1.0 + 0.018*(tempC - 24))-0.55); 
        else{
          if(resistance <8000) *moisture_value2 = (-3.213*(resistance/1000.0)-4.093)/(1-0.009733*(resistance/1000.0)-0.01205*(tempC));
          else{
            *moisture_value2 = -2.246-5.239*(resistance/1000.0)*(1+0.018*(tempC-24))-0.06756*(resistance/1000.0)*(resistance/1000.0)*((1+0.018*(tempC-24))*(1+0.018*(tempC-24)));
          }
        }
      }
    }
  }
  if(flag4){
     digitalWrite(PWR_A2,HIGH);
     delayMicroseconds(100);
     adc2[0] = ads.readADC_SingleEnded(2);
     adc3[0] = ads.readADC_SingleEnded(3);
     digitalWrite(PWR_A2,LOW);
     delay(100);
     pinMode(PWR_A2,INPUT);
    if((adc3[0])*multiplier < 9.0 ){
      Serial.print("******************************Not sensor detected or properly implemented\n");
      *moisture_value4 = 0.0;
    }
    else {
      //CODE
      Serial.println("CALCULANDO hum4");
      float resistance;
      resistance = (4700*(adc3[0]*1.0/adc2[0]))/(1- (adc3[0]*1.0/adc2[0]));
      Serial.print("la resistencia es"); Serial.println(resistance);
      if(resistance < 550) *moisture_value4 = 0.0;
      else{
        if(resistance <1000) *moisture_value4 = -20.00*((resistance/1000.0)*(1.0 + 0.018*(tempC - 24))-0.55); 
        else{
          if(resistance <8000) *moisture_value4 = (-3.213*(resistance/1000.0)-4.093)/(1-0.009733*(resistance/1000.0)-0.01205*(tempC));
          else{
            *moisture_value4 = -2.246-5.239*(resistance/1000.0)*(1+0.018*(tempC-24))-0.06756*(resistance/1000.0)*(resistance/1000.0)*((1+0.018*(tempC-24))*(1+0.018*(tempC-24)));
          }
        }
      }
    }
 } 
}
/////////////////////////////////////////////////

//SD Card module ///////////////////////
void initialize_SD(uint8_t SD_CS){
      SD.begin(SD_CS);
      if(!SD.begin(SD_CS)){
        Serial.println(F("Card Mount Failed"));
        ESP.restart();
      }
      uint8_t cardType = SD.cardType();
      if(cardType == CARD_NONE){
        Serial.println(F("No SD card attached"));
         ESP.restart();
      }
      Serial.println(F("Initializing SD card..."));
}

void initialize_File(const char* filename, const char* initial_message){
  File file = SD.open(filename);
      if(!file) {
        Serial.println(F("File doens't exist"));
        Serial.println(F("Creating file..."));
        writeFile(SD, filename, initial_message);
      }
      else {
        Serial.println(F("File already exists"));  
      }
      file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println(F("Failed to open file for writing"));
        return;
    }
    if(file.print(message)){
        Serial.println(F("File written"));
    } else {
        Serial.println(F("Write failed"));
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println(F("Failed to open file for appending"));
        return;
    }
    if(file.print(message)){
        Serial.println(F("Message appended"));
    } else {
        Serial.println(F("Append failed"));
    }
    file.close();
}
///////////////////////////////////////

//Deep Sleep ///////////////////////////
uint8_t print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason){
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println(F("Wakeup caused by external signal using RTC_IO")); return 1; break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println(F("Wakeup caused by external signal using RTC_CNTL")); return 2; break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println(F("Wakeup caused by timer")); return 2; break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println(F("Wakeup caused by touchpad")); return 4; break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println(F("Wakeup caused by ULP program")); return 5; break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); return 0 ;break;
  }
}
//////////////////////////////////////////////////

//Timestamp /////////////////////////////////////
void init_ds321(){
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    abort();
    ESP.restart();
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}

void printLocalTime(char * time_date){
  char date[30];
  DateTime now = rtc.now();
 // DateTime now (now + TimeSpan(0,0,0,35));
  //tm struct to string format information: http://www.cplusplus.com/reference/ctime/strftime/
  if(OUTPUT_DATA_FORMAT){
    sprintf(date,"%02d/%02d/%4d,%02d:%02d:%02d",now.day(),now.month(),now.year(),now.hour(),now.minute(),now.second());
    strcpy(time_date,date);
  }
  else{
    sprintf(date,"%02d/%02d/%4d - %02d:%02d:%02d",now.day(),now.month(),now.year(),now.hour(),now.minute(),now.second());
    strcpy(time_date,date);
  }
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
  Serial.println(time_date);
}
///////////////////////////////////////////////////////


void setup() {
  //Disconnect WiFi and BT as they are no needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();
  gpio_hold_dis(pwr_pin);
  gpio_hold_dis(pwr_sen);
  pinMode(PWR_PIN,OUTPUT);
  pinMode(PWR_SEN,OUTPUT);
  pinMode(LED_PIN,OUTPUT);
  pinMode(EN1,OUTPUT);
  pinMode(EN2,OUTPUT);
  digitalWrite(LED_PIN,LOW);
  digitalWrite(PWR_PIN,LOW);
  digitalWrite(PWR_SEN,LOW);
  delay(100);
  gpio_hold_en(led_pin);
  digitalWrite(PWR_PIN,HIGH);
  digitalWrite(PWR_SEN,HIGH);
  Serial.begin(115200);
  delay(100);           /* Take some time to open up the Serial Monitor */
  //attachInterrupt(EXT_INTERRUPT_PIN, isr, EXT_INTERRUPT_SOURCE);
  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
 //Executed during the first boot
  if(bootCount==1){
    initialize_SD(SD_CS_PIN);
    if(OUTPUT_DATA_FORMAT){
      char* init_message = "Measurement,Date(dd/mm/yy),Time(hh:mm:ss),Soil Moisture(1),Soil Moisture(2),Soil Moisture(3),Soil Moisture(4),Soil Temperature(1),Soil Temperature(2),Soil Temperature(3)\n";
      strcat(file_name_type,file_name);
      strcat(file_name_type,".csv");
      initialize_File(file_name_type,init_message);
    }
    else {
      char* init_message ="\t\tMeasurement of soil parameters\n\n";
      strcat(file_name_type,file_name);
      strcat(file_name_type,".txt");
      initialize_File(file_name_type,init_message);
    }
  }

  print_wakeup_reason();

  if(bootCount>=2){
    initialize_SD(SD_CS_PIN);

    char date_time[30],counter_format[25];
    float moisture_val1=0.0, moisture_val2=0.0, moisture_val3=0.0, moisture_val4=0.0, temperature_val1=0.0, temperature_val2=0.0, temperature_val3=0.0;

      get_temperature_measure(&temperature_val1,&temperature_val2,&temperature_val3);
      get_moisture_measure(&moisture_val1,&moisture_val2,&moisture_val3,&moisture_val4);

    init_ds321();
    printLocalTime(date_time);
    String message;

    if(OUTPUT_DATA_FORMAT){
      message=String(bootCount-1)+ ","+ String(date_time)+ ","+ String(moisture_val1,2)+ ","+
              String(moisture_val2,2)+ ","+ String(moisture_val3,2)+ ","+ String(moisture_val4,2)+ ","+
              String(temperature_val1,2)+ ","+ String(temperature_val2,2)+ ","+ String(temperature_val3,2)+
              ","+"\n";
    }
    else{
      sprintf(counter_format,"%6d.- Timestamp:",bootCount-1);
      message= String(counter_format)+ "            " + String(date_time)+" \r\n"+
              "\t Soil Moisture(1):      "+ String(moisture_val1,2)+ " cbar / (m^3/m^3) \r\n"+
              "\t Soil Moisture(2):      "+ String(moisture_val2,2)+ " cbar / (m^3/m^3) \r\n"+
              "\t Soil Moisture(3):      "+ String(moisture_val3,2)+ " cbar / (m^3/m^3) \r\n"+
              "\t Soil Moisture(4):      "+ String(moisture_val4,2)+ " cbar / (m^3/m^3) \r\n"+
              "\t Soil Temperature(1):   "+ String(temperature_val1,2)+ " °C \r\n"+
              "\t Soil Temperature(2):   "+ String(temperature_val2,2)+ " °C \r\n"+
              "\t Soil Temperature(3):   "+ String(temperature_val3,2)+ " °C \r\n\n";  
    }
    Serial.print(F("Save data:  \n"));
    Serial.println(message);
    appendFile(SD,file_name_type,message.c_str());
  }
  //ESP_ERROR_CHECK( esp_sleep_enable_ulp_wakeup() );
//  esp_sleep_enable_ext0_wakeup(EXT_WAKEUP_PIN,WAKEUP_STATE);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  digitalWrite(PWR_PIN,LOW);
  digitalWrite(PWR_SEN,LOW);
  gpio_hold_en(pwr_pin);
  gpio_hold_en(pwr_sen);
 //gpio_deep_sleep_hold_en();
  Serial.flush(); 
  esp_deep_sleep_start();
}

void loop() {
  ESP.restart();
}
