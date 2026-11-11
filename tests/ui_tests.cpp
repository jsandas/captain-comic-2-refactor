#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include "comic2/default_handlers.hpp"
#include "comic2/dispatcher.hpp"
#include "comic2/game_state.hpp"
#include "comic2/persistence.hpp"
#include "comic2/renderer.hpp"
#include "comic2/resource_loader.hpp"
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
}
