cmake_minimum_required(VERSION 3.20)
project(comic2_reimpl LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure consistent runtime library on Windows
if(MSVC)
    # Use static runtime to match vcpkg's x64-windows-static triplet
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# SDL2 support for visual presentation is always enabled.
if(WIN32)
    # Windows: Use CONFIG mode for vcpkg compatibility
    find_package(SDL2 CONFIG REQUIRED)
    if(TARGET SDL2::SDL2)
        set(SDL2_TARGET SDL2::SDL2)
    elseif(TARGET SDL2::SDL2-static)
        set(SDL2_TARGET SDL2::SDL2-static)
    else()
        message(FATAL_ERROR "SDL2 is required but neither SDL2::SDL2 nor SDL2::SDL2-static target found")
    endif()
else()
    # Unix-like: Use CMake's find_package (works with Homebrew and system installs)
    find_package(SDL2 REQUIRED)
    if(TARGET SDL2::SDL2)
        set(SDL2_TARGET SDL2::SDL2)
    else()
        message(FATAL_ERROR "SDL2 is required but SDL2::SDL2 target not found")
    endif()
endif()

message(STATUS "SDL2 found - enabling visual presenter (target: ${SDL2_TARGET})")
add_compile_definitions(COMIC2_USE_SDL2)

add_library(comic2_core
    src/audio.cpp
    src/bootstrap.cpp
    src/default_handlers.cpp
    src/dispatcher.cpp
    src/entity_physics.cpp
    src/entity_runtime.cpp
    src/game_state.cpp
    src/input_handler.cpp
    src/player_controller.cpp
    src/projectiles.cpp
    src/persistence.cpp
    src/renderer.cpp
    src/ui.cpp
    src/oracle.cpp
    src/renderer_validation.cpp
    src/room_loader.cpp
    src/resource_loader.cpp
    src/tile_collision.cpp
)

target_include_directories(comic2_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(comic2_core PUBLIC ${SDL2_TARGET})

add_executable(comic2_reimpl
    src/main.cpp
)

target_link_libraries(comic2_reimpl
    PRIVATE
        comic2_core
)

# Renderer demo executable with SDL2 support
add_executable(comic2_renderer_demo src/renderer_demo.cpp)
if(WIN32 AND TARGET SDL2::SDL2main)
    target_link_libraries(comic2_renderer_demo PRIVATE SDL2::SDL2main comic2_core)
elseif(WIN32 AND TARGET SDL2::SDL2main-static)
    target_link_libraries(comic2_renderer_demo PRIVATE SDL2::SDL2main-static comic2_core)
else()
    target_link_libraries(comic2_renderer_demo PRIVATE comic2_core)
endif()

add_executable(comic2_unit_tests
    tests/audio_backend_tests.cpp
    tests/bootstrap_wiring_tests.cpp
    tests/dispatcher_tests.cpp
    tests/entity_physics_tests.cpp
    tests/entity_runtime_tests.cpp
    tests/integration_gates_tests.cpp
    tests/oracle_tests.cpp
    tests/end_to_end_tests.cpp
    tests/player_controller_tests.cpp
    tests/renderer_tests.cpp
    tests/persistence_tests.cpp
    tests/renderer_validation_tests.cpp
    tests/subsystem_scaffold_tests.cpp
    tests/ui_tests.cpp
    tests/tile_collision_tests.cpp
    tests/unit_tests_main.cpp
)

target_link_libraries(comic2_unit_tests
    PRIVATE
        comic2_core
)

# No need to link SDL2 again - inherited from comic2_core via PUBLIC

if(BUILD_TESTING)
    add_test(NAME comic2_unit_tests COMMAND comic2_unit_tests)
endif()
