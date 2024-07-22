#pragma once

#include "Arduino.h"

enum PressLength
{
    NO_PRESS,
    SHORT_PRESS,
    LONG_PRESS,
    SUPER_PRESS,
    DOUBLE_PRESS
};

template <uint32_t pin>
class MinimalButton
{
    typedef void (*CallbackAll)(PressLength);
    typedef void (*CallbackSingle)(void);

    static MinimalButton<pin>* instance;

public:
    MinimalButton(bool useInterrupt = true) : _useInterrupt(useInterrupt)
    {
        instance = this;
    }

    void begin()
    {
        init();
    }

    bool begin(uint16_t newShortDelay, uint16_t newDoubleDelay, uint16_t newLongDelay, uint16_t newSuperDelay)
    {
        if (newLongDelay > newShortDelay && newSuperDelay > newLongDelay && newShortDelay + newDoubleDelay < newLongDelay)
        {
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

    PressLength process()
    {
        if (!_useInterrupt)
        {
            updatePress();
        }

        PressLength returnVal = _currentPress;
        _currentPress = NO_PRESS;

        if (returnVal != NO_PRESS)
        {
            if (_callbackAll)
                _callbackAll(returnVal);

            switch (returnVal)
            {
            case SHORT_PRESS:
                if (_callbackShort)
                    _callbackShort();
                break;
            case DOUBLE_PRESS:
                if (_callbackDouble)
                    _callbackDouble();
                break;
            case LONG_PRESS:
                if (_callbackLong)
                    _callbackLong();
                break;
            case SUPER_PRESS:
                if (_callbackSuper)
                    _callbackSuper();
                break;
            }
        }
        return returnVal;
    }

    bool state()
    {
        return _currentState;
    }

    void attach(CallbackAll funcAll)
    {
        _callbackAll = funcAll;
    }

    void attach(CallbackSingle funcShort = NULL, CallbackSingle funcDouble = NULL, CallbackSingle funcLong = NULL, CallbackSingle funcSuper = NULL)
    {
        _callbackShort = funcShort;
        _callbackDouble = funcDouble;
        _callbackLong = funcLong;
        _callbackSuper = funcSuper;
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
        // Don't have a press waiting to be read...
        if (_currentPress == NO_PRESS)
        {
            _currentState = !digitalRead(pin);
            _changeTime = millis();

            if (_currentState && !_lastState)
            {
                _pressTime = _changeTime;
                _lastState = true;
            }
            else if (!_currentState && _lastState)
            {
                _releaseTime = _changeTime;
                _lastState = false;

                unsigned long deltaTime = _releaseTime - _pressTime;

                if (deltaTime >= _ShortDelay && deltaTime < _LongDelay)
                {
                    if (_pressPending && _releaseTime - _lastShortTime <= _DoubleDelay)
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

            if (_pressPending && millis() - _lastShortTime > _DoubleDelay)
            {
                _currentPress = SHORT_PRESS;
                _pressPending = false;
            }
        }
    }

    PressLength _currentPress = NO_PRESS;

    bool _useInterrupt = true;

    bool _currentState = false;
    bool _lastState = false;

    bool _pressPending = false;

    volatile uint32_t _changeTime = 0;

    uint32_t _pressTime;
    uint32_t _releaseTime;
    uint32_t _lastShortTime;

    uint16_t _ShortDelay = 50;
    uint16_t _DoubleDelay = 250;
    uint16_t _LongDelay = 500;
    uint16_t _SuperDelay = 1500;

    CallbackAll _callbackAll = NULL;
    CallbackSingle _callbackShort = NULL;
    CallbackSingle _callbackDouble = NULL;
    CallbackSingle _callbackLong = NULL;
    CallbackSingle _callbackSuper = NULL;
};

template <uint32_t pin>
MinimalButton<pin>* MinimalButton<pin>::instance = nullptr;