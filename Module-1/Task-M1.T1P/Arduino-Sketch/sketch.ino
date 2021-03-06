static const int sBaudRate = 9600;

enum Pin {
  LED = 13,
  PIR = 2
};

void setup()
{
  Serial.begin(sBaudRate);

  pinMode(Pin::LED, OUTPUT);
  pinMode(Pin::PIR, INPUT);
}

void loop()
{
  bool motionDetected = false;
  if (1 == digitalRead(Pin::PIR)) motionDetected = true;
  
  Serial.print(motionDetected);

  digitalWrite(Pin::LED, motionDetected ? HIGH : LOW);

  delay(500);
}
