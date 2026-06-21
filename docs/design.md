# Design Notes

## Scope

The project follows the reduced kinematic scope for CENG 428 Character Animation Modeling. The plugin does not compute forces, torques, rigid-body dynamics, or contact physics. It controls motion by producing joint angles for the Nathan human model inside N8RO.

## Runtime Integration

The DLL is an ASTSIM simulation plugin. During initialization it registers animation evaluators on `animationModelNathanHuman` through the N8RO model factory registry.

The demo scenario uses `resources/missions/student_motion_sequence.lua`. That script cycles four animation codes:

| Script Code | Submitted Motion |
|-------------|------------------|
| `Idle Neutral` | Walk |
| `Idle Breathing` | Run |
| `Idle Shake` | Jump |
| `Idle Stopped` | Crouch |

The plugin also registers custom student codes for clarity:

| Custom Code | Submitted Motion |
|-------------|------------------|
| `Student Walk` | Walk |
| `Student Run` | Run |
| `Student Jump` | Jump |
| `Student Crouch` | Crouch |

Manual hotkeys can override the script at any time:

| Key | Motion |
|-----|--------|
| `1` | Walk |
| `2` | Run |
| `3` | Jump |
| `4` | Crouch |
| `0` | Return to scenario-controlled sequence |

## Joint Set

Each state drives these ten major joints:

- `leftShoulder`, `rightShoulder`
- `leftElbow`, `rightElbow`
- `leftHip`, `rightHip`
- `leftKnee`, `rightKnee`
- `leftAnkle`, `rightAnkle`

The evaluator writes parent-relative Euler joint angles in radians to `AnimationModelOutput`. Values are clamped before output, and `clearExistingJointOverrides` is set so stale poses do not accumulate between frames.

Elbow and knee angles are intentionally varied over time. Knees carry the lower-body gait, jump compression, and crouch depth changes on the leg hinge axis. Elbows use the Nathan model's flexion axis and change with arm swing, jump takeoff, and crouch balance so the upper body is not held as a static pose.

## Motion Model

### Walk

The walk state is an in-place gait. The left and right hips oscillate in opposite phase. Knees bend only during the backward/stance part of the leg cycle, and ankles counter the knee motion. Shoulders and elbows swing opposite the legs to make the gait readable.

### Run

The run state uses the same structure as walk but with higher frequency, larger hip amplitude, deeper knee bends, and stronger shoulder/elbow motion. This makes the state visibly distinct from walking while staying within stable joint limits.

### Jump

The jump state uses a repeating compression and extension cycle. During compression both hips and knees bend together and the elbows bend with the preparation. During extension the legs straighten and the arms lift forward, creating a recognizable two-foot jump motion without moving the root entity.

### Crouch

The crouch state holds a deep squat with small breathing-like variation. Hips and knees bend together while the ankles compensate. Elbows also pulse slightly with the balance pose so the upper body remains active but controlled.

## Viewer Mode

The GLB viewer should use `Cancel animation: ON`. This prevents the stock Nathan walking animation from being blended on top of the submitted joint overrides. With the plugin active, the viewer should show that all ten configured joints are applied.

## Mission Flow

`Student220201007MotionDemo` contains one Nathan human entity and points its mission component to `resources/missions/student_motion_sequence.lua`. The script advances through Walk, Run, Jump, and Crouch every four seconds. The same four motions can also be forced manually with number keys while recording the demonstration video.

## Root Motion

The final implementation intentionally keeps root/entity translation out of the DLL. The latest assignment scope asks for meaningful kinematic joint-angle states, not full locomotion physics. The visible demo therefore focuses on four coherent body motions rather than world-space navigation.
