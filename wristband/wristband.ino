#define PULSE_PIN 0

void setup() {
  pinMode(PULSE_PIN, INPUT);
  Serial.begin(115200);
}

#define PULSE_AVRG_LEN 5
#define PULSE_LONG_LEN 200
//#define PULSE_DEBUG
int pulseSignalAvrg=0;
int pulseLongAvrg=0;
int pulseLast1000[1000];
int pulse;
bool pulseRaisedEdge = false;
int pulseLastRaisedMillis[5];
void pulseLoop() {
  int read = analogRead(PULSE_PIN);
  pulseSignalAvrg = ((PULSE_AVRG_LEN-1)*pulseSignalAvrg+read)/PULSE_AVRG_LEN;
  int pulseMaxLast1000 = read;
  pulseLongAvrg=read;
  for(int i = 998; i>=0; i--)
  {
    pulseLast1000[i+1]=pulseLast1000[i];
    pulseMaxLast1000=max(pulseMaxLast1000, pulseLast1000[i]);
    if(i<PULSE_LONG_LEN-1)
      pulseLongAvrg+=pulseLast1000[i];
  }
  pulseLongAvrg/=PULSE_LONG_LEN;
  pulseLast1000[0]=read;
  int pulseExists = 0;
  if(pulseSignalAvrg > pulseLongAvrg+20)
  {
    pulseExists=1;
  }
  if(pulseExists==1&&!pulseRaisedEdge)
  {
    pulseRaisedEdge=true;
    int avrgmillis = 0;
    for(int i = 3; i>=0; i--)
    {
      pulseLastRaisedMillis[i+1]=pulseLastRaisedMillis[i];
    }
    pulseLastRaisedMillis[0]=millis();
    for(int i = 0; i<4; i++)
    {
      avrgmillis+=pulseLastRaisedMillis[i]-pulseLastRaisedMillis[i+1];

    }
    avrgmillis/=5;
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
  Serial.println(";");
#endif
}

void loop() {
  pulseLoop();
  delay(5);
}
