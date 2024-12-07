void setup() {
  // put your setup code here, to run once:
  pinMode(A1, INPUT);
  pinMode(9, OUTPUT);
  Serial.begin(9600);
}
int avrg = 0;
void loop() {
  // put your main code here, to run repeatedly:
  int read = analogRead(A1);
  if(read>500)
  {
  Serial.println(read);
    tone(9, 1000);
  }
  else
  {
    noTone(9);
  }
  delay(10);
}
