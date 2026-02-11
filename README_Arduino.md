# Reflex Training System

> Advanced Arduino-based cognitive assessment and reflex training platform with real-time performance tracking

## Overview

The Reflex Training System is a comprehensive embedded solution for measuring and improving cognitive reflexes through interactive gameplay. Built on Arduino, it features multiple game modes, dynamic difficulty scaling, statistical analysis, and web dashboard integration via serial communication.

## Features

### Core Gameplay
- **Multiple Game Modes**
  - Speed Test: Pure reflex speed measurement
  - Pattern Memory: Sequential pattern recall
  - Endurance Mode: Sustained attention testing
  
- **Dynamic Difficulty System**
  - Automatic progression based on performance
  - 4 difficulty levels (Easy → Expert)
  - Adaptive timing intervals

- **Combo & Bonus System**
  - Combo multipliers for consecutive hits
  - Random bonus events with visual/audio feedback
  - Score multipliers for sustained performance

### Performance Tracking
- Real-time reaction time measurement
- Statistical analysis (fastest, average, rolling window)
- High score persistence using EEPROM
- Session-based performance metrics
- Lifetime statistics tracking

### Feedback Systems
- **Visual Feedback**
  - LED indicators for game state
  - RGB color-coded performance feedback
  - Combo and bonus visual effects
  
- **Audio Feedback**
  - Melodic startup sequence
  - Performance-based tones
  - Combo and bonus sound effects
  - Victory/game over melodies

### Serial Communication Protocol
- Real-time score updates
- Performance statistics streaming
- Game state notifications
- Command interface for web dashboard

## Hardware Requirements

### Required Components
- Arduino Uno/Nano
- Push Button
- LED (built-in pin 13 or external)
- Piezo Buzzer

### Optional Components
- RGB LED (for enhanced visual feedback)
- Additional buttons (for multi-button modes)

### Pin Configuration
```
BUTTON_PIN:     Pin 2  (INPUT_PULLUP)
LED_PIN:        Pin 13 (OUTPUT)
BUZZER_PIN:     Pin 8  (OUTPUT)
RGB_RED_PIN:    Pin 9  (PWM OUTPUT)
RGB_GREEN_PIN:  Pin 10 (PWM OUTPUT)
RGB_BLUE_PIN:   Pin 11 (PWM OUTPUT)
```

## Circuit Diagram

```
Arduino Uno
┌─────────────┐
│             │
│  Pin 2 ─────┼──── Push Button ──── GND
│             │
│  Pin 8 ─────┼──── Buzzer (+) 
│  GND ───────┼──── Buzzer (-)
│             │
│  Pin 13 ────┼──── LED (+) ──── 220Ω ──── GND
│             │
│  Pin 9 ─────┼──── RGB LED (R) ──── 220Ω ──── GND
│  Pin 10 ────┼──── RGB LED (G) ──── 220Ω ──── GND
│  Pin 11 ────┼──── RGB LED (B) ──── 220Ω ──── GND
│             │
└─────────────┘
```

## Installation

