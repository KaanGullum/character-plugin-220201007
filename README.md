# Student Character Animation Plugin - 220201007

This project builds an N8RO ASTSIM simulation plugin for the Nathan human animation model. The plugin provides four kinematic motion states and drives ten major joints every frame.

## Motion States

| State | Key | Script code | Description |
|-------|-----|-------------|-------------|
| Walk | `1` | `Idle Neutral` | In-place walking gait with changing hip, knee, ankle, shoulder, and elbow angles |
| Run | `2` | `Idle Breathing` | Faster gait with larger knee motion and stronger coordinated elbow/arm swing |
| Jump | `3` | `Idle Shake` | Two-foot jump cycle with knee compression/extension and elbow-supported arm lift |
| Crouch | `4` | `Idle Stopped` | Deep squat pose with coordinated hip, knee, ankle, and elbow balance motion |

Press `0` to release manual keyboard override and follow the scenario script again.

## What The Plugin Does

- Registers animation evaluators on `animationModelNathanHuman`.
- Outputs parent-relative joint angles in radians.
- Overrides exactly ten joints: shoulders, elbows, hips, knees, and ankles.
- Varies both elbow and knee angles over time as part of the motion states.
- Uses kinematic joint control only; it does not compute forces, torques, rigid-body dynamics, or contact physics.
- Clears previous joint overrides before writing the current pose, so each frame is deterministic.

## Build

Open PowerShell and run:

```powershell
$env:N8RO_RELEASE='C:\N8RO'
$env:N8RO_RELEASE_USER_SIM_PLUGINS='C:\N8RO\userPlugins\sim'
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe' C:\N8RO\character-plugin-220201007\student-220201007-motion-plugin.vcxproj /p:Configuration=Release /p:Platform=x64
```

Close N8RO before rebuilding if the plugin DLL is already loaded.

Build output:

```text
C:\N8RO\character-plugin-220201007\bin\release\student-220201007-motion-plugin.dll
```

Runtime plugin path:

```text
C:\N8RO\userPlugins\sim\student-220201007-motion-plugin.dll
```

## Install Demo Files

```powershell
powershell -ExecutionPolicy Bypass -File C:\N8RO\character-plugin-220201007\tools\install_to_n8ro.ps1 -N8roRoot C:\N8RO
```

The installer copies:

- the plugin DLL to `C:\N8RO\userPlugins\sim`
- the scenario to `C:\N8RO\data\db\AstsimSchema\Profiles\Scenario`
- the mission script to `C:\N8RO\data\resources\missions`

## Run In N8RO

1. Start N8RO.
2. Open Scenario Editor and Simulation Control.
3. Load `Student220201007MotionDemo`.
4. Run the simulation.
5. Press `G` and open the GLB viewer.
6. Select `Human - NeutralCivilian_01`.
7. Use keys `1`, `2`, `3`, and `4` to force the four states, or press `0` and let the scenario script cycle them.

Recommended GLB viewer options:

| Setting | Value |
|---------|-------|
| Cancel animation | ON |
| Fast pose | ON |
| Follow sim pose | ON |
| Filtered skin joints | ON |
| Base x sim | ON |
| Slow GLB fallback | OFF |

`Cancel animation` should stay ON so the stock Nathan animation does not overlap the submitted joint overrides.

## Submitted Files

- `student-220201007-motion-plugin.vcxproj`
- `student-220201007-motion-plugin.slnx`
- `src/StudentMotionPlugin.cpp`
- `include/StudentMotionPlugin.h`
- `missions/student_motion_sequence.lua`
- `scenarios/Student220201007MotionDemo.json.gz`
- `docs/design.md`
- `bin/release/student-220201007-motion-plugin.dll`
