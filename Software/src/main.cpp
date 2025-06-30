#include <Arduino.h>
#include <WiFi.h>
#include <FastLED.h>
#include <time.h>

/** @brief WiFi credentials */
const char* ssid     = "WLAN-SSID"; 
const char* password = "WLAN-PASSWORD";

/** @brief NTP configuration */
const char* ntpServer = "pool.ntp.org"; ///< NTP server address
const long  gmtOffset_sec = 3600;       ///< GMT offset in seconds (e.g., +1 hour for CET)
const int   daylightOffset_sec = 3600;  ///< Daylight saving time offset in seconds

/** @brief LED configuration */
#define NUM_SEGMENTS 114      ///< Number of segment LEDs
#define PIN_SEGMENTS 27       ///< GPIO pin for segment LEDs

#define NUM_CIRCLE 59         ///< Number of circle LEDs
#define PIN_CIRCLE 26         ///< GPIO pin for circle LEDs

#define LED_TYPE WS2813       ///< Type of LED strip
#define COLOR_ORDER GRB       ///< Color order of LEDs
#define BRIGHTNESS 64         ///< LED brightness

/// Array for segment LEDs
CRGB ledsSegments[NUM_SEGMENTS];

/// Array for circle LEDs
CRGB ledsCircle[NUM_CIRCLE];

/**
 * @brief Mapping of digits to 7-segment representation
 * 
 * Each inner array corresponds to a digit from 0 to 9,
 * and each value indicates whether the segment [a-g] is on (1) or off (0).
 */
const uint8_t digitSegments[10][7] = {
 /// a, b, c, d, e, f, g
    {1, 1, 1, 1, 1, 1, 0}, ///< 0
    {0, 1, 1, 0, 0, 0, 0}, ///< 1
    {1, 1, 0, 1, 1, 0, 1}, ///< 2
    {1, 1, 1, 1, 0, 0, 1}, ///< 3
    {0, 1, 1, 0, 0, 1, 1}, ///< 4
    {1, 0, 1, 1, 0, 1, 1}, ///< 5
    {1, 0, 1, 1, 1, 1, 1}, ///< 6
    {1, 1, 1, 0, 0, 0, 0}, ///< 7
    {1, 1, 1, 1, 1, 1, 1}, ///< 8
    {1, 1, 1, 1, 0, 1, 1}  ///< 9
};

/**
 * @brief Mapping from logical segment [a-g] to physical LED indices
 */
const uint8_t segmentMap[7][4] = {
  {0, 1, 2, 3},       ///< Segment a
  {4, 5, 6, 7},       ///< Segment b
  {8, 9, 10, 11},     ///< Segment c
  {12, 13, 14, 15},   ///< Segment d
  {16, 17, 18, 19},   ///< Segment e
  {20, 21, 22, 23},   ///< Segment f
  {24, 25, 26, 27}    ///< Segment g
};

/**
 * @brief Arduino setup function
 * 
 * Initializes serial communication, LEDs, WiFi, and synchronizes time via NTP.
 */
void setup() {
  Serial.begin(115200);

  /// Initialize LED arrays
  FastLED.addLeds<LED_TYPE, PIN_SEGMENTS, COLOR_ORDER>(ledsSegments, NUM_SEGMENTS);
  FastLED.addLeds<LED_TYPE, PIN_CIRCLE, COLOR_ORDER>(ledsCircle, NUM_CIRCLE);
  FastLED.setBrightness(BRIGHTNESS);
  fill_solid(ledsSegments, NUM_SEGMENTS, CRGB::Black);
  fill_solid(ledsCircle, NUM_CIRCLE, CRGB::Black);
  FastLED.show();

  /// Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  /// Synchronize time via NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Waiting for time...");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Time not available...");
    delay(1000);
  }
  Serial.println("Time synchronized!");
}

/**
 * @brief Display a digit on a 7-segment area
 * 
 * @param digit The digit (0–9) to display
 * @param digitIndex The position (0–3) of the digit on the display
 * @param color Color to use for active segments
 */
void displayDigit(uint8_t digit, uint8_t digitIndex, CRGB color) {
  int baseIndex;
  switch (digitIndex) {
    case 0: baseIndex = 0; break;
    case 1: baseIndex = 28; break;
    case 2: baseIndex = 58; break;
    case 3: baseIndex = 86; break;
    default: return;
  }

  for (int s = 0; s < 7; s++) {
    for (int i = 0; i < 4; i++) {
      int ledIndex = baseIndex + segmentMap[s][i];
      ledsSegments[ledIndex] = digitSegments[digit][s] ? color : CRGB::Black;
    }
  }
}

/**
 * @brief Display or hide the colon separator
 * 
 * @param on Whether to turn on (true) or off (false) the colon
 */
void displayColon(bool on) {
  ledsSegments[56] = on ? CRGB::Blue : CRGB::Black;
  ledsSegments[57] = on ? CRGB::Blue : CRGB::Black;
}

/**
 * @brief Arduino main loop
 * 
 * Updates time display and second ring animation continuously.
 */
void loop() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Time not available!");
    return;
  }

  int hour = timeinfo.tm_hour;
  int minute = timeinfo.tm_min;
  int second = timeinfo.tm_sec;

  /// Display 7-segment digits for time
  displayDigit(hour / 10, 0, CRGB::Blue);
  displayDigit(hour % 10, 1, CRGB::Blue);
  displayDigit(minute / 10, 2, CRGB::Blue);
  displayDigit(minute % 10, 3, CRGB::Blue);
  displayColon(true);

  /// Draw second ring
  fill_solid(ledsCircle, NUM_CIRCLE, CRGB::Black); // Turn off all LEDs

  /// Light up LEDs up to the current second
  for (int i = 0; i <= second && i < NUM_CIRCLE; i++) {
    ledsCircle[i] = CHSV(i * (256 / NUM_CIRCLE), 255, 255); ///< Rainbow gradient
  }

  FastLED.show();
  delay(100); ///< Smooth refresh rate
}
