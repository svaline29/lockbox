/* 
 * Project: Lockbox
 * Author: Sebastian Valine
 * Date: 12/03/2025
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"

// Device OS config
SYSTEM_MODE(AUTOMATIC);
SerialLogHandler logHandler(LOG_LEVEL_INFO);

// Button + LED pins
const int BTN_PIN = D3;
const int LED_PIN = D7;

// Servo setup (built-in API)
Servo lockServo;
const int SERVO_PIN = A2;          // signal pin
const int LOCK_POS = 30;           // tune for your mechanism
const int UNLOCK_POS = 210;        // ~180° rotation
const int UNLOCK_HOLD_MS = 5000;   // hold unlocked for 5s

// Cloud-visible lock status
String lockStatus = "LOCKED";      // default state

// Code definition
const int CODE_LEN = 4;
const int SECRET_CODE[CODE_LEN] = {0, 1, 1, 0};

// Timing thresholds
const unsigned long SHORT_MAX   = 400;
const unsigned long RESET_IDLE  = 5000;

// Button state
bool buttonPrev = false;
unsigned long pressStart = 0;
unsigned long lastEventTime = 0;

// Entered code buffer
int entered[CODE_LEN];
int enteredIndex = 0;

// Forward declarations
void recordPress(unsigned long durationMs);
void checkCode();
void unlockBox();
void failSignal();

void setup() {
    pinMode(BTN_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);

    lockServo.attach(SERVO_PIN);
    lockServo.write(LOCK_POS);   // start locked

    digitalWrite(LED_PIN, LOW);

    for (int i = 0; i < CODE_LEN; i++) {
        entered[i] = -1;
    }

    // Expose lockStatus as a cloud variable
    Particle.variable("lockStatus", lockStatus);

    // Ensure initial cloud state is correct
    lockStatus = "LOCKED";
}

void loop() {
    bool pressed = (digitalRead(BTN_PIN) == LOW);
    unsigned long now = millis();

    if (pressed && !buttonPrev) {
        pressStart = now;
    }

    if (!pressed && buttonPrev) {
        unsigned long duration = now - pressStart;
        recordPress(duration);
    }

    if (enteredIndex > 0 && (now - lastEventTime > RESET_IDLE)) {
        enteredIndex = 0;
        for (int i = 0; i < CODE_LEN; i++) {
            entered[i] = -1;
        }
    }

    buttonPrev = pressed;
}

void recordPress(unsigned long durationMs) {
    if (enteredIndex >= CODE_LEN) return;

    int symbol = (durationMs <= SHORT_MAX) ? 0 : 1;

    entered[enteredIndex++] = symbol;
    lastEventTime = millis();

    if (enteredIndex == CODE_LEN) {
        checkCode();
    }
}

void checkCode() {
    bool match = true;
    for (int i = 0; i < CODE_LEN; i++) {
        if (entered[i] != SECRET_CODE[i]) {
            match = false;
            break;
        }
    }

    if (match) {
        unlockBox();
    } else {
        failSignal();
    }

    enteredIndex = 0;
    for (int i = 0; i < CODE_LEN; i++) {
        entered[i] = -1;
    }
}

void unlockBox() {
    digitalWrite(LED_PIN, HIGH);

    // Update cloud status
    lockStatus = "UNLOCKED";

    lockServo.write(UNLOCK_POS);

    delay(300);
}

void failSignal() {
    // On failure we treat the box as locked
    lockStatus = "LOCKED";

    lockServo.write(LOCK_POS);
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }
}


