#include "button.h"

#include <stdint.h>

#include <Arduino.h>

using namespace Button;

/**
 * @brief Button item structure
 */
struct ButtonItem
{
    const Id id;
    const uint8_t pin;
    const uint8_t activeLevel;
    State state;
    Event event;
    bool isActive;
    unsigned long eventTimeMs;
};

namespace
{
    constexpr unsigned long debounceTimeMs = 20;
    constexpr unsigned long holdStartTimeMs = 1000;
    constexpr unsigned long holdContinueTimeMs = 200;

    // Button items list
    ButtonItem buttonList[] = {
        {
            .id = Id::Up,
            .pin = 4,
            .activeLevel = LOW,
        },
        {
            .id = Id::Down,
            .pin = 5,
            .activeLevel = LOW,
        },
        {
            .id = Id::Left,
            .pin = 6,
            .activeLevel = LOW,
        },
        {
            .id = Id::Right,
            .pin = 7,
            .activeLevel = LOW,
        },
    };
} // namespace

/**
 * @brief Initialize buttons
 */
void Button::initialize()
{
    for (const ButtonItem &button : buttonList)
    {
        pinMode(button.pin, INPUT_PULLUP);
    }
}

/**
 * @brief Process buttons changes
 *
 * @return identifier of the button if action detected, None otherwise
 */
Id Button::process()
{
    Id id = Id::None;
    // Get current system time
    unsigned long currentTimeMs = millis();

    for (ButtonItem &button : buttonList)
    {
        button.event = Event::None; // No action by default

        uint8_t buttonPinLevel = digitalRead(button.pin);
        if (buttonPinLevel == button.activeLevel)
        {
            // Button is active
            if (button.isActive == false)
            {
                // First time detect active level
                button.isActive = true;
                button.eventTimeMs = currentTimeMs;
            }
            else if (button.state == State::Released &&
                     currentTimeMs > button.eventTimeMs + debounceTimeMs)
            {
                // Debounce time passed after press
                button.event = Event::PressStart;
                button.state = State::Pressed;
                button.eventTimeMs = currentTimeMs;
            }
            else if (button.state == State::Pressed &&
                     currentTimeMs > button.eventTimeMs + holdStartTimeMs)
            {
                // Hold start time passed after press
                button.event = Event::HoldStart;
                button.state = State::Hold;
                button.eventTimeMs = currentTimeMs;
            }
            else if (button.state == State::Hold &&
                     currentTimeMs > button.eventTimeMs + holdContinueTimeMs)
            {
                // Hold continue time passed after last hold event
                button.event = Event::HoldContinue;
                button.eventTimeMs = currentTimeMs;
            }
        }
        else if (button.isActive == true)
        {
            button.isActive = false;
            if (button.state != State::Released)
            {
                // Determine action type according to state
                button.event = (button.state == State::Pressed) ? Event::PressEnd : Event::HoldEnd;
                // Button is released
                button.state = State::Released;
            }
        }

        if (button.event != Event::None)
        {
            id = button.id;
            break;
        }
    }

    return id;
}

/**
 * @brief Return current state for specified button
 *
 * @param id Button identifier
 * @return Button current state
 */
State Button::getState(Id id)
{
    State state = State::Released;

    for (const ButtonItem &button : buttonList)
    {
        if (button.id == id)
        {
            state = button.state;
        }
    }

    return state;
}

/**
 * @brief Return detected event for specified button
 *
 * @param id Button identifier
 * @return Button detected event
 */
Event Button::getEvent(Id id)
{
    Event event = Event::None;

    for (const ButtonItem &button : buttonList)
    {
        if (button.id == id)
        {
            event = button.event;
        }
    }

    return event;
}
