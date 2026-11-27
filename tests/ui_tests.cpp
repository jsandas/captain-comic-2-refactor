#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include "comic2/default_handlers.hpp"
#include "comic2/dispatcher.hpp"
#include "comic2/game_state.hpp"
#include "comic2/input_handler.hpp"
#include "comic2/persistence.hpp"
#include "comic2/renderer.hpp"
#include "comic2/resource_loader.hpp"
#include "comic2/room_loader.hpp"
#include "comic2/ui.hpp"

namespace {

void expect(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

std::filesystem::path find_reference_original_dir() {
  const std::vector<std::filesystem::path> candidates = {
      "reference/original",
      "../reference/original",
      "../../reference/original",
  };
  const auto it = std::find_if(
      candidates.begin(), candidates.end(), [](const std::filesystem::path &p) {
        return std::filesystem::exists(p / "FRPAK.002");
      });
  if (it != candidates.end()) {
    return *it;
  }
  return {};
}

void test_bcd_counter_helpers() {
  using comic2::bcd_decrement;
  using comic2::bcd_increment;

  expect(bcd_increment(0x00) == 0x01, "BCD increment should advance 00->01");
  expect(bcd_increment(0x09) == 0x10, "BCD increment should carry 09->10");
  expect(bcd_increment(0x99) == 0x99, "BCD increment should clamp at 99");
  expect(bcd_decrement(0x10) == 0x09, "BCD decrement should carry 10->09");
  expect(bcd_decrement(0x00) == 0x00, "BCD decrement should clamp at 00");
}

void test_hud_renders_counters_and_icons() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.player.score = 0x1234;
  state.player.gems = 0x07;
  state.player.lives = 0x03;
  state.ui.active_mode_mask = 0x01;
  state.ui.inventory_mask = 0x03;

  comic2::EgaPlanarSurface frame(320, 200);
  frame.clear(0x00);
  comic2::hud_render_overlay(frame, state);

  bool saw_pixels = false;
  for (std::size_t y = 0; y < frame.height_rows(); ++y) {
    for (std::size_t x = 0; x < frame.width_pixels(); ++x) {
      if (frame.get_plane_byte(0, x / 8, y) != 0x00) {
        saw_pixels = true;
        break;
      }
    }
    if (saw_pixels) {
      break;
    }
  }

  expect(saw_pixels, "HUD overlay should draw visible pixels");
}

void test_modal_prompt_overlay_is_rendered() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.ui.modal_active = true;
  state.ui.modal_prompt = "Continue?";
  state.ui.modal_game_over = false;

  comic2::EgaPlanarSurface frame(320, 200);
  frame.clear(0x00);
  comic2::ui_render_modal_prompt(frame, state);

  expect(
      frame.get_plane_byte(0, 8, 80) != 0x00,
      "modal prompt overlay should draw visible pixels inside the prompt box");
}

void test_hud_renders_mode_and_inventory_labels() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.ui.active_mode_mask = 0x01;
  state.ui.inventory_mask = 0x03;

  comic2::EgaPlanarSurface frame(320, 200);
  frame.clear(0x00);
  comic2::hud_render_overlay(frame, state);

  bool saw_mode_label = false;
  bool saw_inventory_label = false;
  for (std::size_t y = 184; y < 192; ++y) {
    for (std::size_t x = 0; x < 320; ++x) {
      const auto byte = frame.get_plane_byte(0, x / 8, y);
      if ((x >= 144 && x < 168) && byte != 0x00) {
        saw_mode_label = true;
      }
      if ((x >= 168 && x < 200) && byte != 0x00) {
        saw_inventory_label = true;
      }
    }
  }

  expect(saw_mode_label, "HUD should draw a visible mode label strip");
  expect(saw_inventory_label,
         "HUD should draw a visible inventory label strip");
}

void test_hud_renders_mode_inventory_bits() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.progression.mode_inventory_mask = 0x01;
  state.ui.active_mode_mask = 0x00;
  state.ui.inventory_mask = 0x00;

  comic2::EgaPlanarSurface frame(320, 200);
  frame.clear(0x00);
  comic2::hud_render_overlay(frame, state);

  bool saw_mode_inventory_pixel = false;
  for (std::size_t y = 184; y < 192; ++y) {
    for (std::size_t x = 160; x < 176; ++x) {
      if (frame.get_plane_byte(0, x / 8, y) != 0x00) {
        saw_mode_inventory_pixel = true;
        break;
      }
    }
    if (saw_mode_inventory_pixel) {
      break;
    }
  }

  expect(saw_mode_inventory_pixel,
         "HUD should render a visible icon when a mode is collected");
}

void test_menu_state_machine_and_option_navigation() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.ui.menu_state = comic2::MenuState::Pause;
  state.ui.option_labels = {"Resume", "Save", "Quit"};
  state.ui.selected_option_index = 0;

  comic2::ui_select_next_valid_option(state);
  expect(state.ui.selected_option_index == 1,
         "next valid option should advance the selection");

  comic2::ui_option_list_input_loop(state,
                                    comic2::InputState{.pause_pressed = true});
  expect(state.ui.menu_state == comic2::MenuState::Pause,
         "pause input should keep the menu active");
}

void test_frpak002_title_screen_decode_and_render() {
  const auto dir = find_reference_original_dir();
  if (dir.empty()) {
    return;
  }

  const auto title = comic2::load_frpak_fullscreen(dir / "FRPAK.002");
  expect(title.has_value(), "FRPAK.002 must decode successfully");
  expect(title->width_bytes == 40,
         "Title image must have width_bytes == 40 (320 pixels)");
  expect(title->height_rows == 200, "Title image must have height_rows == 200");
  expect(title->row_span_bytes == 8000,
         "Title image row span must be 8000 bytes");
  for (std::size_t p = 0; p < 4; ++p) {
    expect(title->planes[p].size() == 8000, "Each plane must have 8000 bytes");
  }

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.title_screen = *title;

  comic2::EgaPlanarSurface surface(320, 200);
  surface.clear(0x00);
  const bool rendered = comic2::render_title_screen(surface, state);
  expect(rendered,
         "render_title_screen should return true with authentic title asset");

  bool saw_pixels = false;
  for (std::size_t y = 0; y < 200; ++y) {
    for (std::size_t x = 0; x < 320; ++x) {
      if (surface.get_plane_byte(0, x / 8, y) != 0x00 ||
          surface.get_plane_byte(1, x / 8, y) != 0x00 ||
          surface.get_plane_byte(2, x / 8, y) != 0x00 ||
          surface.get_plane_byte(3, x / 8, y) != 0x00) {
        saw_pixels = true;
        break;
      }
    }
    if (saw_pixels) {
      break;
    }
  }
  expect(saw_pixels, "render_title_screen must blit visible pixels");

  // Fallback test
  state.title_screen.reset();
  surface.clear(0x00);
  const bool fallback_rendered = comic2::render_title_screen(surface, state);
  expect(!fallback_rendered,
         "render_title_screen should return false in fallback mode");
}

void test_frpak_intro_cutscenes_decode_and_progression() {
  const auto dir = find_reference_original_dir();
  if (dir.empty()) {
    return;
  }

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  for (int i = 3; i <= 6; ++i) {
    char name[16];
    std::snprintf(name, sizeof(name), "FRPAK.00%d", i);
    auto slide = comic2::load_frpak_fullscreen(dir / name);
    expect(slide.has_value(), "Each intro slide FRPAK.003..006 must decode");
    expect(slide->width_bytes == 40 && slide->height_rows == 200,
           "Intro slide must be 320x200");
    state.intro_screens.push_back(std::move(*slide));
  }
  expect(state.intro_screens.size() == 4,
         "Must have exactly 4 intro screens loaded");

  comic2::EgaPlanarSurface surface(320, 200);
  state.ui.cinematic_frame = 0;

  // Slide 1
  expect(!comic2::play_intro_cinematic(surface, state),
         "Slide 1 should not finish cinematic");
  expect(state.ui.cinematic_frame == 1, "Cinematic frame should advance to 1");

  // Slide 2
  expect(!comic2::play_intro_cinematic(surface, state),
         "Slide 2 should not finish cinematic");
  expect(state.ui.cinematic_frame == 2, "Cinematic frame should advance to 2");

  // Slide 3
  expect(!comic2::play_intro_cinematic(surface, state),
         "Slide 3 should not finish cinematic");
  expect(state.ui.cinematic_frame == 3, "Cinematic frame should advance to 3");

  // Slide 4 (final)
  expect(comic2::play_intro_cinematic(surface, state),
         "Slide 4 should finish cinematic");
  expect(state.ui.cinematic_frame == 4, "Cinematic frame should be 4");

  // Fallback test
  state.intro_screens.clear();
  state.ui.cinematic_frame = 0;
  expect(!comic2::play_intro_cinematic(surface, state),
         "Fallback slide 1 in progress");
  expect(!comic2::play_intro_cinematic(surface, state),
         "Fallback slide 2 in progress");
  expect(!comic2::play_intro_cinematic(surface, state),
         "Fallback slide 3 in progress");
  expect(comic2::play_intro_cinematic(surface, state),
         "Fallback slide 4 completes");
}

