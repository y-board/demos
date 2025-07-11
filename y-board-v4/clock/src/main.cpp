#include "ImprovWiFiLibrary.h"
#include <Preferences.h>
#include <WiFi.h>
#include <time.h>
#include <yboard.h>

#define NTP_SERVER "pool.ntp.org"
#define TZ "MST7MDT,M3.2.0,M11.1.0"

// 'bell-solid', 9x10px
const unsigned char epd_bitmap_bell_solid[] PROGMEM = {0x08, 0x00, 0x1c, 0x00, 0x3e, 0x00, 0x7f,
                                                       0x00, 0x7f, 0x00, 0x7f, 0x00, 0x7f, 0x00,
                                                       0xff, 0x80, 0x08, 0x00, 0x0c, 0x00};
// 'volume-solid', 10x9px
const unsigned char epd_bitmap_volume_solid[] PROGMEM = {0x00, 0x00, 0x0c, 0x00, 0x1c, 0x80,
                                                         0x7d, 0x40, 0xfd, 0x40, 0x7d, 0x40,
                                                         0x1c, 0x80, 0x0c, 0x00, 0x00, 0x00};

// Array of all bitmaps for convenience. (Total bytes used to store images in PROGMEM = 96)
const int epd_bitmap_allArray_LEN = 2;
const unsigned char *epd_bitmap_allArray[2] = {epd_bitmap_bell_solid, epd_bitmap_volume_solid};

time_t now;
tm tm;
int old_sec = 0;
bool old_switch_1;
bool old_switch_2;

ImprovWiFi improvSerial(&Serial);
Preferences preferences;

void update_display();

void onImprovWiFiErrorCb(ImprovTypes::Error err) {
    Yboard.display.setCursor(0, 0);
    Yboard.display.println("Error setting up WiFi!");
    Yboard.display.display();
}

void onImprovWiFiConnectedCb(const char *ssid, const char *password) {
    // Save ssid and password here
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    Yboard.display.setCursor(0, 0);
    Yboard.display.println("Connected to WiFi!");
    Yboard.display.display();

    delay(500);

    Yboard.display.setTextSize(4);
    update_display();
    old_switch_1 = Yboard.get_switch(1);
}

// Function to interpolate between blue and red based on minutes
CRGB get_color_from_minutes(uint8_t minutes) {
    // Calculate interpolation factor (0.0 to 1.0)
    float t = minutes / 59.0;

    // Interpolate red and blue channels
    uint8_t red = 255 * t;
    uint8_t green = 0;
    uint8_t blue = 255 * (1.0 - t);

    return CRGB(red, green, blue);
}

void update_display() {
    int hour = (tm.tm_hour == 0) ? 12 : ((tm.tm_hour > 12) ? tm.tm_hour - 12 : tm.tm_hour);
    bool pm = tm.tm_hour >= 12;

    Yboard.display.clearDisplay();

    // Draw clock
    Yboard.display.setCursor(0, 0);
    Yboard.display.setTextSize(4);
    Yboard.display.printf("%.2d:%.2d", hour, tm.tm_min);

    // Draw AM/PM indicator
    Yboard.display.setTextSize(1);
    Yboard.display.setCursor(Yboard.display.width() - 11, 0);
    Yboard.display.printf(pm ? "PM" : "AM");

    if (Yboard.get_switch(1)) {
        Yboard.display.drawBitmap(Yboard.display.width() - 24, Yboard.display.height() - 10,
                                  epd_bitmap_allArray[1], 10, 9, 1);
    }

    if (Yboard.get_switch(2)) {
        Yboard.display.drawBitmap(Yboard.display.width() - 9, Yboard.display.height() - 10,
                                  epd_bitmap_allArray[0], 9, 10, 1);
    }

    Yboard.display.display();
}

void update_leds() {
    int num_leds_on = (tm.tm_sec * Yboard.num_leds) / 59;
    bool even_minute = tm.tm_min % 2 == 0;

    CRGB color = get_color_from_minutes(tm.tm_min);

    // Loop through each LED that needs to be turned on
    for (int i = 0; i < num_leds_on; i++) {
        if (even_minute) {
            Yboard.set_led_color(i + 1, color.r, color.g, color.b);
        } else {
            Yboard.set_led_color(i + 1, 0, 0, 0);
        }
    }

    // Turn off the rest of the LEDs
    for (int i = num_leds_on; i < Yboard.num_leds; i++) {
        if (even_minute) {
            Yboard.set_led_color(i + 1, 0, 0, 0);
        } else {
            Yboard.set_led_color(i + 1, color.r, color.g, color.b);
        }
    }
}

void play_sound() {
    if (Yboard.get_switch(1)) {
        // This gets played every second
        Yboard.set_sound_file_volume(3);
        Yboard.play_sound_file("/tick-quiet.wav");

        // This gets played every hour
        if (tm.tm_min == 0 && tm.tm_sec == 0) {
            Yboard.set_sound_file_volume(3);
            Yboard.play_sound_file("/beep.wav");
        }
    }
}

void setup() {
    Serial.begin(115200);
    Yboard.setup();
    Yboard.display.setTextSize(1);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(500);

    configTime(0, 0, NTP_SERVER); // 0, 0 because we will use TZ in the next line
    setenv("TZ", TZ, 1);          // Set environment variable with your time zone
    tzset();

    // Set up improv
    improvSerial.setDeviceInfo(ImprovTypes::ChipFamily::CF_ESP32_S3, "Clock", "1.0.0", "Clock");
    improvSerial.onImprovError(onImprovWiFiErrorCb);
    improvSerial.onImprovConnected(onImprovWiFiConnectedCb);

    // See if there are previous credentials saved
    preferences.begin("wifi", true); // Open in read-only mode
    String savedSSID = preferences.getString("ssid", "");
    String savedPassword = preferences.getString("password", "");
    preferences.end();
    if (savedSSID.isEmpty()) {
        Serial.println("No WiFi credentials saved.");
        Yboard.display.setCursor(0, 0);
        Yboard.display.println("No WiFi credentials.");
        Yboard.display.println("Connect to serial.");
        Yboard.display.display();
    } else {
        Serial.println("Found saved WiFi credentials.");
        Yboard.display.setCursor(0, 0);
        Yboard.display.printf("Connecting to \n%s...", savedSSID.c_str());
        Yboard.display.display();
        improvSerial.tryConnectToWifi(savedSSID.c_str(), savedPassword.c_str());
    }
}

void loop() {
    improvSerial.handleSerial();

    if (!improvSerial.isConnected()) {
        return;
    }

    time(&now);
    localtime_r(&now, &tm);

    // If switch has changed, update the display
    if (Yboard.get_switch(1) != old_switch_1) {
        update_display();
        old_switch_1 = Yboard.get_switch(1);
    }
    if (Yboard.get_switch(2) != old_switch_2) {
        update_display();
        old_switch_2 = Yboard.get_switch(2);
    }

    // Skip this loop iteration if the second hasn't changed
    if (tm.tm_sec == old_sec) {
        delay(50);
        return;
    }

    old_sec = tm.tm_sec;

    update_display();
    update_leds();
    play_sound();
}
