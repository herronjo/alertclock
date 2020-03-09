#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include <RTClib.h>
#include <EEPROM.h>

Adafruit_7segment matrix = Adafruit_7segment();
RTC_DS1307 rtc;

bool displayColon = true;
byte colonTmp = 0;
byte curHour = 0;
bool PM = false;
bool alarmOn = false;
bool inCall = false;
int iteration = 0;
String incoming;
int alarmHour = 0;
int alarmMinute = 0;
int displayAHour = 12;
int number [10];
String toDial;
byte displayNum = 0x00;
bool doSetup = false;
int numi = -1;
int curVal = 0;
int lastVal = 0;
int valRotary,lastValRotary;
int encoder0Pos = 0;
bool setTheAlarm = false;
uint8_t blank = 0;
bool tmpb = false;
String data [6];

void setup() {
  Serial.begin(115200);
  Serial.print("AlertClock firmware v1.0\r\n©2019 Joshua Herron - All rights reserved\r\n");
  rtc.begin();
  matrix.begin(0x70);
  matrix.print(0xF00D, HEX);
  matrix.drawColon(true);
  matrix.writeDisplay();
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);
  delay(500);
  digitalWrite(8, LOW);
  Serial2.begin(115200);
  delay(2000);
  Serial2.print("ATE0\r");
  delay(2000);
  Serial2.print("AT+CREG=1\r");
  delay(2000);
  Serial2.print("AT+CFUN=1\r");
  delay(2000);
  Serial2.print("AT+CMGF=1\r");
  delay(2000);
  Serial2.print("AT+CSDVC=2,1\r");
  delay(2000);
  Serial2.print("AT+CRSL=2\r");
  delay(2000);
  Serial2.print("AT+CNMI=1,2,0,0,0\r");
  delay(2000);
  Serial2.print("AT+CHTPSERV=\"ADD\",\"www.google.com\",80,1\r");
  delay(2000);
  for (int i12 = 0; i12 < 5; i12++) {
    incoming += Serial2.readString();
  }
  incoming = "";
  delay(2000);
  Serial2.print("AT+CPTONE=1\r");
  pinMode(A0, OUTPUT);
  pinMode(2, INPUT);
  if (EEPROM.read(0) == 0) {
    doSetup = true;
    matrix.drawColon(false);
    matrix.writeDigitRaw(0, blank);
  } else {
    alarmHour = EEPROM.read(11);
    alarmMinute = EEPROM.read(12);
    for (byte i = 1; i < 11; i++) {
      number[i-1] = EEPROM.read(i);
    }
    toDial = "ATD+1";
    for (byte i = 0; i < 10; i++) {
      toDial.concat(number[i]);
    }
    toDial.concat(";\r");
  }
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  pinMode(18, INPUT_PULLUP);
  pinMode(19, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), alarmToggle, FALLING);
  attachInterrupt(digitalPinToInterrupt(3), alert, FALLING);
  attachInterrupt(digitalPinToInterrupt(19), setAlarm, FALLING);
  attachInterrupt(digitalPinToInterrupt(18), doEncoder, CHANGE);
  setTheAlarm = false;
}

