# F411 UI Simulator

This is the M7 host consumer for the UI exported from the local LVGL Pro
Editor project. It uses the repository's pinned LVGL 9.5.0 sources and the
same `products/f411_watch/ui` generated C that is kept for the firmware path.

Configure and run the deterministic smoke test with:

```text
cmake -S products/f411_watch/simulator -B build/host-m7 -G Ninja
cmake --build build/host-m7
ctest --test-dir build/host-m7 --output-on-failure
```

On Windows, running `build/host-m7/watch_ui_simulator.exe` opens a 240x280
LVGL window. Pass `--smoke` to run the headless CTest path instead. The
simulator does not parse XML at runtime; XML is the Editor source and the
committed generated C is the build input.
