#include <ESP32CAN.h>           // v1.0.0     from https://github.com/nhatuan84/arduino-esp32-can-demo
#include <M5Stack.h>
#include <CAN_config.h>         // as above
#include <SPI.h>                // v1.0
#include <Wire.h>               // v1.0
#include "BluetoothSerial.h"    // v1.0

#define LOVYANLAUNCHER 1

#if LOVYANLAUNCHER == 1
#include "M5StackUpdater.h"
#endif
#define LOAD_GFXFF
#define GFXFF 1
#define GLCD  0
#define FONT2 2
#define FONT4 4
#define FONT6 6
#define FONT7 7
#define FONT8 8
#define FSS9 &FreeSans9pt7b
#define FSSB9 &FreeSansBold9pt7b
#define FSSB12 &FreeSansBold12pt7b
#define FSS12 &FreeSans12pt7b
#define FSS18 &FreeSans18pt7b
#define FSS24 &FreeSans24pt7b
#define FSSB24 &FreeSansBold24pt7b

#include "battery12pt7b.h"
#define BAT12 &battery12pt7b 

// CURRENTLY ESP32 Dev Module Board Definition
// PIN 4  CANTX 
// PIN 5  CANRX 
// PIN 12 BLUETOOTH SWITCH
// PIN 14 NOT IN USE
// PIN 15 10k to ground to remove boot messages
// 3.3v 
// GND to SWITCH CENTER

boolean getSlcancmd       = false;
boolean getSlcancmdBak    = false;
boolean display_count     = false;

    CAN_device_t CAN_cfg;
BluetoothSerial           SerialBT;

boolean working           = false;
boolean working_bak       = false;
boolean working_sniff     = false;
boolean bluetooth         = false;
boolean bluetooth_bak     = false;
boolean timestamp         = false;
boolean timestamp_bak     = false;
boolean cr                = false;
boolean disp_cnt          = false;
int can_speed             = 250;
int ser_speed             = 921600;
//int ser_speed           = 1000000;
int msg_cnt_in            = 0;
int msg_cnt_out           = 0;
unsigned long bakMillis   = 0;
const int SWITCH_PIN_A    = 12;

int txtLeft = 20;
int txtLeft2 = 2;
int txtRight = 318;
int txtRightBat = 314;
int ylines1 = 70;
int ylines2 = 110;
int ylines3 = 145;
int ylines4 = 170;
int ylines5 = 195;

int yHeadLine = 20;
int yFootLine = 220;
int txtLeftButton = 65;
int txtMidButton = 160;
int txtRightButton = 255;

uint16_t textColor = TFT_WHITE;
uint16_t lightBackground = 0x101D;//0x1E9F;
uint16_t darkBackground = 0x0811;//0x439;
int16_t speed[] = {100, 125, 250, 500, 800, 1000};
CAN_speed_t speedc[] = {
    CAN_SPEED_100KBPS,
    CAN_SPEED_125KBPS,
    CAN_SPEED_250KBPS,
    CAN_SPEED_500KBPS,
    CAN_SPEED_800KBPS,
    CAN_SPEED_1000KBPS
};
int16_t speed_nr = 2;
int16_t speed_dif = 1;

int LCD_Brightness = 16;
int LCD_Nr = 4;
int LCD_Dif = -1;
int counter = 0;
boolean firstTime = true;
int bat_bak = 10;
int bat = 0;
char speed_bak[25];

static uint8_t hexval[17] = "0123456789ABCDEF";

//----------------------------------------------------------------

void Serial_println(const char data[])
{
  if (bluetooth)
    SerialBT.println(data);
  else
    Serial.println(data);
}

void Serial_print(const char data[])
{
  if (bluetooth)
    SerialBT.print(data);
  else
    Serial.print(data);
}

