// MPU6050 accelerometer + gyro over I²C.
//
//   platformio.ini → lib_deps:
//     adafruit/Adafruit MPU6050 @ ^2.2.6
//     adafruit/Adafruit Unified Sensor @ ^1.1.14
//
// Wiring: VCC→3V3, GND→GND, SDA→GPIO 21, SCL→GPIO 22.

#include <Wire.h>
#include <Adafruit_MPU6050.h>

// ── file scope ─────────────────────────────────────────────────────────
static Adafruit_MPU6050 mpu;
static bool mpuReady = false;

// ── inside setup() ─────────────────────────────────────────────────────
Wire.begin(21, 22);
mpuReady = mpu.begin();
if (mpuReady) {
    mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

// ── inside registerHandlers() ──────────────────────────────────────────
Bridge.on("imu", [](JsonVariantConst, JsonVariant resp) {
    if (!mpuReady) { resp["ok"] = false; resp["error"] = "MPU not found"; return; }
    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);
    JsonObject accel = resp["accel"].to<JsonObject>();
    accel["x"] = a.acceleration.x;
    accel["y"] = a.acceleration.y;
    accel["z"] = a.acceleration.z;
    JsonObject gyro = resp["gyro"].to<JsonObject>();
    gyro["x"] = g.gyro.x;
    gyro["y"] = g.gyro.y;
    gyro["z"] = g.gyro.z;
    resp["temp_c"] = t.temperature;
});
