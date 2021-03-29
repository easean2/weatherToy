#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

/*
 * 
 * 接线：
 * nodemcu  |  ssd1306
 * 3.3V     |  vcc
 * gnd      |  gnd 
 * io12     |  SCL
 * IO14     |  SDA
 * 
*/

//下面这行代码就是初始化针脚的，根据自己的接线设置clock和data即可，reset没有连接就用参数U8X8_PIN_NONE
U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, /* clock=*/ 12, /* data=*/ 14, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display

void setup(void) {
  u8g2.begin();
}

void loop(void) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_ncenB14_tr);
    u8g2.drawStr(0,24,"Hello World!");
  } while ( u8g2.nextPage() );
}