void test_frpak007_victory_screen_decode_and_render() {
  const auto dir = find_reference_original_dir();
  if (dir.empty()) {
    return;
  }

  const auto victory = comic2::load_frpak_fullscreen(dir / "FRPAK.007");
  expect(victory.has_value(), "FRPAK.007 must decode successfully");
  expect(victory->width_bytes == 40 && victory->height_rows == 200,
         "Victory screen must be 320x200");

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.victory_screen = *victory;
  state.progression.flags = 0;

  comic2::EgaPlanarSurface surface(320, 200);
  surface.clear(0x00);
  const bool done = comic2::event_finale_transition_sequence(surface, state);
  expect(done, "event_finale_transition_sequence must return true");
  expect((state.progression.flags & 0x80) != 0,
         "Victory flag 0x80 must be set in progression flags");
}

void test_frdata0_menu_panel_decode() {
  const auto dir = find_reference_original_dir();
  if (dir.empty()) {
    return;
  }

  const auto bytes = comic2::load_file_bytes(dir / "FRDATA.0");
  expect(bytes.has_value(), "FRDATA.0 must be loaded");
  const auto panel = comic2::decode_frdata0_panel(*bytes);
  expect(panel.has_value(), "FRDATA.0 must decode successfully");
  expect(panel->width_bytes == 26,
         "FRDATA.0 width_bytes must be 26 (208 pixels)");
  expect(panel->height_rows == 160, "FRDATA.0 height_rows must be 160");
  expect(panel->row_span_bytes == 4160, "FRDATA.0 row_span_bytes must be 4160");
  for (std::size_t p = 0; p < 4; ++p) {
    expect(panel->planes[p].size() == 4160, "Plane size must match 4160 bytes");
    const bool has_data =
        std::any_of(panel->planes[p].begin(), panel->planes[p].end(),
                    [](std::uint8_t b) { return b != 0; });
    expect(has_data, "Plane must contain non-zero graphical data");
  }

  // Malformed / overflowing tests
  std::vector<std::uint8_t> tiny = {0x10, 0x00};
  expect(!comic2::decode_frdata0_panel(tiny).has_value(),
         "Payload < 4 bytes must be rejected");

  std::vector<std::uint8_t> zero_dim = {0x00, 0x00, 0x10, 0x00};
  expect(!comic2::decode_frdata0_panel(zero_dim).has_value(),
         "Zero dimensions must be rejected");

  // Plane size overflow: width = 64000 pixels (8000 bytes), height = 10 rows ->
  // 80000 bytes > 65535
  std::vector<std::uint8_t> overflow(4, 0);
  overflow[0] = static_cast<std::uint8_t>(64000 & 0xFF);
  overflow[1] = static_cast<std::uint8_t>((64000 >> 8) & 0xFF);
  overflow[2] = 10;
  overflow[3] = 0;
  expect(!comic2::decode_frdata0_panel(overflow).has_value(),
         "Overflowing plane size must be rejected");
}

void test_frdata1_menu_buttons_decode() {
  const auto dir = find_reference_original_dir();
  if (dir.empty()) {
    return;
  }

  const auto bytes = comic2::load_file_bytes(dir / "FRDATA.1");
  expect(bytes.has_value(), "FRDATA.1 must be loaded");
  const auto buttons = comic2::decode_frdata1_sprites(*bytes);
  expect(buttons.has_value(), "FRDATA.1 must decode successfully");
  expect(buttons->size() == 2,
         "FRDATA.1 must contain exactly 2 masked button sprites");

  for (const auto &btn : *buttons) {
    expect(btn.width_pixels == 48, "Button sprite width must be 48 pixels");
    expect(btn.height_rows == 40, "Button sprite height must be 40 rows");
    expect(btn.mask_bytes.size() == 240,
           "Button sprite mask must be 240 bytes (6 * 40)");
    for (std::size_t p = 0; p < 4; ++p) {
      expect(btn.planes[p].size() == 240,
             "Each plane of button sprite must be 240 bytes");
    }
    const auto img = comic2::make_masked_sprite_image(btn);
    expect(img.width_bytes == 6,
           "make_masked_sprite_image width_bytes must be 6");
    expect(img.height_rows == 40,
           "make_masked_sprite_image height_rows must be 40");
  }
}

void test_ui_render_game_selection_panel_with_authentic_assets() {
  const auto dir = find_reference_original_dir();
  if (dir.empty()) {
    return;
  }

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  comic2::load_ui_assets(state, dir);
  expect(state.menu_panel.has_value(), "load_ui_assets must load menu_panel");
  expect(state.menu_buttons.size() == 2,
         "load_ui_assets must load menu_buttons");

  comic2::EgaPlanarSurface surface(320, 200);
  surface.clear(0x00);

  state.ui.selected_option_index = 2; // TIME
  comic2::ui_render_game_selection_panel(surface, state);

  // Check that pixels exist at button locations
  bool saw_button_pixel = false;
  for (std::size_t y = 24; y < 64; ++y) {
    for (std::size_t x = 24; x < 72; ++x) {
      if (surface.get_plane_byte(0, x / 8, y) != 0x00 ||
          surface.get_plane_byte(1, x / 8, y) != 0x00 ||
          surface.get_plane_byte(2, x / 8, y) != 0x00 ||
          surface.get_plane_byte(3, x / 8, y) != 0x00) {
        saw_button_pixel = true;
        break;
      }
    }
    if (saw_button_pixel) {
      break;
    }
  }
  expect(saw_button_pixel, "Button at (24, 24) must have visible pixels");
}

void test_load_ui_assets_helper() {
  const auto dir = find_reference_original_dir();
  if (dir.empty()) {
    return;
  }

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  comic2::load_ui_assets(state, dir);

  expect(state.title_screen.has_value(), "Title screen must be populated");
  expect(state.intro_screens.size() == 4, "4 intro slides must be populated");
  expect(state.victory_screen.has_value(), "Victory screen must be populated");
  expect(state.menu_panel.has_value(), "Menu panel must be populated");
  expect(state.menu_buttons.size() == 2, "2 menu buttons must be populated");

  // Re-calling with an empty or non-existent path must clear all UI assets
  comic2::load_ui_assets(state, "/nonexistent/path");
  expect(!state.title_screen.has_value(), "Title screen must be cleared");
  expect(state.intro_screens.empty(), "Intro screens must be cleared");
  expect(!state.victory_screen.has_value(), "Victory screen must be cleared");
  expect(!state.menu_panel.has_value(), "Menu panel must be cleared");
  expect(state.menu_buttons.empty(), "Menu buttons must be cleared");
}

