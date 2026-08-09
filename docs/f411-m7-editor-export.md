# M7 Editor Export Contract

M7 uses the LVGL Pro Editor project format without the Pro CLI. The committed
project is `products/f411_watch/ui`, based on the local UI-only template and
fixed to LVGL `9.5.0` with a `240x280` display.

## Source and generated files

- `project.xml` and `screens/**/*.xml` are the Editor-maintained source.
- `*_gen.c` and `*_gen.h` are the generated C interface consumed by the host
  simulator and future firmware integration.
- `CMakeLists.txt`, `file_list_gen.cmake`, and `user_config.cmake` keep the
  generated-file boundary compatible with an Editor export.
- The first screen intentionally uses only `lv_obj` and `lv_label`, matching
  the small F411 `lv_conf.h` component set.

The initial checked-in screen is a small Editor-compatible export fixture based
on the user's UI-only project. When the screen is edited in the Editor, export
the generated C again and review the resulting `*_gen.c`/`*_gen.h` diff. Do not
commit Editor account data, licenses, tokens, or local installation paths.

## Generated-output rule

`*_gen.c`, `*_gen.h`, generated file lists, and generated CMake files are owned
by the LVGL Pro Editor export. They must never be hand-edited. Make the change
in XML, run the Editor Code/export action, and review the resulting generated
diff. If the Editor cannot export, leave the XML change pending and do not
synthesize or patch an equivalent generated file manually.

## Build and test

The simulator uses the LVGL 9.5.0 source already committed under the F411
third-party boundary; it does not fetch LVGL and does not run the CLI.

```text
cmake -S products/f411_watch/simulator -B build/host-m7 -G Ninja
cmake --build build/host-m7
ctest --test-dir build/host-m7 --output-on-failure
```

On Windows, running `build/host-m7/watch_ui_simulator.exe` opens the static
240x280 LVGL window. The `--smoke` argument is the deterministic headless path
used by CTest.

## M7 boundary

M7 proves the XML-to-generated-C boundary, the 240x280 host rendering target,
and a smoke consumer of `watch_core`. It does not add XML parsing to F411,
replace the existing F411 UI task, bind generated widgets to page lifecycle,
or add real hardware input. Those are M8 and later concerns.
