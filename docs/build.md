# Build notes

Real Events is currently written as a 32-bit Windows DLL.

## Current build assumptions

- Visual Studio / MSVC
- Win32 / x86 target
- Release configuration for public builds
- Existing project precompiled header setup (`pch.h`)
- MSVC x86 inline assembly support

The exported entry point required by the modloader is:

```cpp
extern "C" __declspec(dllexport) int start()
```

`DllMain` calls `DisableThreadLibraryCalls`.

## Public build

Leave:

```cpp
#define REAL_EVENTS_DEBUG_TOOLS 0
```

The resulting DLL should be named:

```text
RealEvents.dll
```

## Development build

Set:

```cpp
#define REAL_EVENTS_DEBUG_TOOLS 1
```

This enables the F10/F11/debug-key development controls.

## Release checklist

Before attaching a DLL to a public release:

- Compile Release / Win32.
- Start a new campaign.
- Load an existing campaign.
- Confirm at least one crisis.
- Confirm at least one boom.
- Confirm town production scaling.
- Confirm private/player production scaling.
- Confirm AI production scaling.
- Confirm state persistence after restart.
- Confirm Informer vanilla rumours remain intact.
- Confirm exactly one Real Events rumour is appended.
- Open the debug window repeatedly in a debug build to verify no regression of the format-argument crash.
- Run for an extended fast-forward period.