void test_game_lifecycle_state_machine_transitions() {
  using comic2::GameLifecycleState;
  comic2::RuntimeState state = comic2::make_default_runtime_state();

  expect(comic2::get_game_lifecycle_state(state) == GameLifecycleState::Title,
         "Initial lifecycle state must be Title");

  comic2::set_game_lifecycle_state(state, GameLifecycleState::Menu);
  expect(comic2::get_game_lifecycle_state(state) == GameLifecycleState::Menu,
         "set_game_lifecycle_state must update lifecycle state");
  comic2::set_game_lifecycle_state(state, GameLifecycleState::Title);

  // Invalid transition from Title to Victory should be rejected
  expect(!comic2::can_transition_game_lifecycle(GameLifecycleState::Title,
                                                GameLifecycleState::Victory),
         "Title directly to Victory must be disallowed");
  expect(!comic2::transition_game_lifecycle(state, GameLifecycleState::Victory),
         "transition_game_lifecycle(Title -> Victory) must return false");
  expect(state.ui.lifecycle_state == GameLifecycleState::Title,
         "State must remain Title after failed transition");

  // 1. Title -> Intro
  expect(comic2::can_transition_game_lifecycle(GameLifecycleState::Title,
                                               GameLifecycleState::Intro),
         "Title to Intro must be allowed");
  expect(comic2::transition_game_lifecycle(state, GameLifecycleState::Intro),
         "Transition Title -> Intro must succeed");
  expect(state.ui.lifecycle_state == GameLifecycleState::Intro,
         "State must be Intro");

  // 2. Intro -> Menu
  expect(comic2::can_transition_game_lifecycle(GameLifecycleState::Intro,
                                               GameLifecycleState::Menu),
         "Intro to Menu must be allowed");
  expect(comic2::transition_game_lifecycle(state, GameLifecycleState::Menu),
         "Transition Intro -> Menu must succeed");
  expect(state.ui.lifecycle_state == GameLifecycleState::Menu,
         "State must be Menu");
  expect(state.ui.menu_state == comic2::MenuState::GameSelect,
         "Menu state must be GameSelect");

  // 3. Menu -> Gameplay
  expect(comic2::can_transition_game_lifecycle(GameLifecycleState::Menu,
                                               GameLifecycleState::Gameplay),
         "Menu to Gameplay must be allowed");
  expect(comic2::transition_game_lifecycle(state, GameLifecycleState::Gameplay),
         "Transition Menu -> Gameplay must succeed");
  expect(state.ui.lifecycle_state == GameLifecycleState::Gameplay,
         "State must be Gameplay");
  expect(state.ui.menu_state == comic2::MenuState::None,
         "Menu state must be None in Gameplay");

  // 4. Gameplay -> Save
  expect(comic2::can_transition_game_lifecycle(GameLifecycleState::Gameplay,
                                               GameLifecycleState::Save),
         "Gameplay to Save must be allowed");
  expect(comic2::transition_game_lifecycle(state, GameLifecycleState::Save),
         "Transition Gameplay -> Save must succeed");
  expect(state.ui.lifecycle_state == GameLifecycleState::Save,
         "State must be Save");
  expect(state.saved_snapshot_bytes.size() == comic2::kDosSavegameSize,
         "Save state entry must serialize authentic DOS snapshot");

  // 5. Save -> Victory
  expect(comic2::can_transition_game_lifecycle(GameLifecycleState::Save,
                                               GameLifecycleState::Victory),
         "Save to Victory must be allowed");
  expect(comic2::transition_game_lifecycle(state, GameLifecycleState::Victory),
         "Transition Save -> Victory must succeed");
  expect(state.ui.lifecycle_state == GameLifecycleState::Victory,
         "State must be Victory");
  expect((state.progression.flags & 0x80U) != 0,
         "Victory flag 0x80 must be set on Victory entry");

  // 6. Victory -> Menu
  expect(comic2::can_transition_game_lifecycle(GameLifecycleState::Victory,
                                               GameLifecycleState::Menu),
         "Victory to Menu must be allowed");
  expect(comic2::transition_game_lifecycle(state, GameLifecycleState::Menu),
         "Transition Victory -> Menu must succeed");
  expect(state.ui.lifecycle_state == GameLifecycleState::Menu,
         "State must be Menu");
}

void test_game_lifecycle_update_and_rendering() {
  using comic2::GameLifecycleState;
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  comic2::EgaPlanarSurface surface(320, 200);

  // Starts at Title
  comic2::render_game_lifecycle_frame(surface, state);
  expect(state.ui.lifecycle_state == GameLifecycleState::Title,
         "Should start in Title");

  // Title input triggers transition to Intro
  comic2::InputState input{};
  input.jump_pressed = true;
  comic2::update_game_lifecycle(state, input);
  expect(state.ui.lifecycle_state == GameLifecycleState::Intro,
         "Title input must advance to Intro");

  // Stepping through intro slides
  input = comic2::InputState{};
  state.ui.cinematic_frame = 4; // Complete intro
  comic2::update_game_lifecycle(state, input);
  expect(state.ui.lifecycle_state == GameLifecycleState::Menu,
         "Intro completion must advance to Menu");

  // Menu navigation and selection
  input.left_pressed = true;
  comic2::update_game_lifecycle(state, input);
  input = comic2::InputState{};
  input.fire_pressed = true;
  comic2::update_game_lifecycle(state, input);
  expect(state.ui.lifecycle_state == GameLifecycleState::Gameplay,
         "Menu confirmation must advance to Gameplay");

  // Gameplay render
  comic2::render_game_lifecycle_frame(surface, state);

  // Trigger Save from gameplay
  state.flags.room_event_triggered = true;
  comic2::update_game_lifecycle(state, comic2::InputState{});
  expect(state.ui.lifecycle_state == GameLifecycleState::Save,
         "Room event trigger must transition to Save");
  expect(!state.saved_snapshot_bytes.empty(),
         "Saved snapshot must be generated");

  // Save confirm to Victory
  state.progression.flags |= 0x80U;
  input = comic2::InputState{.fire_pressed = true};
  comic2::update_game_lifecycle(state, input);
  expect(state.ui.lifecycle_state == GameLifecycleState::Victory,
         "Save completion with victory flag must advance to Victory");

  // Victory render
  comic2::render_game_lifecycle_frame(surface, state);
}

void test_game_lifecycle_gameplay_to_save_via_dispatcher_tick() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  comic2::GameDispatcher dispatcher = comic2::make_default_game_dispatcher();
  state.ui.lifecycle_state = comic2::GameLifecycleState::Gameplay;

  // Place player overlapping a proximity room event trigger
  state.player.x = 100;
  state.player.y = 100;
  state.player.is_physics_active = false;
  state.mapped_objects.push_back(comic2::MappedObject12{
      .room_x = 0,
      .room_y = 0,
      .descriptor_ptr = 0x1234,
      .state_flags = 0x0001,
      .world_x = 90,
      .world_y = 90,
  });

  expect(!state.flags.room_event_triggered,
         "Room event trigger flag initially clear");
  expect(state.ui.pending_event_message.empty(),
         "Pending event message initially empty");

  // In normal tick flow, update_game_lifecycle executes
  // dispatcher->run_tick(state) where proximity triggers room_event_triggered,
  // which is queued to pending_event_message. Gameplay must transition to Save.
  comic2::update_game_lifecycle(state, comic2::InputState{}, &dispatcher);

  expect(state.ui.lifecycle_state == comic2::GameLifecycleState::Save,
         "Normal tick flow must transition Gameplay to Save on room event "
         "trigger");
  expect(!state.saved_snapshot_bytes.empty(),
         "Save state must produce a saved snapshot");
  expect(state.ui.modal_active, "Save state modal prompt should be active");
  expect(state.ui.modal_prompt == "Room Event Triggered",
         "Save state modal prompt should match room event");

  // Acknowledging Save prompt transitions back to Gameplay
  const comic2::InputState confirm_input{.fire_pressed = true};
  comic2::update_game_lifecycle(state, confirm_input, &dispatcher);

  expect(state.ui.lifecycle_state == comic2::GameLifecycleState::Gameplay,
         "Acknowledging Save prompt must transition back to Gameplay");
  expect(!state.ui.modal_active,
         "Modal prompt should be deactivated upon returning to Gameplay");
  expect(state.ui.modal_prompt.empty(),
         "Modal prompt should be cleared upon returning to Gameplay");
  expect(!state.transition_state.player_frozen,
         "Player should be unfrozen upon returning to Gameplay");
}

void test_game_lifecycle_gameplay_to_save_via_anchor_trigger() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  comic2::GameDispatcher dispatcher = comic2::make_default_game_dispatcher();
  state.ui.lifecycle_state = comic2::GameLifecycleState::Gameplay;

  state.player.x = 100;
  state.player.y = 100;
  state.player.is_physics_active = false;
  state.room_event_anchor.active = true;
  state.room_event_anchor.x = 96;
  state.room_event_anchor.y = 100;

  comic2::update_game_lifecycle(state, comic2::InputState{}, &dispatcher);

  expect(
      state.ui.lifecycle_state == comic2::GameLifecycleState::Save,
      "Normal tick flow must transition Gameplay to Save on room event anchor");
  expect(!state.saved_snapshot_bytes.empty(),
         "Save state must produce a saved snapshot");
}