void disp_msg_cnt()
{
  char buffer[40];
  int xpos = 200;

  if (LCD_Brightness != 0)
  {
    m5.Lcd.setTextColor(textColor, darkBackground);
    if (firstTime)
    {
      firstTime = false;
      m5.Lcd.setTextDatum(L_BASELINE);
      m5.Lcd.setFreeFont(FSS12);
      m5.Lcd.drawString("count in", txtLeft, ylines1, GFXFF);
      m5.Lcd.drawString("count out", txtLeft, ylines2, GFXFF);
      m5.Lcd.setFreeFont(FSS12);
      m5.Lcd.drawString("slcan", txtLeft, ylines3, GFXFF);
      m5.Lcd.drawString("bluetooth", txtLeft, ylines4, GFXFF);
      m5.Lcd.drawString("timestamp", txtLeft, ylines5, GFXFF);
      m5.Lcd.setTextDatum(R_BASELINE);
      m5.Lcd.drawString("packets/s", txtRight, ylines1, GFXFF);
      m5.Lcd.drawString("packets/s", txtRight, ylines2, GFXFF);
    }
    m5.Lcd.setFreeFont(FSS18);
    m5.Lcd.setTextDatum(R_BASELINE);

    sprintf(buffer, "%*d", 4, msg_cnt_in);
    m5.Lcd.drawString(buffer, xpos, ylines1, GFXFF);
    sprintf(buffer, "%*d", 4, msg_cnt_out);
    m5.Lcd.drawString(buffer, xpos, ylines2, GFXFF);
    m5.Lcd.setFreeFont(FSS12);
    if (working_bak != working)
    {
      if (working)
      {
        m5.Lcd.drawString("  ON", xpos, ylines3, GFXFF);
      }
      else
      {
        m5.Lcd.drawString("OFF", xpos, ylines3, GFXFF);
      }
      working_bak = working;
    }
    if (bluetooth_bak != bluetooth)
    {
      if (bluetooth)
      {
        m5.Lcd.drawString("  ON", xpos, ylines4, GFXFF);
      }
      else
      {
        m5.Lcd.drawString("OFF", xpos, ylines4, GFXFF);
      }
      bluetooth_bak = bluetooth;
    }
    if (timestamp_bak != timestamp)
    {
      if (timestamp)
      {
        m5.Lcd.drawString("  ON", xpos, ylines5, GFXFF);
      }
      else
      {
        m5.Lcd.drawString("OFF", xpos, ylines5, GFXFF);
      }
      timestamp_bak = timestamp;
    }
  }
} //disp-msg-cnt()

//----------------------------------------------------------------

void display_settings_slcan()
{
  char buffer[40];

  sprintf(buffer, "ser:%i can:%i kbps", ser_speed / 1000, can_speed);
  if (strcmp(speed_bak, buffer) != 0)
  {
    m5.Lcd.setTextSize(1);
    m5.Lcd.setTextColor(textColor, lightBackground);
    m5.Lcd.setTextDatum(L_BASELINE);
    m5.Lcd.setFreeFont(FSS12);
    m5.Lcd.drawString(buffer, txtLeft2, yHeadLine, GFXFF);
    m5.Lcd.setTextDatum(CC_DATUM);
    m5.Lcd.drawString("disp", txtRightButton, yFootLine, GFXFF);
    strncpy(speed_bak, buffer, 25);
  }

  bat = m5.Power.getBatteryLevel();
  if (bat_bak != bat)
  {
    bat_bak = bat;
    if (bat < 30)
    {
      m5.Lcd.setTextColor(TFT_RED, lightBackground);
    }
    byte bBat = -bat / 25 + 69; //E = 69 leer A = 65 voll
    m5.Lcd.setTextDatum(R_BASELINE);
    m5.Lcd.setFreeFont(BAT12);
    buffer[0] = (char)bBat;
    buffer[1] = 0x00;
    //Serial.println(buffer);
    m5.Lcd.drawString(buffer, txtRightBat, yHeadLine, GFXFF);
  }
} //diplay_settings_slcan()

//----------------------------------------------------------------

