#include "comic2/input_handler.hpp"

#include <cstdlib>
#include <stdexcept>

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

    // Process all pending events
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        impl_->quit_requested = true;
        break;

      case SDL_KEYDOWN:
        if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
          impl_->quit_requested = true;
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
        }
        break;

      case SDL_KEYUP:
        // Note: We could track key states separately for continuous input,
        // but for now we process only KEYDOWN events. If we need continuous
        // holding, we'd need to maintain state and query it here.
        break;

      default:
        break;
      }
    }

    const Uint8 *keys = SDL_GetKeyboardState(nullptr);
    input.left_pressed = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A];
    input.right_pressed = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
    input.down_pressed = keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S];
    input.jump_pressed = keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_UP] ||
                         keys[SDL_SCANCODE_W];
    input.fire_pressed = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL] ||
                         keys[SDL_SCANCODE_F] || keys[SDL_SCANCODE_J];
    input.mode_activate_pressed =
        keys[SDL_SCANCODE_RETURN] || keys[SDL_SCANCODE_E];

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

  if (read_env_bool("COMIC2_INPUT_QUIT")) {
    impl_->quit_requested = true;
  }

  return !impl_->quit_requested;
}

} // namespace comic2
