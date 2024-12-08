#include <TTP229_SPI.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Bonezegei_DHT11.h>
#include <RtcDS1302.h>
#include <DS18B20.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define PULSE_PIN 2

#define PULSE_AVRG_LEN 5
#define PULSE_LONG_LEN 150
//#define PULSE_DEBUG
int pulseSignalAvrg=0;
int pulseLongAvrg=0;
int pulseLast1000[1000];
int pulse;
bool pulseRaisedEdge = false;
int pulseLastRaisedMillis[15];
void pulseLoop() {
  int read = analogRead(PULSE_PIN);
  pulseSignalAvrg = ((PULSE_AVRG_LEN-1)*pulseSignalAvrg+read)/PULSE_AVRG_LEN;
  int pulseMaxLast1000 = read;
  int pulseMaxLastLong = read;
  pulseLongAvrg=read;
  for(int i = 998; i>=0; i--)
  {
    pulseLast1000[i+1]=pulseLast1000[i];
    pulseMaxLast1000=max(pulseMaxLast1000, pulseLast1000[i]);
    if(i<PULSE_LONG_LEN-1)
    {
      pulseLongAvrg+=pulseLast1000[i];
      pulseMaxLastLong=max(pulseMaxLastLong,pulseLast1000[i]);
    }
  }
  pulseLongAvrg/=PULSE_LONG_LEN;
  pulseLast1000[0]=pulseSignalAvrg;
  int pulseExists = 0;
  if(pulseSignalAvrg > pulseMaxLastLong-5)
  {
    pulseExists=1;
  }
  if(pulseExists==1&&!pulseRaisedEdge)
  {
    pulseRaisedEdge=true;
    int avrgmillis = 0;
    for(int i = 13; i>=0; i--)
    {
      pulseLastRaisedMillis[i+1]=pulseLastRaisedMillis[i];
    }
    pulseLastRaisedMillis[0]=millis();
    for(int i = 0; i<14; i++)
    {
      avrgmillis+=pulseLastRaisedMillis[i]-pulseLastRaisedMillis[i+1];

    }
    avrgmillis/=14;
    pulse = (60*1000)/avrgmillis;
  }
  else if(pulseExists==0)
  {
    pulseRaisedEdge=false;
  }
#ifdef PULSE_DEBUG
  Serial.print("pulseRead:");
  Serial.print(pulseSignalAvrg);
  Serial.print("; pulseLongAvrg:");
  Serial.print(pulseLongAvrg);
  Serial.print("; pulseExists:");
  Serial.print(pulseExists);
  Serial.print("; maxLast1000:");
  Serial.print(pulseMaxLast1000);
  Serial.print("; pulse:");
  Serial.print(pulse);
  Serial.print("; lastLong:");
  Serial.print(pulseMaxLastLong);
  Serial.println(";");
#endif
}

//DHT11:
Bonezegei_DHT11 dht(5);
float temp;
int hum;


//RTC DS1302
ThreeWire myWire(9, 8, 10); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);
int hour;
int minute;

//DS18B20
DS18B20 ds(1);

bool SOS = false;

//TTP299_SPI
TTP229_SPI ttp229(3, 4);

/* Uncomment the initialize the I2C address , uncomment only one, If you get a totally blank screen try the other*/
#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
//#define i2c_Address 0x3d //initialize with the I2C addr 0x3D Typically Adafruit OLED's
//#define i2c_Address 0x78

//MPU6050
Adafruit_MPU6050 mpu;
void mpuInit() {
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");
  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setMotionDetectionThreshold(1);
  mpu.setMotionDetectionDuration(20);
  mpu.setInterruptPinLatch(true);	// Keep it latched.  Will turn off when reinitialized.
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);
}
float ax,ay,az,gx,gy,gz,mpu_temp;
float max_acc_dilated;
void mpuLoop() {
  sensors_event_t a,g,temp;
  mpu.getEvent(&a,&g,&temp);
  ax=a.acceleration.x;
  ay=a.acceleration.y;
  az=a.acceleration.z;
  gx=g.gyro.x;
  gy=g.gyro.y;
  gz=g.gyro.z;
  mpu_temp = temp.temperature;
  float acc = sqrt(ax*ax+ay*ay+az*az);
  if(acc > 19)
    SOS = true;
  max_acc_dilated*=0.995;
  max_acc_dilated=max(max_acc_dilated,acc);
}


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2


#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000
};