void display_settings_sniff()
{
  char buffer[40];


  sprintf(buffer, "slcan waits for command");
  if (strcmp(speed_bak, buffer) != 0)
  {
    m5.Lcd.setTextSize(1);
    m5.Lcd.setTextColor(textColor, lightBackground);
    m5.Lcd.setTextDatum(L_BASELINE);
    m5.Lcd.setFreeFont(FSS12);
    m5.Lcd.drawString(buffer, txtLeft2, yHeadLine, GFXFF);
    strncpy(speed_bak, buffer, 25);
  }

  bat = m5.Power.getBatteryLevel();
  if (bat_bak != bat)
  {
    bat_bak = bat;
    if (bat < 30)
    {
      m5.Lcd.setTextColor(TFT_RED, lightBackground);
    }
    byte bBat = -bat / 25 + 69; //E = 69 leer A = 65 voll
    m5.Lcd.setTextDatum(R_BASELINE);
    m5.Lcd.setFreeFont(BAT12);
    buffer[0] = (char)bBat;
    buffer[1] = 0x00;
    //Serial.println(buffer);
    m5.Lcd.drawString(buffer, txtRightBat - 3, yHeadLine, GFXFF);
  }

  m5.Lcd.setTextDatum(L_BASELINE);
  m5.Lcd.setFreeFont(FSS12);
  m5.Lcd.setTextColor(textColor, darkBackground);
  sprintf(buffer, "sniff CAN BUS: %i kbps ", can_speed);
  m5.Lcd.drawString(buffer, txtLeft2, ylines1, GFXFF);

  m5.Lcd.setTextSize(1);
  m5.Lcd.setTextColor(textColor, lightBackground);
  m5.Lcd.setTextDatum(CC_DATUM);
  m5.Lcd.setFreeFont(FSS12);

  m5.Lcd.drawString("sniff", txtLeftButton, yFootLine, GFXFF);
  m5.Lcd.drawString("speed", txtMidButton, yFootLine, GFXFF);
  m5.Lcd.drawString("disp", txtRightButton, yFootLine, GFXFF);
} //diplay_settings_sniff()

//----------------------------------------------------------------

void display_error(int canspeed)
{
  m5.Lcd.fillRect(0, 210, 320, 30, lightBackground); /* Lower dark blue area */
  m5.Lcd.drawFastHLine(0, 210, 320, TFT_WHITE);
  m5.Lcd.setTextSize(1);
  m5.Lcd.setTextColor(TFT_RED, lightBackground);
  m5.Lcd.setTextDatum(CR_DATUM);
  m5.Lcd.setFreeFont(FSSB12);

  if (canspeed > 0)
  {
    m5.Lcd.drawString("can kbs NOT AVAILABLE", txtLeft, yFootLine, GFXFF);
  } else if (canspeed == -1) {
    m5.Lcd.drawString("STOP FIRST",txtLeft2, yFootLine,GFXFF);
  } else if (canspeed == 0)  {
    m5.Lcd.drawString("CANNOT SEND",txtLeft2, yFootLine,GFXFF);
  } else {
    m5.Lcd.drawString("unknown", txtLeft2, yFootLine, GFXFF);
  }

  delay(2500);
  m5.Lcd.fillRect(0, 210, 320, 30, lightBackground); /* Lower dark blue area */
  m5.Lcd.drawFastHLine(0, 210, 320, TFT_WHITE);
} //display_error()

//----------------------------------------------------------------

void display_main()
{
  m5.Lcd.setTextFont(1);
  m5.Lcd.fillRect(0, 0, 320, 30, lightBackground);                      /* Upper dark blue area */
  m5.Lcd.fillRect(0, 30, 320, 180, darkBackground);                   /* Main light blue area */
  m5.Lcd.fillRect(0, 210, 320, 30, lightBackground);                    /* Lower dark blue area */
  m5.Lcd.drawFastHLine(0, 29, 320, TFT_WHITE);
  m5.Lcd.drawFastHLine(0, 210, 320, TFT_WHITE);
  firstTime = true;
  bat_bak = 30;
  speed_bak[0] = 0x00;
  working_bak = !working;
  bluetooth_bak = !bluetooth;
  timestamp_bak = !timestamp;
} //display_main()

//----------------------------------------------------------------

void slcan_ack()
{
  if (bluetooth) SerialBT.write('\r');
  else Serial.write('\r');
} // slcan_ack()

//----------------------------------------------------------------

void slcan_nack()
{
  if (bluetooth) SerialBT.write('\a');
  else Serial.write('\a');
} // slcan_nack()

