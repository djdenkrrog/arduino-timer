#include <MsTimer2.h>
#include <Bounce2.h>
#include <Wire.h>
#include <EEPROM.h> // подключаем библиотеку EEPROM
#include <TM1637.h>
#include "DateTime.h"

#define VERSION "1.0.1"

#define DEFAULT_PRESSED_DELLAY 2000
#define DEFAULT_CHECK_POWER_STATE_DELAY 10
#define DEFAULT_DELLAY_FOR_CHANGE_MODE_SHOW_TIME 40

#define DS1307_I2C_ADDRESS 0x68

#define btnModePin 11
#define btnPlusPin 12
#define btnMinusPin 10
#define powerPin 13

#define CLK 9 //pins definitions for TM1637 and can be changed to other ports
#define DIO 8
TM1637 tm1637(CLK, DIO);
int8_t timeDisp[] = {0x00, 0x00, 0x00, 0x00};
unsigned char ClockPoint = 1;

typedef enum {
  mShowTime, mSetTimeM, mSetTimeH,
  mShowTimeOn, mSetTimeOnM, mSetTimeOnH,
  mShowTimeOff, mSetTimeOffM, mSetTimeOffH
} enumMode;
enumMode modeOpertion;

typedef enum {btnPlus, btnMinus, btnMode,  btnNone} enumBtn;
enumBtn btnPressed;

bool btnModeLongPress = false;

//Time On and Off light
uint8_t hOn, mOn, hOff, mOff;

// Current Time
uint8_t hour, minute, second;
uint16_t timeOn; //count minutes after 00:00
uint16_t timeOff; //count minutes after 00:00

bool powerOn = false; //for manual power on/off


Bounce bouncerMode = Bounce(); //создаем экземпляр класса Bounce
Bounce bouncerPlus = Bounce(); //создаем экземпляр класса Bounce
Bounce bouncerMinus = Bounce(); //создаем экземпляр класса Bounce

unsigned long timerTime = 0; //
uint16_t timerDellay = 500; //Задержка в мс 1000мс = 1с
uint8_t timerDellayForCheckPowerState = 0; //Задержка для проверки совпадения времени, чтоб часто не дергать долгий расчет
uint8_t timerDellayForChangeModeShowTime = 0; //Задержка после которой включится показ времени, если не нажимаются кнопки

unsigned long pressedMoment = 0; //Время удержания кнопки
unsigned long pressedDellay = DEFAULT_PRESSED_DELLAY; //Задержка в мс 1000мс = 1с

//==========================================================

bool timer()
{
  if (timerTime == 0) {
    timerTime = millis();
    return false;
  }

  if (millis() - timerTime < timerDellay) {
    return false;
  }

  timerTime = millis();
  return true;
}

uint8_t decToBcd(uint8_t val)
{
  return ( (val / 10 * 16) + (val % 10) );
}

uint8_t bcdToDec(uint8_t val)
{
  return ( (val / 16 * 10) + (val % 16) );
}

void setDateDs1307(uint8_t second, // 0-59
                   uint8_t minute, // 0-59
                   uint8_t hour, // 1-23
                   uint8_t dayOfWeek, // 1-7
                   uint8_t dayOfMonth, // 1-28/29/30/31
                   uint8_t month, // 1-12
                   uint8_t year) // 0-99
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}

void setDateDs1307(uint8_t hour, // 1-23
                   uint8_t minute) // 0-59
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.write(decToBcd(0));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(1));
  Wire.write(decToBcd(1));
  Wire.write(decToBcd(1));
  Wire.write(decToBcd(1));
  Wire.endTransmission();
}

void getDateDs1307()
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);

  second = bcdToDec(Wire.read() & 0x7f);
  minute = bcdToDec(Wire.read());
  hour   = bcdToDec(Wire.read() & 0x3f);
}

void setDateTime() {
  DateTime date(__DATE__, __TIME__);
  setDateDs1307(date.second(), date.minute(), date.hour(), date.dayOfTheWeek(), date.day(), date.month(), date.year());
}

