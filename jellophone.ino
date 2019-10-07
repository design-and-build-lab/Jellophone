/*  Jellophone
    2015.12.24
    Kristof Aldenderfer and Dennis Aman
    altered 2019.06.13
    Javian Biswas and Hale Stolberg
    aldenderfer.github.io
    WHAT'S NEW
        - square waves
    TODO:
        - check for proper calibration
        - include automatic gain adjustment?
        - abstract patch cords?
        - add recalibrate button?
*/
// --------------------- BEGIN USER OPTIONS ---------------------
#define ENV_DEL      0     // envelope delay (ms)
#define ENV_ATT      4     // envelope attack (ms)
#define ENV_HOLD     0.5   // envelope hold (ms)
#define ENV_DEC     15     // envelope decay (ms)
#define ENV_SUS      1.0   // envelope sustain (ms)
#define ENV_REL    100     // envelope release (ms)
#define MAX_AMP      0.24  // max amplitude of each wave (0 to 1)
#define MAX_BRITE   31     // max LED brightness (PWM)
#define NUM_PIX     12     // number of pixels per ring
boolean debug = true;
const byte inputs[] = { 0, 1, 17, 16, 32, 25, 33 };
const byte outputPin = 17;
const byte statusLED = 13;
const float freqs[] = { 261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.99 };
// Frequently needed frequencies:
// C Major Diatonic:      261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.99, 523.25
// Two Octaves of 04-EDO: 329.62, 392.00, 466.16, 554.37, 659.26, 784.00, 932.33, 1108.73
// ---------------------- END USER OPTIONS ----------------------
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>
boolean isNotePlaying[] = { false, false, false, false, false, false, false };
const int NUM_NOTES = (sizeof(inputs) / sizeof(inputs[0]));
int calibs[NUM_NOTES];
Adafruit_NeoPixel LEDs = Adafruit_NeoPixel(NUM_PIX*NUM_NOTES, outputPin, NEO_GRB + NEO_KHZ800);
// GUItool: begin automatically generated code
AudioSynthWaveform       waveforms[7];      //xy=95,396
//AudioSynthWaveform       waveform1;      //xy=97,31
//AudioSynthWaveform       waveform3;      //xy=97,123
//AudioSynthWaveform       waveform4;      //xy=98,168
//AudioSynthWaveform       waveform2;      //xy=99,77
AudioEffectEnvelope      envelopes[7];      //xy=301,312
//AudioEffectEnvelope      envelope3;      //xy=304,122
//AudioEffectEnvelope      envelope4;      //xy=304,167
//AudioEffectEnvelope      envelope1;      //xy=305,31
//AudioEffectEnvelope      envelope2;      //xy=305,77
AudioMixer4              mixer1;         //xy=474,102
AudioMixer4              mixer2;         //xy=636,127
AudioOutputI2S           i2s1;           //xy=778,115
AudioConnection          patchCord0(waveforms[0], envelopes[0]);
AudioConnection          patchCord1(waveforms[1], envelopes[1]);
AudioConnection          patchCord2(waveforms[2], envelopes[2]);
AudioConnection          patchCord3(waveforms[3], envelopes[3]);
AudioConnection          patchCord4(waveforms[4], envelopes[4]);
AudioConnection          patchCord5(waveforms[5], envelopes[5]);
AudioConnection          patchCord6(waveforms[6], envelopes[6]);
AudioConnection          patchCord7(envelopes[0], 0, mixer1, 0);
AudioConnection          patchCord8(envelopes[1], 0, mixer1, 1);
AudioConnection          patchCord9(envelopes[2], 0, mixer1, 2);
AudioConnection          patchCord10(envelopes[3], 0, mixer1, 3);
AudioConnection          patchCord11(envelopes[4], 0, mixer2, 1);
AudioConnection          patchCord12(envelopes[5], 0, mixer2, 2);
AudioConnection          patchCord13(envelopes[6], 0, mixer2, 3);
AudioConnection          patchCord14(mixer1, 0, mixer2, 0);
AudioConnection          patchCord15(mixer2, 0, i2s1, 1);
AudioConnection          patchCord16(mixer2, 0, i2s1, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=604,442
// GUItool: end automatically generated code
void setup() {
  AudioMemory(20);
  for ( int i = 0 ; i < NUM_NOTES ; i++ ) {
    pinMode(inputs[i], INPUT);
    waveforms[i].begin(WAVEFORM_SQUARE);
    waveforms[i].amplitude(MAX_AMP);
    waveforms[i].frequency(freqs[i]);
    envelopes[i].delay(ENV_DEL);
    envelopes[i].attack(ENV_ATT);
    envelopes[i].hold(ENV_HOLD);
    envelopes[i].decay(ENV_DEC);
    envelopes[i].sustain(ENV_SUS);
    envelopes[i].release(ENV_REL);
  }
  pinMode(outputPin, OUTPUT);
  pinMode(statusLED, OUTPUT);
  digitalWrite(13, HIGH);
  LEDs.begin();
  LEDs.show();
  Serial.begin(9600);
  calibrateSensors();
  if (debug) {
    testNotes();
  }
}
void loop() {
  int values[NUM_NOTES];
  for ( int i = 0 ; i < NUM_NOTES ; i++ ) {
    values[i] = touchRead(inputs[i]);
    if (debug) {
      Serial.print(values[i]);
      Serial.print("\t");
    }
    if (values[i] > calibs[i] && !isNotePlaying[i]) {
      isNotePlaying[i] = !isNotePlaying[i];
      envelopes[i].noteOn();
      triggerLEDs(i, LEDs.Color(MAX_BRITE, MAX_BRITE, MAX_BRITE));
    } else if (values[i] < calibs[i] && isNotePlaying[i]) {
      envelopes[i].noteOff();
      triggerLEDs(i, LEDs.Color(0, 0, 0));
      isNotePlaying[i] = !isNotePlaying[i];
    }
  }
  if (debug) verbosePrint();
}
void calibrateSensors() {
  unsigned long runningTotals[NUM_NOTES];
  int samples = 0;
  boolean statusState = true;
  Serial.print("Calibrating... ");
  int startTime = millis();
  while ( millis() - startTime < 4000 ) {
    for (int i = 0 ; i < NUM_NOTES ; i++) {
      runningTotals[i] += touchRead(inputs[i]);
    }
    samples++;
    if (samples % 100 == 0) {
      digitalWrite(statusLED, statusState);
      statusState = !statusState;
    }
  }
  Serial.print("Done.");
  if (debug) {
    Serial.println(" Calibration values:");
    for (int i = 0 ; i < NUM_NOTES ; i++) {
      Serial.print(i);
      Serial.print("\t");
    }
  }
  Serial.println();
  for (int i = 0 ; i < NUM_NOTES ; i++) {
    calibs[i] = runningTotals[i] / samples;
    if (calibs[i] > 3000) calibs[i] = 1000; //CALIBRATION HACK
    Serial.print(calibs[i]);
    Serial.print("\t");
    calibs[i] *= 2;
  }
  Serial.println();
}
void testNotes() {
  for (int i = 0 ; i < NUM_NOTES ; i++) {
    Serial.print("Playing note ");
    Serial.print(i);
    Serial.print(", frequency: ");
    Serial.print(freqs[i]);
    Serial.println("Hz.");
    triggerLEDs(i, LEDs.Color(MAX_BRITE, MAX_BRITE, MAX_BRITE));
    envelopes[i].noteOn();
    delay(500);
    triggerLEDs(i, LEDs.Color(0, 0, 0));
    envelopes[i].noteOff();
    delay(ENV_REL);
  }
}
void triggerLEDs(uint8_t ring, uint32_t color) {
  byte pixToStart = ring * NUM_PIX;
  for (int i = pixToStart ; i < pixToStart + NUM_PIX ; i++) {
    LEDs.setPixelColor(i, color);
  }
  LEDs.show();
}
void verbosePrint() {
  Serial.print("Usage: ");
  Serial.print(AudioMemoryUsage());
  Serial.print(" (curr)\t");
  Serial.print(AudioMemoryUsageMax());
  Serial.print(" (max)\t| ");
  Serial.print("note(s) playing: ");
  for ( int i = 0 ; i < NUM_NOTES ; i++ ) {
    if (isNotePlaying[i]) {
      Serial.print(i);
      Serial.print(", ");
    }
  }
  Serial.println();
}
