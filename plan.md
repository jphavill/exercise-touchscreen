# Adapted LVGL Workout Navigation Plan

## 1. Screen Model

Use the existing single-screen container model in `ui.cpp` instead of creating separate LVGL screens with `lv_scr_load_anim`.

`s_dashboardContainer` already exists as a child of `lv_scr_act()` at `ui.cpp:47` and is created inside `ui_init()` at `ui.cpp:114`. Add `s_workoutSelectionContainer` and `s_workoutContainer` as sibling full-screen containers under the same `scr`.

Final hierarchy:

```text
lv_scr_act()
├── s_dashboardContainer
├── s_workoutSelectionContainer
├── s_workoutContainer
├── s_offlineContainer
└── s_refreshBtn
```

Important repo detail: `s_refreshBtn` is currently a floating child of `scr`, not of `s_dashboardContainer`. Hide it whenever leaving the dashboard.

## 2. Static UI State

Add near the existing statics in `ui.cpp`:

```cpp
static lv_obj_t* s_workoutSelectionContainer = nullptr;
static lv_obj_t* s_workoutContainer = nullptr;
static lv_obj_t* s_workoutTitleLabel = nullptr;
static lv_obj_t* s_workoutIdLabel = nullptr;
```

Use repo naming style with a C++ struct:

```cpp
struct WorkoutRoutine {
  uint16_t id;
  const char* name;
  uint16_t durationMin;
};
```

Keep routines private in `ui.cpp`:

```cpp
static constexpr WorkoutRoutine kWorkoutRoutines[] = {
    {1, "Pull-ups", 10},
    {2, "Push", 12},
    {3, "Core", 8},
    {4, "Crimps", 15},
    {5, "Mobility", 10},
    {6, "Custom", 20},
};
```

## 3. Constants

Follow the existing constant style like `kHeatCellSize`:

```cpp
static constexpr uint16_t kNavAnimMs = 250;
static constexpr lv_coord_t kScreenPad = 24;
static constexpr lv_coord_t kGridGap = 16;
static constexpr lv_coord_t kSectionGap = 20;
```

## 4. Navigation Model

Do not use `lv_scr_load_anim()` for these containers.

Add an internal page enum in `ui.cpp`:

```cpp
enum class UiPage {
  Dashboard,
  WorkoutSelection,
  Workout,
};

static UiPage s_currentPage = UiPage::Dashboard;
```

Add a helper:

```cpp
static void show_page(UiPage page, bool forward);
```

Behavior:

```text
Dashboard --swipe left--> WorkoutSelection
WorkoutSelection --swipe right--> Dashboard
WorkoutSelection --tap card--> Workout
Workout --back arrow--> WorkoutSelection
```

Animation direction:

```text
forward: new page starts at screen width, moves to 0
backward: new page starts at -screen width, moves to 0
```

Hide the old page after animation completes, or immediately after the new page becomes visible if keeping the implementation simpler.

## 5. Existing Offline Flow

This repo already has `s_offlineContainer` and public functions:

```cpp
void ui_show_dashboard();
void ui_show_offline();
```

Adapt them carefully.

Recommended behavior:

```text
ui_show_offline()
- hides dashboard/workout selection/workout
- hides refresh button
- shows offline screen

ui_show_dashboard()
- hides offline screen
- shows dashboard
- sets current page to Dashboard
- shows refresh button
```

This preserves current `main.cpp` behavior, where successful API fetch calls `ui_show_dashboard()` and failed API fetch calls `ui_show_offline()`.

Repo-specific caveat: `main.cpp` refreshes every `API_REFRESH_MS` and calls `ui_show_dashboard()` on success. That means the app may navigate back to dashboard every 60 seconds while the user is on workout selection or workout. If that is undesirable, change `ui_show_dashboard()` to only clear the offline overlay and preserve `s_currentPage`; otherwise preserve current behavior.

## 6. Gesture Handling

