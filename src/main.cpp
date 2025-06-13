#include <Arduino.h>

byte segmentBits[11] = {
    0b00111111,
    0b00000110,
    0b01011011,
    0b01001111,
    0b01100110,
    0b01101101,
    0b01111101,
    0b00000111,
    0b01111111,
    0b01101111,
    0b00000000};

const uint8_t displayPins[7] = {2, 3, 4, 5, 6, 7, 8};

// turn off the display
const uint8_t BLANK = 10;
const uint8_t MAX_SEGMENTS = 4;

const int JOY_THRESHOLD_HIGH = 600;
const int JOY_THRESHOLD_LOW = 400;
const int JOY_CENTER = 512;
const int JOY_DEAD_ZONE = 100;

const uint8_t joySW = 11;
const uint8_t joyX = A0;
const uint8_t joyY = A1;

const uint8_t buzzerPin = 10;
const uint8_t ledPin = 9;

struct Level
{
  const uint16_t speed;
  const uint8_t length;
};

Level levels[5] = {
    {5000, 10},
    {4000, 10},
    {3000, 15},
    {1000, 30},
    {500, 50}};

const uint8_t levelsLength = sizeof(levels) / sizeof(levels[0]);
uint8_t currentLevelIndex = 0;

uint8_t levelCounter = 1;
bool isLevelMode = false;

bool isDirectionCorrect = false;
bool yMoved = false;

void displayDigit(int digit)
{
  int segmentBit = segmentBits[digit];
  for (int i = 0; i < 7; i++)
  {
    digitalWrite(displayPins[i], (segmentBit >> i) & 1);
  }
}

void setup()
{
  Serial.begin(9600);
  randomSeed(analogRead(A5));

  for (int pin : displayPins)
  {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  pinMode(joySW, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  displayDigit(1);
}

void pulsePin(uint8_t pin, uint16_t time)
{
  digitalWrite(pin, HIGH);
  delay(time);
  digitalWrite(pin, LOW);
}

// setting a random segment
int drawRandomSegment(int maxValue)
{
  int segmentNumber = random(maxValue);

  // turing off all segments
  for (int pin : displayPins)
  {
    digitalWrite(pin, LOW);
  }

  // checking segments to set the radnom segment
  switch (segmentNumber)
  {
  // top
  case 0:
    digitalWrite(displayPins[0], HIGH);
    break;
  // bottom
  case 1:
    digitalWrite(displayPins[3], HIGH);
    break;
  // right
  case 2:
    digitalWrite(displayPins[1], HIGH);
    digitalWrite(displayPins[2], HIGH);
    break;
  case 3:
    // left
    digitalWrite(displayPins[4], HIGH);
    digitalWrite(displayPins[5], HIGH);
    break;
  }

  return segmentNumber;
}

// checking the joystick to get the joystick direction
int handleJoystick(int xValue, int yValue, int value)
{
  int joyValue;
  int dx = xValue - JOY_CENTER;
  int dy = yValue - JOY_CENTER;

  // absolute value
  int absDx = abs(dx);
  int absDy = abs(dy);

  // dead zone
  if (absDx < JOY_DEAD_ZONE && absDy < JOY_DEAD_ZONE)
  {
    Serial.println("Dead zone");
    return -1;
  }

  // checking x value
  if (absDx > absDy)
  {
    joyValue = (dx > 0) ? 1 : 0;
  }
  else
  {
    joyValue = (dy < 0) ? 2 : 3;
  }

  Serial.println(joyValue);
  return joyValue; // joyValue == value
}

// checking is player win
void checkScore(bool checkingValue)
{
  if (checkingValue)
  {
    Serial.println("TRUE WIN");
    pulsePin(ledPin, 500);
  }
  else
  {
    Serial.println("FALSE LOSE");
    pulsePin(buzzerPin, 125);
    delay(375);
  }
}

void updateLevelCounter(int direction) {
  if (direction == 1) levelCounter++;
  else if (direction == -1) levelCounter--;
  
  if (levelCounter > levelsLength) levelCounter = 1;
  else if (levelCounter < 1) levelCounter = levelsLength;
}

// it allows you to choose the number of level
void chooseLevel(int valueY)
{
  displayDigit(levelCounter);

  // checking the direction
  if (valueY > JOY_THRESHOLD_HIGH) updateLevelCounter(-1);
  else if (valueY < JOY_THRESHOLD_LOW) updateLevelCounter(1);

  pulsePin(buzzerPin, 100);
  displayDigit(levelCounter);
  delay(100);
}

void startLevelMode()
{
  // signal for player
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(buzzerPin, HIGH);
    displayDigit(0);
    delay(400);
    displayDigit(10);
    digitalWrite(buzzerPin, LOW);
    delay(400);
  }
  isLevelMode = true;
}

void endLevelMode()
{
  isLevelMode = false;
  pulsePin(buzzerPin, 500);
}

// it alows you to stop or start the level
void switchLevel()
{
  // listening the button
  if (digitalRead(joySW) == LOW)
  {

    // reset the parametrs
    displayDigit(levelCounter);
    currentLevelIndex = 0;

    isLevelMode ? endLevelMode() : startLevelMode();
  }
}

// handle the menu logic
void menu()
{
  int y = analogRead(joyY);

  // checking player click the button to change mode
  switchLevel();

  // checking the joystick
  if (!yMoved)
  {
    if (y > JOY_THRESHOLD_HIGH || y < JOY_THRESHOLD_LOW)
    {
      chooseLevel(y);
      yMoved = true;
    }
  }
  else
  {
    if (y >= 450 && y <= 570)
    {
      yMoved = false;
    }
  }

  delay(50);
}

// checking the level length
bool checkLevelLength()
{
  if (currentLevelIndex >= levels[levelCounter - 1].length)
  {
    isLevelMode = false;
    currentLevelIndex = 0;
    displayDigit(currentLevelIndex);
    return true;
  }

  return false;
}

// checking player move
int playerMove(int displayingSegment)
{
  Serial.println("Player has Move");
  int x = analogRead(joyX);
  int y = analogRead(joyY);

  return handleJoystick(x, y, displayingSegment);
}

void endLevel() {
  checkScore(isDirectionCorrect);

  // reset segments
  displayDigit(BLANK);
  delay(200);

  isDirectionCorrect = false;

  int y = analogRead(joyY);

  if (y > JOY_THRESHOLD_HIGH || y < JOY_THRESHOLD_LOW)
  {
    chooseLevel(y);
  }
}

// handle the level logic
void level()
{
  if (checkLevelLength()) return;

  currentLevelIndex++;
  int randomSegment = drawRandomSegment(MAX_SEGMENTS);

  unsigned long lastSave = millis();

  // listening the level time
  while ((millis() - lastSave) < levels[levelCounter - 1].speed)
  {
    switchLevel();

    if (!isLevelMode) return;

    int moveValue = playerMove(randomSegment);

    if (moveValue > -1) {
      isDirectionCorrect = (moveValue == randomSegment);
      endLevel();  
      return;
    }

    delay(50);
  }

  isDirectionCorrect = false;  
  endLevel();
}

void loop()
{
  if (!isLevelMode)
  {
    menu();
  }
  else
  {
    level();
  }
}