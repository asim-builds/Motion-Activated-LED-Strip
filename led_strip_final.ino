const int PIR_PIN = 12;    // PIR sensor pin
const int LED_PIN = 8;     // Pin for LED Strip
const int LDR_PIN = A0;    // LDR sensor pin

// Timing constants
const int TIMEOUT = 5000;       // 5 seconds timeout for walking to bathroom
const int CHECK_TIME = 4000;    // Check at 4 seconds
const int DEBOUNCE_TIME = 200;  // Time between debounce readings
const int DEBOUNCE_READS = 3;   // Number of readings for debounce
const int LDR_CHECK_INTERVAL = 1000; // Check light level every 1 second

// LDR constants
const int numReadings = 3;      // Moving average window size for LDR (reduced from 5)
const int hysteresis = 20;      // Hysteresis to prevent flickering

// Debugging
const bool DEBUG = true;        // Set to true for serial debugging, false for normal operation

const int LDR_FAST_INTERVAL = 1000;   // Check light every 1 second when dark or changing
const int LDR_SLOW_INTERVAL = 10000;  // Check light every 10 seconds when consistently bright
const int LIGHT_HISTORY_THRESHOLD = 5; // Number of consecutive light readings to trigger slow mode

// Add these new state variables with other state variables
int consecutiveLightReadings = 0;
int currentLdrInterval = LDR_FAST_INTERVAL;

// State variables for PIR
bool motionDetected = false;
unsigned long motionStartTime = 0;
unsigned long lastSerialTime = 0;
unsigned long lastWatchdogCheck = 0;

// State variables for LDR
int ldrReadings[numReadings];   // Array to store LDR readings
int readIndex = 0;
int ldrTotal = 0;
int ldrAverage = 0;
int ldrThreshold = 0;           // Will be set during calibration
bool isDark = false;
unsigned long lastLdrCheck = 0;

// System state
bool ledsEnabled = false;       // Combined state from both sensors

unsigned long ledOnTime = 0;
const int LDR_LOCKOUT_PERIOD = 6;  // 6 seconds lockout after LEDs turn on


void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  digitalWrite(LED_PIN, LOW);   // Ensure that the LED strip is off at startup
  
  if (DEBUG) {
    Serial.begin(9600);
    Serial.println("⚡ Combined PIR Motion & LDR Light Sensor System Ready...");
  }
  
  // Initialize LDR readings array
  for (int i = 0; i < numReadings; i++) {
    ldrReadings[i] = analogRead(LDR_PIN);
    ldrTotal += ldrReadings[i];
  }
  
  // Calibrate LDR threshold
  calibrateLDR();
  
  // Stabilize PIR Sensor
  if (DEBUG) Serial.println("PIR sensor stabilizing...");
  delay(3000);
  if (DEBUG) Serial.println("System ready!");
}


// LDR calibration function
void calibrateLDR() {
  // The below commented code can be used to dynamically calibrate LDR based on surrounding.
  // if (DEBUG) Serial.println("Calibrating LDR sensor...");
  
  // int sum = 0;
  // for (int i = 0; i < 10; i++) {  // Take 10 readings at startup
  //   sum += analogRead(LDR_PIN);
  //   delay(50);
  // }
  
  // ldrThreshold = sum / 10;  // Set the threshold dynamically
  
  // if (DEBUG) {
  //   Serial.print("Calibrated LDR Threshold: ");
  //   Serial.println(ldrThreshold);
  // }

  ldrThreshold = 30;
  
  // Initial darkness check
  checkLight();
}


// Debounce function for PIR reading
bool debounceMotion() {
  int motionCount = 0;
  
  // Take multiple readings with short pauses
  for (int i = 0; i < DEBOUNCE_READS; i++) {
    if (digitalRead(PIR_PIN) == HIGH) {
      motionCount++;
    }
    delay(DEBOUNCE_TIME / DEBOUNCE_READS);
  }
  
  // Return true if majority of readings show motion
  return (motionCount > DEBOUNCE_READS / 2);
}


// Check motion sensor and update motion state
void checkMotion() {
  unsigned long currentMillis = millis();
  
  // millis() will eventually overflow (~50 days), rolling back to 0. If that happens, currentMillis will suddenly be less than stored motionStartTime. 
  //This resets your timer to avoid weird behavior.
  if (currentMillis < motionStartTime) {
    motionStartTime = currentMillis;
  }
  
  // Only check motion when needed to reduce processor load
  bool motionNow = false;
  
  if (!motionDetected || 
      (motionDetected && (currentMillis - motionStartTime >= CHECK_TIME) && 
                         (currentMillis - motionStartTime < TIMEOUT))) {
    // Only debounce when needed to check for motion
    motionNow = debounceMotion();
  }
  
  if (!motionDetected && motionNow) {
    // New motion detected
    motionDetected = true;
    motionStartTime = currentMillis;
    
    if (DEBUG && (currentMillis - lastSerialTime > 1000)) {
      Serial.println("✅ Motion detected!");
      lastSerialTime = currentMillis;
    }
  } 
  else if (motionDetected) {
    // Already in a motion cycle
    if (currentMillis - motionStartTime >= CHECK_TIME && 
        currentMillis - motionStartTime < TIMEOUT && motionNow) {
      // Motion still present at check time - extend timer
      motionStartTime = currentMillis;
      
      if (DEBUG && (currentMillis - lastSerialTime > 1000)) {
        Serial.println("🔁 Motion still detected, extending time...");
        lastSerialTime = currentMillis;
      }
    } 
    else if (currentMillis - motionStartTime >= TIMEOUT) {
      // Timeout reached
      motionDetected = false;
      
      if (DEBUG && (currentMillis - lastSerialTime > 1000)) {
        Serial.println("🚨 No motion detected for full cycle.");
        lastSerialTime = currentMillis;
      }
    }
  }
}


