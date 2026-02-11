#define BUTTON_PIN 2
#define LED_PIN 13
#define BUZZER_PIN 8

int score = 0;
bool lastButton = HIGH;
unsigned long lastBonusTime = 0;

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.begin(9600);
  randomSeed(analogRead(A0));

  Serial.println("GAME_START");
  beep(2);
}

void loop() {
  bool button = digitalRead(BUTTON_PIN);

  // BUTTON PRESS = SCORE
  if (button == LOW && lastButton == HIGH) {
    score++;
    Serial.print("SCORE:");
    Serial.println(score);

    flashLED();
    tone(BUZZER_PIN, 1000, 100);

    // RANDOM BONUS
    if (random(1, 10) == 5) {
      score += 5;
      Serial.print("BONUS! SCORE:");
      Serial.println(score);
      megaBeep();
    }

    delay(150); // debounce
  }

  lastButton = button;

  // AUTO GAME OVER AT 50
  if (score >= 50) {
    Serial.println("GAME_OVER");
    gameOverEffect();
    score = 0;
    Serial.println("NEW_GAME");
  }
}

// ===== EFFECTS =====

void flashLED() {
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
}

void beep(int times) {
  for (int i = 0; i < times; i++) {
    tone(BUZZER_PIN, 800, 100);
    delay(150);
  }
}

void megaBeep() {
  for (int f = 500; f < 2000; f += 200) {
    tone(BUZZER_PIN, f, 80);
    delay(90);
  }
}

void gameOverEffect() {
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 400, 100);
    delay(150);
    digitalWrite(LED_PIN, LOW);
    delay(150);
  }
}
