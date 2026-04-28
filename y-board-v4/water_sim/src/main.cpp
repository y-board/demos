#include "Arduino.h"
#include "yboard.h"
#include <math.h>

//////////////////////////////// Configuration Constants ///////////////////////

// How quickly gravity pulls the water toward the low side.
// Increase for snappier response; decrease for slower, heavier water.
const float SPRING = 0.012f;
// Fraction of velocity retained each frame (higher = more sloshy overshoot).
const float DAMPING = 0.96f;
// Maximum angular velocity of the water (radians/frame).  Prevents runaway
// spinning when the board is held at a steep angle for a long time.
const float MAX_VELOCITY = 0.15f;
// Fraction of the LED ring that is underwater when at rest (~16 of 35 LEDs).
const float WATER_FILL = 0.45f;
// Base splash zone height above the waterline (in normalized depth units).
const float SPLASH_SPREAD = 0.12f;
// Multiplier for how much speed increases the splash zone.
const float SPLASH_VEL_SCALE = 8.0f;
// Frame delay in milliseconds (~60 fps).
const int FRAME_MS = 16;

// Rotates the LED ring mapping to correct for board orientation.
// If the water pools on the wrong side when the board is tilted, try
// adjusting this in 90-degree steps: 0, 90, 180, or 270.
const float LED_RING_OFFSET_DEG = 0.0f;

//////////////////////////////// Globals ///////////////////////////////////////

float water_angle = 0.0f;
float water_velocity = 0.0f;

//////////////////////////////// Forward Declarations //////////////////////////

static float wrap_angle(float angle);
static void update_physics(float gx, float gy);
static void render_leds(float speed);

//////////////////////////////// Main Functions ////////////////////////////////

void setup() {
  Serial.begin(9600);
  Yboard.setup();
  Yboard.set_led_brightness(180);

  Yboard.display.clearDisplay();
  Yboard.display.setTextColor(WHITE);
  Yboard.display.setTextSize(2);
  Yboard.display.setCursor(8, 0);
  Yboard.display.println("Water");
  Yboard.display.setTextSize(1);
  Yboard.display.setCursor(0, 22);
  Yboard.display.println("Tilt the board to");
  Yboard.display.println("watch the water");
  Yboard.display.println("slosh around!");
  Yboard.display.display();
}

void loop() {
  if (!Yboard.accelerometer_available()) {
    delay(100);
    return;
  }

  accelerometer_data accel = Yboard.get_accelerometer();
  update_physics(accel.x, accel.y);
  render_leds(fabsf(water_velocity));
  delay(FRAME_MS);
}

//////////////////////////////// Helper Functions //////////////////////////////

static float wrap_angle(float angle) {
  while (angle > PI)
    angle -= 2.0f * PI;
  while (angle < -PI)
    angle += 2.0f * PI;
  return angle;
}

// Apply one frame of spring-damper physics using the board's in-plane
// acceleration. gx and gy are the X and Y accelerometer readings in mg.
static void update_physics(float gx, float gy) {
  float gravity_mag = fminf(sqrtf(gx * gx + gy * gy) / 1000.0f, 1.0f);
  float gravity_dir = atan2f(gy, gx);

  float diff = wrap_angle(gravity_dir - water_angle);
  water_velocity += diff * SPRING * gravity_mag;
  water_velocity *= DAMPING;
  if (water_velocity > MAX_VELOCITY)
    water_velocity = MAX_VELOCITY;
  if (water_velocity < -MAX_VELOCITY)
    water_velocity = -MAX_VELOCITY;
  water_angle = wrap_angle(water_angle + water_velocity);
}

// Set LED colors to render the water surface at the current water_angle.
static void render_leds(float speed) {
  float water_threshold = 1.0f - 2.0f * WATER_FILL;
  float splash_zone = SPLASH_SPREAD + speed * SPLASH_VEL_SCALE;
  float ring_offset = LED_RING_OFFSET_DEG * PI / 180.0f;

  for (int i = 0; i < Yboard.num_leds; i++) {
    float led_angle = (2.0f * PI * i) / Yboard.num_leds + ring_offset;
    float cos_depth = cosf(led_angle - water_angle);
    float depth = (cos_depth - water_threshold) / (1.0f - water_threshold);

    uint8_t r = 0, g = 0, b = 0;

    if (depth >= 0.0f) {
      float surface_prox = 1.0f - depth;
      g = (uint8_t)(surface_prox * 80.0f);
      b = (uint8_t)(30.0f + surface_prox * 200.0f);
    } else if (-depth < splash_zone) {
      float t = 1.0f - (-depth / splash_zone);
      float intensity = t * t * fminf(1.0f, 0.4f + speed * 5.0f);
      g = (uint8_t)(intensity * 120.0f);
      b = (uint8_t)(intensity * 220.0f);
    }

    Yboard.set_led_color(i + 1, r, g, b);
  }
}