//----------------------------------------------------------------

void pars_slcancmd(char *buf)
{                           // LAWICEL PROTOCOL
  getSlcancmd = true;
  switch (buf[0]) {
    case 'O':               // OPEN CAN
      working = false;
      ESP32Can.CANStop();
      working = true;
      ESP32Can.CANStop();
      ESP32Can.CANInit();
      msg_cnt_in = 0;
      msg_cnt_out = 0;
      display_settings_slcan();
      slcan_ack();
      break;
    case 'C':               // CLOSE CAN
      working=false;
      ESP32Can.CANStop();
      display_settings_slcan();
      slcan_ack();
      getSlcancmd = false;
      break;
    case 't':               // SEND STD FRAME
      send_canmsg(buf,false,false);
      slcan_ack();
      break;
    case 'T':               // SEND EXT FRAME
      send_canmsg(buf,false,true);
      slcan_ack();
      break;
    case 'r':               // SEND STD RTR FRAME
      send_canmsg(buf,true,false);
      slcan_ack();
      break;
    case 'R':               // SEND EXT RTR FRAME
      send_canmsg(buf,true,true);
      slcan_ack();
      break;
    case 'Z':               // ENABLE TIMESTAMPS
      switch (buf[1]) {
        case '0':           // TIMESTAMP OFF  
          timestamp = false;
          display_settings_slcan();
          slcan_ack();
          break;
        case '1':           // TIMESTAMP ON
          timestamp = true;
          display_settings_slcan();
          slcan_ack();
          break;
        default:
          break;
      }
      break;
    case 'M':               ///set ACCEPTANCE CODE ACn REG
      slcan_ack();
      break;
    case 'm':               // set ACCEPTANCE CODE AMn REG
      slcan_ack();
      break;
    case 's':               // CUSTOM CAN bit-rate
      slcan_nack();
      break;
    case 'S':               // CAN bit-rate
      if (working) {
        display_error(-1);
        display_settings_slcan();
        break;
      }
      switch (buf[1]) {
        case '0':           // 10k  
          display_error(10);
          display_settings_slcan();
          slcan_nack();
          break;
        case '1':           // 20k
          display_error(20);
          display_settings_slcan();
          slcan_nack();
          break;
        case '2':           // 50k
          display_error(50);
          display_settings_slcan();
          slcan_nack();
          break;
        case '3':           // 100k
          CAN_cfg.speed=CAN_SPEED_100KBPS;
          can_speed = 100;
          display_settings_slcan();
          slcan_ack();
          break;
        case '4':           // 125k
          CAN_cfg.speed=CAN_SPEED_125KBPS;
          can_speed = 125;
          display_settings_slcan();
          slcan_ack();
          break;
        case '5':           // 250k
          CAN_cfg.speed=CAN_SPEED_250KBPS;
          can_speed = 250;
          display_settings_slcan();
          slcan_ack();
          break;
        case '6':           // 500k
          CAN_cfg.speed=CAN_SPEED_500KBPS;
          can_speed = 500;
          display_settings_slcan();
          slcan_ack();
          break;
        case '7': // 800k
          CAN_cfg.speed=CAN_SPEED_800KBPS;
          can_speed = 800;
          display_settings_slcan();
          slcan_ack();
          break;
        case '8':           // 1000k
          CAN_cfg.speed=CAN_SPEED_1000KBPS;
          can_speed = 1000;
          display_settings_slcan();
          slcan_ack();
          break;
        default:
          slcan_nack();
          break;
      }
      break;
    case 'F':               // STATUS FLAGS
      Serial_print("F00");
      slcan_ack();
      break;
    case 'V':               // VERSION NUMBER
      Serial_print("V1234");
      slcan_ack();
      break;
    case 'N':               // SERIAL NUMBER
      Serial_print("N2208");
      slcan_ack();
      break;
    case 'l':               // (NOT SPEC) TOGGLE LINE FEED ON SERIAL
      cr = !cr;
      slcan_nack();
      break;
    case 'h':               // (NOT SPEC) HELP SERIAL
      Serial_print("");
      Serial_println("mintynet.com - slcan esp32");
      Serial_println("");
      Serial_println("O\t=\tStart slcan");
      Serial_println("C\t=\tStop slcan");
      Serial_println("t\t=\tSend std frame");
      Serial_println("r\t=\tSend std rtr frame");
      Serial_println("T\t=\tSend ext frame");
      Serial_println("R\t=\tSend ext rtr frame");
      Serial_println("Z0\t=\tTimestamp Off");
      Serial_println("Z1\t=\tTimestamp On");
      Serial_println("snn\t=\tSpeed 0xnnk N/A");
      Serial_println("S0\t=\tSpeed 10k N/A");
      Serial_println("S1\t=\tSpeed 20k N/A");
      Serial_println("S2\t=\tSpeed 50k N/A");
      Serial_println("S3\t=\tSpeed 100k");
      Serial_println("S4\t=\tSpeed 125k");
      Serial_println("S5\t=\tSpeed 250k");
      Serial_println("S6\t=\tSpeed 500k");
      Serial_println("S7\t=\tSpeed 800k");
      Serial_println("S8\t=\tSpeed 1000k");
      Serial_println("F\t=\tFlags N/A");
      Serial_println("N\t=\tSerial No");
      Serial_println("V\t=\tVersion");
      Serial_println("-----NOT SPEC-----");
      Serial_println("h\t=\tHelp");
      Serial_print("l\t=\tToggle CR ");
      if (cr) {
        Serial_println("ON");
      } else {
        Serial_println("OFF");
      }
      Serial_print("CAN_SPEED:\t");
      switch(can_speed) {
        case 100:
          Serial_print("100");
          break;
        case 125:
          Serial_print("125");
          break;
        case 250:
          Serial_print("250");
          break;
        case 500:
          Serial_print("500");
          break;
        case 800:
          Serial_print("800");
          break;
        case 1000:
          Serial_print("1000");
          break;
        default:
          break;
      }
      Serial_print("kbps");
      if (timestamp) {
        Serial_print("\tT");
      }
      if (working) {
        Serial_print("\tON");
      } else {
        Serial_print("\tOFF");
      }
      Serial_println("");
      slcan_nack();
      break;
    default:
      slcan_nack();
      break;
  }
} // pars_slcancmd()

