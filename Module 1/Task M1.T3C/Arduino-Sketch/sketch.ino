static const int sBaudRate = 9600;

enum Pin {
  LED_RED = 12,
  LED_BLUE = 11,
  PIR1 = 2,
  PIR2 = 3
};

bool motionDetected[2] = {false, false};

void setup()
{
  Serial.begin(sBaudRate);

  pinMode(Pin::LED_RED, OUTPUT);
  pinMode(Pin::LED_BLUE, OUTPUT);
  pinMode(Pin::PIR1, INPUT);
  pinMode(Pin::PIR2, INPUT);

  attachInterrupt(digitalPinToInterrupt(Pin::PIR1), toggleLED_RED, CHANGE);
  attachInterrupt(digitalPinToInterrupt(Pin::PIR2), toggleLED_BLUE, CHANGE);
}

void loop() {}

void toggleLED_RED() {
  toggleLED(0);
}

void toggleLED_BLUE() {
  toggleLED(1);
}

void toggleLED(int ledId) {
  int LED = 0 == ledId ? Pin::LED_RED : Pin::LED_BLUE;
  
  motionDetected[ledId] = !motionDetected[ledId];
  
  Serial.print("Led: ");
  Serial.print(0 == ledId ? "Red" : "Blue");
  Serial.print(", Motion Detected: ");
  Serial.println(motionDetected[ledId]);

  digitalWrite(LED, motionDetected[ledId] ? HIGH : LOW);
}
