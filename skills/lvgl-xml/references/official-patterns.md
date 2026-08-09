# Official LVGL XML Patterns

Use this reference only when the task needs a schema example. The source roots
are external learning material supplied on the developer machine; do not copy
their generated projects or encode their absolute paths in repository files.

## Source Map

The following relative paths are the useful examples in the official tutorial
and examples projects:

| Topic | Example |
| --- | --- |
| Minimal screen and styles | `tutorials/screens/hello_world/screen_hello_world.xml` |
| Style selectors and local overrides | `tutorials/screens/styles/screen_styles.xml` |
| Rows, columns, and floating objects | `tutorials/screens/layout/screen_layouts.xml` |
| Timeline animations | `tutorials/screens/animations/screen_animations.xml` |
| Subject data binding | `tutorials/screens/data_bindings/screen_data_bindings.xml` |
| Reusable component API | `tutorials/components/buttons/button_normal.xml` and `tutorials/screens/new_component/screen_components.xml` |
| Custom widget XML API and C hooks | `tutorials/widgets/wd_segment/wd_segment.xml` and `wd_segment.c` |
| Editor screen contract | `examples/screens/README.md` |
| Text style properties | `examples/styles/lv_example_style_text.xml` |
| Subject-bound widget values | `examples/widgets/arc/lv_example_arc_bind_value.xml` |
| Fonts and images | `examples/fonts/README.md` and `examples/images/README.md` |

## Stable Patterns

### Styles

Declare reusable styles in `<styles>` and attach them inside the object:

```xml
<view>
    <style name="style_card" />
    <lv_label text="Title">
        <style name="style_heading" />
    </lv_label>
</view>
```

For widget parts, use the Editor's selector form, for example
`style_bg_opa-knob="0%"` or a `<style name="..." selector="indicator" />`
when that widget supports the selector.

### Layout

Use `<row>` and `<column>` for flex layout. Typical properties include
`style_flex_main_place="space_between"`, `style_pad_all`, and
`style_pad_row`. Use `ignore_layout="true"` and explicit `x`/`y` only for a
deliberately floating child; use `floating="true"` for an object that should
not consume flex space.

### Subjects

Declare a subject in `globals.xml`, then bind compatible widget properties:

```xml
<lv_slider subject="subject_value" />
<lv_label bind_text="subject_value" bind_text-fmt="%d %%" />
```

Keep subject initialization and registration in generated C. Do not add a
subject merely to avoid passing a normal static property.

### Components and Widgets

A reusable component declares an `<api>` and exposes its properties to a view.
A custom widget needs an XML API plus hand-written constructor, event, and
property hooks, and (when XML parsing is enabled) parser registration. This is
not part of the current F411 M7 runtime, where `LV_USE_XML=0`; defer it until a
real consumer and test exist.

### Assets

Register fonts and images in `globals.xml`. Use embedded C data (`as_file="false"`)
when the target has no runtime file system; use `<file>`/`as_file="true"` only
when a real filesystem consumer exists. Match the project's color format and
resource budget before adding an asset.

## Export and Build Evidence

The tutorial documents exporting generated C from the Editor and building its
simulator. The examples project keeps generated lists and CMake separate from
`user_config.cmake`. The F411 project follows the same boundary:

- XML is kept under `products/f411_watch/ui`.
- Generated screen C is listed by `file_list_gen.cmake`.
- Hand-written additions enter through `user_config.cmake`.
- The host simulator links the same generated C and repository LVGL sources.
- CTest uses `watch_ui_simulator --smoke`; the Windows executable is a visual
  check only and must not become a CI-only dependency.

When an Editor preview fails, first compare the XML shape with the minimal
screen and styles examples. When the native simulator is blank but the smoke
test finds labels, check delayed framebuffer initialization and force a first
refresh in the application before changing generated UI code.