//----------------------------------------------------------------

void transfer_tty2can()
{
  int ser_length;
  static char cmdbuf[32];
  static int cmdidx = 0;
  if (bluetooth) {
    if ((ser_length = SerialBT.available()) > 0) {
      for (int i = 0; i < ser_length; i++) {
        char val = SerialBT.read();
        cmdbuf[cmdidx++] = val;
        if (cmdidx == 32)
        {
          slcan_nack();
          cmdidx = 0;
        } else if (val == '\r')
        {
          cmdbuf[cmdidx] = '\0';
          pars_slcancmd(cmdbuf);
          cmdidx = 0;
        }
      }
    }
  } else {
    if ((ser_length = Serial.available()) > 0) {
      for (int i = 0; i < ser_length; i++) {
        char val = Serial.read();
        cmdbuf[cmdidx++] = val;
        if (cmdidx == 32)
        {
          slcan_nack();
          cmdidx = 0;
        } else if (val == '\r')
        {
          cmdbuf[cmdidx] = '\0';
          pars_slcancmd(cmdbuf);
          cmdidx = 0;
        }
      }
    }
  }
} // transfer_tty2can()

//----------------------------------------------------------------

void transfer_can2tty()
{
  CAN_frame_t rx_frame;
  String command = "";
  long time_now = 0;
  //receive next CAN frame from queue
  if(xQueueReceive(CAN_cfg.rx_queue,&rx_frame, 3*portTICK_PERIOD_MS)==pdTRUE) {
    //do stuff!
    if(working) {
      if(rx_frame.FIR.B.FF==CAN_frame_ext) {
        if (rx_frame.FIR.B.RTR==CAN_RTR) {
          command = command + "R";
        } else {
          command = command + "T";
        }
        command = command + char(hexval[ (rx_frame.MsgID>>28)&15]);
        command = command + char(hexval[ (rx_frame.MsgID>>24)&15]);
        command = command + char(hexval[ (rx_frame.MsgID>>20)&15]);
        command = command + char(hexval[ (rx_frame.MsgID>>16)&15]);
        command = command + char(hexval[ (rx_frame.MsgID>>12)&15]);
        command = command + char(hexval[ (rx_frame.MsgID>>8)&15]);
        command = command + char(hexval[ (rx_frame.MsgID>>4)&15]);
        command = command + char(hexval[ rx_frame.MsgID&15]);
        command = command + char(hexval[ rx_frame.FIR.B.DLC ]);
      } else {
        if (rx_frame.FIR.B.RTR==CAN_RTR) {
          command = command + "r";
        } else {
          command = command + "t";
        }
        command = command + char(hexval[ (rx_frame.MsgID>>8)&15]);
        command = command + char(hexval[ (rx_frame.MsgID>>4)&15]);
        command = command + char(hexval[ rx_frame.MsgID&15]);
        command = command + char(hexval[ rx_frame.FIR.B.DLC ]);
      }
      for(int i = 0; i < rx_frame.FIR.B.DLC; i++){
        command = command + char(hexval[ rx_frame.data.u8[i]>>4 ]);
        command = command + char(hexval[ rx_frame.data.u8[i]&15 ]);
        //printf("%c\t", (char)rx_frame.data.u8[i]);
      }
    if (timestamp) {
      time_now = millis() % 60000;
      command = command + char(hexval[ (time_now>>12)&15 ]);
      command = command + char(hexval[ (time_now>>8)&15 ]);
      command = command + char(hexval[ (time_now>>4)&15 ]);
      command = command + char(hexval[ time_now&15 ]);
    }
    command = command + '\r';
    if (bluetooth) SerialBT.print(command);
    else Serial.print(command);
    if (cr) Serial_println("");
    }
    msg_cnt_in++;
  }
} // transfer_can2tty()

