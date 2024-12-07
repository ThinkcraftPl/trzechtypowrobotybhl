void setup() {
  Serial.begin(115200);
  pinMode(2, INPUT);
}

void loop() {
  Serial.println(analogRead(2));
  delay(10);
}
