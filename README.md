# Flight Simulator (C++)

Simple starting point for a flight simulator using SDL2 + OpenGL.

## Controls
- **W / S** : Pitch up / down
- **A / D** : Yaw left / right
- **Q / E** : Roll left / right
- **Up / Down arrows** : Increase / decrease speed
- **Mouse drag** (coming soon) or just keyboard for now

## Build (same as the Mandelbrot project)
```powershell
cd C:\vcpkg
.\vcpkg.exe install sdl2 opengl
```

Then in the project folder:
```powershell
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

The EXE will be in `build\Release\flightsim.exe`.

## Next ideas
- Better terrain
- Aircraft model
- Physics
- Instruments / HUD
- Multiplayer

Created with Grok.
