# Example handlers

Copy/paste these into `registerHandlers()` in `src/main.cpp`. Add the
library to `platformio.ini → lib_deps` if noted, then `pio run -t upload`.

| File | What it does | Extra wiring |
|------|--------------|--------------|
| `button.cpp` | Counts presses on a button between a GPIO and GND | Button → GPIO + GND |
| `servo.cpp` | Sweeps or sets a servo angle | Servo signal → GPIO, **external 5 V supply, common GND** |
| `ultrasonic.cpp` | HC-SR04 distance in cm | Trig/Echo → GPIO (echo via voltage divider) |
| `dht.cpp` | DHT22 temperature & humidity | Data → GPIO with 10 kΩ pullup |
| `neopixel.cpp` | Set colour on a WS2812 strip | Data → GPIO (often 5 V logic; level shifter ideal) |
| `mpu6050.cpp` | Read accelerometer / gyro over I²C | SDA=21, SCL=22 |
| `motion.cpp` | Push notifications on PIR motion detect | OUT → GPIO |
| `pwm.cpp` | Control LED brightness / motor speed | PWM out → MOSFET / driver |

None of these conflict with each other — register as many as you like.
