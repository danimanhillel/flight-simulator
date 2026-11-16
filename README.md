# Flight Simulator (C++)

Simple flight simulator with procedural heightmap terrain (mountain ridges style), height-based coloring matching aerial photos, and basic flight physics.

## Controls
- **A / D** : Ailerons — Roll left / right
- **W / S** : Elevator — Nose up / down
- **Z / X** : Rudder — Yaw left / right
- **+ / -** : Increase / decrease speed

## Terrain
- Procedural fractal heightmap (ridge-like mountains)
- Colors by height: green valleys → brown/green mid → gray rock → light peaks
- Tiled so you can fly forever

## Build
```powershell
cd C:\vcpkg
.\vcpkg.exe install sdl2 opengl
```

Then:
```powershell
git clone https://github.com/danimanhillel/flight-simulator.git
cd flight-simulator
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

EXE: `build\Release\flightsim.exe`

## Notes
- Starts flying forward (positive Z when yaw = 0)
- Simple physics with control rates and mild damping
- Keeps you above the terrain

Created with Grok.
