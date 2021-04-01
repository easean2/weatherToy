#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>
#include <ESP8266WiFi.h>          
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>     

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

#define SUN  0
#define SUN_CLOUD  1
#define CLOUD 2
#define RAIN 3
#define THUNDER 4

#define URL "/v3/weather/now.json?key=SVsCZ5456voABhDvr&location=Chengdu&language=zh-Hans&unit=c HTTP/1.1\r\nHost:api.seniverse.com\r\n\r\n"

String web_address = "api.seniverse.com";
WiFiClient wifi;
HttpClient http(wifi, web_address);
String data;

//下面这行代码就是初始化针脚的，根据自己的接线设置clock和data即可，reset没有连接就用参数U8X8_PIN_NONE
U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, /* clock=*/ 12, /* data=*/ 14, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display

int16_t width = 0,height = 0;
char buf[256];
char temperature[16];

void drawWeatherSymbol(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol)
{
  // fonts used:
  // u8g2_font_open_iconic_embedded_6x_t
  // u8g2_font_open_iconic_weather_6x_t
  // encoding values, see: https://github.com/olikraus/u8g2/wiki/fntgrpiconic
  
  switch(symbol)
  {
    case SUN:
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 69);  
      break;
    case SUN_CLOUD:
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 65); 
      break;
    case CLOUD:
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 64); 
      break;
    case RAIN:
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 67); 
      break;
    case THUNDER:
      u8g2.setFont(u8g2_font_open_iconic_embedded_6x_t);
      u8g2.drawGlyph(x, y, 67);
      break;      
  }
}

void drawWeather(uint8_t symbol, char* degree)
{
  drawWeatherSymbol(0, 48, symbol);
  u8g2.setFont(u8g2_font_logisoso32_tf);
  u8g2.setCursor(48+3, 42);
  u8g2.print(degree);
  u8g2.print("°C");   // requires enableUTF8Print()
}

/*
  Draw a string with specified pixel offset. 
  The offset can be negative.
  Limitation: The monochrome font with 8 pixel per glyph
*/
void drawScrollString(int16_t offset, const char *s)
{
  static char buf[36];  // should for screen with up to 256 pixel width 
  size_t len;
  size_t char_offset = 0;
  u8g2_uint_t dx = 0;
  size_t visible = 0;
  len = strlen(s);
  if ( offset < 0 )
  {
    char_offset = (-offset)/8;
    dx = offset + char_offset*8;
    if ( char_offset >= width/8 )
      return;
    visible = width/8-char_offset+1;
    strncpy(buf, s, visible);
    buf[visible] = '\0';
    u8g2.setFont(u8g2_font_8x13_mf);
    u8g2.drawStr(char_offset*8-dx, 62, buf);
  }
  else
  {
    char_offset = offset / 8;
    if ( char_offset >= len )
      return; // nothing visible
    dx = offset - char_offset*8;
    visible = len - char_offset;
    if ( visible > width/8+1 )
      visible = width/8+1;
    strncpy(buf, s+char_offset, visible);
    buf[visible] = '\0';
    u8g2.setFont(u8g2_font_8x13_mf);
    u8g2.drawStr(-dx, 62, buf);
  }
  
}

void draw(const char *s, uint8_t symbol, char* degree)
{
  int16_t offset = -(int16_t)u8g2.getDisplayWidth();
  int16_t len = strlen(s);
  for(;;)
  {
    u8g2.firstPage();
    do {
      drawWeather(symbol, degree);
      drawScrollString(offset, s);
    } while ( u8g2.nextPage() );
    delay(20);
    offset+=2;
    if ( offset > len*8+1 )
      break;
  }
}

// 发送HTTP请求并且将服务器响应通过串口输出
void httpClientRequest(){
  int httpCode = 0;
  String httpData;
  //发送http请求
  httpCode = http.get("/v3/weather/now.json?key=SVsCZ5456voABhDvr&location=Chengdu&language=en&unit=c");
  //若是有返回就接收数据
  if ( httpCode == 0)
  {
    Serial.println("startedRequest ok");
    httpCode = http.responseStatusCode();
    if (httpCode >= 0)
    {
      int bodyLen = http.contentLength();
      //将接收到的字符存入string中，直到数据接收完毕
      while ( (http.connected() || http.available()) && (!http.endOfBodyReached()))
      {
        if (http.available())
        {
          char c = http.read();
          httpData += c;
        }
        else
          delay(1000);
      }
      //提取出关于天气的那一段字符串
      data = httpData.substring((httpData.indexOf("\"now\":") + 6), httpData.indexOf(",\"last")); 
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, data); //反序列化JSON数据
      if (!error) //检查反序列化是否成功
      {
          //读取json节点
          //{"text":"Cloudy","code":"4","temperature":"10"}
          const char *test = doc["text"]; 
          const char *code = doc["code"]; 
          const char *t = doc["temperature"]; 
          sprintf(temperature, "%s", t);
          Serial.print("weather is :");   
          Serial.println(test);
          Serial.print("code is :");   
          Serial.println(code);
          Serial.print("temperature is :");   
          Serial.println(temperature);
      }
    }
  }
  else
    Serial.print("Connect failed");
  http.stop();
  //串口打印出温度
  Serial.print("end");
}


void setup(void) {
  u8g2.begin();
  u8g2.enableUTF8Print();
  
  width = u8g2.getDisplayWidth();
  height = u8g2.getDisplayHeight();
   
  WiFiManager wifiManager;
  wifiManager.autoConnect("AutoConnectAP");
  sprintf(buf, "%s%s","SSID:", WiFi.SSID().c_str());

  httpClientRequest();  
}

void loop(void) {
  draw(buf, CLOUD, temperature);
}