void loop() {
  if (doSetup) {
    lastVal = curVal;
    curVal = digitalRead(6);
    if (curVal > lastVal) {
      if (numi < 9) {
        numi = numi+1;
      } else {
        doSetup = false;
        for (byte itmp = 0; itmp < 10; itmp++) {
          EEPROM.write(itmp+1, number[itmp]);
        }
        toDial = "ATD+1";
        for (byte i = 0; i < 10; i++) {
          toDial.concat(number[i]);
        }
        toDial.concat(";\r");
        EEPROM.write(0, 1);
      }
    }
    if (valRotary > lastValRotary) {
      number[numi] = abs(number[numi]+1)%10;
    }
    if (valRotary < lastValRotary) {
      number[numi] = number[numi]-1;
      if (number[numi] < 0) {
        number[numi] = 9;
      }
    }
    lastValRotary = valRotary;
    if (numi < 3) {
      matrix.writeDigitNum(1, number[0]);
      matrix.writeDigitNum(3, number[1]);
      matrix.writeDigitNum(4, number[2]);
    } else if (numi < 6) {
      matrix.writeDigitNum(1, number[3]);
      matrix.writeDigitNum(3, number[4]);
      matrix.writeDigitNum(4, number[5]);
    } else {
      matrix.writeDigitNum(0, number[6]);
      matrix.writeDigitNum(1, number[7]);
      matrix.writeDigitNum(3, number[8]);
      matrix.writeDigitNum(4, number[9]);
    }
    matrix.writeDisplay();
  } else if (setTheAlarm) {
    if (valRotary > lastValRotary) {
      if (!tmpb) {
        alarmHour = abs(alarmHour+1)%24;
      } else {
        alarmMinute = abs(alarmMinute+1)%60;
      }
    }
    if (valRotary < lastValRotary) {
      if (!tmpb) {
        alarmHour = alarmHour-1;
        if (alarmHour < 0) {
          alarmHour = 23;
        }
      } else {
        alarmMinute = alarmMinute-1;
        if (alarmMinute < 0) {
          alarmMinute = 59;
        }
      }
    }
    lastValRotary = valRotary;
    displayAHour = alarmHour%12;
    if (displayAHour == 0) {
      displayAHour = 12;
    }
    matrix.print((displayAHour*100)+alarmMinute,DEC);
    matrix.writeDigitRaw(2, 0x02);
    if (alarmHour >= 12) {
      matrix.writeDigitRaw(2, 0x06);
    }
    matrix.writeDisplay();
    lastVal = curVal;
    curVal = digitalRead(6);
    if (curVal > lastVal) {
      if (!tmpb) {
        tmpb = true;
      } else {
        tmpb = false;
        setTheAlarm = false;
        EEPROM.write(11, alarmHour);
        EEPROM.write(12, alarmMinute);
      }
    }
  } else {
    lastVal = curVal;
    curVal = digitalRead(6);
    if (curVal > lastVal) {
      iteration = 300;
    }
    if (iteration == 300) {
      Serial2.print("AT+CHTPUPDATE\r");
    }
    if (iteration == 305) {
      Serial2.print("AT+CCLK?\r");
    }
    if (iteration == 307) {
      incoming = "";
      while (Serial2.available() > 0) {
        incoming += Serial2.readString();
      }
      if (incoming.indexOf("+CCLK: ") > 0) {
        incoming = incoming.substring(incoming.indexOf("+CCLK: "), incoming.indexOf("+CCLK: ")+29);
        data[3] = incoming.substring(11,13);
        data[4] = incoming.substring(14,16);
        data[5] = incoming.substring(8,10);
        data[0] = incoming.substring(17,19);
        data[1] = incoming.substring(20,22);
        data[2] = incoming.substring(23,25);
        rtc.adjust(DateTime(2000+data[5].toInt(), data[3].toInt(), data[4].toInt(), data[0].toInt(), data[1].toInt(), data[2].toInt()));
      }
      iteration = 0;
    }
    DateTime now = rtc.now();
    if (alarmOn && now.hour() == alarmHour && now.minute() == alarmMinute) {
      if (displayColon) {
        digitalWrite(A0, HIGH);
      } else {
        digitalWrite(A0, LOW);
      }
    } else {
      digitalWrite(A0, LOW);
    }
    if (alarmOn && now.hour() == alarmHour && now.minute() == alarmMinute+1 && !inCall) {
      Serial2.print(toDial);
      inCall = true;
    }
    curHour = now.hour();
    if (now.hour() >= 12) {
      PM = true;
    } else {
      PM = false;
    }
    if (curHour > 12) {
      curHour = curHour-12;
    }
    if (curHour == 0) {
      curHour = 12;
    }
    matrix.print((curHour*100)+now.minute(),DEC);
    if (colonTmp < 2) {
      displayColon = true;
    } else {
      displayColon = false;
    }
    if (colonTmp == 3) {
      colonTmp = 0;
    }
    displayNum = 0x00;
    if (displayColon) {
      displayNum = displayNum + 0x02;
    }
    if (PM == true) {
      displayNum = displayNum + 0x04;
    }
    if (alarmOn) {
      displayNum = displayNum + 0x08;
    }
    if (inCall) {
      displayNum = displayNum + 0x10;
    }
    matrix.writeDigitRaw(2, displayNum);
    if ((now.hour() >= 21) || (now.hour() <= 7)) {
      matrix.setBrightness(0);
    } else {
      matrix.setBrightness(15);
    }
    matrix.writeDisplay();
    colonTmp = colonTmp+1;
    iteration = iteration+1;
  }
  if (!doSetup && !setTheAlarm) {
    delay(500);
  } else {
    delay(250);
  }
}

void alarmToggle() {
  alarmOn = !alarmOn;
}

void alert() {
  if (inCall) {
    Serial2.print("AT+CHUP\r");
  } else {
    Serial2.print(toDial);
  }
  inCall = !inCall;
}

void doEncoder() {
  if (digitalRead(18) == digitalRead(7)) {
    encoder0Pos--;
  } else {
    encoder0Pos++;
  }
  valRotary = encoder0Pos/2.5;
}

void setAlarm() {
  setTheAlarm = !setTheAlarm;
  tmpb = false;
  lastVal = 0;
  curVal = 1000;
}