void test_game_lifecycle_gameplay_renders_scene_and_menu_navigation() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  comic2::transition_game_lifecycle(state, comic2::GameLifecycleState::Menu);
  expect(state.ui.selected_option_index == 0,
         "Menu entry should initialize selected_option_index to 0");

  // Navigate right in Menu without manually populating option_labels
  comic2::update_game_lifecycle(state,
                                comic2::InputState{.right_pressed = true});
  expect(state.ui.selected_option_index == 1,
         "Right input in Menu should advance world selection index to 1");

  comic2::update_game_lifecycle(state,
                                comic2::InputState{.left_pressed = true});
  expect(state.ui.selected_option_index == 0,
         "Left input in Menu should return world selection index to 0");

  // Transition to Gameplay and verify render_game_lifecycle_frame draws scene
  comic2::update_game_lifecycle(state,
                                comic2::InputState{.fire_pressed = true});
  expect(state.ui.lifecycle_state == comic2::GameLifecycleState::Gameplay,
         "Fire input in Menu should transition to Gameplay");

  state.player.x = 32;
  state.player.y = 48;
  state.player.is_airborne = false;
  comic2::EgaPlanarSurface surface(320, 200);
  surface.clear(0x00);
  comic2::render_game_lifecycle_frame(surface, state);

  // Verify the player marker/scene was rendered at (32, 48)
  bool saw_player_pixel = false;
  for (std::size_t plane = 0; plane < comic2::EgaPlanarSurface::kPlaneCount;
       ++plane) {
    if (surface.get_plane_byte(plane, 32 / 8, 48) != 0x00) {
      saw_player_pixel = true;
      break;
    }
  }
  expect(saw_player_pixel,
         "render_game_lifecycle_frame in Gameplay must render the gameplay "
         "scene onto the surface");
}

void test_ui_render_game_selection_panel_rejects_oversized_or_malformed_bezel() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.ui.menu_state = comic2::MenuState::GameSelect;

  // Oversized bezel_frame (width_bytes > surface.row_stride_bytes())
  comic2::Ega4PlaneImage oversized_bezel{};
  oversized_bezel.width_bytes = 50U;
  oversized_bezel.height_rows = 220U;
  oversized_bezel.row_span_bytes = 11000U;
  for (auto &plane : oversized_bezel.planes) {
    plane.assign(11000U, 0xFFU);
  }
  state.bezel_frame = oversized_bezel;

  comic2::EgaPlanarSurface surface(320, 200);
  surface.clear(0x00);
  comic2::ui_render_game_selection_panel(surface, state);

  expect(surface.get_plane_byte(0, 0, 0) == 0x00,
         "oversized bezel_frame must be skipped and not blitted at (0, 0)");
  expect(surface.get_plane_byte(1, 80 / 8, 70) == 0xFF,
         "fallback game selection panel must still render");

  // Direct call to gfx_rle_blit_opaque_4plane with oversized image must throw
  bool caught_out_of_range = false;
  try {
    comic2::gfx_rle_blit_opaque_4plane(surface, 0, 0, oversized_bezel);
  } catch (const std::out_of_range &) {
    caught_out_of_range = true;
  }
  expect(caught_out_of_range,
         "gfx_rle_blit_opaque_4plane must throw std::out_of_range when image "
         "exceeds destination bounds");
}

std::uint8_t read_surface_pixel_color(const comic2::EgaPlanarSurface &surface,
                                      std::size_t x, std::size_t y) {
  const std::size_t x_byte = x / 8U;
  const std::uint8_t bit = static_cast<std::uint8_t>(7U - (x % 8U));
  std::uint8_t color = 0U;
  for (std::size_t plane = 0; plane < comic2::EgaPlanarSurface::kPlaneCount;
       ++plane) {
    if ((surface.get_plane_byte(plane, x_byte, y) &
         static_cast<std::uint8_t>(1U << bit)) != 0U) {
      color |= static_cast<std::uint8_t>(1U << plane);
    }
  }
  return color;
}

void test_phase16_fr_script_loading_and_message_extraction() {
  const auto dir = find_reference_original_dir();
  if (dir.empty()) {
    return;
  }

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  expect(comic2::load_room_tilemap_from_asset_root(state, dir, 1, 1),
         "Level 1 Room 1 must load from original assets");
  expect(state.level_script_bytes.size() == comic2::kLevelScriptBlockSize,
         "try_load_level_tileset_for_root must cache 3072-byte decoded FR001.3 "
         "script");

  const auto msg0 =
      comic2::extract_level_script_message(state.level_script_bytes, 0);
  expect(msg0.has_value(), "FR001.3 message 0 must decode");
  expect(msg0->width_cells == 26 && msg0->height_cells == 13,
         "FR001.3 message 0 dimensions must be 26x13 cells");
  expect(msg0->lines.size() == 11,
         "FR001.3 message 0 must have 11 interior text lines");
  expect(msg0->lines[0] == "THANK YOU FOR ANSWERING ",
         "FR001.3 message 0 line 0 must match decrypted string");
  expect(msg0->lines[10] == "OF THESE STAIRS.        ",
         "FR001.3 message 0 line 10 must match decrypted string");
}

void test_phase16_ent_process_interaction_script_and_multi_step_dialog() {
  const auto dir = find_reference_original_dir();
  if (dir.empty()) {
    return;
  }

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  expect(comic2::load_room_tilemap_from_asset_root(state, dir, 1, 1),
         "Level 1 Room 1 must load from original assets");

  // Place player overlapping NPC 0x8976 as an interactive object (state_flags
  // 0x0002)
  state.player.x = 100;
  state.player.y = 100;
  state.mapped_objects.clear();
  state.mapped_objects.push_back(comic2::MappedObject12{
      .room_x = 1,
      .room_y = 1,
      .descriptor_ptr = 0x8976,
      .state_flags = 0x0002,
      .world_x = 100,
      .world_y = 100,
  });

  // Step 0 of 0x8976 -> message index 0
  comic2::handle_input_fallback(state);
  expect(state.ui.modal_active, "NPC interaction must activate modal prompt");
  expect(state.ui.modal_width_cells == 26 && state.ui.modal_height_cells == 13,
         "Modal dimensions must match FR001.3 message 0 (26x13)");
  expect(state.ui.modal_prompt.find("THANK YOU FOR ANSWERING ") == 0,
         "First interaction with 0x8976 must display message 0");

  // Dismiss modal and interact again -> Step 1 of 0x8976 -> message index 9
  state.ui.modal_active = false;
  state.ui.modal_prompt.clear();
  comic2::handle_input_fallback(state);
  expect(state.ui.modal_active,
         "Second NPC interaction must activate modal prompt");
  expect(state.story_script_progress == 1,
         "Second interaction with 0x8976 must advance story_script_progress to "
         "1");
  expect(state.ui.modal_prompt.find("OH, AND BE CAREFUL WITH ") == 0,
         "Second interaction with 0x8976 must display message 9");

  // Interact with leader 0x8C50 across 3 pages (messages 3, 4, 5)
  state.mapped_objects[0].descriptor_ptr = 0x8C50;
  state.ui.modal_active = false;
  state.ui.modal_prompt.clear();
  comic2::handle_input_fallback(state);
  expect(state.ui.modal_prompt.find("WELCOME CAPTAIN, WE ARE ") == 0,
         "Leader 0x8C50 step 0 must display message 3");

  state.ui.modal_active = false;
  state.ui.modal_prompt.clear();
  comic2::handle_input_fallback(state);
  expect(state.ui.modal_prompt.find("OUR TROUBLES HOWEVER,   ") == 0,
         "Leader 0x8C50 step 1 must display message 4");

  state.ui.modal_active = false;
  state.ui.modal_prompt.clear();
  comic2::handle_input_fallback(state);
  expect(state.ui.modal_prompt.find("IT IS UP TO YOU CAPTAIN,") == 0,
         "Leader 0x8C50 step 2 must display message 5");
  expect(state.story_script_progress == 2,
         "Leader 0x8C50 step 2 must advance story_script_progress to 2");

  // Interact with pickaxe giver 0x8ABE -> awards corkscrew/pickaxe and shows
  // message 6
  state.mapped_objects[0].descriptor_ptr = 0x8ABE;
  state.ui.modal_active = false;
  state.ui.modal_prompt.clear();
  comic2::handle_input_fallback(state);
  expect(state.player.corkscrew_equipped &&
             state.progression.corkscrew_acquired,
         "0x8ABE interaction at story_script_progress >= 2 must award "
         "corkscrew/pickaxe");
  expect(state.ui.modal_prompt.find("HERE IS YOUR PICK AXE.") == 0,
         "0x8ABE interaction must display message 6");
}

