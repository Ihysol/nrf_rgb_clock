#include <main.h>

#define USE_NTP

typedef void (*ModeFunction)(const tm &timeinfo);
ModeFunction currentMode = nullptr;

void modeRainbowFade(const tm &timeinfo)
{
  int currentSecond = (timeinfo.tm_sec * NUM_CIRCLE) / 60;

  if (startup)
  {
    startup = false;
    int currentSecond = (timeinfo.tm_sec * NUM_CIRCLE) / 60;

    for (int i = 0; i < FADE_TRAIL_LENGTH; i++)
    {
      int secondIndex = (currentSecond - i + NUM_CIRCLE) % NUM_CIRCLE;
      uint8_t hue = (colorHue + i * HUE_STEP) % 255;
      float fade = (float)i / (float)FADE_TRAIL_LENGTH;
      tempLedsCircle[secondIndex] = lerp(CHSV(hue, 255, 255), CRGB::Black, fade);
    }
  }

  tempLedsCircle[currentSecond] = CHSV(colorHue, 255, 255);

  int nextSecond = (currentSecond +1) % NUM_CIRCLE;
  tempLedsCircle[nextSecond] = CHSV((colorHue + HUE_STEP) % 255, 255, 100);

  for (int i = 0; i < NUM_CIRCLE; i++)
  {
    int age = (currentSecond - i + NUM_CIRCLE) % NUM_CIRCLE;
    float fadeAmount = clamp((float)age / (float)FADE_TRAIL_LENGTH, 0.0f, 1.0f);
    ledsCircle[i] = lerp(tempLedsCircle[i], CRGB::Black, fadeAmount);
  }

  ledsCircle[currentSecond] = CHSV(colorHue, 255*0.75, 255);
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
      if (i % 5 == 0)
      {
        ledsCircle[i] = CHSV(0, 255, 255);
      }
      else
      {
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

  struct tm timeinfo;

/// Synchronize time via NTP
#ifdef USE_NTP
  WiFi.begin(wifi_ssid, wifi_password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  if (getLocalTime(&timeinfo))
  {
    initialEpochTime = mktime(&timeinfo);
    millisAtSync = millis();
  } else 
  {
    Serial.printf("NTP attempt failed. Rebooting ESP32...\n");
    esp_restart();
  }
#endif
}

/**
 * @brief Arduino setup function
 *
 * Initializes serial communication, LEDs, WiFi, and synchronizes time via NTP.
 */
void setup()
{
  initHardware();

  int mode = 0;
  switch (mode)
  {
  case 0:
    currentMode = modeRainbowFade;
    break;
  case 1:
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
  time_t nowEpoch = initialEpochTime + (millis() - millisAtSync) /1000;
  struct tm timeinfo;
  localtime_r(&nowEpoch, &timeinfo);

  unsigned long now = millis();

  // if (!getLocalTime(&timeinfo))
  // {
  //   Serial.println("Time not available!");
  // }
  // else
  // {
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
  // }
  delay(200); ///< Smooth refresh rate
}
