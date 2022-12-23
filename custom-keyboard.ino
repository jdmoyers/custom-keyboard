#include "MIDIUSB.h"

#define ROWS 14
#define COLS 12
#define KEYS 76
#define NA 99
#define MAX_NOTES 98

const double VELOCITY_MAX = 150;
const double VELOCITY_AVG = 60;

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

byte keyStates[COLS][ROWS][2];

void setup()
{
  for (int x = 0; x < COLS; x++)
  {
    pinMode(colPins[x], INPUT);

    for (int y = 0; y < ROWS; y++)
    {
      keyStates[x][y][0] = LOW;
      keyStates[x][y][1] = -1;
    }
  }

  for (int y = 0; y < ROWS; y++)
  {
    pinMode(rowPins[y], INPUT);
  }

  delay(2000);

  Serial.begin(115200);
  Serial.println("Initialized!");
}

void handleChange (byte x, byte y, byte thisState) 
{
  int pairedKey = x % 2 == 0 ? x + 1 : x - 1;
  byte pairedState = keyStates[pairedKey][y][0];
  byte pairedTime = keyStates[pairedKey][y][1];
  byte thisTime = thisState == LOW ? -1 : millis();
  Serial.print("Pair: ");
  Serial.print(pairedKey);
  Serial.print(", ");
  Serial.print(y);
  Serial.print(" - ");
  Serial.print(noteMap[y][x]);

  if(pairedState == LOW && thisState == LOW)
  {
    Serial.print(": Released");
    noteOff(0, noteMap[y][x], 128);
    MidiUSB.flush();
  }

  if(pairedState == HIGH && thisState == HIGH)
  { 
    byte difference = pairedTime > thisTime ? pairedTime - thisTime : thisTime - pairedTime;
    int velocity = calculateVelocity(difference);

    
    Serial.print(": Pressed with velocity ");
    Serial.print(velocity);
    
    noteOn(0, noteMap[y][x], velocity);
    MidiUSB.flush();
  }

  Serial.println();
  keyStates[x][y][0] = thisState;
  keyStates[x][y][1] = thisTime;
}

int calculateVelocity(int difference) 
{
  Serial.print(" DIFF: ");
  Serial.print(difference);
  if(difference > VELOCITY_MAX)
  {
    return 1;
  }

  if(difference < VELOCITY_AVG)
  {
    Serial.print(" (quick - ");
    Serial.print(VELOCITY_AVG - difference);
    Serial.print(" - ");
    Serial.print((VELOCITY_AVG - difference) / 64);
    Serial.print(")");
    return 64 + (VELOCITY_AVG - difference);
  }

  Serial.print(" (slow - ");
  Serial.print(difference - VELOCITY_AVG);
  Serial.print(" - ");
  Serial.print((difference - VELOCITY_AVG) / 64);
  Serial.print(")");
  return 64 - (difference - VELOCITY_AVG);
}

void noteOn(byte channel, byte pitch, byte velocity)
{
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity)
{
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void readMatrix()
{
  int velocity;
  byte readValue;
  byte currentNote;

  for (int x = 0; x < COLS; x++)
  {
    pinMode(colPins[x], OUTPUT);
    digitalWrite(colPins[x], HIGH);

    for (int y = 0; y < ROWS; y++)
    {
      readValue = digitalRead(rowPins[y]);
      currentNote = noteMap[y][x];

      if(keyStates[x][y][0] != readValue && currentNote != NA)
      {
        handleChange(x, y, readValue);        
      }
    }

    pinMode(colPins[x], INPUT);
  }
}

void loop()
{
  readMatrix();
}
