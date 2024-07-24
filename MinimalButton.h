#pragma once

#include "Arduino.h"

enum PressLength
{
    NO_PRESS,
    TINY_PRESS,
    SHORT_PRESS,
    LONG_PRESS,
    SUPER_PRESS,
    DOUBLE_PRESS,
    CONSTANT_PRESS
};

template <uint32_t pin>
class MinimalButton
{
    static MinimalButton<pin> *instance;

public:
    MinimalButton(bool useInterrupt = true) : _useInterrupt(useInterrupt)
    {
        instance = this;
    }

    void begin()
    {
        init();
    }

    bool begin(uint16_t newTinyDelay, uint16_t newShortDelay, uint16_t newDoubleDelay, uint16_t newLongDelay, uint16_t newSuperDelay)
    {
        if (newTinyDelay < newShortDelay && newLongDelay > newShortDelay && newSuperDelay > newLongDelay && newShortDelay + newDoubleDelay < newLongDelay)
        {
            _TinyDelay = newTinyDelay;
            _ShortDelay = newShortDelay;
            _DoubleDelay = newDoubleDelay;
            _LongDelay = newLongDelay;
            _SuperDelay = newSuperDelay;

            init();

            return true;
        }
        else
        {
            Serial.println("error: invalid delay values");
            return false;
        }
    }

    void clear()
    {
        _currentPress = NO_PRESS;
    }

    PressLength process()
    {
        if (!_useInterrupt)
        {
            updatePress();
        }

        uint32_t currentTime = millis();
        if (_currentState && (currentTime - _pressTime > _SuperDelay) && !_constantReported)
        {
            _currentPress = CONSTANT_PRESS;
            _constantReported = true;
        }

        PressLength returnVal = _currentPress;

        clear();

        return returnVal;
    }

private:
#ifdef ESP32
    static void IRAM_ATTR interruptHandler()
#else
#ifdef ARDUINO_ARCH_STM32
    __attribute__((section(".data"))) static void interruptHandler()
#else
    static void interruptHandler()
#endif
#endif
    {
        instance->updatePress();
    }

    void init()
    {
        pinMode(pin, INPUT_PULLUP);
        if (_useInterrupt)
        {
            attachInterrupt(digitalPinToInterrupt(pin), interruptHandler, CHANGE);
        }
    }

    void updatePress()
    {
        uint32_t currentTime = millis();

        if (_currentPress == NO_PRESS)
        {
            _currentState = !digitalRead(pin);
            _changeTime = currentTime;

            if (_currentState && !_lastState)
            {
                _pressTime = _changeTime;
                _lastState = true;
                _constantReported = false;
            }
            else if (!_currentState && _lastState)
            {
                _releaseTime = _changeTime;
                _lastState = false;

                unsigned long deltaTime = _releaseTime - _pressTime;

                if (deltaTime < _ShortDelay)
                {
                    if (deltaTime >= _TinyDelay)
                    {
                        _currentPress = TINY_PRESS;
                    }
                }
                else if (deltaTime >= _ShortDelay && deltaTime < _LongDelay)
                {
                    if (_pressPending && (_releaseTime - _lastShortTime <= _DoubleDelay))
                    {
                        _currentPress = DOUBLE_PRESS;
                        _pressPending = false;
                    }
                    else
                    {
                        _lastShortTime = _releaseTime;
                        _pressPending = true;
                    }
                }
                else if (deltaTime >= _LongDelay && deltaTime < _SuperDelay)
                {
                    _currentPress = LONG_PRESS;
                }
                else if (deltaTime >= _SuperDelay)
                {
                    _currentPress = SUPER_PRESS;
                }
            }

            // Check for pending short press
            if (_pressPending && (currentTime - _lastShortTime > _DoubleDelay))
            {
                _currentPress = SHORT_PRESS;
                _pressPending = false;
            }
        }
    }

    volatile PressLength _currentPress = NO_PRESS;
    volatile bool _currentState = false;
    volatile bool _lastState = false;
    volatile uint32_t _changeTime = 0;
    volatile bool _constantReported = false;

    bool _useInterrupt = true;
    bool _pressPending = false;

    uint32_t _pressTime;
    uint32_t _releaseTime;
    uint32_t _lastShortTime;

    uint16_t _TinyDelay = 1;
    uint16_t _ShortDelay = 50;
    uint16_t _DoubleDelay = 250;
    uint16_t _LongDelay = 500;
    uint16_t _SuperDelay = 1500;
};

template <uint32_t pin>
MinimalButton<pin> *MinimalButton<pin>::instance = nullptr;