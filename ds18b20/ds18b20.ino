#include <DS18B20.h>

DS18B20 ds(2);

void setup() {
  Serial.begin(115200);
}

void loop() {
  while (ds.selectNext()) {
    Serial.println(ds.getTempC());
  }
}