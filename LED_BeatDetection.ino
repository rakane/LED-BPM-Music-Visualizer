#include "FastLED.h"

#define NUM_LEDS 300        // How many leds in your strip?
#define updateLEDS 3
#define DATA_PIN 6          // led data transfer
#define SAMPLEPERIODUS 200

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

CRGB leds[NUM_LEDS];
int red, green, blue;
unsigned int count;

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(2));

  // Set ADC to 77khz, max for 10bit
  sbi(ADCSRA, ADPS2);
  cbi(ADCSRA, ADPS1);
  cbi(ADCSRA, ADPS0);

  // Set initial color and count to zero
  red = 255;
  green = 0;
  blue = 0;
  count = 0;

  // Initialize LEDS and set all off
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  for (int i = 0; i < NUM_LEDS ; i++) {
    leds[i] = CRGB(0, 0, 0);
  }
  FastLED.show();
}

// 20 - 200hz Single Pole Bandpass IIR Filter
float bassFilter(float sample) {
  static float xv[3] = {0, 0, 0}, yv[3] = {0, 0, 0};
  xv[0] = xv[1]; xv[1] = xv[2];
  xv[2] = (sample) / 3.f; // change here to values close to 2, to adapt for stronger or weeker sources of line level audio

  yv[0] = yv[1]; yv[1] = yv[2];
  yv[2] = (xv[2] - xv[0])
          + (-0.7960060012f * yv[0]) + (1.7903124146f * yv[1]);
  return yv[2];
}

// 10hz Single Pole Lowpass IIR Filter
float envelopeFilter(float sample) { //10hz low pass
  static float xv[2] = {0, 0}, yv[2] = {0, 0};
  xv[0] = xv[1];
  xv[1] = sample / 50.f;
  yv[0] = yv[1];
  yv[1] = (xv[0] + xv[1]) + (0.9875119299f * yv[0]);
  return yv[1];
}

// 1.7 - 3.0hz Single Pole Bandpass IIR Filter
float beatFilter(float sample) {
  static float xv[3] = {0, 0, 0}, yv[3] = {0, 0, 0};
  xv[0] = xv[1]; xv[1] = xv[2];
  xv[2] = sample / 2.7f;
  yv[0] = yv[1]; yv[1] = yv[2];
  yv[2] = (xv[2] - xv[0])
          + (-0.7169861741f * yv[0]) + (1.4453653501f * yv[1]);
  return yv[2];
}

// Shift all LEDs to the right by updateLEDS number each time
void shiftLEDS() {
  for (int i = NUM_LEDS - 1; i >= updateLEDS; i--) {
    leds[i] = leds[i - updateLEDS];
  }
}

void loop() {
  unsigned long time = micros(); // Used to track rate
  float sample, value, envelope, beat, thresh;

  if (count == 100) {
    red = random(255);
    green = random(255);
    blue = random(255);
    count = 0;
  }

  // Read ADC and center so +-512
  sample = (float)analogRead(0) - 503.f;

  // Filter only bass component
  value = bassFilter(sample);

  // Take signal amplitude and filter
  value = abs(value);
  envelope = envelopeFilter(value);

  // Filter out repeating bass sounds 100 - 180bpm
  beat = beatFilter(envelope);

  // Adjustable threshold
  thresh = 5.5;
  
  //Shift LEDs
  shiftLEDS();
  
  // Check if beat was detected
  if (beat > thresh) {
    // Set the left most LEDS to new color
    for (int i = 0; i < updateLEDS; i++) {
      leds[i] = CRGB(red, green, blue);
    }
  }
  else {
    // Set the left most LEDS off
    for (int i = 0; i < updateLEDS; i++) {
      leds[i] = CRGB(0, 0, 0);
    }
  }

  FastLED.show();
  count++;

  // Consume excess clock cycles, to keep at 5000 hz
  for (unsigned long up = time + SAMPLEPERIODUS; time > 20 && time < up; time = micros());
}
