#include <FastLED.h>

// Matrix Config
#define MATRIX_WIDTH 32
#define MATRIX_HEIGHT 18
#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)
#define LED_PIN 4

#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

// Joystick pins
#define JOYSTICK1_X 34
#define JOYSTICK2_X 36

// Game state
int paddle1_y = MATRIX_HEIGHT / 2 - 2;
int paddle2_y = MATRIX_HEIGHT / 2 - 2;
const int paddle_height = 4;
int ball_x = MATRIX_WIDTH / 2;
int ball_y = MATRIX_HEIGHT / 2;
int ball_dx = 1, ball_dy = 1;
int score1 = 0, score2 = 0;
bool gameOver = false;
int winningScore = 9;

// Map 2D to 1D serpentine
int XY(int x, int y) {
  if (y % 2 == 0)
    return y * MATRIX_WIDTH + x;
  else
    return y * MATRIX_WIDTH + (MATRIX_WIDTH - 1 - x);
}

// Fixed bit patterns for digits - reading from left to right (as we see them)
const byte digits[10][5] = {
  {0b111, 0b101, 0b101, 0b101, 0b111},  // 0
  {0b110, 0b010, 0b010, 0b010, 0b111},  // 1
  {0b111, 0b001, 0b111, 0b100, 0b111},  // 2
  {0b111, 0b001, 0b111, 0b001, 0b111},  // 3
  {0b101, 0b101, 0b111, 0b001, 0b001},  // 4
  {0b111, 0b100, 0b111, 0b001, 0b111},  // 5
  {0b111, 0b100, 0b111, 0b101, 0b111},  // 6
  {0b111, 0b001, 0b001, 0b001, 0b001},  // 7
  {0b111, 0b101, 0b111, 0b101, 0b111},  // 8
  {0b111, 0b101, 0b111, 0b001, 0b111}   // 9
};

// Fixed 'W' and 'L' letters (3x5 font)
const byte letterW[5] = {0b101, 0b101, 0b101, 0b111, 0b101};  // W
const byte letterL[5] = {0b100, 0b100, 0b100, 0b100, 0b111};  // L

// Helper function to reverse bit patterns for proper display
byte reverseBits(byte b) {
  byte result = 0;
  for (int i = 0; i < 3; i++) {
    result = (result << 1) | (b & 1);
    b >>= 1;
  }
  return result;
}

void drawDigit(int digit, int x, int y, CRGB color) {
  if (digit < 0 || digit > 9) return;
  for (int row = 0; row < 5; row++) {
    byte line = digits[digit][row];
    for (int col = 0; col < 3; col++) {
      if (line & (1 << col)) {  // Changed to read from right to left
        leds[XY(x + col, y + row)] = color;
      }
    }
  }
}

void drawLetter(const byte* letter, int x, int y, CRGB color) {
  for (int row = 0; row < 5; row++) {
    byte line = letter[row];
    for (int col = 0; col < 3; col++) {
      if (line & (1 << col)) {  // Changed to read from right to left
        leds[XY(x + col, y + row)] = color;
      }
    }
  }
}

void drawPaddle(int x, int y) {
  for (int i = 0; i < paddle_height; i++) {
    if (y + i >= 0 && y + i < MATRIX_HEIGHT) {
      leds[XY(x, y + i)] = CRGB::White;
    }
  }
}

void drawBall(int x, int y) {
  if (x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT)
    leds[XY(x, y)] = CRGB::Red;
}

void drawMiddleLine() {
  for (int y = 0; y < MATRIX_HEIGHT; y += 2) {
    leds[XY(MATRIX_WIDTH / 2, y)] = CRGB::White;
  }
}

void updatePaddles() {
  if (gameOver) return;  // Don't update paddles if game is over
  
  int joy1 = analogRead(JOYSTICK1_X);
  int joy2 = analogRead(JOYSTICK2_X);

  // Paddle 1 control (joystick 1)
  if (joy1 < 1000) paddle1_y = max(0, paddle1_y - 1);
  if (joy1 > 3000) paddle1_y = min(MATRIX_HEIGHT - paddle_height, paddle1_y + 1);

  // Paddle 2 control (joystick 2)
  if (joy2 < 500) paddle2_y = max(0, paddle2_y - 1);
  if (joy2 > 1500) paddle2_y = min(MATRIX_HEIGHT - paddle_height, paddle2_y + 1);
}

