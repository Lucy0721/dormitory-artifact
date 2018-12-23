#include "userDef.h"
#include <U8glib.h> 
#include "oled.h"
#include <Microduino_Key.h>
#include <Microduino_ColorLED.h>
#include <ESP8266.h>
#include "audio.h"
#include <Wire.h>
#include <EEPROM.h>
#include <Rtc_Pcf8563.h>

Rtc_Pcf8563 rtc;
#ifdef ESP32
#error "This code is not recommended to run on the ESP32 platform! Please check your Tools->Board setting."
#endif

/**
**CoreUSB UART Port: [Serial1] [D0,D1]
**Core+ UART Port: [Serial1] [D2,D3]
**/
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1284P__) || defined (__AVR_ATmega644P__) || defined(__AVR_ATmega128RFA1__)
#define EspSerial Serial1
#define UARTSPEED  115200
#endif

/**
**Core UART Port: [SoftSerial] [D2,D3]
**/
#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__)
#include <SoftwareSerial.h>
//SoftwareSerial mySerial(2, 3); /* RX:D2, TX:D3 */

#define EspSerial mySerial
#define UARTSPEED  9600
#endif

#define SSID        "Honor 10"
#define PASSWORD    "zmf1124072183"
#define HOST_NAME   F("api.heclouds.com")
#define HOST_PORT   (80)
ESP8266 wifi(&EspSerial);
static const byte  GETDATA[]  PROGMEM = {
  "GET https://api.heclouds.com/devices/503139172/datapoints?datastream_id=id,status,vol&limit=1 HTTP/1.1\r\napi-key: ZA0V=3BEajfyhX59diDNA=qHSfs=\r\nHost: api.heclouds.com\r\nConnection: close\r\n\r\n"
};
int rollCount=0,rollRoomMembers=0;
//define another three variables...
bool volChange=false,statusChange=false,idChange=false;
int music_status=0,temp_music_status=0;
int music_vol=10,temp_music_vol=10;
int music_num_MAX=9;
int current_music=1,temp_current_music=1;
bool isConnected=false;
bool bottomBar=true;
bool canPlay=false;


DigitalKey KeyButton(keyPin);

ColorLED strip = ColorLED(1, LEDPIN);