Keep callback naming consistent with existing `on_refresh_btn`:

```cpp
static void on_dashboard_gesture(lv_event_t* e);
static void on_workout_selection_gesture(lv_event_t* e);
static void on_workout_card(lv_event_t* e);
static void on_workout_back(lv_event_t* e);
```

Attach gestures to full-screen containers:

```cpp
lv_obj_add_event_cb(s_dashboardContainer, on_dashboard_gesture, LV_EVENT_GESTURE, nullptr);
lv_obj_add_event_cb(s_workoutSelectionContainer, on_workout_selection_gesture, LV_EVENT_GESTURE, nullptr);
```

Because this repo currently calls `register_tap_target()` on many child objects, gesture interception may happen on children. To make gestures reliable, either use `LV_OBJ_FLAG_GESTURE_BUBBLE` where supported by this LVGL version, or attach the same gesture callback to major child containers/cards that can receive touch events.

## 7. Workout Selection Screen

Create this inside `ui_init()` after dashboard creation and before offline creation.

Use existing style approach:

```cpp
s_workoutSelectionContainer = lv_obj_create(scr);
lv_obj_remove_style_all(s_workoutSelectionContainer);
lv_obj_set_size(s_workoutSelectionContainer, LV_PCT(100), LV_PCT(100));
lv_obj_set_style_bg_color(s_workoutSelectionContainer, lv_color_hex(0x000000), 0);
lv_obj_set_style_bg_opa(s_workoutSelectionContainer, LV_OPA_COVER, 0);
lv_obj_set_style_text_color(s_workoutSelectionContainer, lv_color_hex(0xF2F7FF), 0);
lv_obj_set_style_pad_all(s_workoutSelectionContainer, kScreenPad, 0);
```

Use `LV_PCT(100)`, not `lv_pct(100)`, matching current code.

Layout:

```text
s_workoutSelectionContainer
├── title label: "Workouts"
└── grid
    ├── 6 card objects
```

Use existing colors:

```text
primary text: 0xF2F7FF
muted text: 0xB9C8D8
accent: 0x78C7FF
background: 0x000000
```

Cards should be `lv_obj_create(grid)`, not buttons, matching the requirement.

## 8. Workout Screen

Create as another full-screen sibling:

```text
s_workoutContainer
├── back button top-left
└── centered flex container
    ├── s_workoutTitleLabel
    └── s_workoutIdLabel
```

Back button can be an `lv_btn_create()` because the constraint only says workout selection uses cards, not buttons.

Use repo font aliases:

```cpp
UI_FONT_TITLE
UI_FONT_META
UI_FONT_BUTTON
```

## 9. Card Click

Callback:

```cpp
static void on_workout_card(lv_event_t* e) {
  const WorkoutRoutine* routine = static_cast<const WorkoutRoutine*>(lv_event_get_user_data(e));

  lv_label_set_text(s_workoutTitleLabel, routine->name);

  char buf[24];
  snprintf(buf, sizeof(buf), "ID: %u", static_cast<unsigned>(routine->id));
  lv_label_set_text(s_workoutIdLabel, buf);

  show_page(UiPage::Workout, true);
}
```

Tap immediately opens the workout screen.

## 10. Minimal File Scope

All implementation can stay in `ui.cpp`.

No `ui.h` changes are needed unless external code needs to navigate directly to workout pages. Current user behavior is entirely UI-driven, so keep the API unchanged.

## 11. Execution Order

1. Add new static state, constants, `WorkoutRoutine`, and callbacks in `ui.cpp`.
2. Add `show_page()` helper for sibling-container navigation.
3. Attach dashboard gesture to existing `s_dashboardContainer`.
4. Create `s_workoutSelectionContainer` in `ui_init()`.
5. Create `s_workoutContainer` in `ui_init()`.
6. Update `ui_show_dashboard()` and `ui_show_offline()` to include new containers and refresh button visibility.
7. Verify compile behavior and test touch navigation on device.
