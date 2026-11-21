#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "comic2/game_state.hpp"

namespace comic2 {

// Returns the authentic DOS key label from scratch_key_names_table (0x0035F)
// for a 1-based DOS hardware scancode (0x01..0x52).
std::string_view dos_scancode_name(std::uint8_t dos_scancode);

// Validates a candidate DOS scancode during interactive 6-step key rebinding
// (Left, Right, Up, Down, Jump, Fire) matching input_read_mapped_key
// (1000:054b): must be in 0x02..0x52 (not ESC=0x01) and not duplicate any key
// already bound in steps [0, current_step).
bool is_valid_unique_dos_key_binding(std::uint8_t dos_scancode,
                                     const DosKeyBindings &partial_bindings,
                                     std::size_t current_step);

// Maps a DOS scancode to InputState actions using the provided DosKeyBindings.
void apply_dos_scancode_to_input(InputState &input, std::uint8_t dos_scancode,
                                 const DosKeyBindings &bindings);

// Keyboard input handler using SDL2
// Provides real keyboard event processing instead of environment variables
class KeyboardInputHandler {
public:
  KeyboardInputHandler();
  ~KeyboardInputHandler();

  void set_key_bindings(const DosKeyBindings &bindings);
  const DosKeyBindings &key_bindings() const;

  // Poll current keyboard state and update InputState
  // Returns true if the window should continue running (false on quit request)
  bool poll_events(InputState &input);

  // Disable copying
  KeyboardInputHandler(const KeyboardInputHandler &) = delete;
  KeyboardInputHandler &operator=(const KeyboardInputHandler &) = delete;

private:
  struct Impl;
  Impl *impl_{};
};

} // namespace comic2
