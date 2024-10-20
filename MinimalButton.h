#pragma once

#include "Arduino.h"

enum PressLength
{
    NO_PRESS,
    TINY_PRESS,
    SHORT_PRESS,
    LONG_PRESS,
    SUPER_PRESS,
    DOUBLE_PRESS
};

template <uint32_t pin>
class MinimalButton
{
    static volatile bool currentState;
    static volatile uint32_t changeTime;

    typedef void (*CallbackAll)(PressLength);
    typedef void (*CallbackSingle)(void);

public:
    MinimalButton(bool useInterrupt = true) : _useInterrupt(useInterrupt)
    {
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
            return false;
        }
    }

    PressLength process()
    {
        PressLength returnVal = NO_PRESS;

        if (!_useInterrupt)
        {
            currentState = !digitalRead(pin);
            changeTime = millis();
        }

        if (currentState && !_lastState)
        {
            _pressTime = changeTime;
            _lastState = true;
        }
        else if (!currentState && _lastState)
        {
            _releaseTime = changeTime;
            _lastState = false;

            uint32_t deltaTime = _releaseTime - _pressTime;

            if (deltaTime >= _ShortDelay && deltaTime < _LongDelay)
            {
                if (_pressPending && _releaseTime - _lastShortTime <= _DoubleDelay)
                {
                    returnVal = DOUBLE_PRESS;
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
                returnVal = LONG_PRESS;
            }
            else if (deltaTime >= _SuperDelay)
            {
                returnVal = SUPER_PRESS;
            }
        }

        if (_pressPending && millis() - _lastShortTime > _DoubleDelay)
        {
            returnVal = SHORT_PRESS;
            _pressPending = false;
        }

        return returnVal;
    }

    bool state()
    {
        return currentState;
    }

private:
    static void interruptHandler()
    {
        currentState = !digitalRead(pin);
        changeTime = millis();
    }

    void init()
    {
        pinMode(pin, INPUT_PULLUP);
        if (_useInterrupt)
        {
            attachInterrupt(digitalPinToInterrupt(pin), interruptHandler, CHANGE);
        }
        else
        {
        }
    }

    bool _useInterrupt = true;

    // bool currentState = false;
    bool _lastState = false;

    bool _pressPending = false;

    uint32_t _pressTime;
    uint32_t _releaseTime;
    uint32_t _lastShortTime;

    uint16_t _ShortDelay = 30;
    uint16_t _DoubleDelay = 250;
    uint16_t _LongDelay = 500;
    uint16_t _SuperDelay = 1500;
};

template <uint32_t pin>
volatile bool MinimalButton<pin>::currentState;

template <uint32_t pin>
volatile uint32_t MinimalButton<pin>::changeTime;
