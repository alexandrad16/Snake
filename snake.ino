#include <LedControl.h> // LedControl library is used for controlling a LED matrix.
#include <LiquidCrystal.h> //LiquidCrystal library is used for controlling LCD displays.

LiquidCrystal lcd (2, 3, 4, 5, 6, 7);
LedControl lc = LedControl (12, 11, 10, 1);

//there are defined all the pins
#define JOYSTICK_X  A0
#define JOYSTICK_Y  A1

const short initialSnakeLength = 3; //initial snake length (1-63, recommended 3)

// snake parameters
int snakeLength = initialSnakeLength;
int snakeSpeed = 1; // can not be 0
int snakeDirection = 0; // if it is 0, the snake does not move

// direction constants
const short up     = 1;
const short right  = 2;
const short down   = 3;
const short left   = 4;

bool win = false;
bool gameOver = false;

const int joystickThreshold = 160; // threshold where movement of the joystick will be accepted

struct Point {
  int row = 0;
  int col = 0;
  Point(int row = 0, int col = 0): row(row), col(col) {}
};

struct Coordinate {
  int x = 0;
  int y = 0;
  Coordinate(int x = 0, int y = 0): x(x), y(y) {}
};

Point snake; // the coordinates from which the snake start it will be randomly generated
Point food (-1, -1); // the point for food is not anywhere yet
Coordinate initialJoystick (500, 500);  //construct with default values in case the user turns off the calibration
Coordinate values;

int matrix[8][8]; // the "matrix" array: holds an "age" of the every pixel in the matrix. If the number > 0, it glows.
// when the age of some pixel exceeds the length of the snake, it goes out.
// 1 is added in the current snake direction next to the last position of the snake head.

void setup() {
  Serial.begin(9600); // sets the data rate in bits per second for serial data transmission.

  lc.shutdown(0, false); // turn off power saving, enables display
  lc.setIntensity(0, 9);
  lc.clearDisplay(0); // clear screen

  randomSeed(analogRead(0));
  snake.row = random(8);
  snake.col = random(8);

  calibrateJoystick();

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SNAKE GAME<Start");
  lcd.setCursor(0, 1);
  lcd.print("  Move Joystick");
}

void loop() {
  generateFood(); // if there is no food, generate one
  joystickMove(); // watches joystick movements & blinks with food
  snakeMove(); // calculates snake parameters
  gameStates();
}

//if there is no food, generate one, also check for victory
void generateFood() {
  if (food.row == -1 || food.col == -1) {
    if (snakeLength >= 64) {
      win = true;
      return; // prevent the food generator from running, in this case it would run forever
    }
    // generate food
    do {
      food.col = random(8);
      food.row = random(8);
    } while (matrix[food.row][food.col] > 0);
  }
}

// watches joystick movements & blinks with food
void joystickMove() {
  int previousDirection = snakeDirection; // save the last direction
  long timestamp = millis() + snakeSpeed; // when the next frame will be rendered

  while (millis() < timestamp) {
    snakeSpeed = 1000 - map(snakeLength, 0, 16, 10, 1000);
    if (snakeSpeed == 0) snakeSpeed = 1; // safety: speed can not be 0

    // determine the direction of the snake

    analogRead(JOYSTICK_Y) < initialJoystick.y - joystickThreshold ? snakeDirection = up    : 0;
    analogRead(JOYSTICK_Y) > initialJoystick.y + joystickThreshold ? snakeDirection = down  : 0;
    analogRead(JOYSTICK_X) < initialJoystick.x - joystickThreshold ? snakeDirection = left  : 0;
    analogRead(JOYSTICK_X) > initialJoystick.x + joystickThreshold ? snakeDirection = right : 0;

    // ignore directional change by 180 degrees (no effect for non-moving snake)
    snakeDirection + 2 == previousDirection && previousDirection != 0 ? snakeDirection = previousDirection : 0;
    snakeDirection - 2 == previousDirection && previousDirection != 0 ? snakeDirection = previousDirection : 0;

    // blink with the food
    lc.setLed(0, food.row, food.col, millis() % 100 < 50 ? 1 : 0);
  }
}

// calculate snake movement data
void snakeMove() {
  switch (snakeDirection) {
    case up:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Try not to lose!");
      snake.row--;
      edge();
      lc.setLed(0, snake.row, snake.col, 1);
      break;

    case right:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Try not to lose!");
      snake.col++;
      edge();
      lc.setLed(0, snake.row, snake.col, 1);
      break;

    case down:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Try not to lose!");
      snake.row++;
      edge();
      lc.setLed(0, snake.row, snake.col, 1);
      break;

    case left:
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Try not to lose!");
      snake.col--;
      edge();
      lc.setLed(0, snake.row, snake.col, 1);
      break;

    default:
      return;
  }

  if (matrix[snake.row][snake.col] != 0 && snakeDirection != 0) {
    gameOver = true;
    return;
  }

  // check if the food was eaten
  if (snake.row == food.row && snake.col == food.col) {
    snakeLength++;
    food.row = -1; // reset food
    food.col = -1;
  }

  // increment ages if all lit leds
  updateMatrix();

  // change the age of the snake head from 0 to 1
  matrix[snake.row][snake.col]++;
}

// causes the snake to appear on the other side of the screen if it gets out of the edge
void edge() {

  if (snake.col < 0) {
    snake.col = 7;
  }
  else if (snake.col > 7) {
    snake.col = 0;
  }

  if (snake.row  < 0) {
    snake.row = 7;
  }
  else if (snake.row > 7) {
    snake.row = 0;
  }
}


// increment ages if all lit leds, turn off to old ones depending on the length of the snake
void updateMatrix() {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      if (matrix[row][col] > 0 ) {
        matrix[row][col]++;
      }

      // if the age exceeds the length of the snake, switch it off
      if (matrix[row][col] > snakeLength) {
        lc.setLed(0, row, col, 0);
        matrix[row][col] = 0;
      }
    }
  }
}

void gameStates() {
  if (gameOver || win) {
    unrollSnake();
    scoreMessage(snakeLength);
    if (gameOver) {
      lcd.setCursor(2, 1);
      lcd.print("Start Again!");
    }
    win = false;
    gameOver = false;
    snakeLength = initialSnakeLength;
    snakeDirection = 0;
    memset(matrix, 0, sizeof(matrix[0][0]) * 8 * 8);
    lc.clearDisplay(0);
  }
}

void unrollSnake() {
  // switch off the food LED
  lc.setLed(0, food.row, food.col, 0);
  delay(600);
  for (int i = 1; i <= snakeLength; i++) {
    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        if (matrix[row][col] == i) {
          lc.setLed(0, row, col, 0);
          delay(100);
        }
      }
    }
  }
}

// calibrate the joystick  for 10 times
void calibrateJoystick() {
  for (int i = 0; i < 10; i++) {
    values.x += analogRead(JOYSTICK_X);
    values.y += analogRead(JOYSTICK_Y);
  }

  initialJoystick.x = values.x / 10;
  initialJoystick.y = values.y / 10;
}

void scoreMessage(int score) {
  if (score < 0 || score > 99) return;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GameOverScore:");
  lcd.setCursor(14, 0);
  lcd.print(score);
}
