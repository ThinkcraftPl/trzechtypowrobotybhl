void setup() {
  // put your setup code here, to run once:
  pinMode(A1, INPUT);
  pinMode(9, OUTPUT);
  Serial.begin(9600);
}
int avrg = 0;
int max_read = 0;
void loop() {
  // put your main code here, to run repeatedly:
  int read = analogRead(A1);
  max_read = max(max_read, read);
  if(read>500)
  {
    tone(9, 1000);
  }
  else
  {
    noTone(9);
  }
  Serial.print("z:0; r:");
  Serial.print(read);
  Serial.print("; mr:");
  Serial.println(max_read);
  delay(10);
}
