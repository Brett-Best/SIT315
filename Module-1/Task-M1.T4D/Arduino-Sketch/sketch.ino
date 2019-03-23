#include <YetAnotherPcInt.h>

static const int sBaudRate = 9600;

const uint16_t tl_load = 0;
const uint16_t tl_comp = 31250;

enum Pin {
  LED_BOARD = PB5,
  LED_RED = 12,
  LED_BLUE = 11,
  PIR1 = 2,
  PIR2 = 3,
  PIR3 = 4,
  PIR4 = 5
};

struct PIR {
  int pin;
};

PIR monitoredPIRs[4] = {};

void pinChanged(PIR *pinWrapped, bool pinState);

void setup()
{
  Serial.begin(sBaudRate);

  pinMode(Pin::LED_RED, OUTPUT);
  pinMode(Pin::LED_BLUE, OUTPUT);

  for (int pin = Pin::PIR1; pin <= Pin::PIR4; pin++)
  {
    pinMode(pin, INPUT_PULLUP);
    monitoredPIRs[pin-Pin::PIR1] = { pin };
    PcInt::attachInterrupt(pin, pinChanged, &monitoredPIRs[pin-Pin::PIR1], CHANGE);
  }

  // Set the onboard led pin to OUTPUT
  DDRB |= (1 << Pin::LED_BOARD);

  // Reset Timer1 Control Register
  TCCR1A = 0;

  // CTC mode configuration
  TCCR1B &= (1 << WGM13);
  TCCR1B |= (1 << WGM12);

  // Set to prescaler 1024
  TCCR1B |= (1 << CS12);
  TCCR1B &= ~(1 << CS11);
  TCCR1B |= (1 << CS10);

  // Reset Timer1 and set the compare value
  TCNT1 = tl_load;
  OCR1A = tl_comp;

  // Enable Timer1 compare interrupt
  TIMSK1 = (1 << OCIE1A);

  // Enable global interrupts
  sei();
}

void loop() {
}

ISR(TIMER1_COMPA_vect) {
  Serial.println("Timer Fired");
  PORTB ^= (1 << Pin::LED_BOARD);
}

void pinChanged(PIR *pir, bool pinState) {
  int LED = 0 == pir->pin % 2 ? Pin::LED_RED : Pin::LED_BLUE;
  
  Serial.print("Led: ");
  Serial.print(0 == pir->pin % 2 ? "Red" : "Blue");
  Serial.print(", Pin: ");
  Serial.print(pir->pin);
  Serial.print(", Motion Detected: ");
  Serial.println(pinState);

  digitalWrite(LED, pinState ? HIGH : LOW);
}
