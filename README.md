# ESP32 RC Plane Controller

This project turns your ESP32 into an Access Point hosting a web user interface to control an RC plane:  
- **2 smooth servos (one in each wing, on pins 18 & 19)**
- **1 throttle motor via MOSFET (on pin 21)**

You can connect your device to the ESP32's WiFi, open the provided web page, and control the plane’s surfaces and throttle smoothly (no auto-return/centering).

## Features

- **WiFi Access Point:** ESP32 hosts its own AP (default SSID: `ESP32_RC_Plane`)
- **Web UI:** Interactive joystick for two control surfaces, slider for throttle  
- **Smooth movement:** All controls use interpolated (tweened) motion for realism and reduced servo/binding stress
- **Throttle holds position:** Slider remains where you set it (does not auto return to 0)
- **Uses ESPAsyncWebServer, Servo library**

## Pinout

| Function  | ESP32 pin |
|-----------|-----------|
| Servo L   | 18        |
| Servo R   | 19        |
| Throttle  | 21        |

## Usage

1. Flash the `esp32_rc_plane.ino` to your ESP32 (uses Arduino framework)
2. On power-up, connect your phone/tablet to WiFi SSID `ESP32_RC_Plane`
3. Open your web browser at [http://192.168.4.1/](http://192.168.4.1/)
4. Use the on-screen joystick (roll/pitch) and throttle slider – set sticks/sliders wherever you want, smooth movement to those positions

## Required Libraries

- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP)
- [Servo](https://github.com/arduino-libraries/Servo)

## Notes

- No security (no password for AP by default) for ease of use. Set a password in the sketch if desired.
- For best results, check your servos and MOSFET wiring for correct orientation and power management.
- This code does not “center” the controls when you let go. The plane will stay at the last set positions until changed.

---

Feel free to modify this project for your specific RC application!