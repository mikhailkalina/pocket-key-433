#pragma once

#include <stdint.h>

namespace Button
{
  /**
   * @brief Button identifiers
   */
  enum class Id : uint8_t
  {
    None,
    Up,
    Down,
    Left,
    Right,
  };

  /**
   * @brief Button states
   */
  enum class State : uint8_t
  {
    Released,
    Pressed,
    HeldDown,
  };

  /**
   * @brief Button events
   */
  enum class Event : uint8_t
  {
    None,
    PressStart,
    PressEnd,
    HoldStart,
    HoldContinue,
    HoldEnd,
  };

  /**
   * @brief Initialize buttons
   */
  void initialize();

  /**
   * @brief Process buttons changes
   *
   * @return identifier of the button if action detected, None otherwise
   */
  Id process();

  /**
   * @brief Return current state for specified button
   *
   * @param id Button identifier
   * @return Button current state
   */
  State getState(Id id);

  /**
   * @brief Return detected event for specified button
   *
   * @param id Button identifier
   * @return Button detected event
   */
  Event getEvent(Id id);

  /**
   * @brief Check raw button levels before entering sleep
   *
   * @return true if any button is currently pressed
   */
  bool anyPressed();

  /**
   * @brief Ignore the wake press until all buttons have been released and debounced
   */
  void ignoreUntilReleased();
} // namespace Button
