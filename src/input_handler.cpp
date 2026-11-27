#include "comic2/input_handler.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <stdexcept>
#include <string_view>

#ifdef COMIC2_USE_SDL2
#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#else
#error "SDL2 headers not found"
#endif
#endif

namespace comic2 {

namespace {

// Authentic 82-entry key name table from scratch_key_names_table (0x0035F in
// comic2_data.asm), indexed by (dos_scancode - 1) for scancodes 0x01..0x52.
constexpr std::array<std::string_view, 0x52> kDosScancodeNames = {
    "ESC",   "1",       "2",    "3",     "4",       "5",       "6",    "7",
    "8",     "9",       "0",    "-",     "=",       "BACK SP", "TAB",  "Q",
    "W",     "E",       "R",    "T",     "Y",       "U",       "I",    "O",
    "P",     "[",       "]",    "ENTER", "CTRL",    "A",       "S",    "D",
    "F",     "G",       "H",    "J",     "K",       "L",       ";",    "'",
    "`",     "L SHIFT", "\\",   "Z",     "X",       "C",       "V",    "B",
    "N",     "M",       ",",    ".",     "/",       "R SHIFT", "*",    "ALT",
    "SPACE", "CAPS",    "F1",   "F2",    "F3",      "F4",      "F5",   "F6",
    "F7",    "F8",      "F9",   "F10",   "NUMLOCK", "SCROLL",  "HOME", "UP",
    "PGUP",  "- KEY",   "LEFT", "5 KEY", "RIGHT",   "+ KEY",   "END",  "DOWN",
    "PGDN",  "INS",
};

#ifdef COMIC2_USE_SDL2
SDL_Scancode dos_to_sdl_scancode(std::uint8_t dos_scancode) {
  switch (dos_scancode) {
  case 0x01:
    return SDL_SCANCODE_ESCAPE;
  case 0x02:
    return SDL_SCANCODE_1;
  case 0x03:
    return SDL_SCANCODE_2;
  case 0x04:
    return SDL_SCANCODE_3;
  case 0x05:
    return SDL_SCANCODE_4;
  case 0x06:
    return SDL_SCANCODE_5;
  case 0x07:
    return SDL_SCANCODE_6;
  case 0x08:
    return SDL_SCANCODE_7;
  case 0x09:
    return SDL_SCANCODE_8;
  case 0x0A:
    return SDL_SCANCODE_9;
  case 0x0B:
    return SDL_SCANCODE_0;
  case 0x0C:
    return SDL_SCANCODE_MINUS;
  case 0x0D:
    return SDL_SCANCODE_EQUALS;
  case 0x0E:
    return SDL_SCANCODE_BACKSPACE;
  case 0x0F:
    return SDL_SCANCODE_TAB;
  case 0x10:
    return SDL_SCANCODE_Q;
  case 0x11:
    return SDL_SCANCODE_W;
  case 0x12:
    return SDL_SCANCODE_E;
  case 0x13:
    return SDL_SCANCODE_R;
  case 0x14:
    return SDL_SCANCODE_T;
  case 0x15:
    return SDL_SCANCODE_Y;
  case 0x16:
    return SDL_SCANCODE_U;
  case 0x17:
    return SDL_SCANCODE_I;
  case 0x18:
    return SDL_SCANCODE_O;
  case 0x19:
    return SDL_SCANCODE_P;
  case 0x1A:
    return SDL_SCANCODE_LEFTBRACKET;
  case 0x1B:
    return SDL_SCANCODE_RIGHTBRACKET;
  case 0x1C:
    return SDL_SCANCODE_RETURN;
  case 0x1D:
    return SDL_SCANCODE_LCTRL;
  case 0x1E:
    return SDL_SCANCODE_A;
  case 0x1F:
    return SDL_SCANCODE_S;
  case 0x20:
    return SDL_SCANCODE_D;
  case 0x21:
    return SDL_SCANCODE_F;
  case 0x22:
    return SDL_SCANCODE_G;
  case 0x23:
    return SDL_SCANCODE_H;
  case 0x24:
    return SDL_SCANCODE_J;
  case 0x25:
    return SDL_SCANCODE_K;
  case 0x26:
    return SDL_SCANCODE_L;
  case 0x27:
    return SDL_SCANCODE_SEMICOLON;
  case 0x28:
    return SDL_SCANCODE_APOSTROPHE;
  case 0x29:
    return SDL_SCANCODE_GRAVE;
  case 0x2A:
    return SDL_SCANCODE_LSHIFT;
  case 0x2B:
    return SDL_SCANCODE_BACKSLASH;
  case 0x2C:
    return SDL_SCANCODE_Z;
  case 0x2D:
    return SDL_SCANCODE_X;
  case 0x2E:
    return SDL_SCANCODE_C;
  case 0x2F:
    return SDL_SCANCODE_V;
  case 0x30:
    return SDL_SCANCODE_B;
  case 0x31:
    return SDL_SCANCODE_N;
  case 0x32:
    return SDL_SCANCODE_M;
  case 0x33:
    return SDL_SCANCODE_COMMA;
  case 0x34:
    return SDL_SCANCODE_PERIOD;
  case 0x35:
    return SDL_SCANCODE_SLASH;
  case 0x36:
    return SDL_SCANCODE_RSHIFT;
  case 0x37:
    return SDL_SCANCODE_KP_MULTIPLY;
  case 0x38:
    return SDL_SCANCODE_LALT;
  case 0x39:
    return SDL_SCANCODE_SPACE;
  case 0x3A:
    return SDL_SCANCODE_CAPSLOCK;
  case 0x3B:
    return SDL_SCANCODE_F1;
  case 0x3C:
    return SDL_SCANCODE_F2;
  case 0x3D:
    return SDL_SCANCODE_F3;
  case 0x3E:
    return SDL_SCANCODE_F4;
  case 0x3F:
    return SDL_SCANCODE_F5;
  case 0x40:
    return SDL_SCANCODE_F6;
  case 0x41:
    return SDL_SCANCODE_F7;
  case 0x42:
    return SDL_SCANCODE_F8;
  case 0x43:
    return SDL_SCANCODE_F9;
  case 0x44:
    return SDL_SCANCODE_F10;
  case 0x45:
    return SDL_SCANCODE_NUMLOCKCLEAR;
  case 0x46:
    return SDL_SCANCODE_SCROLLLOCK;
  case 0x47:
    return SDL_SCANCODE_HOME;
  case 0x48:
    return SDL_SCANCODE_UP;
  case 0x49:
    return SDL_SCANCODE_PAGEUP;
  case 0x4A:
    return SDL_SCANCODE_KP_MINUS;
  case 0x4B:
    return SDL_SCANCODE_LEFT;
  case 0x4C:
    return SDL_SCANCODE_KP_5;
  case 0x4D:
    return SDL_SCANCODE_RIGHT;
  case 0x4E:
    return SDL_SCANCODE_KP_PLUS;
  case 0x4F:
    return SDL_SCANCODE_END;
  case 0x50:
    return SDL_SCANCODE_DOWN;
  case 0x51:
    return SDL_SCANCODE_PAGEDOWN;
  case 0x52:
    return SDL_SCANCODE_INSERT;
  default:
    return SDL_SCANCODE_UNKNOWN;
  }
}

std::uint8_t sdl_to_dos_scancode(SDL_Scancode sc) {
  for (std::uint8_t dos = 0x01U; dos <= 0x52U; ++dos) {
    if (dos_to_sdl_scancode(dos) == sc) {
      return dos;
    }
  }
  switch (sc) {
  case SDL_SCANCODE_KP_ENTER:
    return 0x1CU;
  case SDL_SCANCODE_RCTRL:
    return 0x1DU;
  case SDL_SCANCODE_RALT:
    return 0x38U;
  case SDL_SCANCODE_KP_7:
    return 0x47U;
  case SDL_SCANCODE_KP_8:
    return 0x48U;
  case SDL_SCANCODE_KP_9:
    return 0x49U;
  case SDL_SCANCODE_KP_4:
    return 0x4BU;
  case SDL_SCANCODE_KP_6:
    return 0x4DU;
  case SDL_SCANCODE_KP_1:
    return 0x4FU;
  case SDL_SCANCODE_KP_2:
    return 0x50U;
  case SDL_SCANCODE_KP_3:
    return 0x51U;
  case SDL_SCANCODE_KP_0:
    return 0x52U;
  default:
    return 0U;
  }
}
#endif

} // namespace

std::string_view dos_scancode_name(std::uint8_t dos_scancode) {
  if (dos_scancode >= 0x01U && dos_scancode <= 0x52U) {
    return kDosScancodeNames[static_cast<std::size_t>(dos_scancode - 1U)];
  }
  return "UNKNOWN";
}

bool is_valid_unique_dos_key_binding(std::uint8_t dos_scancode,
                                     const DosKeyBindings &partial_bindings,
                                     std::size_t current_step) {
  // Matches input_read_mapped_key (1000:054b):
  // dec bx; jz reject (rejects 0x01 ESC and 0x00); cmp bx, 0x52; jg reject
  if (dos_scancode <= 0x01U || dos_scancode > 0x52U) {
    return false;
  }
  const std::array<std::uint8_t, 6> assigned = {
      partial_bindings.left, partial_bindings.right, partial_bindings.up,
      partial_bindings.down, partial_bindings.jump,  partial_bindings.fire,
  };
  const std::size_t limit =
      current_step < assigned.size() ? current_step : assigned.size();
  return !std::any_of(assigned.begin(),
                      assigned.begin() + static_cast<std::ptrdiff_t>(limit),
                      [dos_scancode](std::uint8_t bound) {
                        return bound != 0U && bound == dos_scancode;
                      });
}

void apply_dos_scancode_to_input(InputState &input, std::uint8_t dos_scancode,
                                 const DosKeyBindings &bindings) {
  if (dos_scancode == 0U) {
    return;
  }
  if (dos_scancode == bindings.left) {
    input.left_pressed = true;
  }
  if (dos_scancode == bindings.right) {
    input.right_pressed = true;
  }
  if (dos_scancode == bindings.up) {
    input.mode_activate_pressed = true;
  }
  if (dos_scancode == bindings.down) {
    input.down_pressed = true;
  }
  if (dos_scancode == bindings.jump) {
    input.jump_pressed = true;
  }
  if (dos_scancode == bindings.fire) {
    input.fire_pressed = true;
  }
}

// Helper function to read boolean environment variables
// This mirrors read_bootstrap_bool_env behavior
static bool read_env_bool(const char *name) {
  const char *value = std::getenv(name);
  return value != nullptr && (*value == '1' || *value == 't' || *value == 'T' ||
                              *value == 'y' || *value == 'Y');
}

struct KeyboardInputHandler::Impl {
#ifdef COMIC2_USE_SDL2
  bool sdl_available{false};
  bool sdl_initialized{false};
#endif
  bool quit_requested{false};
  DosKeyBindings bindings{};

#ifdef COMIC2_USE_SDL2
  Impl() {
    // Allow deterministic/headless runs to avoid touching SDL at all.
    if (read_env_bool("COMIC2_FORCE_ENV_INPUT")) {
      return;
    }

    // Ensure the events subsystem is initialized for SDL_PollEvent.
    if (SDL_WasInit(SDL_INIT_EVENTS) == 0) {
      if (SDL_InitSubSystem(SDL_INIT_EVENTS) == 0) {
        sdl_available = true;
        sdl_initialized = true;
      }
      return;
    }

    sdl_available = true;
  }

