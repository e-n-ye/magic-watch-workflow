# Magic Watch Workflow Agent Instructions

This repository is developed through a guarded branch and pull-request workflow. Read this file before changing files.

## Mandatory delivery flow

```text
branch -> code change -> Build + Cppcheck -> commit -> push
       -> pull request -> GitHub Actions (build + analysis)
       -> merge or rework
```

1. Start every change on a non-`main` branch. Keep one focused change per commit.
2. Inspect the existing code and local rules before editing. Do not edit `main` directly.
3. For F411 changes, run the Debug build and Cppcheck target from `firmware/stm32/f411_watch`:

   ```sh
   cmake --preset Debug
   cmake --build --preset Debug
   cmake --build --preset Debug --target cppcheck
   ```

4. Commit only after local validation. Use `<type>:<scope>:<description>`, then push the branch and open a PR targeting `main`.
5. Wait for `CI / CI Gate`. A failed check means rework; do not bypass it.
6. Merge only with `Rebase and merge`. Direct pushes, force pushes, merge commits, and squash merges are not part of this workflow.

Documentation-only changes may skip the F411 job through the path filter, but the PR must still finish with a successful `CI / CI Gate`.

## Repository boundaries

- Do not edit `think.md`.
- Do not directly edit CubeMX `.ioc` files. Regenerate CubeMX output through CubeMX when configuration changes are required.
- Keep CubeMX-generated files inside their generated boundary; add hand-written sources through the existing user CMake entry points.
- Keep `learn/` local and untracked.
- Never commit credentials, local tool paths, or machine-specific device settings.

The complete human-facing procedure is in [CONTRIBUTING.md](CONTRIBUTING.md).
