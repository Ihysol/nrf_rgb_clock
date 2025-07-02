#include <main.h>

typedef void (*ModeFunction)(const tm &timeinfo);
ModeFunction currentMode = nullptr;

void modeRainbow(const tm &timeinfo)
{
  int currentSecond = timeinfo.tm_sec-1; // offset for carry over
  if (currentSecond == NUM_CIRCLE)
  {
    fill_solid(ledsCircle, NUM_CIRCLE, CRGB::Black);
  }
  else
  {

    for (int i = 0; i <= currentSecond && i < NUM_CIRCLE; i++)
    {
      ledsCircle[i] = currentColorCircle;
    }
  }
}

void modeRainbowFade(const tm &timeinfo)
{
  int currentSecond = timeinfo.tm_sec-1; // offset by 1
  if (currentSecond < 0)
  {
    fill_solid(ledsCircle, NUM_CIRCLE, CRGB::Black);
  }
  else
  {

    // initial fill at startup
    if (startup == true)
    {
      startup = false;
      fill_solid(ledsCircle, NUM_CIRCLE, CRGB::Black); // Turn off all LEDs

      for (int i = 0; i <= currentSecond && i < NUM_CIRCLE; i++)
      {
        ledsCircle[i] = CHSV(i * HUE_STEP, 255, 255);
        tempLedsCircle[i] = ledsCircle[i];
      }
      colorHue += currentSecond * HUE_STEP;
    }

    // rainbow with fade effect
    for (int i = 0; i <= currentSecond && i < NUM_CIRCLE; i++)
    {
      // fade leds that are not displaying current second
      if (i < currentSecond)
      {
        int age = currentSecond - i;
        float fadeAmount = clamp((float)age / (float)FADE_TRAIL_LENGTH, 0.0f, 1.0f);
        ledsCircle[i] = lerp(tempLedsCircle[i], CRGB::Black, fadeAmount);
      }
      else
      {
        ledsCircle[i] = CRGB::Black;
        tempLedsCircle[i] = ledsCircle[i];
      }
    }
    ledsCircle[currentSecond] = currentColorCircle;
    tempLedsCircle[currentSecond] = ledsCircle[currentSecond];
  }
}

void modeSimple(const tm &timeinfo)
{
  // if startup, populate led colors up to current second with initial color
  if (startup == true)
  {
    startup = false;
    fill_solid(ledsCircle, NUM_CIRCLE, CRGB::Black); // Turn off all LEDs

    for (int i = 0; i <= timeinfo.tm_sec; i++)
    {
      if(i%5 == 0)
      {
        ledsCircle[i] = CHSV(0, 255, 255);
      } else {
        ledsCircle[i] = CHSV(144, 255, 255);
      }
    }
  }

  // rainbow with fade effect
  int currentSecond = timeinfo.tm_sec;
  for (int i = 0; i < NUM_CIRCLE; i++)
  {
    // fade leds that are not displaying current second
    if (i < currentSecond)
    {
      ledsCircle[i] = CHSV(144, 255, 255);
    }
    else
    {
      ledsCircle[i] = CRGB::Black;
    }
    ledsCircle[currentSecond] = CHSV(144, 255, 255);
  }
}

void initHardware()
{
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
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  /// Synchronize time via NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  bool ntpSuccess = true;

  struct tm timeinfo;
  for (int attempt = 1; attempt <= maxNtpRetires; attempt++)
  {
    if (getLocalTime(&timeinfo))
    {
      ntpSuccess = true;
      Serial.printf("Time synced via NTP after %d attempt(s).\n", attempt);
      break;
    }
    else
    {
      Serial.printf("NTP attempt %d failed. Retrying...\n", attempt);
      delay(retryDelayMs);
    }
  }
}

/**
 * @brief Arduino setup function
 *
 * Initializes serial communication, LEDs, WiFi, and synchronizes time via NTP.
 */
void setup()
{
  initHardware();

  int mode = 1;
  switch (mode)
  {
  case 0:
    currentMode = modeRainbow;
    break;
  case 1:
    currentMode = modeRainbowFade;
    break;
  case 2:
    currentMode = modeSimple;
    break;
  }
}

/**
 * @brief Arduino main loop
 *
 * Updates time display and second ring animation continuously.
 */
void loop()
{
  unsigned long now = millis();

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Time not available!");
  }
  else
  {
    currentColorCircle = CHSV(colorHue, 255, 255);
    currentColorDigits = CHSV(colorHue + 16, 255, 255);

    /// Display 7-segment digits for time
    displayDigit(timeinfo.tm_hour / 10, 0, currentColorDigits);
    displayDigit(timeinfo.tm_hour % 10, 1, currentColorDigits);
    displayDigit(timeinfo.tm_min / 10, 2, currentColorDigits);
    displayDigit(timeinfo.tm_min % 10, 3, currentColorDigits);
    displayColon(true, currentColorDigits);

    /// Draw second ring

    if (currentMode)
      currentMode(timeinfo); // execute mode
    if (now - lastHueUpdate >= 100)
    {
      lastHueUpdate = now;
      colorHue += HUE_STEP;
      FastLED.show();
    }
  }
  delay(200); ///< Smooth refresh rate
}