void sniff_can2tty()
{
  CAN_frame_t rx_frame;
  String command = "";
  String command_head = "";
  long time_now = 0;
  //receive next CAN frame from queue
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE)
  {
    //do stuff!
    if (rx_frame.FIR.B.FF == CAN_frame_ext)
    {
      if (rx_frame.FIR.B.RTR == CAN_RTR)
      {
        command_head = command_head + "R";
      }
      else
      {
        command_head = command_head + "T";
      }
      command_head = command_head + char(hexval[(rx_frame.MsgID >> 28) & 15]);
      command_head = command_head + char(hexval[(rx_frame.MsgID >> 24) & 15]);
      command_head = command_head + char(hexval[(rx_frame.MsgID >> 20) & 15]);
      command_head = command_head + char(hexval[(rx_frame.MsgID >> 16) & 15]);
      command_head = command_head + char(hexval[(rx_frame.MsgID >> 12) & 15]);
      command_head = command_head + char(hexval[(rx_frame.MsgID >> 8) & 15]);
      command_head = command_head + char(hexval[(rx_frame.MsgID >> 4) & 15]);
      command_head = command_head + char(hexval[rx_frame.MsgID & 15]);
      command_head = command_head + char(hexval[rx_frame.FIR.B.DLC]);
    }
    else
    {
      if (rx_frame.FIR.B.RTR == CAN_RTR)
      {
        command_head = command_head + "r";
      }
      else
      {
        command_head = command_head + "t";
      }
      command_head = command_head + char(hexval[(rx_frame.MsgID >> 8) & 15]);
      command_head = command_head + char(hexval[(rx_frame.MsgID >> 4) & 15]);
      command_head = command_head + char(hexval[rx_frame.MsgID & 15]);
      command_head = command_head + char(hexval[rx_frame.FIR.B.DLC]);
    }
    for (int i = 0; i < rx_frame.FIR.B.DLC; i++)
    {
      command = command + char(hexval[rx_frame.data.u8[i] >> 4]);
      command = command + char(hexval[rx_frame.data.u8[i] & 15]);
      //printf("%c\t", (char)rx_frame.data.u8[i]);
    }
    if (timestamp)
    {
      time_now = millis() % 60000;
      command = command + char(hexval[(time_now >> 12) & 15]);
      command = command + char(hexval[(time_now >> 8) & 15]);
      command = command + char(hexval[(time_now >> 4) & 15]);
      command = command + char(hexval[time_now & 15]);
    }
    command = command + ' ';
    command_head = command_head + ' ';

    if (not display_count)
    {
      m5.Lcd.setFreeFont(FSS12);
      m5.Lcd.setTextDatum(L_BASELINE);
      m5.Lcd.setTextColor(textColor, darkBackground);
      m5.Lcd.fillRect(txtLeft, ylines1+4, 320, ylines4 - ylines1-4, darkBackground); /* Main light blue area */

      m5.Lcd.drawString(command_head, txtLeft, ylines2, GFXFF);
      m5.Lcd.drawString(command, txtLeft, ylines3, GFXFF);
    }

    msg_cnt_in++;
  }
} // transfer_can2tty()