float dsTemp;
int ds_last_time = 0;
void dsTryGetReading() {
  if(millis() - ds_last_time < 100)
    return;
  ds_last_time = millis();
  if(ds.selectNext())
  {
    dsTemp = ds.getTempC();
  }
}

// ESP NOW SETUP

uint8_t broadcastAddress[] = {0x98, 0x3d, 0xae, 0x52, 0x97, 0x4c};
esp_now_peer_info_t peerInfo;
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
int last_key_down = 0;
int disp_last_time = 0;
void keyDownCb(uint16_t key) {
  if(key == 0 && last_key_down == 0)
    SOS=!SOS;
  if(key != last_key_down)
    disp_last_time=0;
  last_key_down=key;
}

void setup()   {
  Wire.begin(6, 7);
  //Serial.begin(9600);
  pinMode(21, OUTPUT);
  digitalWrite(21, LOW);
  dht.begin();



  delay(250); // wait for the OLED to power up
  display.begin(i2c_Address, true); // Address 0x3C default
  display.display();
  delay(2000);
  display.clearDisplay();

  //RTC

  Serial.print("compiled: ");
  Serial.print(__DATE__);
  Serial.println(__TIME__);
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  // printDateTime(compiled);
  Serial.println();

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  ttp229.begin();
  ttp229.setCbDown(keyDownCb);

  pinMode(0, INPUT);

  mpuInit();
  pinMode(PULSE_PIN, INPUT);
}
int RTC_last_time = 0;
void RTC() {
  if(millis() - RTC_last_time < 1000)
    return;
  RTC_last_time = millis();
  RtcDateTime now = Rtc.GetDateTime();
  printDateTime(now);
  // printDateTime(now);
}
void printDateTime(const RtcDateTime& dt)
{
  Serial.print(dt.Hour());
  Serial.print(":");
  Serial.println(dt.Minute());
  hour = dt.Hour();
  minute = dt.Minute();
}
int last_dht = 0;
void DHT11() {
  if(millis() - last_dht < 2000)
    return;
  last_dht=millis();
  if (dht.getData()) {                         // get All data from DHT11
    temp = dht.getTemperature();      // return temperature in celsius
    //float tempFar = dht.getTemperature(true);  // return temperature in fahrenheit if true celsius of false
    hum = dht.getHumidity();               // return humidity
    String str  = "Temperature: ";
    str += temp;
    str += "°C  ";
    str += "Humidity:";
    str += hum;
    Serial.println(str.c_str());
  }
}
// mq2
int mq2_filtered = 0;
#define MQ2_FILTER_LEN 10
void readMQ2() {
  int mq2_read = analogRead(0);
  mq2_filtered = (mq2_filtered*(MQ2_FILTER_LEN-1)+mq2_read)/MQ2_FILTER_LEN;
}

struct DataToCyclops {
  float temp;
  float body_temp;
  int pulse;
  int min;
  int hour;
  bool SOS;
  float ax;
  float ay;
  float az;
  float gx;
  float gy;
  float gz;
};
DataToCyclops latestData;

void disptest() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Failure is always an option");
  display.setTextColor(SH110X_BLACK, SH110X_WHITE); // 'inverted' text
  display.println(3.141592);
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.print("0x"); display.println(0xDEADBEEF, HEX);
  display.display();
  delay(2000);
  display.clearDisplay();
}
bool display_sos = false;
void disp0() {
  if(millis() - disp_last_time < 100)
    return;
  disp_last_time = millis();
  display.clearDisplay();
  display.drawLine(64,0,64,64,SH110X_WHITE);
  display.drawLine(96,0,96,8,SH110X_WHITE);
  display.drawLine(0,44,128,44,SH110X_WHITE);
  display.drawLine(64,9,128,9,SH110X_WHITE);

  display.setCursor(66,1);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.printf("%02.1fC",latestData.temp);

  display.setCursor(98,1);
  display.printf("%02d:%02d",latestData.hour,latestData.min);

  display.setCursor(80, 11);
  display.setTextSize(2);
  display.printf("%3d",latestData.pulse);
  display.setCursor(80, 29);
  display.printf("BPM");

  display.setCursor(68, 47);
  display.printf("%02.1fC",latestData.body_temp);

  if(latestData.SOS && !display_sos)
  {
    display_sos = true;
    display.fillRect(0,45,64,64,SH110X_WHITE);
    display.setTextSize(2);
    display.setTextColor(SH110X_BLACK);
    display.setCursor(2, 47);
    display.print("S.O.S");
  } else if(latestData.SOS)
  {
    display_sos = false;
    display.fillRect(0,45,64,64,SH110X_BLACK);
    display.setTextSize(2);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(2, 47);
    display.print("S.O.S");
  } else
  {
    display.fillRect(0,45,64,64,SH110X_BLACK);
  }

  // xaxis
  display.drawLine(6,34,28,34,SH110X_WHITE);
  display.drawLine(20,30,28,34,SH110X_WHITE);
  display.drawLine(20,38,28,34,SH110X_WHITE);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(28,24);
  display.printf("%2.1fg",latestData.ax);

  // zaxis
  display.drawLine(6,10,6,34,SH110X_WHITE);
  display.drawLine(2,18,6,10,SH110X_WHITE);
  display.drawLine(10,18,6,10,SH110X_WHITE);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(14,14);
  display.printf("%2.1fg",latestData.az);

  // yaxis
  display.drawLine(6,34,22,22,SH110X_WHITE);
  display.drawLine(16,22,22,22,SH110X_WHITE);
  display.drawLine(22,22,22,28,SH110X_WHITE);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(2,1);
  display.printf("%2.1fg",latestData.ay);
  display.display();
}

