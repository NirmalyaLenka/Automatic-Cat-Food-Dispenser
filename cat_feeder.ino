/*
  ============================================================
  Automatic Cat Food Dispenser - ESP32 Firmware
  ============================================================
  
  How it works:
    - Connects to your WiFi and syncs the real time using NTP
    - Checks the time every minute
    - Feeds the cat at 8:00, 12:00, 16:00, and 20:00 (every 4 hrs)
    - No feeding happens after 9:00 PM (21:00)
    - Before dispensing, plays 3 buzzer beeps to call the cat
    - Opens the servo valve for 3 seconds, then closes it
    - Servo at 0 degrees = valve OPEN
    - Servo at 90 degrees = valve CLOSED

  Hardware needed:
    - ESP32 Dev Board
    - DS3225MG Servo (25kg torque) or equivalent high-torque servo
    - Active Buzzer (5V)
    - 5V power supply (servo needs its own power, don't run it from ESP32 3.3V!)
    - Common ground between ESP32 and servo power supply

  Wiring:
    - Servo Signal  --> GPIO 13
    - Servo VCC     --> External 5V (NOT the ESP32 5V pin)
    - Servo GND     --> GND (shared with ESP32)
    - Buzzer +      --> GPIO 12
    - Buzzer -      --> GND
============================================================
*/

#include <WiFi.h>
#include <ESP32Servo.h>
#include <time.h>

// ---- CHANGE THESE TO YOUR WIFI CREDENTIALS ----
const char* WIFI_SSID     = "YourWiFiName";
const char* WIFI_PASSWORD = "YourWiFiPassword";
// ------------------------------------------------

// ---- PIN DEFINITIONS ----
#define SERVO_PIN   13
#define BUZZER_PIN  12

// ---- SERVO ANGLE SETTINGS ----
#define VALVE_OPEN   0    // Servo position = valve is open (food falls)
#define VALVE_CLOSED 90   // Servo position = valve is closed (food blocked)

// ---- TIMING SETTINGS ----
#define VALVE_OPEN_DURATION_MS  3000   // Keep valve open for 3 seconds
#define BUZZER_BEEP_DURATION_MS  200   // How long each beep lasts
#define BUZZER_BEEP_PAUSE_MS     300   // Pause between beeps
#define LAST_FEED_HOUR           20    // Last allowed feeding = 8PM (20:00)
#define NO_FEED_AFTER_HOUR       21    // Never feed at or after 9PM (21:00)

// ---- NTP SERVER SETTINGS ----
// These free public servers give you accurate internet time
const char* NTP_SERVER   = "pool.ntp.org";
const long  GMT_OFFSET   = 19800;  // IST = UTC+5:30 = 19800 seconds (change for your timezone)
const int   DAYLIGHT_OFF = 0;      // India doesn't use daylight saving time

// ---- FEEDING SCHEDULE ----
// The dispenser feeds at these hours: 8AM, 12PM, 4PM, 8PM
// Add or remove hours here. Must be < NO_FEED_AFTER_HOUR
const int FEED_HOURS[] = { 8, 12, 16, 20 };
const int NUM_FEED_SLOTS = sizeof(FEED_HOURS) / sizeof(FEED_HOURS[0]);

Servo valveServo;

// Tracks whether we already fed during the current hour
// so the loop doesn't trigger multiple times in the same minute
bool alreadyFedThisHour = false;
int  lastFedHour        = -1;

// ================================================================
// SETUP
// ================================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("==== Cat Food Dispenser Starting ====");

  // Set up buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Attach servo and close the valve immediately on startup
  valveServo.attach(SERVO_PIN);
  valveServo.write(VALVE_CLOSED);
  Serial.println("[SERVO] Valve closed at startup.");
  delay(500);

  // Connect to WiFi
  connectToWiFi();

  // Sync time from NTP server
  configTime(GMT_OFFSET, DAYLIGHT_OFF, NTP_SERVER);
  Serial.print("[TIME] Syncing time from NTP");
  
  struct tm timeInfo;
  int retries = 0;
  while (!getLocalTime(&timeInfo) && retries < 20) {
    Serial.print(".");
    delay(500);
    retries++;
  }

  if (retries >= 20) {
    Serial.println("\n[TIME] ERROR: Could not sync time. Check your WiFi connection.");
  } else {
    Serial.println("\n[TIME] Time synced successfully!");
    printCurrentTime();
  }

  Serial.println("[READY] Dispenser is running. Waiting for feeding times...");
  printFeedingSchedule();
}

