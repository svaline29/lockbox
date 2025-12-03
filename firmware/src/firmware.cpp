/* 
 * Project: Lockbox
 * Author: Sebastian Valine
 * Date: 12/03/2025
 * For comprehensive documentation and examples, please visit:
 * https://docs.particle.io/firmware/best-practices/firmware-template/
 */

// Include Particle Device OS APIs
#include "Particle.h"

// Let Device OS manage the connection to the Particle Cloud
SYSTEM_MODE(AUTOMATIC);

// Run the application and system concurrently in separate threads
SYSTEM_THREAD(ENABLED);

// Show system, cloud connectivity, and application logs over USB
// View logs with CLI using 'particle serial monitor --follow'
SerialLogHandler logHandler(LOG_LEVEL_INFO);


// variables and constants

const int BTN_PIN = D3;       // button pin
const int LED_PIN = D7;       // inidcator LED

const int CODE_LEN = 4; //0 = short press, 1 = long press
const int SECRET_CODE[CODE_LEN] = {0, 1, 1, 0};  // short, long, long, short

// timing thresholds, just have to define the shortest and the timeout
const unsigned long SHORT_MAX   = 400;
const unsigned long RESET_IDLE  = 5000; 


bool buttonPrev = false;          // previous sampled state of button
unsigned long pressStart = 0;     // when the current press started
unsigned long lastEventTime = 0;  // last time a press was recorded

int entered[CODE_LEN];            // buffer for entered press sequence
int enteredIndex = 0;             // how many symbols we've collected so far



void recordPress(unsigned long durationMs);
void checkCode();
void unlockBox();
void failSignal();


void setup() {
    pinMode(BTN_PIN, INPUT_PULLUP);  // button to GND, so LOW = pressed
    pinMode(LED_PIN, OUTPUT);

    digitalWrite(LED_PIN, LOW);      // start "locked"
    
    // Optional: clear entered buffer
    for (int i = 0; i < CODE_LEN; i++) {
        entered[i] = -1;
    }
}

void loop() {
    bool pressed = (digitalRead(BTN_PIN) == LOW); // pressed when LOW
    unsigned long now = millis();

    // if button just changed state
    if (pressed && !buttonPrev) {
        pressStart = now;
    }

    if (!pressed && buttonPrev) {
        unsigned long duration = now - pressStart;
        recordPress(duration);
    }

    // reset if idle too long
    if (enteredIndex > 0 && (now - lastEventTime > RESET_IDLE)) {
        enteredIndex = 0;
        for (int i = 0; i < CODE_LEN; i++) {
            entered[i] = -1;
        }
    }

    buttonPrev = pressed;
}


void recordPress(unsigned long durationMs) {
    if (enteredIndex >= CODE_LEN) {
        return;
    }

    int symbol;
    // short press: <= SHORT_MAX
    // long press:  > SHORT_MAX
    if (durationMs <= SHORT_MAX) {
        symbol = 0;  // short
    } else {
        symbol = 1;  // long
    }

    entered[enteredIndex++] = symbol;
    lastEventTime = millis();

    // When we have a full sequence, check it
    if (enteredIndex == CODE_LEN) {
        checkCode();
    }
}



// Comppare code 


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

    //reset 
    enteredIndex = 0;
    for (int i = 0; i < CODE_LEN; i++) {
        entered[i] = -1;
    }
}

void unlockBox() {
    // TODO: implement unlocking mechanism
    // for now just turn on LED
    digitalWrite(LED_PIN, HIGH);
}

void failSignal() {
    // incorrect just flashes LED
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }
}

