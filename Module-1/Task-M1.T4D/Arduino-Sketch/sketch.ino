#include <YetAnotherPcInt.h>

static const int sBaudRate = 9600;

enum Pin {
  LED_BOARD = 13,
  LED_RED = 12,
  LED_BLUE = 11,
  PIR1 = 2,
  PIR2 = 3,
  PIR3 = 4,
  PIR4 = 5,
  PIR5 = 6,
  PIR6 = 7,
  PIR7 = 8,
  PIR8 = 9,
  PIR9 = 10
};

struct PIR {
  int pin;
};

PIR monitoredPIRs[9] = {};

void pinChanged(PIR *pinWrapped, bool pinState);

void setup()
{
  Serial.begin(sBaudRate);

  pinMode(Pin::LED_BOARD, OUTPUT);
  pinMode(Pin::LED_RED, OUTPUT);
  pinMode(Pin::LED_BLUE, OUTPUT);

  digitalWrite(Pin::LED_BOARD, LOW);

  for(int pin = Pin::PIR1; pin <= Pin::PIR9; pin++)
  {
    pinMode(pin, INPUT_PULLUP);
    monitoredPIRs[pin-Pin::PIR1] = { pin };
    PcInt::attachInterrupt(pin, pinChanged, &monitoredPIRs[pin-Pin::PIR1], CHANGE);
  }
}

void loop() {}

void pinChanged(PIR *pir, bool pinState) {
  int LED = 0 == pir->pin % 2 ? Pin::LED_RED : Pin::LED_BLUE;
  
  Serial.print("Led: ");
  Serial.print(0 == pir->pin % 2 ? "Red" : "Blue");
  Serial.print(", Motion Detected: ");
  Serial.println(pir->pin % 2);

  digitalWrite(LED, pinState ? HIGH : LOW);
}