void setDateTimeShort(uint8_t hour, uint8_t minute) {
  setDateDs1307(hour, minute);
}

void saveTimeOn()
{
  EEPROM.update(0, hOn); // записываем 1-ю ячейку
  EEPROM.update(1, mOn); // записываем 2-ю ячейку
  timeOn = hOn * 60 + mOn; //count minutes after 00:00
}

void saveTimeOff()
{
  EEPROM.update(2, hOff); // записываем 3-ю ячейку
  EEPROM.update(3, mOff); // записываем 4-ю ячейку
  timeOff = hOff * 60 + mOff; //count minutes after 00:00
}

void setTimeDisp(uint8_t hour, uint8_t minute)
{
  timeDisp[0] = hour / 10;
  timeDisp[1] = hour % 10;
  timeDisp[2] = minute / 10;
  timeDisp[3] = minute % 10;
}

void viewCurrTime()
{
  ClockPoint = (~ClockPoint) & 0x01;
  if (ClockPoint) {
    tm1637.point(POINT_ON);
  } else {
    tm1637.point(POINT_OFF);
  }

  setTimeDisp(hour, minute);
  tm1637.display(timeDisp);
}

void viewSetTimeH()
{
  setTimeDisp(hour, minute);
  ClockPoint = (~ClockPoint) & 0x01;
  if (ClockPoint) {
    tm1637.point(POINT_ON);
    timeDisp[0] = 16;
    timeDisp[1] = 16;
  } else {
    tm1637.point(POINT_OFF);
  }
  tm1637.display(timeDisp);
}

void viewSetTimeM()
{
  setTimeDisp(hour, minute);
  ClockPoint = (~ClockPoint) & 0x01;
  if (ClockPoint) {
    tm1637.point(POINT_ON);
    timeDisp[2] = 16; //clear minute
    timeDisp[3] = 16; //clear minute
  } else {
    tm1637.point(POINT_OFF);
  }
  tm1637.display(timeDisp);
}

void viewTimeOn()
{
  setTimeDisp(hOn, mOn);
  tm1637.point(POINT_ON);
  tm1637.display(timeDisp);;
}

void viewSetTimeOnH()
{
  setTimeDisp(hOn, mOn);
  tm1637.point(POINT_ON);
  ClockPoint = (~ClockPoint) & 0x01;
  if (ClockPoint) {
    timeDisp[0] = 16; //clear hour
    timeDisp[1] = 16; //clear hour
  }
  tm1637.display(timeDisp);
}

void viewSetTimeOnM()
{
  setTimeDisp(hOn, mOn);
  tm1637.point(POINT_ON);
  ClockPoint = (~ClockPoint) & 0x01;
  if (ClockPoint) {
    timeDisp[2] = 16; //clear minute
    timeDisp[3] = 16; //clear minute
  }
  tm1637.display(timeDisp);
}

void viewTimeOff()
{
  setTimeDisp(hOff, mOff);
  tm1637.point(POINT_OFF);
  tm1637.display(timeDisp);
}

void viewSetTimeOffH()
{
  setTimeDisp(hOff, mOff);
  tm1637.point(POINT_OFF);
  ClockPoint = (~ClockPoint) & 0x01;
  if (ClockPoint) {
    timeDisp[0] = 16; //clear hour
    timeDisp[1] = 16; //clear hour
  }
  tm1637.display(timeDisp);
}

void viewSetTimeOffM()
{
  setTimeDisp(hOff, mOff);
  tm1637.point(POINT_OFF);
  ClockPoint = (~ClockPoint) & 0x01;
  if (ClockPoint) {
    timeDisp[2] = 0x7f; //clear minute
    timeDisp[3] = 0x7f; //clear minute
  }
  tm1637.display(timeDisp);
}

