#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <JPEGDecoder.h>
#include <LittleFS.h>
#include <Adafruit_SHT31.h>
#include "Free_Fonts.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <string>
#include <cstring>

#define XPT2046_IRQ 26
#define XPT2046_MOSI 21
#define XPT2046_MISO 19
#define XPT2046_CLK 18
#define XPT2046_CS 25
#define SDA_PIN 23
#define SCL_PIN 22

// WiFi credentials
char ssid[] = "";
char pass[] = "";


// Static IP configuration
const char *mqtt_server = "";
const int mqtt_port = 2883;
const char *mqtt_topic_humidity = "esp32/humidity";
const char *mqtt_topic_temperature = "esp32/temperature";
const char *mqtt_topic_fan = "esp32/fan";
const char *mqtt_topic_off = "esp32/off";
const char *mqtt_topic_heater = "esp32/heater";
const char *mqtt_topic_mode = "esp32/mode";

const char *mqtt_topic_target_temperature = "esp32/target_temperature";
const char *mqtt_topic_fan_control = "esp32/fan_control";
const char *mqtt_topic_off_control = "esp32/off_control";
const char *mqtt_topic_heater_control = "esp32/heater_control";
const char *mqtt_topic_mode_control = "esp32/mode_control";

WiFiClient espClient;
PubSubClient client(espClient);

TFT_eSPI tft = TFT_eSPI();
SPIClass touchscreenSPI = SPIClass(VSPI);

XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSprite sprite = TFT_eSprite(&tft);

Adafruit_SHT31 sht31 = Adafruit_SHT31();


int sliderCenterX = 160; 
int sliderCenterY = 170;  
int sliderRadius = 100;   
int sliderThickness = 20; 
int sliderValue = 50;     
int Tempreture = 15;
bool Heater_state = false;
bool Off_state = true;
bool Fan_state = false;
bool Menu_toggle = false;
int mode = 1;

void reconnect()
{
    while (!client.connected())
    {
        Serial.print("Connecting to MQTT...");
        if (client.connect("ESP32Client"))
        {
            Serial.println("Connected to MQTT");
            client.subscribe(mqtt_topic_mode_control);
            client.subscribe(mqtt_topic_heater_control);
            client.subscribe(mqtt_topic_off_control);
            client.subscribe(mqtt_topic_fan_control);
            client.subscribe(mqtt_topic_target_temperature);
        }
        else
        {
            Serial.print("Failed. State: ");
            Serial.println(client.state());
            delay(2000);
        }
    }
}

uint32_t getColorGradient(int value)
{
    if (value <= 50)
    {
        return tft.color565(0, map(value, 0, 50, 255, 255), map(value, 0, 50, 0, 255)); 
    }
    else
    {
        return tft.color565(map(value, 50, 100, 255, 255), map(value, 50, 100, 255, 0), 0); 
    }
}
void clearSliderEdges()
{
    tft.fillRect(sliderCenterX - sliderRadius, sliderCenterY, sliderThickness, 15, TFT_WHITE);

    tft.fillRect(sliderCenterX + sliderRadius - sliderThickness, sliderCenterY, sliderThickness, 15, TFT_WHITE);
}
void draw_SHT31(float temperature, float humidity)
{

    tft.setCursor(70, 203);
    tft.setTextSize(2);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.printf("%.0f%cC", temperature, 247);

    tft.setCursor(248, 203);
    tft.printf("%.0f%%", humidity);
}
void clearSliderArea()
{
    for (int angle = 180; angle <= 360; angle++)
    { 
        float rad = radians(angle);

        
        for (int r = 0; r <= sliderRadius; r++)
        {
            int x = sliderCenterX + cos(rad) * r;
            int y = sliderCenterY + sin(rad) * r;

            tft.drawPixel(x, y, TFT_WHITE);
        }
    }
}
void drawSlider(int value)
{

    tft.fillRect(sliderCenterX - sliderRadius, sliderCenterY - sliderRadius, sliderRadius * 2, sliderRadius, TFT_WHITE);
    clearSliderEdges();

    float angle = map(value, 0, 100, 180, 360); 
    for (int i = 180; i <= angle; i++)
    {
        float rad = radians(i);
        int innerX = sliderCenterX + cos(rad) * (sliderRadius - sliderThickness);
        int innerY = sliderCenterY + sin(rad) * (sliderRadius - sliderThickness);
        int outerX = sliderCenterX + cos(rad) * sliderRadius;
        int outerY = sliderCenterY + sin(rad) * sliderRadius;

        uint32_t color = getColorGradient(value);
        tft.drawLine(innerX, innerY, outerX, outerY, color);
    }

    for (float i = 180; i <= 360; i += 1)
    {
        float rad = radians(i);
        int innerX = sliderCenterX + cos(rad) * (sliderRadius - sliderThickness);
        int innerY = sliderCenterY + sin(rad) * (sliderRadius - sliderThickness);
        int outerX = sliderCenterX + cos(rad) * sliderRadius;
        int outerY = sliderCenterY + sin(rad) * sliderRadius;

        tft.drawPixel(innerX, innerY, TFT_LIGHTGREY);
        tft.drawPixel(outerX, outerY, TFT_LIGHTGREY);
    }

    float rad = radians(angle);
    int circleX = sliderCenterX + cos(rad) * ((sliderRadius - sliderThickness) + sliderThickness / 2);
    int circleY = sliderCenterY + sin(rad) * ((sliderRadius - sliderThickness) + sliderThickness / 2);
    tft.fillCircle(circleX, circleY, sliderThickness / 2, TFT_BLUE);

    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(sliderCenterX - 28, sliderCenterY - 10);
    Tempreture = value / 2 - 10;
    tft.printf("%d", Tempreture);
    tft.setFreeFont(FF0);
    tft.print(char(247));
    tft.print("C");
    tft.printf(" ");
}