//----------------------------------------------------------------

void send_canmsg(char *buf, boolean rtr, boolean ext) {
  if (!working) {
    display_error(0);
    display_settings_slcan();
  } else {
    CAN_frame_t tx_frame;
    int msg_id = 0;
    int msg_ide = 0;
    if (rtr) {
      if (ext) {
        sscanf(&buf[1], "%04x%04x", &msg_ide, &msg_id);
        tx_frame.FIR.B.RTR = CAN_RTR;
        tx_frame.FIR.B.FF = CAN_frame_ext;
      } else {
        sscanf(&buf[1], "%03x", &msg_id);
        tx_frame.FIR.B.RTR = CAN_RTR;
        tx_frame.FIR.B.FF = CAN_frame_std;
      }
    } else {
      if (ext) {
        sscanf(&buf[1], "%04x%04x", &msg_ide, &msg_id);
        tx_frame.FIR.B.RTR = CAN_no_RTR;
        tx_frame.FIR.B.FF = CAN_frame_ext;
      } else {
        sscanf(&buf[1], "%03x", &msg_id);
        tx_frame.FIR.B.RTR = CAN_no_RTR;
        tx_frame.FIR.B.FF = CAN_frame_std;
      }
    }
    tx_frame.MsgID = msg_ide*65536 + msg_id;
    int msg_len = 0;
    if (ext) {
      sscanf(&buf[9], "%01x", &msg_len);
    } else {
      sscanf(&buf[4], "%01x", &msg_len);
    }
    tx_frame.FIR.B.DLC = msg_len;
    int candata = 0;
    if (ext) {
      for (int i = 0; i < msg_len; i++) {
        sscanf(&buf[10 + (i*2)], "%02x", &candata);
        tx_frame.data.u8[i] = candata;
      }
    } else {
      for (int i = 0; i < msg_len; i++) {
        sscanf(&buf[5 + (i*2)], "%02x", &candata);
        tx_frame.data.u8[i] = candata;
      }
    }
    ESP32Can.CANWriteFrame(&tx_frame);
    msg_cnt_out++;
  }
} // send_canmsg()

//----------------------------------------------------------------


void setup() {
  delay(100);
  m5.begin();
  m5.Power.begin();
  M5.Power.setPowerVin(false);
  Wire.begin();
  dacWrite(SPEAKER_PIN, 0); // disable speker noise
#if LOVYANLAUNCHER == 1
      if (digitalRead(BUTTON_A_PIN) == 0){
    Serial.println("Will load menu binary");
    updateFromFS(SD);
    ESP.restart();
  }
#endif
  //Wire.begin(21,22);
  pinMode(SWITCH_PIN_A,INPUT_PULLUP);
  CAN_cfg.speed=CAN_SPEED_250KBPS;
  CAN_cfg.tx_pin_id = GPIO_NUM_2;
  CAN_cfg.rx_pin_id = GPIO_NUM_5;
  CAN_cfg.rx_queue = xQueueCreate(10,sizeof(CAN_frame_t));
  m5.Lcd.setFreeFont(FSS12);
  m5.Lcd.setTextDatum(TL_DATUM);
  m5.Lcd.setTextSize(GFXFF);
  m5.Lcd.setTextColor(WHITE);
  m5.Lcd.setCursor(15,40);
  m5.Lcd.clearDisplay();
  //m5.Lcd.println("slcan device");

  setCpuFrequencyMhz(80);
  delay(300);
  if (digitalRead(BUTTON_B_PIN) == 0)
  {
    SerialBT.begin("SLCAN");
    bluetooth = true;
    Serial.println("BT Switch ON");
    m5.Lcd.println("bluetooth");
    do {
    } while (digitalRead(BUTTON_B_PIN) == 1);
    delay(500);
  }
  else
  {
    m5.Lcd.println("no bt");
    bluetooth = false;
    Serial.println("BT Switch OFF");
  }
  if (bluetooth) Serial.println("BLUETOOTH ON");
  bakMillis = millis();
  display_main();
  //display_settings();
  delay(200);
  Serial.flush();
  Serial.updateBaudRate(ser_speed);  

  delay(100);
} // setup()

