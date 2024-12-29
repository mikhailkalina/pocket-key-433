#pragma once

namespace Button
{
  /**
   * @brief Button identifiers
   */
  enum class Id
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
  enum class State
  {
    Released,
    Pressed,
    Hold,
  };

  /**
   * @brief Button action types
   */
  enum class Action
  {
    None,
    PressStart,
    PressEnd,
    HoldStart,
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
   * @brief Return detected action for specified button
   *
   * @param id Button identifier
   * @return Button detected action
   */
  Action getAction(Id id);
} // namespace Button
