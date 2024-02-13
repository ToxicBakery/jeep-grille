#include <FastLED.h>

#define BLINKER_PIN_1 4
#define BLINKER_PIN_2 5
#define HEADLIGHT_PIN_1 6
#define HEADLIGHT_PIN_2 7

#define NUM_LEDS_BLINKER 40
#define NUM_LEDS_HEADLIGHT 15
#define LIGHT_COUNT 4

CHSV white = CHSV(33, 12, 100);
CHSV whiteDim = CHSV(47, 15, 100);
CHSV amber = CHSV(45, 100, 100);
CHSV amberDim = CHSV(40, 100, 100);
CHSV black = CHSV(0, 0, 0);

CRGB ledsBlinker1[NUM_LEDS_BLINKER];
CRGB ledsBlinker2[NUM_LEDS_BLINKER];
CRGB ledsHeadlight1[NUM_LEDS_HEADLIGHT];
CRGB ledsHeadlight2[NUM_LEDS_HEADLIGHT];

struct pattern {
  CHSV color;
  unsigned long duration;
  bool interpolate;
};

struct light {
  CRGB* ledPtr;
  int ledCount;
  pattern* currentPattern;
  pattern* nextPattern;
  unsigned long endTime;
};

struct pattern patternsHeadlight[] = {
  { white, 150000l, false },
  { black, 300l, false },
  { white, 3000l, false },
  { black, 500l, false },
  { white, 100l, true },
  { whiteDim, 1000l, true },
};

struct pattern patternsBlinker[] = {
  { amber, 125000l, false },
  { black, 300l, false },
  { amber, 3000l, false },
  { black, 500l, false },
  { amber, 100l, true },
  { amberDim, 1000l, true },
};

struct light lights[] = {
  { ledsBlinker1, NUM_LEDS_BLINKER, 0, 0, 0l },
  { ledsBlinker2, NUM_LEDS_BLINKER, 0, 0, 0l },
  { ledsHeadlight1, NUM_LEDS_HEADLIGHT, 0, 0, 0l },
  { ledsHeadlight2, NUM_LEDS_HEADLIGHT, 0, 0, 0l },
};

pattern* getNextPatternHeadlight() {
  int randomValue = random(2);
  if (randomValue == 0) {
    return &patternsHeadlight[0];
  } else {
    randomValue = random(5) + 1;
    return &patternsHeadlight[randomValue];
  }
}

pattern* getNextPatternBlinker() {
  int randomValue = random(2);
  if (randomValue == 0) {
    return &patternsBlinker[0];
  } else {
    randomValue = random(5) + 1;
    return &patternsBlinker[randomValue];
  }
}

int difference(int a, int b) {
  return abs(a - b);
}

void lerp1D(CHSV c1, CHSV c2, double interpolation, struct light* lightPtr) {
  int hue = difference(c1.hue, c2.hue) * interpolation;
  int sat = difference(c1.sat, c2.sat) * interpolation;
  int val = difference(c1.val, c2.val) * interpolation;
  for (int j = 0; j < lightPtr->ledCount; j++) {
    lightPtr->ledPtr[j].setHSV(hue, sat, val);
  }
}

void setup() {
  Serial.begin(9600);
  FastLED.addLeds<WS2812B, BLINKER_PIN_1>(ledsBlinker1, NUM_LEDS_BLINKER);
  FastLED.addLeds<WS2812B, BLINKER_PIN_2>(ledsBlinker2, NUM_LEDS_BLINKER);
  FastLED.addLeds<WS2812B, HEADLIGHT_PIN_1>(ledsHeadlight1, NUM_LEDS_HEADLIGHT);
  FastLED.addLeds<WS2812B, HEADLIGHT_PIN_2>(ledsHeadlight2, NUM_LEDS_HEADLIGHT);

  unsigned long currentTime = millis();
  struct light* lightPtr = lights;
  for (int i = 0; i < LIGHT_COUNT; i++, lightPtr++) {
    if (i < 2) {
      lightPtr->currentPattern = getNextPatternHeadlight();
      lightPtr->nextPattern = getNextPatternHeadlight();
    } else {
      lightPtr->currentPattern = getNextPatternBlinker();
      lightPtr->nextPattern = getNextPatternBlinker();
    }
    lightPtr->endTime = currentTime + lightPtr->currentPattern->duration;
  }
}

void (*resetFunc)(void) = &setup;

void loop() {
  unsigned long currentTime = millis();

  // Max unsigned long value is 4294967295 for an atmega, reset before we roll over
  if (currentTime > 4233600000l) {
    resetFunc();
  }

  struct light* lightPtr = lights;
  for (int i = 0; i < LIGHT_COUNT; i++, lightPtr++) {
    // Shift the patterns
    if (lightPtr->endTime < currentTime) {
      lightPtr->currentPattern = lightPtr->nextPattern;
      if (i < 2) {
        lightPtr->nextPattern = getNextPatternHeadlight();
      } else {
        lightPtr->nextPattern = getNextPatternBlinker();
      }
      lightPtr->endTime = currentTime + lightPtr->currentPattern->duration;
    }

    if (lightPtr->currentPattern->interpolate) {
      lerp1D(
        lightPtr->currentPattern->color,
        lightPtr->nextPattern->color,
        (double)(lightPtr->endTime - currentTime) / (double)lightPtr->currentPattern->duration,
        lightPtr);
    } else {
      for (int j = 0; j < lightPtr->ledCount; j++) {
        lightPtr->ledPtr[j] = lightPtr->currentPattern->color;
        if (i >= 2) {
          lightPtr->ledPtr[j].nscale8_video(100);
        }
      }
    }
  }

  FastLED.show();
  delay(16);
}