  ~Impl() {
    if (sdl_initialized) {
      SDL_QuitSubSystem(SDL_INIT_EVENTS);
    }
  }
#else
  Impl() {}
  ~Impl() {}
#endif
};

KeyboardInputHandler::KeyboardInputHandler() : impl_(new Impl()) {}

KeyboardInputHandler::~KeyboardInputHandler() { delete impl_; }

void KeyboardInputHandler::set_key_bindings(const DosKeyBindings &bindings) {
  impl_->bindings = bindings;
}

const DosKeyBindings &KeyboardInputHandler::key_bindings() const {
  return impl_->bindings;
}

bool KeyboardInputHandler::poll_events(InputState &input) {
  impl_->quit_requested = false;

#ifdef COMIC2_USE_SDL2
  // Check if we should force environment variable mode (for testing)
  bool force_env_input = read_env_bool("COMIC2_FORCE_ENV_INPUT");
  if (!force_env_input && impl_->sdl_available) {
    // Use SDL2 keyboard input
    SDL_Event event;

    // Reset input state
    input.jump_pressed = false;
    input.left_pressed = false;
    input.right_pressed = false;
    input.down_pressed = false;
    input.fire_pressed = false;
    input.mode_activate_pressed = false;
    input.pause_pressed = false;
    input.any_key_pressed = false;
    input.sdl_keyboard_active = true;
    input.last_dos_scancode = 0;

    const SDL_Scancode custom_left = dos_to_sdl_scancode(impl_->bindings.left);
    const SDL_Scancode custom_right =
        dos_to_sdl_scancode(impl_->bindings.right);
    const SDL_Scancode custom_up = dos_to_sdl_scancode(impl_->bindings.up);
    const SDL_Scancode custom_down = dos_to_sdl_scancode(impl_->bindings.down);
    const SDL_Scancode custom_jump = dos_to_sdl_scancode(impl_->bindings.jump);
    const SDL_Scancode custom_fire = dos_to_sdl_scancode(impl_->bindings.fire);

    // Process all pending events
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        impl_->quit_requested = true;
        break;

      case SDL_KEYDOWN:
        if (event.key.repeat == 0) {
          input.any_key_pressed = true;
          const std::uint8_t dos_sc =
              sdl_to_dos_scancode(event.key.keysym.scancode);
          if (dos_sc != 0U) {
            input.last_dos_scancode = dos_sc;
          }
        }
        if (custom_left != SDL_SCANCODE_UNKNOWN &&
            event.key.keysym.scancode == custom_left) {
          input.left_pressed = true;
        } else if (custom_right != SDL_SCANCODE_UNKNOWN &&
                   event.key.keysym.scancode == custom_right) {
          input.right_pressed = true;
        } else if (custom_jump != SDL_SCANCODE_UNKNOWN &&
                   event.key.keysym.scancode == custom_jump) {
          input.jump_pressed = true;
        } else if (custom_down != SDL_SCANCODE_UNKNOWN &&
                   event.key.keysym.scancode == custom_down) {
          input.down_pressed = true;
        } else if (custom_fire != SDL_SCANCODE_UNKNOWN &&
                   event.key.keysym.scancode == custom_fire) {
          input.fire_pressed = true;
        } else if (custom_up != SDL_SCANCODE_UNKNOWN &&
                   event.key.keysym.scancode == custom_up) {
          input.mode_activate_pressed = true;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_LEFT ||
                   event.key.keysym.sym == SDLK_a) {
          input.left_pressed = true;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT ||
                   event.key.keysym.sym == SDLK_d) {
          input.right_pressed = true;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_SPACE ||
                   event.key.keysym.sym == SDLK_UP ||
                   event.key.keysym.scancode == SDL_SCANCODE_W) {
          input.jump_pressed = true;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_DOWN ||
                   event.key.keysym.sym == SDLK_s) {
          input.down_pressed = true;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_LCTRL ||
                   event.key.keysym.scancode == SDL_SCANCODE_RCTRL ||
                   event.key.keysym.sym == SDLK_f ||
                   event.key.keysym.sym == SDLK_j) {
          input.fire_pressed = true;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_RETURN ||
                   event.key.keysym.sym == SDLK_e) {
          input.mode_activate_pressed = true;
        } else if (event.key.keysym.scancode == SDL_SCANCODE_T ||
                   event.key.keysym.sym == SDLK_t) {
          input.teleport_pressed = true;
        }
        break;

      case SDL_KEYUP:
        break;

      default:
        break;
      }
    }