//Show date on dispaly
void dataView()
{
  switch (modeOpertion) {
  case mShowTime: viewCurrTime(); break;
  case mSetTimeH: viewSetTimeH(); break;
  case mSetTimeM: viewSetTimeM(); break;
  case mShowTimeOn: viewTimeOn(); break;
  case mSetTimeOnH: viewSetTimeOnH(); break;
  case mSetTimeOnM: viewSetTimeOnM(); break;
  case mShowTimeOff: viewTimeOff(); break;
  case mSetTimeOffH: viewSetTimeOffH(); break;
  case mSetTimeOffM: viewSetTimeOffM(); break;
  }
}

void prin2dig(uint8_t val)
{
  if (val < 10) {
    Serial.print(0, DEC);
  }
  Serial.print(val, DEC);
}

void incrementTimeOnH()
{
  if (hOn + 1 > 23) {
    hOn = 0;
    return;
  }
  hOn = hOn + 1;
}

void incrementTimeOnM()
{
  if (mOn + 1 > 59) {
    mOn = 0;
    return;
  }
  mOn = mOn + 1;
}

void incrementTimeOffH()
{
  if (hOff + 1 > 23) {
    hOff = 0;
    return;
  }
  hOff = hOff + 1;
}

void incrementTimeOffM()
{
  if (mOff + 1 > 59) {
    mOff = 0;
    return;
  }
  mOff = mOff + 1;
}

void incrementDateTimeH()
{
  uint8_t hNew;
  hNew = hour;

  if (hNew + 1 > 23) {
    hNew = 0;
  } else {
    hNew = hNew + 1;
  }

  setDateTimeShort(hNew, minute);
}

void incrementDateTimeM()
{
  uint8_t mNew;
  mNew = minute;

  if (mNew + 1 > 59) {
    mNew = 0;
  } else {
    mNew = mNew + 1;
  }

  setDateTimeShort(hour, mNew);
}

void incrementTime()
{
  switch (modeOpertion) {
  case mSetTimeH: incrementDateTimeH(); break;
  case mSetTimeM: incrementDateTimeM(); break;
  case mSetTimeOnH: incrementTimeOnH(); break;
  case mSetTimeOnM: incrementTimeOnM(); break;
  case mSetTimeOffH: incrementTimeOffH(); break;
  case mSetTimeOffM: incrementTimeOffM(); break;
  default:
    powerOn = true;
    timerDellayForCheckPowerState = DEFAULT_CHECK_POWER_STATE_DELAY; //Чтоб сразу включилось
    break;
  }
}

void decrementTimeOnH()
{
  if (hOn - 1 < 0) {
    hOn = 23;
  } else {
    hOn = hOn - 1;
  }
}

void decrementTimeOnM()
{
  if (mOn - 1 < 0) {
    mOn = 59;
  } else {
    mOn = mOn - 1;
  }
}

void decrementTimeOffH()
{
  if (hOff - 1 < 0) {
    hOff = 23;
  } else {
    hOff = hOff - 1;
  }
}

void decrementTimeOffM()
{
  if (mOff - 1 < 0) {
    mOff = 59;
  } else {
    mOff = mOff - 1;
  }
}

void decrementDateTimeH()
{
  uint8_t hNew;
  hNew = hour;

  if (hNew - 1 < 0) {
    hNew = 23;
  } else {
    hNew = hNew - 1;
  }
  setDateTimeShort(hNew, minute);
}

void decrementDateTimeM()
{
  uint8_t mNew;
  mNew = minute;

  if (mNew - 1 < 0) {
    mNew = 59;
  } else {
    mNew = mNew - 1;
  }
  setDateTimeShort(hour, mNew);
}

void decrementTime()
{
  switch (modeOpertion) {
  case mSetTimeH: decrementDateTimeH(); break;
  case mSetTimeM: decrementDateTimeM(); break;
  case mSetTimeOnH: decrementTimeOnH(); break;
  case mSetTimeOnM: decrementTimeOnM(); break;
  case mSetTimeOffH: decrementTimeOffH(); break;
  case mSetTimeOffM: decrementTimeOffM(); break;
  default:
    powerOn = false;
    timerDellayForCheckPowerState = DEFAULT_CHECK_POWER_STATE_DELAY; //Чтоб сразу выключилось
    break;
  }
}

