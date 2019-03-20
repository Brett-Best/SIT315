static const int sBaudRate = 9600;

enum Pin {
  LED = 13,
  PIR = 2
};

bool motionDetected = false;

void setup()
{
  Serial.begin(sBaudRate);

  pinMode(Pin::LED, OUTPUT);
  pinMode(Pin::PIR, INPUT);

  attachInterrupt(digitalPinToInterrupt(Pin::PIR), toggleLED, CHANGE);
}

void loop()
{
}

void toggleLED() {
  motionDetected = !motionDetected;
  
  Serial.print(motionDetected);

  digitalWrite(Pin::LED, motionDetected ? HIGH : LOW);
}
