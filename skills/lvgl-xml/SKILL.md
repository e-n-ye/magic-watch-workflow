---
name: lvgl-xml
description: Maintain this repository's LVGL 9.5 UI through LVGL Pro Editor XML and manual C export. Use when editing project.xml, globals.xml, screen XML, styles, components, bindings, assets, generated UI CMake/C files, Editor preview errors, or host simulator rendering while preserving the F411 240x280 and no-Pro-CLI contract.
---

# LVGL XML

Use this skill for the repository's declarative LVGL UI and its manual export
workflow. Treat the XML as the design source and the committed generated C as
the firmware and simulator input.

## Contract

- Keep the F411 profile at LVGL `9.5.0` and `240x280`.
- Maintain XML in LVGL Pro Editor. Do not require Pro CLI, a token, a network
  fetch, or a local tool path in the repository.
- Do not parse XML at F411 runtime. The F411 configuration keeps `LV_USE_XML=0`;
  XML parser work belongs to a later, explicitly scoped round.
- Treat `*_gen.c`, `*_gen.h`, generated file lists, and generated CMake as
  export output. Never hand-edit any generated file. Every generated-file
  change must come from the Editor Code/export action; if the Editor cannot
  export, stop with the XML change pending and ask for an Editor export rather
  than synthesizing equivalent C by hand.
- Put hand-written UI extensions in the existing user hook, normally
  `products/f411_watch/ui/user_config.cmake`, and add a real consumer and test
  before creating a new component or widget.
- Do not edit third-party LVGL sources, CubeMX `.ioc` files, `think.md`, or
  generated CubeMX code for an XML task.

## Inspect Before Editing

1. Read `docs/f411-m7-editor-export.md` and the current UI `project.xml`.
2. Check `firmware/stm32/f411_watch/user/ui/lv_conf.h` before using a widget,
   font, image, animation, or binding. The F411 profile intentionally enables a
   small subset of LVGL.
3. Inspect a nearby working screen before inventing syntax. When available,
   compare the official `tutorials/` and `examples/` roots supplied for the
   machine; never commit their absolute paths.
4. Confirm the target display, LVGL version, and output paths before exporting.

## File Roles

| File | Role | Rule |
| --- | --- | --- |
| `project.xml` | Editor project metadata and target display | Keep `lvgl_version="9.5.0"` and F411 target `240x280` |
| `globals.xml` | Shared constants, styles, subjects, images, and fonts | Add only resources with a current consumer |
| `screens/**/screen_*.xml` | Screen source | Keep the screen root and declarative view tree valid |
| `*_gen.c/h` | Editor-generated C interface | Regenerate in Editor; review the diff; do not hand-edit |
| `file_list_gen.cmake` and `CMakeLists.txt` | Editor export/build integration | Preserve generated lists and the user hook |
| `user_config.cmake` | Hand-written source extension point | Add user sources here, not to generated lists |

## XML Shape

Use the Editor schema demonstrated by the official examples:

```xml
<screen>
    <consts>
        <color name="accent" value="0x64D2FF" />
    </consts>
    <styles>
        <style name="style_screen" bg_color="0x101820" />
    </styles>
    <view>
        <style name="style_screen" />
        <lv_label text="MAGIC WATCH" align="top_mid" y="22"
                  style_text_color="#accent" />
    </view>
</screen>
```

The style attachment is a child element. Do **not** write
`<view style="style_screen">`; that form is valid XML but is rejected by the
Editor schema used by this project. The same child-style form applies to
widgets and reusable components. Use style selectors such as
`selector="indicator"` only when the widget exposes that part.

Prefer the established forms for the rest of the schema:

- Use `align="top_mid"`, `center`, `bottom_mid`, and signed `x`/`y` offsets for
  simple fixed-format screens.
- Use `<row>`/`<column>` or flex properties for content that must flow. Keep
  fixed display dimensions in `project.xml`, not scattered magic sizes.
- Use `<consts>` for screen-local colors and dimensions; use `globals.xml` only
  for values shared by multiple screens.
- Use `&#10;` for line breaks in XML text. A color swatch shown by Editor before
  a hexadecimal value is a UI decoration, not file corruption.
- Add subjects, asset data, fonts, animations, components, or custom widgets
  only when the current round has a consumer and a test. Follow the examples
  for subject bindings and component APIs instead of inventing attributes.

## Editor Export

1. Edit and preview the XML in LVGL Pro Editor.
2. Use the Editor Code/export action (the official tutorial also documents
   `Ctrl+B`/the hammer button) to regenerate C.
3. Review the generated C/H and file-list diff. Keep the XML and generated C in
   the same change; do not commit unrelated Editor metadata or local paths.
4. Confirm the generated screen constructor and wrapper are linked by the
   existing UI CMake entry. Add hand-written files through `user_config.cmake`.
5. Do not claim XML regeneration in CI: CI builds committed generated C only.

## Validation

Run the smallest relevant checks, then broaden them when an exported file or
shared build path changes:

```sh
cmake -S products/f411_watch/simulator -B build/host-m7 -G Ninja
cmake --build build/host-m7
ctest --test-dir build/host-m7 --output-on-failure
build/host-m7/watch_ui_simulator.exe --smoke
```

On Windows, run `build/host-m7/watch_ui_simulator.exe` briefly to inspect the
240x280 native window. The Windows backend allocates its framebuffer from a
timer; if the first frame is blank, let one `LV_DEF_REFR_PERIOD` elapse, run
the timer handler, invalidate the screen, and force a refresh in the simulator
application. Do not patch the third-party backend.

For any change that affects the F411 project, also run from
`firmware/stm32/f411_watch`:

```sh
cmake --preset Debug
cmake --build --preset Debug
cmake --build --preset Debug --target format-check
cmake --build --preset Debug --target cppcheck
```

Parse changed XML with an available XML parser (`xmllint --noout` or the
PowerShell `[xml]` cast) and run `git diff --check`. Check that no diff adds a
Pro token, an absolute tool path, or the forbidden `<view style="...">` form.

## Boundaries

M7 proves Editor XML, committed generated C, host rendering, and the smoke
contract. Page lifecycle, the full page stack, real input routing, and runtime
XML parsing are later concerns. Keep those out of a small XML/export fix unless
the user explicitly starts that round.

Load [official-patterns.md](references/official-patterns.md) when a task needs a
concrete style, layout, subject, asset, animation, component, or custom-widget
pattern from the supplied official examples.
