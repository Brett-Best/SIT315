#include "arduino.hpp"
#include "board.h"
#include "arduino.hpp"
#include "board.h"

static const int sBaudRate = 9600;

enum Pin {
  LED1 = GPIO_PIN(0, 12),
  LED2 = GPIO_PIN(0, 11),
  PIR = GPIO_PIN(3, 2)
};

volatile bool motionDetected = false;

void toggleLED(void *arg);

void setup()
{
  Serial.begin(sBaudRate);

  pinMode(Pin::LED1, OUTPUT);
  pinMode(Pin::LED2, OUTPUT);
  pinMode(Pin::PIR, INPUT);

  gpio_init_int(GPIO_PIN(3, 2), gpio_mode_t::GPIO_IN, gpio_flank_t::GPIO_BOTH, toggleLED, nullptr);
  
  delay(50);
  
  Serial.println("");
}

void loop()
{
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void toggleLED(void *arg) {
#pragma GCC diagnostic pop
  motionDetected = !motionDetected;
  
  Serial.print("Motion Detected: ");
  Serial.println(motionDetected);

  int state = motionDetected ? HIGH : LOW;
  digitalWrite(Pin::LED1, state);
  digitalWrite(Pin::LED2, state);
}

int main(void)
{
    /* run the Arduino setup */
    setup();
    /* and the event loop */
    while (1) {
        loop();
    }
    /* never reached */
    return 0;
}
