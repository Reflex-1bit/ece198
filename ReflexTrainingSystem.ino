/*
 * ====================================================================
 * REFLEX TRAINING SYSTEM - Advanced Arduino Game Platform
 * ====================================================================
 * 
 * A comprehensive cognitive assessment and reflex training system 
 * featuring multiple game modes, difficulty progression, statistical
 * tracking, and real-time serial communication with web dashboard.
 * 
 * Author: Aditya Sharma
 * Repository: github.com/Reflex-1bit
 * License: MIT
 * 
 * Hardware Requirements:
 * - Arduino Uno/Nano
 * - Push Button (Pin 2)
 * - LED Indicator (Pin 13)
 * - Piezo Buzzer (Pin 8)
 * - Optional: RGB LED (Pins 9, 10, 11)
 * 
 * Features:
 * - Multiple game modes (Speed Test, Pattern Memory, Endurance)
 * - Dynamic difficulty scaling
 * - Statistical tracking and performance metrics
 * - Serial communication protocol for web dashboard integration
 * - High score persistence using EEPROM
 * - Configurable gameplay parameters
 * ====================================================================
 */

#include <EEPROM.h>

// ===== PIN DEFINITIONS =====
#define BUTTON_PIN 2
#define LED_PIN 13
#define BUZZER_PIN 8
#define RGB_RED_PIN 9
#define RGB_GREEN_PIN 10
#define RGB_BLUE_PIN 11

// ===== GAME CONFIGURATION =====
#define MAX_SCORE 100
#define BONUS_PROBABILITY 15  // Percentage chance
#define BONUS_POINTS 5
#define COMBO_THRESHOLD 5     // Consecutive hits for combo
#define COMBO_MULTIPLIER 2
#define DIFFICULTY_STEP 10    // Score interval for difficulty increase

// ===== EEPROM ADDRESSES =====
#define EEPROM_HIGH_SCORE 0
#define EEPROM_TOTAL_GAMES 2
#define EEPROM_TOTAL_PLAYTIME 4

// ===== GAME MODES =====
enum GameMode {
  MODE_SPEED_TEST,      // Pure reflex speed
  MODE_PATTERN_MEMORY,  // Remember and repeat patterns
  MODE_ENDURANCE        // Sustained attention test
};

// ===== DIFFICULTY LEVELS =====
enum Difficulty {
  DIFF_EASY,
  DIFF_MEDIUM,
  DIFF_HARD,
  DIFF_EXPERT
};

// ===== GAME STATE =====
struct GameState {
  int score;
  int highScore;
  int comboCount;
  int consecutiveHits;
  unsigned long gameStartTime;
  unsigned long lastActionTime;
  unsigned long reactionTimes[10];  // Store last 10 reaction times
  int reactionIndex;
  GameMode currentMode;
  Difficulty currentDifficulty;
  bool gameActive;
  int totalGames;
  unsigned long totalPlaytime;
} game;

// ===== TIMING VARIABLES =====
bool lastButtonState = HIGH;
unsigned long buttonPressTime = 0;
unsigned long ledOnTime = 0;
bool ledActive = false;

// ===== STATISTICS =====
struct Statistics {
  int totalButtonPresses;
  int totalBonuses;
  int totalCombos;
  unsigned long fastestReaction;
  unsigned long averageReaction;
} stats;

// ====================================================================
// SETUP
// ====================================================================
void setup() {
  // Initialize pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  
  // Initialize serial communication
  Serial.begin(9600);
  
  // Seed random number generator
  randomSeed(analogRead(A0));
  
  // Load saved data from EEPROM
  loadGameData();
  
  // Initialize game state
  initializeGame();
  
  // Startup sequence
  playStartupSequence();
  
  // Send initial status
  sendSerialStatus();
}

// ====================================================================
// MAIN LOOP
// ====================================================================
void loop() {
  if (!game.gameActive) {
    handleMenuInput();
    return;
  }
  
  // Handle button input
  handleButtonPress();
  
  // Update game logic based on current mode
  switch (game.currentMode) {
    case MODE_SPEED_TEST:
      updateSpeedTestMode();
      break;
    case MODE_PATTERN_MEMORY:
      updatePatternMemoryMode();
      break;
    case MODE_ENDURANCE:
      updateEnduranceMode();
      break;
  }
  
  // Check win/lose conditions
  checkGameConditions();
  
  // Update LED state
  updateLEDState();
}