void drawJPEGToSprite(const char *filename, int xpos, int ypos)
{
    File jpegFile = LittleFS.open(filename, "r");
    if (!jpegFile)
    {
        Serial.print("File not found: ");
        Serial.println(filename);
        return;
    }

    bool decoded = JpegDec.decodeFsFile(jpegFile);
    if (!decoded)
    {
        Serial.println("JPEG decoding failed!");
        return;
    }

    int16_t imgWidth = JpegDec.width;
    int16_t imgHeight = JpegDec.height;

    sprite.createSprite(imgWidth, imgHeight);

    uint16_t *pImg;
    while (JpegDec.readSwappedBytes())
    {
        pImg = JpegDec.pImage;
        int mcu_w = JpegDec.MCUWidth;
        int mcu_h = JpegDec.MCUHeight;

        int mcu_x = JpegDec.MCUx * mcu_w;
        int mcu_y = JpegDec.MCUy * mcu_h;

        sprite.pushImage(mcu_x, mcu_y, mcu_w, mcu_h, pImg);
    }

    sprite.pushSprite(xpos, ypos);

    sprite.deleteSprite();
}

void callback(char *topic, byte *message, unsigned int length)
{
    Serial.print("Message arrived on topic: ");
    Serial.println(topic);
    Serial.print("Message: ");
    String receivedMessage;
    for (int i = 0; i < length; i++)
    {
        receivedMessage += (char)message[i];
    }
    Serial.println(receivedMessage);

    if (String(topic) == mqtt_topic_target_temperature)
    {
        Tempreture = atoi(receivedMessage.c_str());
        sliderValue = (Tempreture + 10) * 2;
        drawSlider(sliderValue);
    }
    if (String(topic) == mqtt_topic_fan_control)
    {
        if (receivedMessage == "1")
        {
            if (Fan_state)
            {
                Heater_state = false;
                Off_state = true;
                Fan_state = false;
                Menu_toggle = false;
                client.publish(mqtt_topic_heater, "0");
                client.publish(mqtt_topic_off, "1");
                client.publish(mqtt_topic_fan, "0");
                client.publish(mqtt_topic_mode, "0");
            }
            else
            {
                Heater_state = false;
                Off_state = false;
                Fan_state = true;
                client.publish(mqtt_topic_heater, "0");
                client.publish(mqtt_topic_off, "0");
                client.publish(mqtt_topic_fan, "1");
                client.publish(mqtt_topic_mode, std::to_string(mode).c_str());
            }
        }
        else if (receivedMessage == "0")
        {
            if (Fan_state)
            {
                Heater_state = false;
                Off_state = true;
                Fan_state = false;
                Menu_toggle = false;
                client.publish(mqtt_topic_heater, "0");
                client.publish(mqtt_topic_off, "1");
                client.publish(mqtt_topic_fan, "0");
                client.publish(mqtt_topic_mode, "0");
            }
            else
            {
                Heater_state = false;
                Off_state = false;
                Fan_state = true;
                client.publish(mqtt_topic_heater, "0");
                client.publish(mqtt_topic_off, "0");
                client.publish(mqtt_topic_fan, "1");
                client.publish(mqtt_topic_mode, std::to_string(mode).c_str());
            }
        }
        else
        {
            Serial.println("Unknown command");
        }
    }
    if (String(topic) == mqtt_topic_heater_control)
    {
        if (receivedMessage == "1")
        {
            if (Heater_state)
            {
                Heater_state = false;
                Off_state = true;
                Fan_state = false;
                client.publish(mqtt_topic_heater, "0");
                client.publish(mqtt_topic_off, "1");
                client.publish(mqtt_topic_fan, "0");
                client.publish(mqtt_topic_mode, "0");
            }
            else
            {
                Heater_state = true;
                Off_state = false;
                Fan_state = false;
                Menu_toggle = false;
                client.publish(mqtt_topic_heater, "1");
                client.publish(mqtt_topic_off, "0");
                client.publish(mqtt_topic_fan, "0");
                client.publish(mqtt_topic_mode, "0");
            }
        }
        else if (receivedMessage == "0")
        {
            if (Heater_state)
            {
                Heater_state = false;
                Off_state = true;
                Fan_state = false;
                client.publish(mqtt_topic_heater, "0");
                client.publish(mqtt_topic_off, "1");
                client.publish(mqtt_topic_fan, "0");
                client.publish(mqtt_topic_mode, "0");
            }
            else
            {
                Heater_state = true;
                Off_state = false;
                Fan_state = false;
                Menu_toggle = false;
                client.publish(mqtt_topic_heater, "1");
                client.publish(mqtt_topic_off, "0");
                client.publish(mqtt_topic_fan, "0");
                client.publish(mqtt_topic_mode, "0");
            }
        }
        else
        {
            Serial.println("Unknown command");
        }
    }
    if (String(topic) == mqtt_topic_off_control)
    {
        if (receivedMessage == "1")
        {
            Heater_state = false;
            Off_state = true;
            Fan_state = false;
            Menu_toggle = false;
            client.publish(mqtt_topic_heater, "0");
            client.publish(mqtt_topic_off, "1");
            client.publish(mqtt_topic_fan, "0");
            client.publish(mqtt_topic_mode, "0");
        }
        else if (receivedMessage == "0")
        {
            Heater_state = false;
            Off_state = true;
            Fan_state = false;
            Menu_toggle = false;
            client.publish(mqtt_topic_heater, "0");
            client.publish(mqtt_topic_off, "1");
            client.publish(mqtt_topic_fan, "0");
            client.publish(mqtt_topic_mode, "0");
        }
        else
        {
            Serial.println("Unknown command");
        }
    }

    if (String(topic) == mqtt_topic_mode_control)
    {
        if (receivedMessage == "1")
        {
            if (Fan_state)
            {
                mode = 1;
                client.publish(mqtt_topic_mode, "0");
                client.publish(mqtt_topic_mode, "1");
            }
            else
                client.publish(mqtt_topic_mode, "0");
        }
        else if (receivedMessage == "2")
        {
            if (Fan_state)
            {
                mode = 2;
                client.publish(mqtt_topic_mode, "0");
                client.publish(mqtt_topic_mode, "2");
            }
            else
                client.publish(mqtt_topic_mode, "0");
        }
        else if (receivedMessage == "3")
        {
            if (Fan_state)
            {
                mode = 3;
                client.publish(mqtt_topic_mode, "0");
                client.publish(mqtt_topic_mode, "3");
            }
            else
                client.publish(mqtt_topic_mode, "0");
        }
        else
        {
            Serial.println("Unknown command");
        }
    }
}

