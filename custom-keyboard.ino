#include "MIDIUSB.h"

#define ROWS 14
#define COLS 12
#define KEYS 76
#define NA 99
#define MAX_NOTES 98

const double VELOCITY_MAX = 150;
int keyPresses[] = {};

uint8_t colPins[COLS] = {28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39};
uint8_t rowPins[ROWS] = {40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53};

// Notes can be found here: https://syntheway.com/MIDI_Keyboards_Middle_C_MIDI_Note_Number_60_C4.htm

// NA denotes unmapped matrix value (defined above)
const byte noteMap[ROWS][COLS] = {
// 01  02  03  04  05  06  07  08  09  10  11  12
  {27, 27, 26, 26, 25, 25, 30, 30, 29, 29, 28, 28}, // 1
  {21, 21, 20, 20, 19, 19, 24, 24, 23, 23, 22, 22}, // 2
  {NA, NA, NA, NA, NA, NA, 18, 18, 17, 17, 16, 16}, // 3
  {33, 33, 32, 32, 31, 31, 36, 36, 35, 35, 34, 34}, // 4
  {39, 39, 38, 38, 37, 37, 42, 42, 41, 41, 40, 40}, // 5
  {45, 45, 44, 44, 43, 43, 48, 48, 47, 47, 46, 46}, // 6
  {51, 51, 50, 50, 49, 49, 54, 54, 53, 53, 52, 52}, // 7
  {57, 57, 56, 56, 55, 55, 60, 60, 59, 59, 58, 58}, // 8
  {63, 63, 62, 62, 61, 61, 66, 66, 65, 65, 64, 64}, // 9
  {NA, NA, NA, NA, 91, 91, NA, NA, NA, NA, NA, NA}, // 10
  {87, 87, 86, 86, 85, 85, 90, 90, 89, 89, 88, 88}, // 11
  {81, 81, 80, 80, 79, 79, 84, 84, 83, 83, 82, 82}, // 12
  {75, 75, 74, 74, 73, 73, 78, 78, 77, 77, 76, 76}, // 13
  {69, 69, 68, 68, 67, 67, 72, 72, 71, 71, 70, 70}, // 14
};

byte keyStates[COLS][ROWS];
byte noteStates[MAX_NOTES];
int velocityStates[MAX_NOTES];

void setup() {
  for(int x = 0; x < COLS; x++) {
    pinMode(colPins[x], INPUT);

    for(int y = 0; y < ROWS; y++) {
      keyStates[x][y] = LOW;
    }
  }

  for(int y = 0; y < ROWS; y++) {
    pinMode(rowPins[y], INPUT);
  }

  for(int i = 0; i < MAX_NOTES; i++) {
    noteStates[i] = LOW;
    velocityStates[i] = -1;
  }

  delay(2000);
  
  Serial.begin(115200);
  Serial.println("Initialized!");
}

int pressIteration = 0;

double averageVelocity(int newValue) {
  keyPresses[pressIteration] = newValue;
  double total = 0;
  double average = 0;

  for(int i = 0; i <= pressIteration; i++) {
    total = total + keyPresses[i];
  }
  pressIteration++;
  Serial.print("New velocity added: ");
  Serial.print(newValue);  
  Serial.print(" Total: ");
  Serial.print(total);
  Serial.print(" - Iteration: ");
  Serial.print(pressIteration);
  
  average = total / pressIteration;
  
  
  Serial.print(" - Average speed of ");
  Serial.print(average);
  Serial.println();
}

int getVelocity(byte key) {
  const int currentTime = millis();
  const int savedTime = velocityStates[key];
  Serial.print("Key: ");
  Serial.print(key);
  Serial.print(" - Current: ");
  Serial.print(currentTime);
  Serial.print(" - Saved: ");
  Serial.print(savedTime);
  
  if(savedTime != -1) {
    velocityStates[key] = -1;
    double difference = currentTime - savedTime;
    //averageVelocity(difference);
    Serial.print(" - Diff: ");
    Serial.print(difference);
    Serial.println();
    return calculateVelocity(difference);
  }
  Serial.print(" - Diff: N/A");
  Serial.println();
  velocityStates[key] = currentTime;

  return -1;
}

int calculateVelocity(double time) {
  double cappedTime = time > VELOCITY_MAX ? VELOCITY_MAX : time;
  double standardVelocity = 64;

  double percentage = cappedTime / VELOCITY_MAX;
  int velocity = 128 - (percentage * 128);

  return velocity > 0 ? velocity : 10;
}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void readMatrix() {
  int velocity;
  byte currentPin;
  byte currentNote;

  for(int x = 0; x < COLS; x++) {
    pinMode(colPins[x], OUTPUT);
    digitalWrite(colPins[x], HIGH);

    for(int y = 0; y < ROWS; y++) {
      currentPin = digitalRead(rowPins[y]);
      currentNote = noteMap[y][x];

      if(keyStates[x][y] != currentPin) {
        if(currentNote != NA) {
          if(currentPin == HIGH) {
            velocity = getVelocity(currentNote);

            if(velocity != -1) {
              // Serial.print(currentNote);
              // Serial.print(" pressed with velocity: ");
              // Serial.print(velocity);
              // Serial.println();
              noteStates[currentNote] = currentPin;
              noteOn(0, currentNote, 64); // Tweak velocity later. For now, just play
            }           
          } else if(noteStates[currentNote] != currentPin) {
            Serial.print(currentNote);
            Serial.print(" released.");
            Serial.println();
            noteStates[currentNote] = currentPin;
            noteOff(0, currentNote, 64); // Will need to handle velocity later!
          }
        }

        keyStates[x][y] = currentPin;
      }
    }

    pinMode(colPins[x], INPUT);
  }
}

void loop() {
  readMatrix();
}