void disp1() {
  if(millis() - disp_last_time < 100)
    return;
  disp_last_time = millis();
  display.clearDisplay();

  display.display();
}
void disp2() {
  if(millis() - disp_last_time < 100)
    return;
  disp_last_time = millis();
  display.clearDisplay();
  
  display.display();
}
void disp3() {
  if(millis() - disp_last_time < 100)
    return;
  disp_last_time = millis();
  display.clearDisplay();
  
  display.display();
}
void disp4() {
  if(millis() - disp_last_time < 100)
    return;
  disp_last_time = millis();
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("GAS");
  display.setTextSize(4);
  display.printf("%02.0f",(((float)mq2_filtered/4096)*100.0));
  display.print("%");
  
  display.display();
}
void disp5() {
  if(millis() - disp_last_time < 100)
    return;
  disp_last_time = millis();
  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(40, 0);
  display.printf("%02d",hour);
  display.setCursor(40, 32);
  display.printf("%02d",minute);
  
  display.display();
}
void disp6() {
  if(millis() - disp_last_time < 100)
    return;
  disp_last_time = millis();
  display.clearDisplay();
  
  display.display();
}
void disp7() {
  if(millis() - disp_last_time < 100)
    return;
  disp_last_time = millis();
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.print("Temp: ");
  display.printf("%.0fC\n",temp);
  display.print("Hum: ");
  display.println(hum);
  display.print("T: ");
  display.printf("%02d",hour);
   display.print(":");
  display.printf("%02d\n",minute);
  display.printf("%.1f\n",dsTemp);
  display.printf("%d\n",last_key_down);
  display.printf("%d\n",mq2_filtered);
  display.printf("%f\n",mpu_temp);
  display.printf("%f\n",max_acc_dilated);
  display.display();
}
void SOSBeacon()
{
  if(SOS)
    digitalWrite(21, HIGH);
  else
    digitalWrite(21, LOW);
}


int send_last_time = 0;
void prepareDataAndSend() {
  if(millis() - send_last_time < 500)
    return;
  send_last_time = millis();
  latestData.temp = temp;
  latestData.body_temp = dsTemp;
  latestData.hour = hour;
  latestData.min = minute;
  latestData.pulse = pulse;
  latestData.SOS = SOS;
  latestData.ax = ax/10;
  latestData.ay = ay/10;
  latestData.az = az/10;
  latestData.gx = gx;
  latestData.gy = gy;
  latestData.gz = gz;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &latestData, sizeof(latestData));
}
void loop() {
  DHT11();
  RTC();
  readMQ2();
  dsTryGetReading();
  prepareDataAndSend();
  SOSBeacon();
  ttp229.readKeys();
  mpuLoop();
  pulseLoop();
  switch(last_key_down) {
    case 0:
      disp0();
      break;
    case 1:
      disp1();
      break;
    case 2:
      disp2();
      break;
    case 3:
      disp3();
      break;
    case 4:
      disp4();
      break;
    case 5:
      disp5();
      break;
    case 6:
      disp6();
      break;
    case 7:
      disp7();
      break;
  }
  delay(5);
}