void test_phase16_framed_8x8_font_modal_rendering() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.ui.modal_active = true;
  state.ui.modal_prompt = "HELLO CAPTAIN\nSECOND LINE!";
  state.ui.modal_width_cells = 15; // 13 inner + 2 border
  state.ui.modal_height_cells = 4; // 2 inner + 2 border
  state.ui.modal_game_over = false;

  comic2::EgaPlanarSurface surface(320, 200);
  surface.clear(0x00);
  comic2::ui_render_modal_prompt(surface, state);

  // Box dimensions: 15 * 8 = 120 px wide, 4 * 8 = 32 px high
  // x0 = (((208 - 120) >> 1) & ~7) + 8 = (44 & ~7) + 8 = 40 + 8 = 48
  // y0 = (((160 - 32) >> 1) & ~7) + 8 = (64 & ~7) + 8 = 64 + 8 = 72
  expect(read_surface_pixel_color(surface, 48, 72) == 0x09U,
         "Top-left border corner must be drawn in light blue (0x09)");

  // Interior text row 0 is at y = 80..87, x = 56..159
  // Verify at least one pixel in 'H' (x = 56..63, y = 80..87) and row 1 'S'
  // (x = 56..63, y = 88..95) is drawn in text color 0x07
  bool saw_row0_text = false;
  bool saw_row1_text = false;
  for (std::size_t y = 80; y < 88; ++y) {
    for (std::size_t x = 56; x < 160; ++x) {
      if (read_surface_pixel_color(surface, x, y) == 0x07U) {
        saw_row0_text = true;
      }
    }
  }
  for (std::size_t y = 88; y < 96; ++y) {
    for (std::size_t x = 56; x < 160; ++x) {
      if (read_surface_pixel_color(surface, x, y) == 0x07U) {
        saw_row1_text = true;
      }
    }
  }
  expect(saw_row0_text, "First line of modal text must render 0x07 glyphs");
  expect(saw_row1_text, "Second line of modal text must render 0x07 glyphs");
}

void test_phase16_hud_update_mode_icons_on_bezel() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  comic2::Ega4PlaneImage bezel{};
  bezel.width_bytes = 40U;
  bezel.height_rows = 200U;
  bezel.row_span_bytes = 8000U;
  for (auto &plane : bezel.planes) {
    plane.assign(8000U, 0x00U);
  }
  state.bezel_frame = bezel;

  comic2::EgaPlanarSurface surface(320, 200);
  surface.clear(0x00);

  // Initially no equipment icons collected -> region x=240..255 (0xF0..0xFF)
  // at rows 0x5E, 0x77, 0x90, 0xA9 should remain blank
  comic2::hud_render_overlay(surface, state);
  for (std::size_t row_y : {0x5EU, 0x77U, 0x90U, 0xA9U}) {
    bool any_pixel = false;
    for (std::size_t y = row_y; y < row_y + 16U; ++y) {
      for (std::size_t x = 240U; x < 256U; ++x) {
        if (read_surface_pixel_color(surface, x, y) != 0U) {
          any_pixel = true;
        }
      }
    }
    expect(!any_pixel,
           "Uncollected mode icon slot must remain blank on bezel panel");
  }

  // Equip all 4 items: Corkscrew (0x5E), Boots (0x77), Lantern (0x90), Wand
  // (0xA9) and set active_mode_mask, score, counters, and keys
  state.player.corkscrew_equipped = true;
  state.player.boots_equipped = true;
  state.player.lantern_equipped = true;
  state.player.wand_equipped = true;
  state.ui.active_mode_mask = 0x02U;
  state.player.score = 1234U;
  state.player.hp = 12U;
  state.player.firepower = 3U;
  state.player.gems = 15U;
  state.player.fuel = 25U;
  state.player.lives = 4U;
  state.player.key_count = 2U;

  surface.clear(0x00);
  comic2::hud_render_overlay(surface, state);

  for (std::size_t row_y : {0x5EU, 0x77U, 0x90U, 0xA9U}) {
    bool saw_icon_pixel = false;
    for (std::size_t y = row_y; y < row_y + 16U; ++y) {
      for (std::size_t x = 240U; x < 256U; ++x) {
        if (read_surface_pixel_color(surface, x, y) != 0U) {
          saw_icon_pixel = true;
          break;
        }
      }
      if (saw_icon_pixel) {
        break;
      }
    }
    expect(saw_icon_pixel,
           "Collected equipment icon must blit visible 16x16 graphic at x=240");
  }

  // Verify 8x15 mode indicator bar at x=256..263 (0x100..0x107)
  bool saw_mode_bar = false;
  for (std::size_t y = 0x77U; y < 0x77U + 15U; ++y) {
    for (std::size_t x = 256U; x < 264U; ++x) {
      if (read_surface_pixel_color(surface, x, y) == 0x0AU) {
        saw_mode_bar = true;
        break;
      }
    }
  }
  expect(saw_mode_bar,
         "Active mode selection bar must render bright green (0x0A) at x=256");

  // Verify 6-digit 8x16 red LED score digits inside the score box (x=240..287,
  // y=16..31) and ensure bezel art to the right (x=288..303, y=16..31) is not
  // overdrawn
  bool saw_score_led = false;
  bool overdrew_right_of_score = false;
  for (std::size_t y = 16U; y < 32U; ++y) {
    for (std::size_t x = 240U; x < 288U; ++x) {
      if (read_surface_pixel_color(surface, x, y) == 0x0CU) {
        saw_score_led = true;
      }
    }
    for (std::size_t x = 288U; x < 304U; ++x) {
      if (read_surface_pixel_color(surface, x, y) != 0x00U) {
        overdrew_right_of_score = true;
      }
    }
  }
  expect(saw_score_led, "Score must render 6 8x16 red LED digits (0x0C) inside "
                        "score box at y=16");
  expect(!overdrew_right_of_score,
         "Score must not overdraw bezel art to the right of x=287 at y=16..31");

  // Verify Fireball meter (y=42..57) and Shield meter (y=67..82) light up
  // (0x0C)
  bool saw_lit_fireball_meter = false;
  bool saw_lit_shield_meter = false;
  for (std::size_t x = 256U; x < 304U; ++x) {
    if (read_surface_pixel_color(surface, x, 46U) == 0x0CU) {
      saw_lit_fireball_meter = true;
    }
    if (read_surface_pixel_color(surface, x, 71U) == 0x0CU) {
      saw_lit_shield_meter = true;
    }
  }
  expect(
      saw_lit_fireball_meter,
      "Fireball meter at x=256..303, y=42 must light up in bright red (0x0C)");
  expect(saw_lit_shield_meter,
         "Shield meter at x=256..303, y=67 must light up in bright red (0x0C)");

  // Verify 2-digit 8x16 LED counters at x=288..303 across all 4 rows
  for (std::size_t row_y : {0x5EU, 0x77U, 0x90U, 0xA9U}) {
    bool saw_counter_led = false;
    for (std::size_t y = row_y; y < row_y + 16U && !saw_counter_led; ++y) {
      for (std::size_t x = 288U; x < 304U && !saw_counter_led; ++x) {
        if (read_surface_pixel_color(surface, x, y) == 0x0CU) {
          saw_counter_led = true;
        }
      }
    }
    expect(saw_counter_led,
           "2-digit counter must render 8x16 red LED digits (0x0C) at x=288");
  }

  // Verify bottom inventory window at y=177 (0xB1), slots 0..1 populated
  bool saw_inv_key = false;
  for (std::size_t y = 177U; y < 193U && !saw_inv_key; ++y) {
    for (std::size_t x = 8U; x < 40U && !saw_inv_key; ++x) {
      if (read_surface_pixel_color(surface, x, y) != 0U) {
        saw_inv_key = true;
      }
    }
  }
  expect(saw_inv_key,
         "Bottom inventory window at y=177 must blit collected key icons");
}

