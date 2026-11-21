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
  case 0x2A:
    return SDL_SCANCODE_LSHIFT;
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
  case 0x36:
    return SDL_SCANCODE_RSHIFT;
  case 0x38:
    return SDL_SCANCODE_LALT;
  case 0x39:
    return SDL_SCANCODE_SPACE;
  case 0x48:
    return SDL_SCANCODE_UP;
  case 0x4B:
    return SDL_SCANCODE_LEFT;
  case 0x4D:
    return SDL_SCANCODE_RIGHT;
  case 0x50:
    return SDL_SCANCODE_DOWN;
  case 0x52:
    return SDL_SCANCODE_INSERT;
  default:
    return SDL_SCANCODE_UNKNOWN;
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
        if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
          impl_->quit_requested = true;
        } else if (custom_left != SDL_SCANCODE_UNKNOWN &&
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

  if (const char *scancode_env = std::getenv("COMIC2_INPUT_SCANCODE");
      scancode_env != nullptr && *scancode_env != '\0') {
    const auto scancode =
        static_cast<std::uint8_t>(std::strtoul(scancode_env, nullptr, 0));
    apply_dos_scancode_to_input(input, scancode, impl_->bindings);
  }

  if (read_env_bool("COMIC2_INPUT_QUIT")) {
    impl_->quit_requested = true;
  }

  return !impl_->quit_requested;
}

} // namespace comic2