// Check light sensor and update darkness state
void checkLight() {
  // Remove oldest reading
  ldrTotal -= ldrReadings[readIndex];

  // Read new value and update moving average
  ldrReadings[readIndex] = analogRead(LDR_PIN);
  ldrTotal += ldrReadings[readIndex];
  readIndex = (readIndex + 1) % numReadings;
  ldrAverage = ldrTotal / numReadings;  // Compute moving average

  if (DEBUG) {
    Serial.print("LDR Average: ");
    Serial.print(ldrAverage);
    Serial.print(" (Threshold: ");
    Serial.print(ldrThreshold);
    Serial.println(")");
  }

  // Update darkness state with hysteresis
  if (!isDark && ldrAverage <= ldrThreshold - hysteresis) {  
    isDark = true;
    if (DEBUG) Serial.println("🌙 Dark environment detected");
  } 
  else if (isDark && ldrAverage >= ldrThreshold + hysteresis) {  
    isDark = false;
    if (DEBUG) Serial.println("☀️ Light environment detected");
  }
}


// Watchdog function to detect inconsistent states
void runWatchdog() {
  unsigned long currentMillis = millis();
  
  // Watchdog - check for inconsistent states every 30 seconds
  if (currentMillis - lastWatchdogCheck > 30000) {
    lastWatchdogCheck = currentMillis;
    
    // Fix inconsistent LED states
    if (ledsEnabled != digitalRead(LED_PIN)) {
      digitalWrite(LED_PIN, ledsEnabled ? HIGH : LOW);
      
      if (DEBUG) {
        Serial.println("⚠️ Watchdog reset - inconsistent LED state detected");
      }
    }
  }
}


void loop() {
  unsigned long currentMillis = millis();
  
  // Check if we're in the lockout period after turning on LEDs
  bool inLockoutPeriod = ledsEnabled && (currentMillis - ledOnTime < LDR_LOCKOUT_PERIOD);
  
  // Only check light level when not in lockout period and enough time has passed since the last check.
  if (!inLockoutPeriod && currentMillis - lastLdrCheck >= currentLdrInterval) {
    lastLdrCheck = currentMillis;
    
    // Check light before doing anything else
    bool previousDarkState = isDark;
    checkLight();
    
    // Adaptive light checking interval logic
    if (!isDark) {
      // Environment is light, increment counter
      consecutiveLightReadings++;
      if (consecutiveLightReadings >= LIGHT_HISTORY_THRESHOLD && currentLdrInterval == LDR_FAST_INTERVAL) {
        // Switch to slow checking after consistent light readings
        currentLdrInterval = LDR_SLOW_INTERVAL;
        if (DEBUG) Serial.println("☀️ Consistently bright environment detected, reducing LDR check frequency");
      }
      
      // Ensure LEDs are off when it's light (do this immediately after light check)
      if (ledsEnabled) {
        ledsEnabled = false;
        digitalWrite(LED_PIN, LOW);
        if (DEBUG) Serial.println("💡 LEDs OFF - Room is bright");
      }
    } else {
      // Environment is dark, reset counter and ensure fast checking
      consecutiveLightReadings = 0;
      if (currentLdrInterval == LDR_SLOW_INTERVAL) {
        currentLdrInterval = LDR_FAST_INTERVAL;
        if (DEBUG) Serial.println("🌙 Dark environment detected, increasing LDR check frequency");
      }
    }
  }
  
  // Only proceed with motion detection and LED control if it's dark
  if (isDark) {
    checkMotion();
    
    // Update LED state based on motion (only if dark)
    bool newLedState = motionDetected;
    if (ledsEnabled != newLedState) {
      ledsEnabled = newLedState;
      digitalWrite(LED_PIN, ledsEnabled ? HIGH : LOW);
      
      // Record time when LEDs turn on
      if (ledsEnabled) {
        ledOnTime = currentMillis;
      }
      
      if (DEBUG) {
        if (ledsEnabled) {
          Serial.println("💡 LEDs ON - Dark + Motion");
        } else {
          Serial.println("💡 LEDs OFF - No motion");
        }
      }
    }
  }
  
  // Run watchdog regardless of light conditions
  runWatchdog();
  
  // Short delay to reduce CPU usage
  delay(300);
}