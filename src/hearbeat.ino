#include <Wire.h>
#include <TM1637Display.h>
#include "MAX30105.h"
#include "heartRate.h"

// ---------------- DISPLAY ----------------
#define DISPLAY_CLK 28
#define DISPLAY_DAT 29

TM1637Display display(DISPLAY_CLK, DISPLAY_DAT);

// ---------------- SENSOR ----------------
MAX30105 sensor;

// ---------------- BPM FILTER ----------------
const int RATE_SIZE = 4;
int rates[RATE_SIZE];
int rateSpot = 0;

long lastBeat = 0;
int avgBpm = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // --- SENSOR INIT ---
  if (!sensor.begin(Wire)) {
    Serial.println("MAX30102 not found!");
    while (1);
  }

  // Stronger LED signal (important!)
  sensor.setup();
  sensor.setPulseAmplitudeRed(0x2F);
  sensor.setPulseAmplitudeGreen(0);

  // initialize buffer
  for (int i = 0; i < RATE_SIZE; i++) {
    rates[i] = 0;
  }

  display.setBrightness(7);
  display.clear();

  Serial.println("Place finger on sensor...");
}

void loop() {

  sensor.check();
  while (sensor.available()) {

    long irValue = sensor.getIR();

    sensor.nextSample();

    Serial.println(irValue);

    // ---------------- FINGER DETECTION ----------------
    if (irValue < 50000) {
      display.showNumberDec(0);
      continue;
    }

    // ---------------- BEAT DETECTION ----------------
    if (checkForBeat(irValue)) {

      long now = millis();
      long delta = now - lastBeat;
      lastBeat = now;

      if (delta > 0) {
        int bpm = 60000 / delta;

        // ignore unrealistic values
        if (bpm > 30 && bpm < 220) {

          rates[rateSpot++] = bpm;
          rateSpot %= RATE_SIZE;

          int sum = 0;
          int count = 0;

          for (int i = 0; i < RATE_SIZE; i++) {
            if (rates[i] > 0) {
              sum += rates[i];
              count++;
            }
          }

          if (count > 0) {
            avgBpm = sum / count;

            Serial.print("BPM: ");
            Serial.println(avgBpm);

            display.showNumberDec(avgBpm);
          }
        }
      }
    }
  }
}
