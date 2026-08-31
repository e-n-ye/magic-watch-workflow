---
name: lvgl-xml
description: Maintain this repository's LVGL 9.5 UI from visual briefs through LVGL Pro Editor XML, approved raster references, manual C export, and host validation. Use when designing or editing project.xml, globals.xml, screen XML, styles, components, bindings, assets, generated UI CMake/C files, Right image drafts, Editor preview errors, or simulator rendering while preserving the F411 240x280 and no-Pro-CLI contract.
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

## Visual-to-XML workflow

Use this skill for the complete visual pipeline, not only XML syntax:

1. Write a visual brief before touching the XML. State the target screen,
   `240x280` canvas, information hierarchy, interaction states, palette, type
   constraints, and what must remain readable on the F411. For the first demo
   pass, keep the brief scoped to the watch face: large time, date/weekday,
   battery, and a step or sensor summary.
2. Generate concept images only as visual references. The concept must show a
   bare watch screen at the target aspect ratio; it is not a bitmap background
   to burn into firmware. Record the prompt, model, size, reference images,
   and selected variant in an untracked `build/design-evidence/<job>/`
   directory. Keep intermediate drafts and thumbnails out of Git.
3. Before any paid request, show the user the model, variant count, size,
   prompt summary, and reference count and wait for explicit approval. Use the
   repository helper in `scripts/right_image_job.py`; it defaults to dry-run
   and requires `--confirm-paid` for network submission. Read `RIGHT_API_KEY`
   (or `RIGHTAPI_API_KEY`) from the environment or a local `.env.local`; never
   put a key in XML, manifests, commits, or chat. See
   [right-image.md](references/right-image.md) for the command and response
   contract.
4. Inspect generated thumbnails with the user and record one selected draft.
   Do not translate an unconfirmed concept into production XML. If the visual
   is not suitable, change one prompt dimension at a time and regenerate.
5. Translate the selected concept into declarative XML. XML owns structure,
   spacing, colors, typography, stable object names, and visual states. Prefer
   `lv_obj` + `lv_label` combinations already enabled by `lv_conf.h`; add image
   assets only with a measured Flash/RAM budget and a real consumer. Runtime
   hooks may update text, subjects, selected states, and degraded notices, but
   must not create duplicate visual objects or reach into HAL code.
6. Ask the user to open `project.xml` in LVGL Pro Editor and verify the preview.
   Every XML change must be exported with the Editor Code/hammer action (or
   `Ctrl+B`) before firmware or simulator validation. If export fails, fix the
   XML/schema problem and wait for a fresh Editor export; never hand-synthesize
   `*_gen.c/h`.
7. Validate the exported result at native `240x280` in the host simulator,
   checking overlap, clipping, object names, selected/degraded states, and
   repeated enter/leave behavior. Capture evidence under
   `build/design-evidence/<scenario>/` and keep only a redacted manifest or
   final approved assets in Git.
8. Integrate dynamic data only after the visual preview is approved. Feed pages
   through `watch_ui_frame_t` (or the repository equivalent), preserve
   `normal/degraded/low_battery/no_storage` scenarios, and keep F411 and the
   simulator on the same generated UI contract.

### Visual asset boundary

Concept images are exploratory artifacts. Do not use them as full-screen
backgrounds in the watch; map their hierarchy into lightweight XML objects so
text stays crisp and the UI remains maintainable. Icons should come from an
existing vector/resource system where possible. If a generated raster icon is
truly needed, keep it small, document its dimensions and consumer, and verify
Flash/RAM impact before committing it.

### Right image helper

The standard-library helper is intentionally provider-specific and async:

```sh
python skills/lvgl-xml/scripts/right_image_job.py \
  --prompt "<approved visual brief>" --model gpt-image-2 --n 3 \
  --size 9:16 --image-size 1K --output-dir build/design-evidence/watchface \
  --dry-run
```

After the user approves that exact request, rerun with `--confirm-paid` and a
local key. The script polls `/v1/tasks/{task_id}`, downloads URL or base64
results, writes SHA-256 values, and emits a manifest without secrets. It does
not add Python dependencies. Use `gpt-image-2` for first-pass 1K drafts;
reserve `gpt-image-2-vip` for a user-approved refinement.

### Design evidence and handoff

Use `build/design-evidence/` for untracked concepts, thumbnails, task results,
and confirmation notes. A handoff should identify the approved image, the
corresponding XML screen path, the Editor export timestamp, and simulator
scenario screenshots. Do not commit machine-specific Editor paths, API
responses containing credentials, or unapproved drafts.