void test_phase16_option_list_and_menu_panel_render_8x8_text_and_high_scores() {
  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.dos_config.player_count = 2;
  state.dos_config.player_names[0] = "COMIC";
  state.dos_config.player_names[1] = "HERO";
  state.dos_config.player_awards[0] = 12;
  state.dos_config.player_awards[1] = 5;
  comic2::ui_init_player_slot_menu(state);

  comic2::EgaPlanarSurface surface(320, 200);
  surface.clear(0x00);
  comic2::ui_render_option_list(surface, state);

  // Verify row 0 ("COMIC" at x=64..104, y=44..51) has selected text color 0x0A
  bool saw_player_text = false;
  for (std::size_t y = 44U; y < 52U; ++y) {
    for (std::size_t x = 64U; x < 104U; ++x) {
      if (read_surface_pixel_color(surface, x, y) == 0x0AU) {
        saw_player_text = true;
      }
    }
  }
  expect(saw_player_text,
         "ui_render_option_list must render selected player name with 8x8 "
         "glyphs");

  // Verify awards column at x=200..216, y=44..51 has yellow (0x0E) digits
  bool saw_awards_text = false;
  for (std::size_t y = 44U; y < 52U; ++y) {
    for (std::size_t x = 200U; x < 216U; ++x) {
      if (read_surface_pixel_color(surface, x, y) == 0x0EU) {
        saw_awards_text = true;
      }
    }
  }
  expect(saw_awards_text,
         "ui_render_option_list must render player awards from "
         "ds_high_scores_buffer");
}

void test_phase16_interactive_player_slot_management_and_world_save_loading() {
  const auto temp_dir =
      std::filesystem::temp_directory_path() / "test_comic2_phase16_slots";
  std::filesystem::remove_all(temp_dir);
  std::filesystem::create_directories(temp_dir);

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.ui.persistence_dir = temp_dir;
  comic2::ui_init_player_slot_menu(state);

  // Initially 0 players -> first selectable option is index 6 ("ENTER A NEW
  // PLAYER")
  expect(state.ui.selected_option_index == 6U,
         "Empty player list must select 'ENTER A NEW PLAYER' (index 6)");
  expect(!comic2::ui_is_option_selectable(state, 7U),
         "'RETIRE A PLAYER' must not be selectable when player_count == 0");

  // Enter two new players via live Menu lifecycle
  state.ui.lifecycle_state = comic2::GameLifecycleState::Menu;
  state.ui.pending_player_name_input = "comic one";
  comic2::update_game_lifecycle(state,
                                comic2::InputState{.fire_pressed = true});
  expect(state.dos_config.player_count == 1U,
         "Entering a new player must increment player_count to 1");
  expect(state.dos_config.player_names[0] == "COMIC ONE",
         "Player name must be uppercased and stored in slot 0");
  expect(std::filesystem::exists(temp_dir / "FR.CFG"),
         "Entering a new player must persist FR.CFG");
  expect(std::filesystem::exists(temp_dir / "GAME.001"),
         "Entering a new player must create initial GAME.001 snapshot");

  // Navigate to "ENTER A NEW PLAYER" (index 6) and add second player
  state.ui.selected_option_index = 6U;
  state.ui.pending_player_name_input = "hero two";
  comic2::update_game_lifecycle(state,
                                comic2::InputState{.fire_pressed = true});
  expect(state.dos_config.player_count == 2U,
         "Second player must increment player_count to 2");
  expect(state.dos_config.player_names[1] == "HERO TWO",
         "Second player must be stored in slot 1");

  // Navigate to "RETIRE A PLAYER" (index 7), activate retire mode, and retire
  // slot 0
  state.ui.selected_option_index = 7U;
  comic2::update_game_lifecycle(state,
                                comic2::InputState{.fire_pressed = true});
  expect(state.ui.retire_player_mode,
         "Selecting 'RETIRE A PLAYER' must enter retire_player_mode");
  expect(state.ui.option_labels[6] == "ERASE WHICH PLAYER?",
         "Retire mode must display 'ERASE WHICH PLAYER?' prompt");
  expect(state.ui.selected_option_index == 0U,
         "Retire mode must select first populated player slot 0");

  comic2::update_game_lifecycle(state,
                                comic2::InputState{.fire_pressed = true});
  expect(!state.ui.retire_player_mode,
         "Confirming slot in retire mode must exit retire_player_mode");
  expect(state.dos_config.player_count == 1U,
         "Retiring slot 0 must decrement player_count to 1");
  expect(state.dos_config.player_names[0].empty(),
         "Retired slot 0 name must be cleared");
  expect(state.dos_config.player_names[1] == "HERO TWO",
         "Remaining slot 1 must be preserved");

  // Prepare a customized save snapshot in GAME.004 and select slot 4 from Menu
  comic2::RuntimeState custom_slot = comic2::make_default_runtime_state();
  custom_slot.player.x = 416;
  custom_slot.player.y = 112;
  custom_slot.player.score = 4321;
  custom_slot.player.boots_equipped = true;
  custom_slot.player.corkscrew_equipped = true;
  custom_slot.current_level = 8;
  custom_slot.current_room = 2;
  expect(comic2::savegame_write_slot_file(custom_slot, temp_dir, 4U),
         "Writing GAME.004 must succeed");

  state.ui.player_slot_menu_active = false;
  state.ui.menu_state = comic2::MenuState::GameSelect;
  state.ui.option_labels = {"NATURE", "MAGIC",  "TIME",
                            "ICE",    "TECHNO", "DREAMS"};
  state.ui.selected_option_index = 4U; // TECHNO / slot 4
  comic2::update_game_lifecycle(state,
                                comic2::InputState{.fire_pressed = true});
  expect(state.ui.lifecycle_state == comic2::GameLifecycleState::Gameplay,
         "Confirming world/slot 4 must transition to Gameplay");
  expect(state.player.score == 4321 && state.current_level == 8 &&
             state.player.boots_equipped && state.player.corkscrew_equipped,
         "Confirming slot 4 must load GAME.004 snapshot into runtime state");

  std::filesystem::remove_all(temp_dir);
}

void test_phase16_interactive_key_rebinding_ui_and_input_handler() {
  const auto temp_dir =
      std::filesystem::temp_directory_path() / "test_comic2_phase16_keys";
  std::filesystem::remove_all(temp_dir);
  std::filesystem::create_directories(temp_dir);

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  state.ui.persistence_dir = temp_dir;

  comic2::ui_begin_key_rebinding(state);
  expect(state.ui.key_rebinding_active && state.ui.modal_active,
         "ui_begin_key_rebinding must activate modal key-rebinding flow");

  // Bind Left = 0x1E ('A')
  expect(comic2::ui_submit_key_rebinding_scancode(state, 0x1EU),
         "Valid Left scancode 0x1E must be accepted");
  expect(state.ui.key_rebinding_step == 1U, "Step must advance to 1 (Right)");

  // Duplicate scancode 0x1E must be rejected!
  expect(!comic2::ui_submit_key_rebinding_scancode(state, 0x1EU),
         "Duplicate scancode 0x1E must be rejected");
  expect(state.ui.key_rebinding_step == 1U,
         "Step must remain 1 after duplicate rejection");

  // Bind Right = 0x20 ('D'), Up = 0x11 ('W'), Down = 0x1F ('S'),
  // Jump = 0x39 (Space), Fire = 0x2A (LShift)
  expect(comic2::ui_submit_key_rebinding_scancode(state, 0x20U),
         "Right scancode 0x20 must be accepted");
  expect(comic2::ui_submit_key_rebinding_scancode(state, 0x11U),
         "Up scancode 0x11 must be accepted");
  expect(comic2::ui_submit_key_rebinding_scancode(state, 0x1FU),
         "Down scancode 0x1F must be accepted");
  expect(comic2::ui_submit_key_rebinding_scancode(state, 0x39U),
         "Jump scancode 0x39 must be accepted");
  expect(comic2::ui_submit_key_rebinding_scancode(state, 0x2AU),
         "Fire scancode 0x2A must be accepted and complete rebinding");

  expect(!state.ui.key_rebinding_active,
         "Key rebinding must complete after 6 unique keys");
  expect(
      state.key_bindings.left == 0x1EU && state.key_bindings.right == 0x20U &&
          state.key_bindings.up == 0x11U && state.key_bindings.down == 0x1FU &&
          state.key_bindings.jump == 0x39U && state.key_bindings.fire == 0x2AU,
      "RuntimeState key_bindings must match newly bound scancodes");
  expect(std::filesystem::exists(temp_dir / "KEYS.DEF"),
         "Completed key rebinding must persist KEYS.DEF");

  // Verify apply_dos_scancode_to_input maps custom scancodes
  comic2::InputState mapped_input{};
  comic2::apply_dos_scancode_to_input(mapped_input, 0x2AU, state.key_bindings);
  expect(mapped_input.fire_pressed,
         "Custom Fire scancode 0x2A must map to fire_pressed");

  std::filesystem::remove_all(temp_dir);
}

