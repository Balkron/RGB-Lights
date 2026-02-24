#include <EEPROM.h>

const int ADJUSTMENT_MULTIPLIER = 5; // Schrittgröße für die Anpassung von Hue, Brightness und Saturation

struct ColorRGB
{
    int red, green, blue;
};

struct ColorHSV
{
    int hue;
    double saturation;
    double brightness;
};

enum PINS
{
    ROTARY_ENCODER_A = 5,
    ROTARY_ENCODER_B,
    BUTTON,
    LED_RED = 9,
    LED_GREEN,
    LED_BLUE
};

enum MODES
{
    SOLID,
    BREATHING,
    RAINBOW,
    FLASHING
};

enum INPUT_STATE
{
    HUE,
    BRIGHTNESS,
    SATURATION
};

// Funktion zum Umwandeln von Helligkeit und Farbe in RGB
ColorRGB brightnessHueToRGB(ColorHSV hsv)
{
    double c = hsv.brightness * hsv.saturation;
    double x = c * (1 - fabs(fmod(hsv.hue / 60.0, 2) - 1));
    double m = hsv.brightness - c;

    double r = 0, g = 0, b = 0;
    if (0 <= hsv.hue && hsv.hue < 60)
    {
        r = c;
        g = x;
        b = 0;
    }
    else if (60 <= hsv.hue && hsv.hue < 120)
    {
        r = x;
        g = c;
        b = 0;
    }
    else if (120 <= hsv.hue && hsv.hue < 180)
    {
        r = 0;
        g = c;
        b = x;
    }
    else if (180 <= hsv.hue && hsv.hue < 240)
    {
        r = 0;
        g = x;
        b = c;
    }
    else if (240 <= hsv.hue && hsv.hue < 300)
    {
        r = x;
        g = 0;
        b = c;
    }
    else if (300 <= hsv.hue && hsv.hue < 360)
    {
        r = c;
        g = 0;
        b = x;
    }

    // Umwandlung in den Bereich 0-255
    return {static_cast<int>((r + m) * 255), static_cast<int>((g + m) * 255), static_cast<int>((b + m) * 255)};
}

float radian = 0.0f;
float stepSize = 0.001f;

ColorHSV current_hsv = {0, 1.0, 1.0};
ColorRGB color;
bool buttonPressedLastState = false;
bool encoderALastState = false;
bool encoderBLastState = false;
INPUT_STATE inputState = INPUT_STATE::HUE;

inline void updateLEDs()
{
    ColorRGB color = brightnessHueToRGB(current_hsv);
    analogWrite(PINS::LED_RED, color.red);
    analogWrite(PINS::LED_GREEN, color.green);
    analogWrite(PINS::LED_BLUE, color.blue);
}

void setup()
{
    Serial.begin(115200);
    pinMode(PINS::ROTARY_ENCODER_A, INPUT);
    pinMode(PINS::ROTARY_ENCODER_B, INPUT);
    pinMode(PINS::BUTTON, INPUT);
    pinMode(PINS::LED_RED, OUTPUT);
    pinMode(PINS::LED_GREEN, OUTPUT);
    pinMode(PINS::LED_BLUE, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    if (EEPROM.length() > 0)
    {
        EEPROM.get(0, current_hsv);
    }
    if (current_hsv.hue < 0 || current_hsv.hue >= 360)
    {
        randomSeed(analogRead(0));
        current_hsv.hue = random(360);
    }
    if (current_hsv.saturation < 0 || current_hsv.saturation > 1.0)
    {
        current_hsv.saturation = 1.0;
    }
    if (current_hsv.brightness < 0 || current_hsv.brightness > 1.0)
    {
        current_hsv.brightness = 1.0;
    }
    buttonPressedLastState = digitalRead(PINS::BUTTON);
    updateLEDs();
}

void rotaryRead(PINS pin_1, PINS pin_2, int adjustmentDirection)
{
    while (digitalRead(pin_2))
    {
    }

    while (!digitalRead(pin_1))
    {
    }

    while (!digitalRead(pin_2))
    {
    }

    switch (inputState)
    {
    case INPUT_STATE::HUE:
        current_hsv.hue = (360 + current_hsv.hue + adjustmentDirection) % 360;
        break;

    case INPUT_STATE::BRIGHTNESS:
        current_hsv.brightness = max(0.0, current_hsv.brightness + 0.01 * adjustmentDirection);
        break;

    case INPUT_STATE::SATURATION:
        current_hsv.saturation = max(0.0, current_hsv.saturation + 0.01 * adjustmentDirection);
        break;
    }

    if (EEPROM.length() > 0)
    {
        EEPROM.put(0, current_hsv);
    }
    updateLEDs();
}

void loop()
{
    bool buttonPressed = digitalRead(PINS::BUTTON);
    digitalWrite(LED_BUILTIN, buttonPressed);

    if (buttonPressed && !buttonPressedLastState)
    {
        inputState = static_cast<INPUT_STATE>((inputState + 1) % 3);
        buttonPressedLastState = buttonPressed;
        delayMicroseconds(100);
        return;
    }

    buttonPressedLastState = buttonPressed;

    bool encoderA = digitalRead(PINS::ROTARY_ENCODER_A);
    bool encoderB = digitalRead(PINS::ROTARY_ENCODER_B);

    if (!encoderA && encoderB)
    {
        rotaryRead(PINS::ROTARY_ENCODER_A, PINS::ROTARY_ENCODER_B, ADJUSTMENT_MULTIPLIER);
    }
    else if (!encoderB && encoderA)
    {
        rotaryRead(PINS::ROTARY_ENCODER_B, PINS::ROTARY_ENCODER_A, -ADJUSTMENT_MULTIPLIER);
    }

    // Serial.print("\rButton State: " + String(buttonPressed) + " " + String(encoderA) + " " + String(encoderB) + " Hue: " + String(current_hsv.hue) + " Brightness: " + String(current_hsv.brightness) + " Saturation: " + String(current_hsv.saturation) + " RGB: (" + String(color.red) + ", " + String(color.green) + ", " + String(color.blue) + ")   ");
    delayMicroseconds(10); // Kurze Verzögerung für die PWM-Steuerung
}