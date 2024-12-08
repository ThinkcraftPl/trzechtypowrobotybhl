#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#define BUTTON_1 2
#define BUZZER 3
#define LASER 1
#define PHOTORESISTOR 0
#define LED 5
struct DataFromPip {
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
DataFromPip lastData = {
  .temp = 23.5,
  .body_temp = 36.5,
  .pulse = 95,
  .min = 37,
  .hour = 21,
  .SOS = false,
  .ax = 0,
  .ay = 0,
  .az = 9.81,
  .gx = 0.1,
  .gy = -1,
  .gz = 0.2
};
Adafruit_SSD1306 display(128, 64, &Wire, -1);

void displaySetup()
{
  display.display();
  delay(2000);

  display.clearDisplay();
  display.display();
}
#define DISPLAY_UPDATE_SPEED 100 //ms
int display_last_millis = 0;
bool display_sos = false;
void displayLoop()
{
  if(millis() <= display_last_millis + DISPLAY_UPDATE_SPEED)
    return;
  display_last_millis = millis();
  display.clearDisplay();
  display.drawLine(64,0,64,64,SSD1306_WHITE);
  display.drawLine(96,0,96,8,SSD1306_WHITE);
  display.drawLine(0,44,128,44,SSD1306_WHITE);
  display.drawLine(64,9,128,9,SSD1306_WHITE);

  display.setCursor(66,1);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.printf("%02.1fC",lastData.temp);

  display.setCursor(98,1);
  display.printf("%02d:%02d",lastData.hour,lastData.min);

  display.setCursor(80, 11);
  display.setTextSize(2);
  display.printf("%3d",lastData.pulse);
  display.setCursor(80, 29);
  display.printf("BPM");

  display.setCursor(68, 47);
  display.printf("%02.1fC",lastData.body_temp);

  if(lastData.SOS && !display_sos)
  {
    display_sos = true;
    display.fillRect(0,45,64,64,SSD1306_WHITE);
    display.setTextSize(2);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(2, 47);
    display.print("S.O.S");
  } else if(lastData.SOS)
  {
    display_sos = false;
    display.fillRect(0,45,64,64,SSD1306_BLACK);
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(2, 47);
    display.print("S.O.S");
  } else
  {
    display.fillRect(0,45,64,64,SSD1306_BLACK);
  }

  // xaxis
  display.drawLine(6,34,28,34,SSD1306_WHITE);
  display.drawLine(20,30,28,34,SSD1306_WHITE);
  display.drawLine(20,38,28,34,SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(28,24);
  display.printf("%2.1fg",lastData.ax);

  // zaxis
  display.drawLine(6,10,6,34,SSD1306_WHITE);
  display.drawLine(2,18,6,10,SSD1306_WHITE);
  display.drawLine(10,18,6,10,SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(14,14);
  display.printf("%2.1fg",lastData.az);

  // yaxis
  display.drawLine(6,34,22,22,SSD1306_WHITE);
  display.drawLine(16,22,22,22,SSD1306_WHITE);
  display.drawLine(22,22,22,28,SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2,1);
  display.printf("%2.1fg",lastData.ay);


  display.display();
}

void espNowRecv(const uint8_t* mac, const uint8_t *incomingData, int len)
{
  memcpy(&lastData, incomingData, sizeof(DataFromPip));
  Serial.printf("%f %f %f",lastData.gx,lastData.gy,lastData.gz);
}

void setup() {
  Wire.begin(8,9);
  Serial.begin(115200);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  displaySetup();
  
  WiFi.mode(WIFI_STA);
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  uint8_t baseMac[6];
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret == ESP_OK) {
    Serial.printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                  baseMac[0], baseMac[1], baseMac[2],
                  baseMac[3], baseMac[4], baseMac[5]);
  } else {
    Serial.println("Failed to read MAC address");
  }
  
  esp_now_register_recv_cb(esp_now_recv_cb_t(espNowRecv));
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  pinMode(LASER, OUTPUT);
  pinMode(PHOTORESISTOR, INPUT);
  pinMode(LED, OUTPUT);
}
bool button_falling = false;
bool laser = false;
int resistor = 0;
void loop() {
  displayLoop();
  if(!digitalRead(BUTTON_1))
  {
    if(!button_falling) {
      button_falling = true;
      laser = !laser;
    }
  }
  else
  {
    button_falling = false;
  }
  digitalWrite(LASER, laser);
  resistor = analogRead(PHOTORESISTOR);
  Serial.println(resistor);
  if(resistor < 2048)
  {
    digitalWrite(LED, HIGH);
  }
  else
  {
    digitalWrite(LED, LOW);
  }
  if(lastData.SOS)
  {
    tone(BUZZER, 1000);
  }
  else
  {
    noTone(BUZZER);
  }
  delay(10);
}