void changeModeLongPress()
{
  switch (modeOpertion) {
  case mShowTime:
    modeOpertion = mSetTimeH;
    break;
  case mShowTimeOn:
    modeOpertion = mSetTimeOnH;
    break;
  case mShowTimeOff:
    modeOpertion = mSetTimeOffH;
    break;
  }
  btnModeLongPress = true;
}

void updateData()
{
  timerDellayForChangeModeShowTime = 0;
  switch (btnPressed) {
  case btnPlus: incrementTime(); break;
  case btnMinus: decrementTime(); break;
  case btnMode: changeModeLongPress(); break;
  case btnNone: break;
  }
}

void printDeviceInfo()
{
  getDateDs1307();

  //Print version
  Serial.println("//=====      Timer power on/off     =======//");
  Serial.print("  version: ");
  Serial.print(VERSION);
  Serial.println();

  //Print time
  Serial.println("//=======================================//");
  Serial.print("  Time: ");
  prin2dig(hour);
  Serial.print(":");
  prin2dig(minute);
  Serial.print(":");
  prin2dig(second);
  Serial.println();

  //Print value on/off
  Serial.println("//=======================================//");
  Serial.print("  Light params: ");
  Serial.println();
  Serial.print("    Time light On: ");
  prin2dig(hOn);
  Serial.print(":");
  prin2dig(mOn);
  Serial.println();
  Serial.print("    Time light Off: ");
  prin2dig(hOff);
  Serial.print(":");
  prin2dig(mOff);
  Serial.println();
  Serial.println("//=======================================//");
  Serial.println();
  Serial.println();
}

bool isPowerOn()
{
  uint16_t timeCurr = hour * 60 + minute; //count minutes after 00:00
  // Serial.println("======= isPowerOn =======");
  // Serial.print("timeCurr: ");
  // Serial.println(timeCurr);
  // Serial.print("timeOn: ");
  // Serial.println(timeOn);
  // Serial.print("timeOff: ");
  // Serial.println(timeOff);
  // Serial.println("=======================");
  if (timeCurr >= timeOn && timeCurr < timeOff) {
    powerOn = false;
    return true;
  } else if (powerOn) {
    return true;
  }

  return false;
}

void setup()
{
  Serial.begin(9600);

  Wire.begin();

  pinMode(powerPin, OUTPUT);
  digitalWrite(powerPin, LOW);

  pinMode(btnModePin, INPUT_PULLUP);
  // set up button instace
  bouncerMode.attach(btnModePin);
  bouncerMode.interval(100);

  pinMode(btnPlusPin, INPUT_PULLUP);
  // set up button instace
  bouncerPlus.attach(btnPlusPin);
  bouncerPlus.interval(100);

  pinMode(btnMinusPin, INPUT_PULLUP);
  // set up button instace
  bouncerMinus.attach(btnMinusPin);
  bouncerMinus.interval(100);

  tm1637.set();
  tm1637.init();

  // Взять время старта из памяти
  hOn = EEPROM[0];
  mOn = EEPROM[1];
  if (hOn > 23) {
    hOn = 19;
  }
  if (mOn > 59) {
    mOn = 0;
  }
  saveTimeOn();
  // Взять время окончания из памяти
  hOff = EEPROM[2];
  mOff = EEPROM[3];
  if (hOff > 23) {
    hOff = 21;
  }
  if (mOff > 59) {
    mOff = 30;
  }
  saveTimeOff();

  modeOpertion = mShowTime;
  btnPressed = btnNone;

  printDeviceInfo();
  powerOn = false;
  timeOn = hOn * 60 + mOn; //count minutes after 00:00
  timeOff = hOff * 60 + mOff; //count minutes after 00:00
  // Раскоментировать для установки даты на плате DS1307, после закоментировать и залить еще раз
  // setDateTime();
}

