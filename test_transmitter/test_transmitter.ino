void setup() {
  pinMode(21, OUTPUT);
}
#define OUR_BIT_COUNT 14

void loop() {
  digitalWrite(21, HIGH);
  delay(1000);
  digitalWrite(21, LOW);
  delay(1000);
}