void test_startup_demo_backstory_and_config_options_screens() {
  const auto dir = find_reference_original_dir();
  if (dir.empty()) {
    return;
  }

  comic2::RuntimeState state = comic2::make_default_runtime_state();
  comic2::load_ui_assets(state, dir);

  expect(state.starfield_screen.has_value(),
         "load_ui_assets must populate starfield_screen (SEG_PALETTE:0x1CD0)");
  expect(state.credits_overlay.has_value(),
         "load_ui_assets must populate credits_overlay (SEG_PALETTE:0x0000)");
  expect(state.config_prompt_overlay.has_value(),
         "load_ui_assets must populate config_prompt_overlay "
         "(SEG_PALETTE:0x0CDC)");
  expect(state.demo_ship_sprites.size() == 8U,
         "load_ui_assets must load 8 FRDEMO.0 ship sprites");
  expect(state.demo_flash_sprites.size() == 2U,
         "load_ui_assets must load 2 FRDEMO.1 flash sprites");
  expect(state.demo_overlay_images.size() == 4U,
         "load_ui_assets must load 4 FRDEMO.1 48x48 overlay images");
  expect(state.demo_beam_sprites.size() == 6U,
         "load_ui_assets must load 6 FRDEMO.2 beam sprites");
  expect(state.demo_walk_sprites.size() == 6U,
         "load_ui_assets must load 6 SEG_STACK Comic walk sprites");

  comic2::EgaPlanarSurface surface(320, 200);

  // Phase 0 (tick 0): Initial credits splash with palette DS:0x0013 and no
  // music yet
  surface.clear(0x00);
  comic2::render_game_lifecycle_frame(surface, state);
  expect(state.palette.entries == comic2::make_ega_palette_ds_0013(),
         "Startup Phase 0 must apply palette DS:0x0013");
  expect(!state.ui.title_music_started && state.audio.pending_events.empty(),
         "Title music must not play during startup credits splash");

  // Phase 1 (tick 25..119): Spaceship landing on Planet Ogron with palette
  // DS:0xABC0
  state.ui.startup_demo_tick = 70U;
  surface.clear(0x00);
  comic2::render_game_lifecycle_frame(surface, state);
  expect(state.palette.entries == comic2::make_ega_palette_ds_abc0(),
         "Startup Demo Part 1 must apply palette DS:0xABC0");

  // Phase 2 (tick 120..431): Planet Ogron surface (FRPAK.003/004 + FRDEMO.1)
  state.ui.startup_demo_tick =
      120U + 90U + 17U; // Flash 1 tick 1 -> IntroFlashB
  state.audio.pending_events.clear();
  surface.clear(0x00);
  comic2::render_game_lifecycle_frame(surface, state);
  expect(!state.audio.pending_events.empty() &&
             state.audio.pending_events.back() ==
                 comic2::AudioEvent::IntroFlashB,
         "Demo Part 2 flash must queue IntroFlashB (DS:0xAC92)");

  // Phase 3 (tick 432..443): Comic beaming up through space (FRDEMO.2)
  state.ui.startup_demo_tick = 435U;
  surface.clear(0x00);
  comic2::render_game_lifecycle_frame(surface, state);
  expect(state.palette.entries == comic2::make_ega_palette_ds_abc0(),
         "Startup Demo Part 3 must apply palette DS:0xABC0");

  // Phase 4a (tick 444): Title Logo screen (SEG_PALETTE:0x1CD0 + FRPAK.005)
  // with green palette DS:0xABE2 and TitleAndIntroTheme (DS:0x00DB) starting!
  state.ui.startup_demo_tick = 444U;
  state.audio.pending_events.clear();
  surface.clear(0x00);
  comic2::render_game_lifecycle_frame(surface, state);
  expect(state.palette.entries == comic2::make_ega_palette_ds_abe2(),
         "Title logo phase 4a must apply green palette DS:0xABE2");
  expect(state.ui.title_music_started && !state.audio.pending_events.empty() &&
             state.audio.pending_events.back() ==
                 comic2::AudioEvent::StartupChime,
         "Title logo phase 4a must start TitleAndIntroTheme (DS:0x00DB)");

  // Phase 4b (tick 484): Title Banner + Backstory screen
  // (SEG_PALETTE:0x1CD0 + FRPAK.006) with gold/yellow/red palette DS:0x0024
  state.ui.startup_demo_tick = 484U;
  surface.clear(0x00);
  comic2::render_game_lifecycle_frame(surface, state);
  expect(state.palette.entries == comic2::make_ega_palette_ds_0024(),
         "Backstory phase 4b must apply gold/yellow/red palette DS:0x0024");
  // Verify both starfield (planes 0..1) and backstory text (planes 2..3) are
  // composited onto the surface
  bool saw_starfield_plane = false;
  bool saw_backstory_plane = false;
  for (std::size_t i = 0; i < 8000U; ++i) {
    if (surface.plane(0)[i] != 0U || surface.plane(1)[i] != 0U) {
      saw_starfield_plane = true;
    }
    if (surface.plane(2)[i] != 0U || surface.plane(3)[i] != 0U) {
      saw_backstory_plane = true;
    }
  }
  expect(saw_starfield_plane && saw_backstory_plane,
         "Backstory screen must composite FRPAK.006 over SEG_PALETTE:0x1CD0");

  // Pressing any key on the Title/Backstory screen transitions to Intro (the
  // Title Screen with Keyboard and Joystick Configuration Options)
  comic2::InputState any_key{};
  any_key.sdl_keyboard_active = true;
  any_key.any_key_pressed = true;
  any_key.last_dos_scancode = 0x39U; // Space
  state.audio.pending_events.clear();
  expect(comic2::update_game_lifecycle(state, any_key),
         "Pressing any key on Title/Backstory screen must succeed");
  expect(state.ui.lifecycle_state == comic2::GameLifecycleState::Intro,
         "Pressing any key on Title/Backstory must advance to Intro (Config "
         "screen)");
  expect(!state.audio.pending_events.empty() &&
             state.audio.pending_events.back() ==
                 comic2::AudioEvent::StartupChime,
         "Config screen entry must play TitleAndIntroTheme (DS:0x00DB)");

  surface.clear(0x00);
  comic2::render_game_lifecycle_frame(surface, state);
  expect(state.palette.entries == comic2::make_ega_palette_ds_0024(),
         "Keyboard/Joystick config screen must apply palette DS:0x0024");

  // Pressing 'J' (0x24) opens Calibrate Joystick screen; pressing any key
  // aborts back to Config screen
  comic2::InputState press_j{};
  press_j.sdl_keyboard_active = true;
  press_j.any_key_pressed = true;
  press_j.last_dos_scancode = 0x24U;
  comic2::update_game_lifecycle(state, press_j);
  expect(state.ui.joystick_calibration_active,
         "Pressing J on Config screen must enter Calibrate Joystick screen");
  comic2::render_game_lifecycle_frame(surface, state);
  comic2::update_game_lifecycle(state, any_key);
  expect(!state.ui.joystick_calibration_active &&
             state.ui.lifecycle_state == comic2::GameLifecycleState::Intro,
         "Pressing any key in Calibrate Joystick must abort back to Config "
         "screen");

  // Pressing 'K' (0x25) opens Define Keys screen; binding 6 keys returns to
  // Config screen
  const auto temp_dir = std::filesystem::temp_directory_path() /
                        "test_comic2_startup_player_menu";
  std::filesystem::remove_all(temp_dir);
  std::filesystem::create_directories(temp_dir);
  state.ui.persistence_dir = temp_dir;

  comic2::InputState press_k{};
  press_k.sdl_keyboard_active = true;
  press_k.any_key_pressed = true;
  press_k.last_dos_scancode = 0x25U;
  comic2::update_game_lifecycle(state, press_k);
  expect(state.ui.key_rebinding_active,
         "Pressing K on Config screen must enter Define Keys screen");
  comic2::render_game_lifecycle_frame(surface, state);
  for (std::uint8_t sc :
       {std::uint8_t(0x4B), std::uint8_t(0x4D), std::uint8_t(0x48),
        std::uint8_t(0x50), std::uint8_t(0x39), std::uint8_t(0x52)}) {
    comic2::InputState bind_key{};
    bind_key.sdl_keyboard_active = true;
    bind_key.any_key_pressed = true;
    bind_key.last_dos_scancode = sc;
    comic2::update_game_lifecycle(state, bind_key);
  }
  expect(!state.ui.key_rebinding_active &&
             state.ui.lifecycle_state == comic2::GameLifecycleState::Intro,
         "Completing 6 key bindings must finish Define Keys");

  // Pressing any other key on Config screen advances to Menu with the Player
  // Save Slot menu active (NOT the 6-world stage selection menu!)
  comic2::update_game_lifecycle(state, any_key);
  expect(state.ui.lifecycle_state == comic2::GameLifecycleState::Menu,
         "Pressing any other key on Config screen must advance to Menu");
  expect(state.ui.player_slot_menu_active,
         "Menu after Config screen must open the Player Save Slot menu");
  expect(state.ui.selected_option_index == 6U &&
             state.ui.option_labels[6] == "ENTER A NEW PLAYER",
         "Empty player roster must highlight 'ENTER A NEW PLAYER'");

  surface.clear(0x00);
  comic2::render_game_lifecycle_frame(surface, state);

  // Press Enter (0x1C) on 'ENTER A NEW PLAYER' to begin inline name entry, type
  // "COMIC", and press Enter (0x1C) to save the new player slot
  comic2::InputState press_enter{};
  press_enter.sdl_keyboard_active = true;
  press_enter.any_key_pressed = true;
  press_enter.last_dos_scancode = 0x1CU;
  comic2::update_game_lifecycle(state, press_enter);
  expect(state.ui.editing_player_name && state.ui.selected_option_index == 0U,
         "Confirming 'ENTER A NEW PLAYER' must start inline name editing on "
         "slot 0");

  for (std::uint8_t sc :
       {std::uint8_t(0x2E), std::uint8_t(0x18), std::uint8_t(0x32),
        std::uint8_t(0x17), std::uint8_t(0x2E)}) { // C-O-M-I-C
    comic2::InputState type_key{};
    type_key.sdl_keyboard_active = true;
    type_key.any_key_pressed = true;
    type_key.last_dos_scancode = sc;
    comic2::update_game_lifecycle(state, type_key);
  }
  comic2::update_game_lifecycle(state, press_enter);
  expect(!state.ui.editing_player_name &&
             state.dos_config.player_names[0] == "COMIC" &&
             state.ui.selected_option_index == 0U,
         "Pressing Enter after typing name must create player 'COMIC' in slot "
         "0");

  // Menu music (DS:0x9D3F) must be playing while in Menu
  expect(state.audio.sound.is_playing && state.audio.sound.stream_offset ==
                                             comic2::kSoundOffsetMainMenuTheme,
         "MainMenuTheme must be playing while in player selection Menu");

  // Confirming slot 0 ("COMIC") stops menu music, loads GAME.001 (Level 0
  // Room 0 at authentic spawn x=568, y=80, camera_x=472, camera_y=0), displays
  // the FRPAK.001 game UI first, and then blits the 208x160 gameplay window
  comic2::update_game_lifecycle(state, press_enter);
  expect(state.ui.lifecycle_state == comic2::GameLifecycleState::Gameplay &&
             state.current_level == 0U && state.current_room == 0U,
         "Selecting player slot 0 must enter Gameplay at Level 0 Room 0");
  expect(!state.audio.sound.is_playing,
         "Selecting a player save slot must stop the menu music (1000:5EEA)");
  expect(state.player.x == 568 && state.player.y == 80,
         "New player save slot must spawn at authentic Level 0 Room 0 position "
         "(568, 80)");
  expect(state.camera_x == 464 && state.camera_y == 0,
         "Initial camera must center on Comic's starting position (camera_x = "
         "568 - 104 = 464)");
  expect(state.ui.gameplay_ui_pre_blit_ticks == 1U &&
             state.ui.gameplay_initial_spawn_pending,
         "Selecting player save slot must stage 1-tick FRPAK.001 UI pre-blit "
         "before gameplay window blit");

  // Frame 1 in Gameplay: displays the FRPAK.001 game UI + HUD before the room
  // tilemap/player overwrites the 208x160 playfield window
  comic2::EgaPlanarSurface ui_first_surface(320, 200);
  ui_first_surface.clear(0x00);
  comic2::render_game_lifecycle_frame(ui_first_surface, state);
  expect(state.ui.gameplay_ui_pre_blit_ticks == 0U,
         "First Gameplay frame must consume gameplay_ui_pre_blit_ticks");

  // Frame 2 in Gameplay: blits the 208x160 gameplay window with the player at
  // the starting position (568, 80) before physics moves the player
  comic2::update_game_lifecycle(state, comic2::InputState{});
  expect(!state.ui.gameplay_initial_spawn_pending && state.player.x == 568 &&
             state.player.y == 80,
         "Initial gameplay window blit frame must hold player at starting "
         "position (568, 80)");
  comic2::EgaPlanarSurface gameplay_surface(320, 200);
  gameplay_surface.clear(0x00);
  comic2::render_game_lifecycle_frame(gameplay_surface, state);

  bool playfield_changed = false;
  for (std::size_t y = comic2::kPlayfieldY;
       y < comic2::kPlayfieldY + comic2::kPlayfieldHeight && !playfield_changed;
       ++y) {
    for (std::size_t x_byte = comic2::kPlayfieldX / 8U;
         x_byte < (comic2::kPlayfieldX + comic2::kPlayfieldWidth) / 8U;
         ++x_byte) {
      for (std::size_t plane = 0; plane < comic2::EgaPlanarSurface::kPlaneCount;
           ++plane) {
        if (ui_first_surface.get_plane_byte(plane, x_byte, y) !=
            gameplay_surface.get_plane_byte(plane, x_byte, y)) {
          playfield_changed = true;
          break;
        }
      }
    }
  }
  expect(playfield_changed,
         "Second Gameplay frame must overwrite the 208x160 playfield cutout "
         "with the room tilemap and player at the starting position");

  std::filesystem::remove_all(temp_dir);
}

} // namespace

