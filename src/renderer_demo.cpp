#include <chrono>
#include <iostream>
#include <thread>

#include "comic2/renderer.hpp"
#include "comic2/resource_formats.hpp"
#include "comic2/resource_loader.hpp"

#ifdef COMIC2_USE_SDL2
#if defined(_WIN32)
#define SDL_MAIN_HANDLED
#endif
#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#else
#error "SDL2 headers not found"
#endif
#endif

int main() {
  try {
    std::cout << "Captain Comic II Reimplementation - Rendering Demo\n";
    std::cout << "================================================\n\n";

    // Create a test surface
    comic2::EgaPlanarSurface surface(320, 200);

    // Clear to black
    surface.clear(0);

    // Draw a simple test pattern - create a gradient across planes
    for (std::size_t y = 0; y < 200; ++y) {
      for (std::size_t x_byte = 0; x_byte < 40; ++x_byte) {
        // Create a pattern using different plane combinations
        surface.set_plane_byte(0, x_byte, y, (x_byte + y) & 0xAA);
        surface.set_plane_byte(1, x_byte, y, (x_byte * 2) & 0x55);
        surface.set_plane_byte(2, x_byte, y, (y * 2) & 0xAA);
        surface.set_plane_byte(3, x_byte, y, (x_byte + y * 3) & 0x55);
      }
    }

    // Create presenter
#ifdef COMIC2_USE_SDL2
    std::unique_ptr<comic2::IFramePresenter> presenter =
        std::make_unique<comic2::Sdl2FramePresenter>(640, 400);
    std::cout << "Using SDL2 presenter (visual output enabled)\n\n";

    // Handle SDL events and display for a few seconds
    bool running = true;
    auto start_time = std::chrono::steady_clock::now();
    const auto duration = std::chrono::seconds(5);

    while (running) {
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN &&
                                       event.key.keysym.sym == SDLK_ESCAPE)) {
          running = false;
        }
      }

      auto elapsed = std::chrono::steady_clock::now() - start_time;
      if (elapsed >= duration) {
        running = false;
      }
    }

    std::cout << "Demo completed. Window closed.\n";
#else
    std::unique_ptr<comic2::IFramePresenter> presenter =
        std::make_unique<comic2::MemoryFramePresenter>();
    std::cout << "Using MemoryFramePresenter (headless mode)\n\n";

    presenter->present(surface);

    const auto *mem_presenter =
        dynamic_cast<const comic2::MemoryFramePresenter *>(presenter.get());
    if (mem_presenter && mem_presenter->has_frame()) {
      const auto &captured = mem_presenter->last_frame();
      std::cout << "Frame captured successfully!\n";
      std::cout << "Surface dimensions: " << captured.width_pixels() << "x"
                << captured.height_rows() << "\n";
      std::cout << "Row stride: " << captured.row_stride_bytes() << " bytes\n";
      std::cout << "Plane size: " << captured.plane_size_bytes() << " bytes\n";

      // Verify some pixel data
      std::cout
          << "\nSample pixel values (color index at various positions):\n";
      for (std::size_t y : {0, 99, 199}) {
        std::cout << "  Row " << y << ": ";
        for (std::size_t x : {0, 159, 319}) {
          std::uint8_t color_index = 0;
          for (std::size_t plane = 0; plane < 4; ++plane) {
            const auto byte_x = x / 8;
            const auto bit_x = 7 - (x % 8);
            const auto plane_data = captured.plane(plane).data();
            const auto byte_value = plane_data[y * 40 + byte_x];
            if (byte_value & (1 << bit_x)) {
              color_index |= (1 << plane);
            }
          }
          std::cout << "x=" << x << " color=" << static_cast<int>(color_index)
                    << "  ";
        }
        std::cout << "\n";
      }
    }
#endif

    std::cout << "\nDemo finished successfully!\n";
    return 0;

  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
  }
}