### Arduino IDE
1. Download and install [Arduino IDE](https://www.arduino.cc/en/software)
2. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/reflex-training-system.git
   ```
3. Open `ReflexTrainingSystem.ino` in Arduino IDE
4. Select your board: Tools → Board → Arduino Uno
5. Select your port: Tools → Port → (your Arduino port)
6. Upload to Arduino

### Web Dashboard (Optional)
1. Open `index.html` in a modern browser (Chrome/Edge recommended)
2. Click "Connect Arduino"
3. Select your Arduino's serial port
4. Real-time statistics will display in the dashboard

## Usage

### Serial Commands

The system accepts commands via Serial Monitor (9600 baud):

| Command | Description |
|---------|-------------|
| `START` | Begin a new game session |
| `MODE` | Cycle through game modes |
| `DIFFICULTY` | Cycle through difficulty levels |
| `STATS` | Display lifetime statistics |
| `RESET` | Reset all saved data |

### Game Controls

**Button Press**: Score points and trigger actions

**Game Flow**:
1. Send `START` command or press button to begin
2. Watch for LED activation
3. Press button as quickly as possible
4. Receive visual/audio feedback
5. Continue until reaching max score or timeout

### Performance Feedback

**Reaction Time Ratings**:
- **Excellent**: < 200ms (Green, high tone)
- **Good**: 200-400ms (Cyan, medium tone)
- **OK**: 400-600ms (Yellow, low-medium tone)
- **Slow**: > 600ms (Red, low tone)

## Serial Protocol

### Output Format

```
SCORE:<value>           # Current score update
REACTION:<ms>           # Reaction time in milliseconds
COMBO:<count>           # Combo activation
BONUS! +<points>        # Bonus event
DIFFICULTY_UP:<level>   # Difficulty increased
FEEDBACK:<rating>       # Performance feedback
GAME_OVER              # Game ended
NEW_HIGH_SCORE!        # High score achieved
```

### Statistics Output

```
=== GAME STATISTICS ===
Final Score: 45
High Score: 52
Total Button Presses: 45
Total Combos: 8
Total Bonuses: 3
Fastest Reaction: 187ms
Average Reaction: 234ms
Session Time: 95s
Result: VICTORY
=======================
```

## Configuration

### Gameplay Parameters

Modify these constants in `ReflexTrainingSystem.ino`:

```cpp
#define MAX_SCORE 100              // Score needed to win
#define BONUS_PROBABILITY 15       // Bonus chance (%)
#define BONUS_POINTS 5             // Points per bonus
#define COMBO_THRESHOLD 5          // Hits needed for combo
#define COMBO_MULTIPLIER 2         // Combo score multiplier
#define DIFFICULTY_STEP 10         // Score interval for difficulty increase
```

### Difficulty Timing

```cpp
DIFF_EASY:   2000ms interval
DIFF_MEDIUM: 1500ms interval
DIFF_HARD:   1000ms interval
DIFF_EXPERT:  700ms interval
```

## Technical Architecture

### State Machine
```
MENU → START → ACTIVE_GAME → END_GAME → MENU
         ↓
    MODE_SELECTION
         ↓
  DIFFICULTY_SELECTION
```

### Data Persistence

Uses Arduino EEPROM for persistent storage:
- High Score (Address 0-1)
- Total Games Played (Address 2-3)
- Total Playtime (Address 4-7)

### Performance Optimization

- Interrupt-driven button detection
- Ring buffer for reaction time history
- Efficient serial communication
- Minimal delay() usage in main loop

## Web Dashboard Integration

The included web dashboard (`index.html`) provides:
- Real-time score display
- Performance graphs
- Reaction time analysis
- Session statistics
- Score history export (PDF)
- Camera integration for recording sessions

### Dashboard Features
- Live serial communication via Web Serial API
- Local storage for score history
- PDF export functionality
- Real-time performance metrics
- Patient/session management

## Troubleshooting

**Button Not Responding**
- Check wiring and pull-up resistor
- Verify `BUTTON_PIN` definition
- Test with Serial Monitor: should see HIGH/LOW states

**No Serial Communication**
- Ensure baud rate is 9600
- Check USB connection
- Verify correct COM port selection

**EEPROM Data Corruption**
- Send `RESET` command
- Re-upload code
- Check power supply stability

**LED/Buzzer Not Working**
- Verify pin connections
- Check component polarity
- Test with simple blink/tone sketch

## Future Enhancements

- [ ] Multi-player support
- [ ] Bluetooth connectivity
- [ ] Mobile app integration
- [ ] Advanced pattern recognition modes
- [ ] Calibration system for different skill levels
- [ ] Sound-based reaction testing
- [ ] Detailed performance analytics dashboard

## Performance Metrics

Typical performance on Arduino Uno:
- Loop execution: ~2ms
- Button debounce: 50ms
- Reaction time accuracy: ±5ms
- EEPROM write cycles: 100,000+ guaranteed

## Contributing

Contributions welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Submit a pull request

## License

MIT License - see LICENSE file for details

## Author

**Aditya Sharma**
- GitHub: [@Reflex-1bit](https://github.com/Reflex-1bit)
- Email: aditya.shm64@gmail.com

## Acknowledgments

- Arduino community for hardware support
- Web Serial API documentation
- Embedded systems design patterns

---

**Built with ⚡ for cognitive assessment and reflex training**