void run_ui_tests() {
  test_bcd_counter_helpers();
  test_hud_renders_counters_and_icons();
  test_hud_renders_mode_and_inventory_labels();
  test_hud_renders_mode_inventory_bits();
  test_modal_prompt_overlay_is_rendered();
  test_menu_state_machine_and_option_navigation();
  test_frpak002_title_screen_decode_and_render();
  test_frpak_intro_cutscenes_decode_and_progression();
  test_frpak007_victory_screen_decode_and_render();
  test_frdata0_menu_panel_decode();
  test_frdata1_menu_buttons_decode();
  test_ui_render_game_selection_panel_with_authentic_assets();
  test_load_ui_assets_helper();
  test_game_lifecycle_state_machine_transitions();
  test_game_lifecycle_update_and_rendering();
  test_game_lifecycle_gameplay_to_save_via_dispatcher_tick();
  test_game_lifecycle_gameplay_to_save_via_anchor_trigger();
  test_game_lifecycle_gameplay_renders_scene_and_menu_navigation();
  test_ui_render_game_selection_panel_rejects_oversized_or_malformed_bezel();
  test_phase16_fr_script_loading_and_message_extraction();
  test_phase16_ent_process_interaction_script_and_multi_step_dialog();
  test_phase16_framed_8x8_font_modal_rendering();
  test_phase16_hud_update_mode_icons_on_bezel();
  test_phase16_option_list_and_menu_panel_render_8x8_text_and_high_scores();
  test_phase16_interactive_player_slot_management_and_world_save_loading();
  test_phase16_interactive_key_rebinding_ui_and_input_handler();
  test_startup_demo_backstory_and_config_options_screens();
}