    const Uint8 *keys = SDL_GetKeyboardState(nullptr);
    auto is_bound_to_other = [&](SDL_Scancode sc, SDL_Scancode own) {
      if (sc == own) {
        return false;
      }
      return sc == custom_left || sc == custom_right || sc == custom_up ||
             sc == custom_down || sc == custom_jump || sc == custom_fire;
    };
    auto fallback_active = [&](SDL_Scancode sc, SDL_Scancode own) {
      return !is_bound_to_other(sc, own) && keys[sc] != 0;
    };

    input.left_pressed =
        (custom_left != SDL_SCANCODE_UNKNOWN && keys[custom_left]) ||
        fallback_active(SDL_SCANCODE_LEFT, custom_left) ||
        fallback_active(SDL_SCANCODE_A, custom_left);
    input.right_pressed =
        (custom_right != SDL_SCANCODE_UNKNOWN && keys[custom_right]) ||
        fallback_active(SDL_SCANCODE_RIGHT, custom_right) ||
        fallback_active(SDL_SCANCODE_D, custom_right);
    input.down_pressed =
        (custom_down != SDL_SCANCODE_UNKNOWN && keys[custom_down]) ||
        fallback_active(SDL_SCANCODE_DOWN, custom_down) ||
        fallback_active(SDL_SCANCODE_S, custom_down);
    input.jump_pressed =
        (custom_jump != SDL_SCANCODE_UNKNOWN && keys[custom_jump]) ||
        fallback_active(SDL_SCANCODE_SPACE, custom_jump) ||
        fallback_active(SDL_SCANCODE_UP, custom_jump) ||
        fallback_active(SDL_SCANCODE_W, custom_jump);
    input.fire_pressed =
        (custom_fire != SDL_SCANCODE_UNKNOWN && keys[custom_fire]) ||
        fallback_active(SDL_SCANCODE_LCTRL, custom_fire) ||
        fallback_active(SDL_SCANCODE_RCTRL, custom_fire) ||
        fallback_active(SDL_SCANCODE_F, custom_fire) ||
        fallback_active(SDL_SCANCODE_J, custom_fire);
    input.mode_activate_pressed =
        (custom_up != SDL_SCANCODE_UNKNOWN && keys[custom_up]) ||
        fallback_active(SDL_SCANCODE_RETURN, custom_up) ||
        fallback_active(SDL_SCANCODE_E, custom_up);
    input.teleport_pressed = keys[SDL_SCANCODE_T];