// ================================================================
// MAIN LOOP
// ================================================================
void loop() {
  struct tm timeInfo;

  if (!getLocalTime(&timeInfo)) {
    Serial.println("[ERROR] Failed to get local time. Retrying in 10 seconds...");
    delay(10000);
    return;
  }

  int currentHour   = timeInfo.tm_hour;
  int currentMinute = timeInfo.tm_min;

  // Reset the "already fed" flag when the hour changes
  if (currentHour != lastFedHour) {
    alreadyFedThisHour = false;
  }

  // Don't feed if we already fed this hour
  if (alreadyFedThisHour) {
    delay(15000); // Check again in 15 seconds
    return;
  }

  // Don't feed after 9 PM
  if (currentHour >= NO_FEED_AFTER_HOUR) {
    // Just sleep until midnight to save processing
    delay(60000);
    return;
  }

  // Check if current time matches any feeding slot
  // We trigger at minute 0 of each feed hour (e.g., 8:00, 12:00...)
  if (currentMinute == 0 && isFeedingHour(currentHour)) {
    Serial.print("[FEEDING] Feeding time triggered at ");
    printCurrentTime();

    callCatWithBuzzer();     // 3 beeps to call the cat
    delay(2000);             // Give the cat a moment to come
    openValveAndDispense();  // Open valve, feed, close valve

    alreadyFedThisHour = true;
    lastFedHour        = currentHour;

    Serial.println("[DONE] Feeding complete. Next check in 60 seconds.");
  }

  // Sleep for 30 seconds before checking again
  // This is short enough to not miss the :00 minute trigger
  delay(30000);
}

// ================================================================
// HELPER FUNCTIONS
// ================================================================

/**
 * Returns true if the given hour is in the feeding schedule
 */
bool isFeedingHour(int hour) {
  for (int i = 0; i < NUM_FEED_SLOTS; i++) {
    if (FEED_HOURS[i] == hour) {
      return true;
    }
  }
  return false;
}

/**
 * Plays 3 short buzzer beeps to train the cat to come to the feeder
 */
void callCatWithBuzzer() {
  Serial.println("[BUZZER] Calling cat with 3 beeps...");
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(BUZZER_BEEP_DURATION_MS);
    digitalWrite(BUZZER_PIN, LOW);
    delay(BUZZER_BEEP_PAUSE_MS);
  }
  Serial.println("[BUZZER] Done.");
}

/**
 * Rotates servo to open position, waits 3 seconds, then closes it
 */
void openValveAndDispense() {
  Serial.println("[VALVE] Opening valve...");
  valveServo.write(VALVE_OPEN);
  delay(500); // Give servo time to physically move

  Serial.println("[VALVE] Dispensing food for 3 seconds...");
  delay(VALVE_OPEN_DURATION_MS);

  Serial.println("[VALVE] Closing valve...");
  valveServo.write(VALVE_CLOSED);
  delay(500); // Give servo time to physically move

  Serial.println("[VALVE] Valve closed.");
}

/**
 * Connects the ESP32 to WiFi. Retries until connected.
 */
void connectToWiFi() {
  Serial.print("[WIFI] Connecting to ");
  Serial.print(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] Connected!");
    Serial.print("[WIFI] IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[WIFI] Failed to connect. Check SSID and password.");
    Serial.println("[WIFI] Restarting in 10 seconds...");
    delay(10000);
    ESP.restart();
  }
}

/**
 * Prints the current local time to Serial Monitor
 */
void printCurrentTime() {
  struct tm timeInfo;
  if (getLocalTime(&timeInfo)) {
    Serial.print("[TIME] Current time: ");
    Serial.printf("%02d:%02d:%02d on %02d/%02d/%04d\n",
      timeInfo.tm_hour,
      timeInfo.tm_min,
      timeInfo.tm_sec,
      timeInfo.tm_mday,
      timeInfo.tm_mon + 1,
      timeInfo.tm_year + 1900
    );
  }
}

/**
 * Prints the full feeding schedule to Serial Monitor
 */
void printFeedingSchedule() {
  Serial.println("[SCHEDULE] Today's feeding times:");
  for (int i = 0; i < NUM_FEED_SLOTS; i++) {
    Serial.printf("  --> %02d:00\n", FEED_HOURS[i]);
  }
  Serial.printf("[SCHEDULE] No feeding after %02d:00\n", NO_FEED_AFTER_HOUR);
}