void setup()
{
    pinMode(13, OUTPUT);
    pinMode(12, OUTPUT);
    pinMode(14, OUTPUT);
    pinMode(27, OUTPUT);
    Serial.begin(115200);
    tft.begin();
    Wire.begin(SDA_PIN, SCL_PIN);
    tft.setRotation(1);

    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");

    if (!LittleFS.begin(true))
    {
        Serial.println("Failed to mount LittleFS");
        while (1)
            ;
    }

    ts.begin();
    if (!sht31.begin(0x44))
    { 
        Serial.println("Failed to find SHT31 sensor!");
        while (1)
            ;
    }
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    tft.fillScreen(TFT_WHITE); 

    drawJPEGToSprite("/test.jpg", 210, 188);
    drawJPEGToSprite("/test1.jpg", 30, 180);
    drawJPEGToSprite("/test2.jpg", 70, 15);
    drawJPEGToSprite("/test3.jpg", 210, 15);
    drawJPEGToSprite("/test4_1.jpg", 145, 15);

    drawSlider(sliderValue);
}

void loop()
{
    if (!client.connected())
    {
        reconnect();
    }
    client.loop();
    float temperature = sht31.readTemperature();
    float humidity = sht31.readHumidity();
    if (!isnan(temperature) && !isnan(humidity))
    {
        draw_SHT31(temperature, humidity);

        static unsigned long lastMsg = 0;
        unsigned long now = millis();
        if (now - lastMsg > 7000)
        {
            Serial.print("Temperature: ");
            Serial.print(temperature);
            Serial.println(" °C");

            Serial.print("Humidity: ");
            Serial.print(humidity);
            Serial.println(" %");
            lastMsg = now;
            client.publish(mqtt_topic_temperature, std::to_string(temperature).c_str());
            client.publish(mqtt_topic_humidity, std::to_string(humidity).c_str());
        }
    }
    else
    {
        Serial.println("Failed to read from SHT31-D sensor!");
    }

    if (ts.touched())
    {
        TS_Point p = ts.getPoint();

        int16_t x = map(p.x, 0, 4095, 0, tft.width());
        int16_t y = map(p.y, 0, 4095, 0, tft.height());

        // Touching Heater
        if (x >= 200 && x <= 240 && y >= 175 && y <= 230)
        {
            if (Heater_state)
            {
                Heater_state = false;
                Off_state = true;
                Fan_state = false;
                client.publish(mqtt_topic_heater, "0");
                client.publish(mqtt_topic_off, "1");
                client.publish(mqtt_topic_fan, "0");
                client.publish(mqtt_topic_mode, "0");
            }
            else
            {
                Heater_state = true;
                Off_state = false;
                Fan_state = false;
                Menu_toggle = false;
                client.publish(mqtt_topic_heater, "1");
                client.publish(mqtt_topic_off, "0");
                client.publish(mqtt_topic_fan, "0");
                client.publish(mqtt_topic_mode, "0");
            }
        }
        // Touching OFF
        if (x >= 145 && x <= 175 && y >= 175 && y <= 230)
        {
            Heater_state = false;
            Off_state = true;
            Fan_state = false;
            Menu_toggle = false;
            client.publish(mqtt_topic_heater, "0");
            client.publish(mqtt_topic_off, "1");
            client.publish(mqtt_topic_fan, "0");
            client.publish(mqtt_topic_mode, "0");
        }
        // Touching Fan
        if (x >= 80 && x <= 120 && y >= 175 && y <= 230)
        {
            if (Fan_state)
            {
                Heater_state = false;
                Off_state = true;
                Fan_state = false;
                Menu_toggle = false;
                client.publish(mqtt_topic_heater, "0");
                client.publish(mqtt_topic_off, "1");
                client.publish(mqtt_topic_fan, "0");
                client.publish(mqtt_topic_mode, "0");
            }
            else
            {
                Heater_state = false;
                Off_state = false;
                Fan_state = true;
                client.publish(mqtt_topic_heater, "0");
                client.publish(mqtt_topic_off, "0");
                client.publish(mqtt_topic_fan, "1");
                client.publish(mqtt_topic_mode, std::to_string(mode).c_str());
            }
        }
        if (Fan_state)
        {
            if (x >= 64 && x <= 79 && y >= 173 && y <= 188)
            {
                Menu_toggle = !Menu_toggle;
            }
            if (Menu_toggle)
            {
                if (x >= 29 && x <= 62 && y >= 134 && y <= 164)
                {
                    mode = 1;
                    client.publish(mqtt_topic_mode, "0");
                    client.publish(mqtt_topic_mode, "1");
                }
                if (x >= 29 && x <= 62 && y >= 97 && y <= 127)
                {
                    mode = 2;
                    client.publish(mqtt_topic_mode, "0");
                    client.publish(mqtt_topic_mode, "2");
                }
                if (x >= 29 && x <= 62 && y >= 58 && y <= 88)
                {
                    mode = 3;
                    client.publish(mqtt_topic_mode, "0");
                    client.publish(mqtt_topic_mode, "3");
                }
            }
        }
        int dx = x - sliderCenterX - 6;
        int dy = y - sliderCenterY + 110;
        float distance = sqrt(dx * dx + dy * dy);
        float angle = atan2(dy, dx) * 180 / PI;

        if (distance >= (sliderRadius - sliderThickness) && distance <= sliderRadius && angle >= 0 && angle <= 180)
        {
            sliderValue = map(angle, 0, 180, 0, 100);
            drawSlider(sliderValue);
        }
    }

    float goal_temp = sliderValue / 2 - 10;
    if (Heater_state)
    {
        drawJPEGToSprite("/test2_1.jpg", 70, 15);
        if (temperature < goal_temp)
            digitalWrite(13, HIGH);
        else
            digitalWrite(13, LOW);
    }
    else
    {
        drawJPEGToSprite("/test2.jpg", 70, 15);
        digitalWrite(13, LOW);
    }
    if (Off_state)
        drawJPEGToSprite("/test4_1.jpg", 145, 15);
    else
        drawJPEGToSprite("/test4.jpg", 145, 15);
    if (Fan_state)
    {
        if(mode==1){
            if (temperature > goal_temp)
                digitalWrite(12, HIGH);
            else
                digitalWrite(12, LOW);
            digitalWrite(14, LOW);
            digitalWrite(27, LOW);
        }
        else if(mode==2){
            if (temperature > goal_temp)
                    digitalWrite(14, HIGH);
                else
                    digitalWrite(14, LOW);
                digitalWrite(12, LOW);
                digitalWrite(27, LOW);
        }
        else{
            if (temperature > goal_temp)
                    digitalWrite(27, HIGH);
                else
                    digitalWrite(27, LOW);
                digitalWrite(12, LOW);
                digitalWrite(14, LOW);
        }
        drawJPEGToSprite("/test3_1.jpg", 210, 15);
        if (Menu_toggle)
        {
            drawJPEGToSprite("/mode3.jpg", 261, 43);
            if (mode == 1)
            {
                drawJPEGToSprite("/mode_1.jpg", 280, 65);
                drawJPEGToSprite("/mode1.jpg", 280, 107);
                drawJPEGToSprite("/mode2.jpg", 280, 149);
                
            }
            else if (mode == 2)
            {
                drawJPEGToSprite("/mode.jpg", 280, 65);
                drawJPEGToSprite("/mode1_1.jpg", 280, 107);
                drawJPEGToSprite("/mode2.jpg", 280, 149);
                
            }
            else
            {
                drawJPEGToSprite("/mode.jpg", 280, 65);
                drawJPEGToSprite("/mode1.jpg", 280, 107);
                drawJPEGToSprite("/mode2_1.jpg", 280, 149);
                
            }
        }
        else
        {
            drawJPEGToSprite("/mode3_2.jpg", 261, 43);
            drawJPEGToSprite("/white.jpg", 280, 65);
            drawJPEGToSprite("/white.jpg", 280, 107);
            drawJPEGToSprite("/white.jpg", 280, 149);
        }
    }
    else
    {
        drawJPEGToSprite("/mode3_1.jpg", 261, 43);
        drawJPEGToSprite("/white.jpg", 280, 65);
        drawJPEGToSprite("/white.jpg", 280, 107);
        drawJPEGToSprite("/white.jpg", 280, 149);
        drawJPEGToSprite("/test3.jpg", 210, 15);
        digitalWrite(12, LOW);
        digitalWrite(14, LOW);
        digitalWrite(27, LOW);
    }
}