//----------------------------------------------------------------

void loop() {
  m5.update();
  transfer_tty2can();

  if (getSlcancmd)
  {
    if (not getSlcancmdBak)
    {
      display_main();
      display_settings_slcan();
    }
    transfer_can2tty();
    if ((millis() - bakMillis) > 1000)
    {
      bakMillis = millis();
      disp_msg_cnt();
      msg_cnt_out = 0;
      msg_cnt_in  = 0;
    }
  }
  else //sniff can packets on different baud rates
  {
    if (display_count)
    {
      if ((millis() - bakMillis) > 1000)
      {
        char buffer[40];

        m5.Lcd.setFreeFont(FSS12);
        m5.Lcd.setTextDatum(L_BASELINE);
        m5.Lcd.setTextColor(textColor, darkBackground);
        sprintf(buffer, "count CAN: %i Packet/s       ", msg_cnt_in);
        m5.Lcd.drawString(buffer, txtLeft, ylines2, GFXFF);

        bakMillis = millis();
        msg_cnt_in = 0;
      }
    }

    if (m5.BtnA.wasReleasefor(50) == true)
    { /* Button A pressed ? --> Change mode (display CAN message / count of can messages)*/

      display_count = not display_count;
      m5.Lcd.fillRect(txtLeft, ylines1 + 4, 320, ylines5 - ylines1 - 4, darkBackground); /* Main light blue area */
    }

    //if (m5.BtnB.isPressed())
    if (m5.BtnB.wasReleasefor(50) == true)
    {
      //if (m5.BtnC.isPressed())
      { /* Button C pressed ? --> Change speed */
        if (speed_nr >= 5)
          speed_dif = -1;
        else if (speed_nr <= 0)
          speed_dif = 1;

        speed_nr += speed_dif;
        can_speed = speed[speed_nr];      /* Change speed value */
        can_speed = (int16_t)speedc[speed_nr];      /* Change speed value */
        CAN_cfg.speed = speedc[speed_nr]; /* Change speed value */

        display_settings_sniff();
        ESP32Can.CANStop();
        slcan_ack();
        ESP32Can.CANInitLom();  // todo listen only mode doesn't work!!!
        slcan_ack();
        m5.Lcd.fillRect(txtLeft, ylines1 + 4, 320, ylines5 - ylines1 - 4, darkBackground); /* Main light blue area */
      }
    }

    if (not working_sniff)
    {
        working_sniff = true;
        ESP32Can.CANInitLom();
        display_settings_sniff();
        slcan_ack();
      }
      sniff_can2tty();

      /* code */
    }

  //Button right  brightness
  if (m5.BtnC.wasReleasefor(50) == true)
  { /* Button C pressed ? --> Change brightness */
    if (LCD_Nr >= 8)
      LCD_Dif = -1; /* Decrease brightness */
    else if (LCD_Nr <= 0)
      LCD_Dif = 1; /* Increase brightness */

    LCD_Nr += LCD_Dif;
    LCD_Brightness = (1 << LCD_Nr) -1;
    m5.Lcd.setBrightness(LCD_Brightness); /* Change brightness value */
  }
  
  getSlcancmdBak = getSlcancmd;

} // loop();