void updateBall() {
  if (gameOver) return;  // Don't update ball if game is over
  
  int next_x = ball_x + ball_dx;
  int next_y = ball_y + ball_dy;

  // Bounce top/bottom
  if (next_y <= 0 || next_y >= MATRIX_HEIGHT - 1) ball_dy *= -1;

  // Bounce off paddles
  if (next_x == 1 && next_y >= paddle1_y && next_y < paddle1_y + paddle_height) ball_dx *= -1;
  if (next_x == MATRIX_WIDTH - 2 && next_y >= paddle2_y && next_y < paddle2_y + paddle_height) ball_dx *= -1;

  // Score
  if (next_x <= 0) {
    score2 = min(score2 + 1, winningScore);
    ball_x = MATRIX_WIDTH / 2; ball_dx = 1;
    
    // Check for win
    if (score2 >= winningScore) {
      gameOver = true;
    }
  }
  if (next_x >= MATRIX_WIDTH - 1) {
    score1 = min(score1 + 1, winningScore);
    ball_x = MATRIX_WIDTH / 2; ball_dx = -1;
    
    // Check for win
    if (score1 >= winningScore) {
      gameOver = true;
    }
  }

  ball_x += ball_dx;
  ball_y += ball_dy;
}

void renderGame() {
  fill_solid(leds, NUM_LEDS, CRGB::Green);  // Green background
  drawPaddle(0, paddle1_y);
  drawPaddle(MATRIX_WIDTH - 1, paddle2_y);
  drawBall(ball_x, ball_y);
  drawMiddleLine();

  // Scoreboard
  if (gameOver) {
    // Game is over, show winner (W) and loser (L)
    if (score1 >= winningScore) {
      drawLetter(letterW, 5, 0, CRGB::White);      // Left player wins
      drawLetter(letterL, MATRIX_WIDTH - 8, 0, CRGB::White); // Right player loses
    } else {
      drawLetter(letterL, 5, 0, CRGB::White);      // Left player loses
      drawLetter(letterW, MATRIX_WIDTH - 8, 0, CRGB::White); // Right player wins
    }
  } else {
    // Game in progress, show scores
    drawDigit(score1, 5, 0, CRGB::White);          // Left score
    drawDigit(score2, MATRIX_WIDTH - 8, 0, CRGB::White); // Right score
  }

  FastLED.show();
}

void resetGame() {
  // Reset game state
  score1 = 0;
  score2 = 0;
  paddle1_y = MATRIX_HEIGHT / 2 - 2;
  paddle2_y = MATRIX_HEIGHT / 2 - 2;
  ball_x = MATRIX_WIDTH / 2;
  ball_y = MATRIX_HEIGHT / 2;
  ball_dx = 1;
  ball_dy = 1;
  gameOver = false;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n----- LED Ping Pong Game -----");
  
  pinMode(JOYSTICK1_X, INPUT);
  pinMode(JOYSTICK2_X, INPUT);

  // Set analog read resolution
  analogReadResolution(12);  // 12-bit resolution (0-4095)
  
  // Initialize LED strip
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  
  // Reduced brightness to 20% (51/255) for eye comfort
  FastLED.setBrightness(55);  
  
  Serial.println("Initial joystick readings:");
  Serial.println("JOY1: " + String(analogRead(JOYSTICK1_X)));
  Serial.println("JOY2: " + String(analogRead(JOYSTICK2_X)));
  
  FastLED.clear();
  FastLED.show();
}

void loop() {
  // Check if we should reset the game (both joysticks pushed fully)
  if (gameOver) {
    int joy1 = analogRead(JOYSTICK1_X);
    int joy2 = analogRead(JOYSTICK2_X);
    
    // If both joysticks are pushed, reset the game
    if (joy1 > 3500 && joy2 > 1500) {
      resetGame();
      delay(500); // Avoid immediate re-triggering
    }
  }
  
  updatePaddles();
  updateBall();
  renderGame();
  delay(60);  // Game speed
}