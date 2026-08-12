# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

`workshop` is a toy game engine (C++20) aiming for a modern, parallel-first design. The renderer uses a
DX12/Vulkan deferred bindless architecture. This is a sandbox/learning project, not production software —
prefer clarity and correctness over defensive engineering.

## Build

Building requires generating platform project files via CMake, then compiling.

- Windows: run `engine/tools/scripts/generate_vs2022.bat` or `generate_vs2026.bat` (needs `C:\Program Files\CMake\bin\cmake.exe`).
  This writes project files into `intermediate/vs2022/` or `intermediate/vs2026/`. Open `workshop.sln`/`workshop.slnx`
  in Visual Studio, or build from the CLI with MSBuild, e.g.:
  `MSBuild /m /p:Configuration=Debug intermediate/vs2022/workshop.sln /t:Build`
  Configurations are `Debug`, `Profile`, `Release`.
- Linux: no generate script is checked in; configure directly with CMake (`cmake -S . -B intermediate/linux`) and build
  with the generated Makefiles/Ninja files.
- CI (`.github/workflows/ci.yml`) only builds Windows (`windows-2022`, Release, via `generate_vs2022.bat` + MSBuild),
  but the codebase and CMake are written to support Linux too — avoid introducing Windows-only headers/APIs into
  cross-platform modules (anything outside `*.win32`/`*.dx12`/platform-suffixed folders). See "Cross-platform gotchas" below.
- Output binaries go to `bin/<arch>_<config>/` (e.g. `bin/x64_debug/`); static libs go to `intermediate/libs/<arch>_<config>/`.
- There is no first-party automated test suite (no `tests/` directory, no gtest/catch2 usage outside of vendored
  thirdparty libraries) — verification is "does it build and run".

### Warnings-as-errors

Both platforms build with warnings promoted to errors (`/WX` on Windows, `-Werror` on Linux, set in
`engine/tools/build/platform.win32/win32.cmake` / `platform.linux/linux.cmake`). A stray narrowing conversion or
unused variable will fail the build, not just warn.

## Cross-platform gotchas

- Modules that must compile on both Windows and Linux (e.g. `workshop.render_interface.vulkan`, `workshop.core`)
  cannot pull in Windows-only headers unconditionally. Guard with `#ifdef _WIN32`.
- The DXC shader compiler headers (`engine/source/thirdparty/dxcompiler/`) come in two variants: `dxc_2022_07_18`
  (Windows) and `dxc_2024_07_31_linux` (Linux, selected in `engine/source/thirdparty/dxcompiler/CMakeLists.txt`).
  On Windows, `dxcapi.h` assumes COM basics (`IUnknown`, `HRESULT`, `REFIID`, `CComPtr`, ...) are already declared —
  it does **not** pull them in itself. On Linux it falls back to its own `WinAdapter.h`, which defines all of these
  itself (including a custom `CComPtr`). So Windows-side code using `dxcapi.h`/`CComPtr` must explicitly
  `#include <windows.h>` + `#include <atlbase.h>` behind `#ifdef _WIN32` before including `dxcapi.h`; don't assume
  it "just works" the way it does on Linux.
- `workshop.core.win32` / `workshop.core.linux` / `workshop.core.stub` are alternatives selected by CMake based on
  platform (see `engine/source/CMakeLists.txt`) — cross-platform code must go through the `workshop.core` interfaces,
  not depend on the win32/linux variant directly, unless the module itself is platform-specific (named `*.win32`,
  `*.dx12`, etc).

## Architecture

### Tiered module layout

`engine/source/CMakeLists.txt` lays out modules in explicit tiers (also mirrored in IDE folder structure via
`util_setup_folder_structure(... "engine/tierN/...")` calls in each module's `CMakeLists.txt`):

- **Tier 0** — `workshop.core` (containers, math, reflection, debug/logging, filesystem, etc.) plus one platform
  backend: `workshop.core.win32`, `workshop.core.linux`, or `workshop.core.stub`.
- **Tier 1** — Interface + backend pattern for engine subsystems. Each subsystem has an abstract interface module
  (e.g. `workshop.render_interface`) and one or more concrete backends selected by platform/config in CMake
  (e.g. `workshop.render_interface.dx12` [Windows only], `workshop.render_interface.vulkan` [Windows + Linux],
  `workshop.render_interface.stub` [fallback/headless]). The same pattern repeats for `platform_interface` (`.sdl`),
  `window_interface` (`.sdl`), `input_interface` (`.sdl`), `physics_interface` (`.jolt`). `workshop.renderer` sits
  above `render_interface` as the actual deferred/bindless renderer implementation.
- **Tier 2** — `workshop.engine` (ECS core: `object`/`component`/`system`/`object_manager` in `engine/ecs/`, plus
  `app`, `assets`, `presentation`), `workshop.editor`, `workshop.game_framework` (gameplay-level `components`/`systems`
  built on the Tier 2 ECS).
- **Tier 3** — Games, which live outside `engine/` entirely, under `games/` (see below).

When adding a new subsystem backend, follow the existing interface/backend split rather than special-casing platforms
inside a single module.

### Games

Games are separate from the engine tree: `games/<name>/source/...` with their own `CMakeLists.txt`, added via
`games/CMakeLists.txt` (currently just `games/example`, a minimal app built on the engine). Game/engine assets are
kept as sibling `assets/` directories (`engine/assets/`, `games/example/assets/`).

### Reflection

`workshop.core/reflection/` implements a hand-rolled runtime reflection system (`reflect_class`, `reflect_field`,
`reflect_enum`, `reflect_constraint`) since C++ has no native reflection. Field names prefixed with `m_` have that
prefix stripped for display/serialization purposes. This reflection metadata backs serialization and the editor.

### Assets

Assets are authored as YAML (e.g. `engine/assets/shaders/*.yaml` describe shader effects/techniques/imports) and
processed through `workshop.assets` (`asset_importer` → `asset_cache` → `asset_loader`/`asset_manager`) into
runtime/cached forms.

### Shaders

Shaders are HLSL, compiled via DirectXShaderCompiler (DXC) — to DXIL for the DX12 backend and SPIR-V for the Vulkan
backend (`*_ri_shader_compiler.cpp` in each render_interface backend). Shader source can use the same `db_log`/
`db_warning`/`db_error`/etc. logging macros as regular engine code (see `engine/source/workshop.core/debug/log.h`) —
these are special-cased to also work when compiled as shader code.

### Logging/assert macros

Use `db_verbose/db_log/db_success/db_warning/db_error/db_fatal(source, format, ...)` from
`workshop.core/debug/log.h` rather than raw `printf`/`std::cout`, and `db_assert`/`db_assert_message` for assertions.
`log_source` values scope where a message came from (e.g. `render_interface`).

## Code Style Rules

- Do not add explanatory comments above code except in cases where the behaviour is not apparently from the code itself.
- Code is compiled for both linux and windows, do not add anything to non-platform projects that would break this.
- Changes should not be made in third party code unless absolutely neccessary.
- If changes are made to third party code the line before the change should be the explicit comment: // WS_CHANGE