# Street Fight Dojo

Street Fight Dojo is a tiny Arduboy training toy for practicing classic fighting-game special move inputs.

The goal is not to build a fighting game. The goal is the pocket-dojo moment: enter a motion correctly and see `HADOUKEN!` appear on a 128x64 screen.

## Features

- Main menu with Free Practice and Dojo Challenge
- Character choices: Ryu, Ken, Guile
- Move practice for Hadouken, Shoryuken, Tatsumaki, Sonic Boom, and Flash Kick
- Horizontal arcade-style input history using directional glyphs and A/B labels
- Combo input history resets after 3 seconds without a new button press
- Immediate success feedback with move name and short sound
- Dojo Challenge score and EEPROM-backed high score
- EEPROM-backed moves discovered and secrets found
- One secret code: boss unlock
- ProjectABE-ready build artifact included at `build/arduboy/StreetFightDojo.ino.hex`

## Desktop Testing

Open ProjectABE, then drag this file into the browser window:

```text
build/arduboy/StreetFightDojo.ino.hex
```

ProjectABE:

```text
https://felipemanga.github.io/ProjectABE/
```

## Development

Set up or refresh the local Arduboy toolchain:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup-arduboy.ps1
```

Compile all Arduboy sketches:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\test-arduboy.ps1
```

Build the prototype:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-arduboy.ps1 -Sketch arduboy\StreetFightDojo\StreetFightDojo.ino
```

Upload to an Arduboy-compatible device:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\upload-arduboy.ps1 -Port COMx
```

## Controls

- D-pad: directional inputs
- A/B: attack inputs
- A: confirm menu selection
- B: back from selection screens
- A+B: exit training

For diagonal inputs, press the two directions together, such as Down + Right for the Hadouken diagonal.

## Design Constraints

- Target: Arduboy
- Screen: 128 x 64 pixels
- Tiny codebase
- Immediate input feedback
- No character animation in MVP
- Secrets are actual controller inputs
