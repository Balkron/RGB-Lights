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

enum INPUT_MODE
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

enum Rotation
{
    NONE,
    CLOCKWISE,
    COUNTERCLOCKWISE
};

class RotaryEncoder
{
private:
    uint8_t inputBuffer;
    const uint8_t patternLeftRotation = 0b10000111;
    const uint8_t patternRightRotation = 0b01001011;

public:
    RotaryEncoder()
    {
        inputBuffer = 0b00000011;
    }

    void update(bool inputA, bool inputB)
    {
        int8_t currentState = (inputA << 1) | inputB;

        if (currentState != (inputBuffer & 0b11))
        {
            inputBuffer = (inputBuffer << 2) | currentState;
        }
    }

    Rotation read()
    {
        if (inputBuffer == patternLeftRotation)
        {
            inputBuffer = 0;
            return Rotation::COUNTERCLOCKWISE;
        }
        else if (inputBuffer == patternRightRotation)
        {
            inputBuffer = 0;
            return Rotation::CLOCKWISE;
        }

        return Rotation::NONE;
    }
};

ColorHSV current_hsv = {0, 1.0, 1.0};
INPUT_MODE inputMode = INPUT_MODE::HUE;

inline void updateLEDs()
{
    ColorRGB color = brightnessHueToRGB(current_hsv);
    analogWrite(PINS::LED_RED, color.red);
    analogWrite(PINS::LED_GREEN, color.green);
    analogWrite(PINS::LED_BLUE, color.blue);
}

RotaryEncoder rotaryEncoder;

unsigned long lastColorChange = 0UL;

void setup()
{
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
        current_hsv.hue = 0;
    }
    if (current_hsv.saturation < 0 || current_hsv.saturation > 1.0)
    {
        current_hsv.saturation = 1.0;
    }
    if (current_hsv.brightness < 0 || current_hsv.brightness > 1.0)
    {
        current_hsv.brightness = 1.0;
    }
    updateLEDs();
}

void loop()
{
    if (lastColorChange)
    {
        if ((millis() - lastColorChange) > 60000)
        {
            if (EEPROM.length() > 0)
            {
                EEPROM.put(0, current_hsv);
            }
            lastColorChange = 0UL;
        }
    }

    rotaryEncoder.update(digitalRead(PINS::ROTARY_ENCODER_A), digitalRead(PINS::ROTARY_ENCODER_B));

    if (!digitalRead(PINS::BUTTON))
    {
        unsigned long start = millis();
        while (!digitalRead(PINS::BUTTON))
        {
            if ((millis() - start) > 2000)
            {
                current_hsv = {0, 1.0, 1.0};
                if (EEPROM.length() > 0)
                {
                    lastColorChange = millis();
                }
                updateLEDs();
                inputMode = INPUT_MODE::HUE;
            }
        }

        if ((millis() - start) < 2000)
            inputMode = static_cast<INPUT_MODE>((inputMode + 1) % 3);

        delayMicroseconds(100);
        return;
    }

    Rotation rotation = rotaryEncoder.read();

    if (rotation != Rotation::NONE)
    {
        int adjustmentDirection = (rotation == Rotation::CLOCKWISE) ? ADJUSTMENT_MULTIPLIER : -ADJUSTMENT_MULTIPLIER;

        switch (inputMode)
        {
        case INPUT_MODE::HUE:
            current_hsv.hue = (360 + current_hsv.hue + adjustmentDirection) % 360;
            break;

        case INPUT_MODE::BRIGHTNESS:
            current_hsv.brightness = min(max(0.0, current_hsv.brightness + 0.01 * adjustmentDirection), 1.0);
            break;

        case INPUT_MODE::SATURATION:
            current_hsv.saturation = min(max(0.0, current_hsv.saturation + 0.01 * adjustmentDirection), 1.0);
            break;
        }

        if (EEPROM.length() > 0)
        {
            lastColorChange = millis();
        }

        updateLEDs();
    }


    delayMicroseconds(10); // Kurze Verzögerung für die PWM-Steuerung
}