// ====================================================================
// GAME INITIALIZATION
// ====================================================================
void initializeGame() {
  game.score = 0;
  game.comboCount = 0;
  game.consecutiveHits = 0;
  game.gameStartTime = millis();
  game.lastActionTime = millis();
  game.reactionIndex = 0;
  game.currentMode = MODE_SPEED_TEST;
  game.currentDifficulty = DIFF_EASY;
  game.gameActive = false;
  
  stats.totalButtonPresses = 0;
  stats.totalBonuses = 0;
  stats.totalCombos = 0;
  stats.fastestReaction = 999999;
  stats.averageReaction = 0;
  
  for (int i = 0; i < 10; i++) {
    game.reactionTimes[i] = 0;
  }
  
  Serial.println("SYSTEM_READY");
  Serial.println("Commands: START, MODE, DIFFICULTY, STATS, RESET");
}

// ====================================================================
// BUTTON HANDLING
// ====================================================================
void handleButtonPress() {
  bool currentButtonState = digitalRead(BUTTON_PIN);
  
  // Detect button press (falling edge)
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    unsigned long reactionTime = millis() - buttonPressTime;
    
    // Process button press
    processButtonAction(reactionTime);
    
    // Update statistics
    stats.totalButtonPresses++;
    
    // Debounce delay
    delay(50);
  }
  
  lastButtonState = currentButtonState;
}

void processButtonAction(unsigned long reactionTime) {
  // Increment score
  game.score++;
  game.consecutiveHits++;
  
  // Store reaction time
  game.reactionTimes[game.reactionIndex] = reactionTime;
  game.reactionIndex = (game.reactionIndex + 1) % 10;
  
  // Update fastest reaction
  if (reactionTime < stats.fastestReaction) {
    stats.fastestReaction = reactionTime;
  }
  
  // Calculate average reaction time
  calculateAverageReaction();
  
  // Check for combo
  if (game.consecutiveHits >= COMBO_THRESHOLD) {
    activateCombo();
  }
  
  // Random bonus chance
  if (random(0, 100) < BONUS_PROBABILITY) {
    activateBonus();
  }
  
  // Provide feedback
  giveFeedback(reactionTime);
  
  // Send score update
  Serial.print("SCORE:");
  Serial.println(game.score);
  Serial.print("REACTION:");
  Serial.println(reactionTime);
  
  // Check for difficulty increase
  checkDifficultyProgression();
}

// ====================================================================
// GAME MODES
// ====================================================================
void updateSpeedTestMode() {
  // In speed test mode, flash LED at intervals based on difficulty
  static unsigned long lastFlash = 0;
  unsigned long flashInterval = getDifficultyInterval();
  
  if (millis() - lastFlash > flashInterval) {
    if (!ledActive) {
      activateLED();
      buttonPressTime = millis();
    }
    lastFlash = millis();
  }
}

void updatePatternMemoryMode() {
  // Pattern memory mode logic
  // Generate and display patterns for player to remember
  static int patternLength = 3;
  static int pattern[10];
  static bool showingPattern = false;
  
  // Implementation for pattern generation and verification
  // (Simplified for this example)
}

void updateEnduranceMode() {
  // Endurance mode: sustained attention over time
  // Gradually increase difficulty and speed
  unsigned long gameTime = millis() - game.gameStartTime;
  
  if (gameTime > 60000) {  // After 1 minute, increase difficulty
    game.currentDifficulty = DIFF_MEDIUM;
  }
  if (gameTime > 120000) {  // After 2 minutes, increase to hard
    game.currentDifficulty = DIFF_HARD;
  }
}

// ====================================================================
// DIFFICULTY SYSTEM
// ====================================================================
unsigned long getDifficultyInterval() {
  switch (game.currentDifficulty) {
    case DIFF_EASY:   return 2000;
    case DIFF_MEDIUM: return 1500;
    case DIFF_HARD:   return 1000;
    case DIFF_EXPERT: return 700;
    default:          return 2000;
  }
}

void checkDifficultyProgression() {
  if (game.score % DIFFICULTY_STEP == 0 && game.score > 0) {
    if (game.currentDifficulty < DIFF_EXPERT) {
      game.currentDifficulty = (Difficulty)(game.currentDifficulty + 1);
      Serial.print("DIFFICULTY_UP:");
      Serial.println(game.currentDifficulty);
      playDifficultyUpSound();
    }
  }
}

// ====================================================================
// COMBO & BONUS SYSTEM
// ====================================================================
void activateCombo() {
  game.comboCount++;
  stats.totalCombos++;
  
  int comboBonus = COMBO_MULTIPLIER * game.consecutiveHits;
  game.score += comboBonus;
  
  Serial.print("COMBO:");
  Serial.print(game.comboCount);
  Serial.print(" +");
  Serial.println(comboBonus);
  
  playComboSound();
  comboLEDEffect();
}