void loop()
{
  if (bouncerMode.update()) {
    timerDellayForChangeModeShowTime = 0;
    if (bouncerMode.read() == 0) {
      // Serial.println("pressed Mode"); //вывод сообщения о нажатии
      pressedMoment = millis();
      btnPressed = btnMode;
    } else {
      pressedMoment = 0;
      pressedDellay = DEFAULT_PRESSED_DELLAY;
      btnPressed = btnNone;

      if (btnModeLongPress) { // If after long pressed mode button not change mode
        btnModeLongPress = false;
        return;
      }
      // Short press button mode
      switch (modeOpertion) {
      case mShowTime:
        modeOpertion = mShowTimeOn;
        break;
      case mShowTimeOn:
        modeOpertion = mShowTimeOff;
        break;
      case mShowTimeOff:
        modeOpertion = mShowTime;
        break;
      case mSetTimeH:
        modeOpertion = mSetTimeM;
        break;
      case mSetTimeM:
        modeOpertion = mShowTime;
        break;
      case mSetTimeOnH:
        modeOpertion = mSetTimeOnM;
        break;
      case mSetTimeOnM:
        modeOpertion = mShowTimeOn;
        saveTimeOn();
        break;
      case mSetTimeOffH:
        modeOpertion = mSetTimeOffM;
        break;
      case mSetTimeOffM:
        modeOpertion = mShowTimeOff;
        saveTimeOff();
        break;
      }
    }
  }

  if (bouncerPlus.update()) {
    if (bouncerPlus.read() == 0 ) {
      // Serial.println("pressed Pluss"); //вывод сообщения о нажатии
      pressedMoment = millis();
      btnPressed = btnPlus;
      updateData();
    } else {
      pressedMoment = 0;
      pressedDellay = DEFAULT_PRESSED_DELLAY;
      btnPressed = btnNone;
    }
  }

  if (bouncerMinus.update()) {
    if (bouncerMinus.read() == 0 ) {
      // Serial.println("pressed Minus"); //вывод сообщения о нажатии
      pressedMoment = millis();
      btnPressed = btnMinus;
      updateData();
    } else {
      pressedMoment = 0;
      pressedDellay = DEFAULT_PRESSED_DELLAY;
      btnPressed = btnNone;
    }
  }

  if (pressedMoment != 0 && millis() - pressedDellay > pressedMoment) {
    // Serial.println("dataView");
    updateData();
    dataView();
    if (btnPressed == btnMode) { // Not repeat click mode button
      pressedMoment = 0;
      return;
    }
    // Repeat click button Plus or Minus
    pressedMoment = millis();
    pressedDellay = 500;
  }

  if (timer()) {
    // getDateDs1307(&hour, &minute, &second);
    getDateDs1307();
    dataView();
    if (modeOpertion != mShowTime) {
      if (timerDellayForChangeModeShowTime >= DEFAULT_DELLAY_FOR_CHANGE_MODE_SHOW_TIME) {
        modeOpertion = mShowTime;
        timerDellayForChangeModeShowTime = 0;
        saveTimeOn();
        saveTimeOff();
      }
      timerDellayForChangeModeShowTime += 1;
    }

    // Serial.println("//=======================================//");
    if (timerDellayForCheckPowerState >= DEFAULT_CHECK_POWER_STATE_DELAY ) {
      timerDellayForCheckPowerState = 0;
      if (isPowerOn()) {
        digitalWrite(powerPin, HIGH);
        // Serial.println("             =====Light On====");
      } else {
        digitalWrite(powerPin, LOW);
        // Serial.println("             ====Light Off====");
      }
    }
    timerDellayForCheckPowerState = timerDellayForCheckPowerState + 1;
  }

  // printDeviceInfo();
  // delay(250);
}
