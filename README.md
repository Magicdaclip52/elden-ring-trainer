# ERTrainer for Elden Ring 1.17.2

Open source Windows overlay for Elden Ring App Ver. 1.17.2 (eldenring.exe). ImGui and DirectX 11, C++ / CMake. External process, memory patches, no DLL inject.

Открытый оверлей под Windows для Elden Ring 1.17.2. Отдельное окно, хоткеи, патчи памяти процесса. Исходники в репозитории.

Single player / offline only.

## Features

Survival: god mode, infinite HP / FP / stamina, zero equip load, stealth, status immunity, Torrent HP.

Combat: one shot, damage multiplier, defense multiplier, freeze AI.

Items: drop rate 100, infinite item use.

World: runes, rune multiplier, keep runes on death, time of day, game speed, fly.

Stats: level, vigor, mind, endurance, strength, dexterity, intelligence, faith, arcane, max HP / FP / stamina.

## Run

Start Elden Ring 1.17.2 first, then this exe. If attach fails, run as administrator.

Сначала игру 1.17.2, потом exe. Если не цепляется, запусти от администратора.

## Source

Ready build: [Releases](https://github.com/Magicdaclip52/elden-ring-trainer/releases)

Code is in `src`. You can read it in the browser without building.

- `src/game` attach to eldenring.exe
- `src/memory` read / write
- `src/features` feature list
- `src/ui` ImGui DX11
- `src/hooks` patches
- `src/signatures` AOB

```
src/app          window loop
src/game         process attach, version 1.17.2
src/memory       read write, signatures
src/features     catalog
src/hooks        hooks
src/ui           ImGui DX11
src/worker       background tick
resources        icon, manifest
```

## Build

Windows, Visual Studio 2022, CMake 3.24+.

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Output: `build\bin\Release\ERTrainer129.exe`

Or `build.bat`. Pulls Dear ImGui and nlohmann json.

## Notes

Antivirus may flag OpenProcess and writes into eldenring.exe (PUA / HackTool). Unsigned overlay tools get that. Source is public.

Online play can get you banned. Use at your own risk. Source as is.