void activateBonus() {
  game.score += BONUS_POINTS;
  stats.totalBonuses++;
  
  Serial.print("BONUS! +");
  Serial.println(BONUS_POINTS);
  
  playBonusSound();
  bonusLEDEffect();
}

// ====================================================================
// FEEDBACK SYSTEM
// ====================================================================
void giveFeedback(unsigned long reactionTime) {
  if (reactionTime < 200) {
    Serial.println("FEEDBACK:EXCELLENT");
    setRGBColor(0, 255, 0);  // Green
    tone(BUZZER_PIN, 2000, 100);
  } else if (reactionTime < 400) {
    Serial.println("FEEDBACK:GOOD");
    setRGBColor(0, 255, 255);  // Cyan
    tone(BUZZER_PIN, 1500, 100);
  } else if (reactionTime < 600) {
    Serial.println("FEEDBACK:OK");
    setRGBColor(255, 255, 0);  // Yellow
    tone(BUZZER_PIN, 1000, 100);
  } else {
    Serial.println("FEEDBACK:SLOW");
    setRGBColor(255, 0, 0);  // Red
    tone(BUZZER_PIN, 500, 100);
  }
  
  delay(100);
  setRGBColor(0, 0, 0);  // Off
}

// ====================================================================
// LED EFFECTS
// ====================================================================
void activateLED() {
  digitalWrite(LED_PIN, HIGH);
  ledOnTime = millis();
  ledActive = true;
}

void updateLEDState() {
  if (ledActive && millis() - ledOnTime > 1000) {
    digitalWrite(LED_PIN, LOW);
    ledActive = false;
  }
}

void comboLEDEffect() {
  for (int i = 0; i < 3; i++) {
    setRGBColor(255, 0, 255);  // Magenta
    delay(100);
    setRGBColor(0, 0, 0);
    delay(100);
  }
}

void bonusLEDEffect() {
  for (int i = 0; i < 5; i++) {
    setRGBColor(random(0, 255), random(0, 255), random(0, 255));
    delay(80);
  }
  setRGBColor(0, 0, 0);
}

void setRGBColor(int red, int green, int blue) {
  analogWrite(RGB_RED_PIN, red);
  analogWrite(RGB_GREEN_PIN, green);
  analogWrite(RGB_BLUE_PIN, blue);
}

// ====================================================================
// SOUND EFFECTS
// ====================================================================
void playStartupSequence() {
  int melody[] = {262, 294, 330, 349, 392, 440, 494, 523};
  for (int i = 0; i < 8; i++) {
    tone(BUZZER_PIN, melody[i], 100);
    delay(120);
  }
  Serial.println("STARTUP_COMPLETE");
}

void playComboSound() {
  for (int f = 800; f < 2000; f += 200) {
    tone(BUZZER_PIN, f, 50);
    delay(60);
  }
}

void playBonusSound() {
  for (int f = 500; f < 2500; f += 300) {
    tone(BUZZER_PIN, f, 80);
    delay(90);
  }
}

void playDifficultyUpSound() {
  tone(BUZZER_PIN, 1000, 100);
  delay(150);
  tone(BUZZER_PIN, 1500, 100);
  delay(150);
  tone(BUZZER_PIN, 2000, 200);
}

void playGameOverSound() {
  int melody[] = {494, 440, 392, 349, 330, 294, 262};
  for (int i = 0; i < 7; i++) {
    tone(BUZZER_PIN, melody[i], 150);
    delay(180);
  }
}

// ====================================================================
// GAME CONDITIONS
// ====================================================================
void checkGameConditions() {
  // Check win condition
  if (game.score >= MAX_SCORE) {
    endGame(true);
  }
  
  // Check timeout (for endurance mode)
  if (game.currentMode == MODE_ENDURANCE) {
    if (millis() - game.lastActionTime > 5000) {
      endGame(false);
    }
  }
}

void endGame(bool won) {
  game.gameActive = false;
  
  // Update statistics
  game.totalGames++;
  unsigned long sessionTime = (millis() - game.gameStartTime) / 1000;
  game.totalPlaytime += sessionTime;
  
  // Check and update high score
  if (game.score > game.highScore) {
    game.highScore = game.score;
    EEPROM.put(EEPROM_HIGH_SCORE, game.highScore);
    Serial.println("NEW_HIGH_SCORE!");
  }
  
  // Save game data
  saveGameData();
  
  // Send final statistics
  sendGameOverStats(won);
  
  // Play game over sequence
  if (won) {
    playVictorySequence();
  } else {
    playGameOverSound();
  }
  
  Serial.println("GAME_OVER");
}

