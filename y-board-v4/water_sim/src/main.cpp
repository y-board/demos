
#include "Arduino.h"
#include "yboard.h"
#include <math.h>

//////////////////////////////// Configuration Constants ///////////////////////

// How quickly gravity pulls the water toward the low side
const float SPRING = 0.003f;
// Fraction of velocity retained each frame (higher = more sloshy)
const float DAMPING = 0.97f;
// Fraction of the LED ring that is underwater when at rest
const float WATER_FILL = 0.45f;
// Base splash zone height above the waterline (in normalized depth units)
const float SPLASH_SPREAD = 0.12f;
// Multiplier for how much speed increases the splash zone
const float SPLASH_VEL_SCALE = 8.0f;
// Frame delay in milliseconds (~60fps)
const int FRAME_MS = 16;

//////////////////////////////// Globals ///////////////////////////////////////

// Direction toward which the water has shifted (radians, board X-Y plane)
float water_angle = 0.0f;
// Angular velocity of the water mass (radians/frame)
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

// Normalize an angle to the range [-PI, PI].
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
  // In-plane gravity magnitude, normalized to [0, 1] (1000 mg = 1 g)
  float gravity_mag = sqrtf(gx * gx + gy * gy) / 1000.0f;
  gravity_mag = fminf(gravity_mag, 1.0f);

  // Direction the water should flow toward (the low side of the board)
  float gravity_dir = atan2f(gy, gx);

  // Pull the water angle toward the gravity direction
  float diff = wrap_angle(gravity_dir - water_angle);
  water_velocity += diff * SPRING * gravity_mag;
  water_velocity *= DAMPING;
  water_angle = wrap_angle(water_angle + water_velocity);
}

// Set LED colors to render the water surface at the current water_angle.
// speed is the absolute angular velocity used to scale the splash effect.
static void render_leds(float speed) {
  // cos(angle) must exceed this threshold for an LED to be underwater.
  // WATER_FILL=0.45 gives ~46% of the ring submerged (~16 of 35 LEDs).
  float water_threshold = 1.0f - 2.0f * WATER_FILL;

  // Splash zone expands above the waterline as the water moves faster.
  float splash_zone = SPLASH_SPREAD + speed * SPLASH_VEL_SCALE;

  for (int i = 0; i < Yboard.num_leds; i++) {
    float led_angle = (2.0f * PI * i) / Yboard.num_leds;

    // 1.0 at the deepest point (water center), -1.0 at the opposite peak.
    float cos_depth = cosf(led_angle - water_angle);

    // Normalized depth: 1 = deepest, 0 = at waterline, negative = above water.
    float depth = (cos_depth - water_threshold) / (1.0f - water_threshold);

    uint8_t r = 0, g = 0, b = 0;

    if (depth >= 0.0f) {
      // Underwater: dark blue at the bottom, bright blue-cyan at the surface.
      float surface_prox = 1.0f - depth;
      g = (uint8_t)(surface_prox * 80.0f);
      b = (uint8_t)(30.0f + surface_prox * 200.0f);
    } else if (-depth < splash_zone) {
      // Splash zone: fades out above the waterline, brighter when moving fast.
      float t = 1.0f - (-depth / splash_zone);
      float intensity = t * t * fminf(1.0f, 0.4f + speed * 5.0f);
      g = (uint8_t)(intensity * 120.0f);
      b = (uint8_t)(intensity * 220.0f);
    }

    Yboard.set_led_color(i + 1, r, g, b);
  }
}
