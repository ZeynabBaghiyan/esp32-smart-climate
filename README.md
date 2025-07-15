# ESP32 Smart Climate Controller 🌡️🔥

This is a touch-enabled climate controller project built with an **ESP32**, featuring a TFT LCD, MQTT communication, and a DHT-like temperature/humidity sensor (SHT31). It allows you to:

- Read real-time temperature & humidity
- Control heater and fan outputs
- Toggle modes from an Android app (via MQTT)
- View status on a TFT display with touch interaction

## 📷 Project Screenshots

| LCD Interface | Android App |
|--------------|-------------|
| ![LCD Screenshot](images/lcd.jpg) | ![Mobile App Screenshot](images/mobile.jpg) |

## 🧠 Features

- 📶 Wi-Fi & MQTT communication
- 🌈 Semi-circular touch slider for temperature setting
- 🖼️ JPEG rendering via LittleFS
- 🌡️ Real-time temp & humidity monitoring (SHT31)
- 🔥 Fan/Heater/Off control (3 fan modes) 
- 📲 Remote control via MQTT (mobile app)

## 🔧 Hardware Requirements

- ESP32 board (e.g., DOIT Devkit v1)
- TFT LCD (using `TFT_eSPI`)
- XPT2046 Touch controller
- Adafruit SHT31 sensor
- 4x output relays (for heater and fans)
- Wi-Fi connection
- Power supply

## 📦 Software Stack

- Arduino Framework (PlatformIO / Arduino IDE)
- Libraries:
  - `TFT_eSPI`
  - `XPT2046_Touchscreen`
  - `Adafruit_SHT31`
  - `PubSubClient`
  - `LittleFS`
  - `JPEGDecoder`

## 📡 MQTT Topics

| Topic                      | Description               |
|---------------------------|---------------------------|
| `esp32/temperature`       | Publishes temperature     |
| `esp32/humidity`          | Publishes humidity        |
| `esp32/heater`            | Heater ON/OFF             |
| `esp32/fan`               | Fan ON/OFF                |
| `esp32/off`               | Power OFF                 |
| `esp32/mode`              | Fan mode (1/2/3)          |
| `esp32/target_temperature`| Target temp via MQTT      |
| `esp32/heater_control`    | Remote heater toggle      |
| `esp32/fan_control`       | Remote fan toggle         |
| `esp32/off_control`       | Remote off toggle         |
| `esp32/mode_control`      | Remote mode select        |

## 🧪 Getting Started

1. Clone the repo:
    ```bash
    git clone https://github.com/yourusername/esp32-climate-controller.git
    ```
2. Open in Arduino IDE or PlatformIO.
3. Update WiFi and MQTT credentials:
    ```cpp
    char ssid[] = "YOUR_SSID";
    char pass[] = "YOUR_PASSWORD";
    const char *mqtt_server = "YOUR_MQTT_BROKER_IP";
    ```
4. Upload the code to your ESP32 board.
5. Enjoy your smart controller!


## 🤝 Acknowledgments

Thanks to the open-source community and the following libraries:

- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
- [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen)
- [Adafruit_SHT31](https://github.com/adafruit/Adafruit_SHT31)