void playVictorySequence() {
  for (int i = 0; i < 10; i++) {
    digitalWrite(LED_PIN, HIGH);
    setRGBColor(0, 255, 0);
    tone(BUZZER_PIN, 1000 + (i * 100), 100);
    delay(150);
    digitalWrite(LED_PIN, LOW);
    setRGBColor(0, 0, 0);
    delay(100);
  }
}

// ====================================================================
// STATISTICS
// ====================================================================
void calculateAverageReaction() {
  unsigned long sum = 0;
  int count = 0;
  
  for (int i = 0; i < 10; i++) {
    if (game.reactionTimes[i] > 0) {
      sum += game.reactionTimes[i];
      count++;
    }
  }
  
  if (count > 0) {
    stats.averageReaction = sum / count;
  }
}

void sendGameOverStats(bool won) {
  Serial.println("=== GAME STATISTICS ===");
  Serial.print("Final Score: ");
  Serial.println(game.score);
  Serial.print("High Score: ");
  Serial.println(game.highScore);
  Serial.print("Total Button Presses: ");
  Serial.println(stats.totalButtonPresses);
  Serial.print("Total Combos: ");
  Serial.println(stats.totalCombos);
  Serial.print("Total Bonuses: ");
  Serial.println(stats.totalBonuses);
  Serial.print("Fastest Reaction: ");
  Serial.print(stats.fastestReaction);
  Serial.println("ms");
  Serial.print("Average Reaction: ");
  Serial.print(stats.averageReaction);
  Serial.println("ms");
  Serial.print("Session Time: ");
  Serial.print((millis() - game.gameStartTime) / 1000);
  Serial.println("s");
  Serial.print("Result: ");
  Serial.println(won ? "VICTORY" : "GAME_OVER");
  Serial.println("=======================");
}

void sendSerialStatus() {
  Serial.print("STATUS:");
  Serial.print("Score=");
  Serial.print(game.score);
  Serial.print(",HighScore=");
  Serial.print(game.highScore);
  Serial.print(",Mode=");
  Serial.print(game.currentMode);
  Serial.print(",Difficulty=");
  Serial.println(game.currentDifficulty);
}

// ====================================================================
// EEPROM PERSISTENCE
// ====================================================================
void loadGameData() {
  EEPROM.get(EEPROM_HIGH_SCORE, game.highScore);
  EEPROM.get(EEPROM_TOTAL_GAMES, game.totalGames);
  EEPROM.get(EEPROM_TOTAL_PLAYTIME, game.totalPlaytime);
  
  // Validate loaded data
  if (game.highScore > 10000 || game.highScore < 0) {
    game.highScore = 0;
  }
  
  Serial.println("DATA_LOADED");
}

void saveGameData() {
  EEPROM.put(EEPROM_HIGH_SCORE, game.highScore);
  EEPROM.put(EEPROM_TOTAL_GAMES, game.totalGames);
  EEPROM.put(EEPROM_TOTAL_PLAYTIME, game.totalPlaytime);
  
  Serial.println("DATA_SAVED");
}

// ====================================================================
// MENU SYSTEM
// ====================================================================
void handleMenuInput() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "START") {
      game.gameActive = true;
      game.gameStartTime = millis();
      Serial.println("GAME_STARTED");
      playStartupSequence();
    } 
    else if (command == "MODE") {
      cycleGameMode();
    }
    else if (command == "DIFFICULTY") {
      cycleDifficulty();
    }
    else if (command == "STATS") {
      displayStats();
    }
    else if (command == "RESET") {
      resetAllData();
    }
  }
}

void cycleGameMode() {
  game.currentMode = (GameMode)((game.currentMode + 1) % 3);
  Serial.print("MODE_CHANGED:");
  Serial.println(game.currentMode);
}

void cycleDifficulty() {
  game.currentDifficulty = (Difficulty)((game.currentDifficulty + 1) % 4);
  Serial.print("DIFFICULTY_CHANGED:");
  Serial.println(game.currentDifficulty);
}

void displayStats() {
  Serial.println("=== ALL-TIME STATISTICS ===");
  Serial.print("High Score: ");
  Serial.println(game.highScore);
  Serial.print("Total Games Played: ");
  Serial.println(game.totalGames);
  Serial.print("Total Playtime: ");
  Serial.print(game.totalPlaytime / 60);
  Serial.println(" minutes");
  Serial.println("===========================");
}

void resetAllData() {
  game.highScore = 0;
  game.totalGames = 0;
  game.totalPlaytime = 0;
  saveGameData();
  Serial.println("ALL_DATA_RESET");
}