void setup(void) {
  Serial.begin(9600);
  pinMode(micPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(buzzer_pin,OUTPUT);
  pinMode(LEDPIN, OUTPUT);
  strip.begin();
  strip.show();
  delay(3000);
  initTime();

  while (!Serial); // wait for Leonardo enumeration, others continue immediately
  Serial.print(F("setup begin\r\n"));
  delay(100);

  WifiInit(EspSerial, UARTSPEED);

  Serial.print(F("FW Version: "));
  Serial.println(wifi.getVersion().c_str());


  if (wifi.setOprToStation()) {
    Serial.print(F("to station ok\r\n"));
  } else {
    Serial.print(F("to station err\r\n"));
  }

  if (wifi.joinAP(SSID, PASSWORD)) {
    Serial.print(F("Join AP success\r\n"));
    Serial.print(F("IP: "));
    Serial.println(wifi.getLocalIP().c_str());
    isConnected=true;
  } else {
    Serial.print(F("Join AP failure\r\n"));
    isConnected=false;
  }

  Serial.print(F("setup end\r\n"));
  wifi.disableMUX();
  //initialize  audio module 
  audio_init(DEVICE_TF,MODE_One_END,music_vol); 
}


void initTime() {
  rtc.initClock();
  //set a time to start with.
  //day, weekday, month, century(1=1900, 0=2000), year(0-99)
  rtc.setDate(4, 1, 12, 0, 18);
  //hr, min, sec
  rtc.setTime(0, 59, 0);
}





 void updateOLED() {
  //OLED display
  if (OLEDShowTime > millis()) OLEDShowTime = millis();
  if(millis()-OLEDShowTime>INTERVAL_OLED) {
    OLEDShow(); //调用显示库
    OLEDShowTime = millis();
  } 
}

double getDB() {
  int voice_data = analogRead(micPin);
  voice_data=map(voice_data,0,1023,0,5);
  double db = (20. * log(10)) * (voice_data / 1.0);
  if(db>recodeDB) {
    recodeDB=db;
  }
  //Serial.println(db);
  return db;
}

void analyticDB(double db) {
  if(db > voice) {
    numNoise++;
    //Serial.println(numNoise);
  }
  if (analytic_time > millis()) analytic_time = millis();
  if (millis() - analytic_time > INTERVALOLED) {
    if(numNoise>maxNoise) {
        i = 200;
        isAlaram= true;
    }
//     Serial.print(numNoise);
//    Serial.print("\t");
//    Serial.println(maxNoise);
    numNoise=0;
    analytic_time = millis();
  }
}


void buzzer() {
  if (millis() - timer > 10) {
    if (!add) {
      i++;
      if (i >= 800)
        add = true;
    } else {
      i--;
      if (i <= 200) {
        add = false;
      }
    }
    tone(buzzerPin, i);
    timer = millis();
  }
}





void updateButton() {
    if(KeyButton.readEvent()==SHORT_PRESS) {
      delay(15);
      recodeDB=0;
      isAlaram = false;
    }
}



void speakerDoing(boolean isAlaram) {
  if (isAlaram) {
    buzzer();
    strip.setPixelColor(0, strip.Color(125, 125, 125));
    strip.show();
  } else {
    noTone(buzzerPin);
    strip.setPixelColor(0, strip.Color(0, 0, 0));
    strip.show();
  }
}




void getCurrentTime() {

  rtc.formatDate();
  rtc.formatTime();

  timeHH=rtc.getHour();
  timeMM=rtc.getMinute();
  timeSS=rtc.getSecond();
  year=2000+rtc.getYear();
  month=rtc.getMonth();
  day=rtc.getDay();


  stringDate=String(year);
  stringDate+="/";
  stringDate+=month;
  stringDate+="/";
  stringDate+=day;

  stringTime=String(timeHH);
  stringTime+=":";
  stringTime+=timeMM;
  stringTime+=":";
  stringTime+=timeSS;

}

void updateTime() {
  //update GPS and UT during INTERVAL_GPS
  if (Time_millis > millis()) Time_millis = millis();
  if(millis()-Time_millis>INTERVAL_Time) {
    getCurrentTime();
    Time_millis = millis();
  } 
}

void updateAlarm() {

  if(timeHH==1&&timeMM<5) {
        Serial.println(timeHH);
        Serial.println(micValue);
   //     Serial.println(timeMM);  
        delay(1000);
      if(micValue>500) {
        isRoar=true;
      }
      if(isRoar) {
        noTone(buzzer_pin);
      } else {
        tone(buzzer_pin,800); //在端口输出频率  
      }
  } 
  else {
      isRoar=false;
      noTone(buzzer_pin);
  }
}

void updateMic() {
  micValue = analogRead(micPin);
  //value = map(micValue, 0, 1023, 0, 255);
  //Serial.println(micValue);
}






void drawNotConnected(){
  //it will draw like this:✡ (NO BORDER!!)
  u8g.drawTriangle(64,4,32,48,96,48);
  u8g.drawTriangle(64,60,32,16,96,16);
}
bool networkHandle() {
  //do something with net work ,include handle response message.
  canPlay=true;
  uint8_t buffer[415]={0};
  uint32_t len = wifi.recv(buffer, sizeof(buffer), 2000);
  if (len > 0) {
    for (uint32_t i = 0; i < len; i++) {
      Serial.print((char)buffer[i]);
    }
  }
  //the ram of the device is too limited,so i enhered the length of response message,at specific index,there are nessicity value.
//  272，273 vol
//344 id
//414 status
  temp_music_vol=((int)buffer[272]-48)*10+((int)buffer[273]-48)-10;
  temp_current_music=(int)buffer[344]-48;
  temp_music_status=(int)buffer[414]-48;
  Serial.println(temp_music_vol);
  Serial.println(temp_current_music);
  Serial.println(temp_music_status);  
  wifi.releaseTCP();
}
void mp3Handle(){
  //Handle play things.
  if(canPlay){
  if(current_music!=temp_current_music){
    idChange=true;
    current_music=temp_current_music;
  }
  if(music_vol!=temp_music_vol){
    volChange=true;
    music_vol=temp_music_vol;
  }
  if(music_status!=temp_music_status){
    statusChange=true;
    music_status=temp_music_status;
  }
  if(statusChange){
    if(music_status==1){
      audio_play();
    }else{
      audio_pause();
    }
  }
  if(volChange){
    audio_vol(music_vol);
  }
  if(idChange){
    audio_choose(current_music+1);
    audio_play();
  }
  volChange=false;
  idChange=false;
  statusChange=false;
}
}

void loop(void) {
  
  double db=getDB();//获得分贝数
  analyticDB(db);//分析分贝
  speakerDoing(isAlaram);//蜂鸣器处理
  updateButton();//按键检测
  updateOLED();//刷新OLED
  updateMic();
  updateTime();
  updateAlarm();

  if(isConnected){
    if (wifi.createTCP(HOST_NAME, HOST_PORT)) {
      Serial.print(F("TCP\n"));
      isConnected=true;
    } else {
      Serial.print(F("No TCP\n"));
      isConnected=true;
    }
    wifi.sendFromFlash(GETDATA, sizeof(GETDATA)); 
    networkHandle();
    mp3Handle();
    u8g.firstPage();
  }
  else
  {
    u8g.firstPage();
    do{
      drawNotConnected();
    }while(u8g.nextPage());
    delay(5000);
    setup();
  }
}
 