    return !impl_->quit_requested;
  }
#endif

  // Fallback: use environment variables when SDL2 is not available
  input.jump_pressed = read_env_bool("COMIC2_INPUT_JUMP");
  input.left_pressed = read_env_bool("COMIC2_INPUT_LEFT");
  input.right_pressed = read_env_bool("COMIC2_INPUT_RIGHT");
  input.down_pressed = read_env_bool("COMIC2_INPUT_DOWN");
  input.fire_pressed = read_env_bool("COMIC2_INPUT_FIRE");
  input.mode_activate_pressed = read_env_bool("COMIC2_INPUT_MODE_ACTIVATE");
  input.teleport_pressed = read_env_bool("COMIC2_INPUT_TELEPORT");
  input.any_key_pressed = false;
  input.sdl_keyboard_active = false;
  input.last_dos_scancode = 0;

  if (const char *scancode_env = std::getenv("COMIC2_INPUT_SCANCODE");
      scancode_env != nullptr && *scancode_env != '\0') {
    const auto scancode =
        static_cast<std::uint8_t>(std::strtoul(scancode_env, nullptr, 0));
    input.last_dos_scancode = scancode;
    input.any_key_pressed = (scancode != 0U);
    apply_dos_scancode_to_input(input, scancode, impl_->bindings);
  }

  if (read_env_bool("COMIC2_INPUT_QUIT")) {
    impl_->quit_requested = true;
  }

  return !impl_->quit_requested;
}

} // namespace comic2
