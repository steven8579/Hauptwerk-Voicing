# Hauptwerk Voicing App — Project Handoff

This document is the authoritative spec for starting work on the Hauptwerk Voicing app. It captures the full context so a fresh Claude Code session can pick up without re-deriving decisions.

---

## 1. What this app is

A measurement-driven voicing tool for Hauptwerk sample sets, used in a live church acoustic. It replaces the current workflow of *Audacity → export WAV → import to REW → analyze spectrograph → manually translate to Hauptwerk voicing parameters*.

The app owns the full pipeline: audio capture from a calibrated measurement chain, SPL metering, FFT/spectrograph analysis, room characterization, and a rules-based voicing engine that recommends the eleven Hauptwerk voicing parameters per stop, per octave, and per note — grounded in a database of organ-builder voicing profiles (E.M. Skinner, Aeolian-Skinner, Casavant).

**Non-goal:** This is not a generic audio measurement tool. It is specifically for pipe-organ voicing in Hauptwerk. Every design choice should be made in service of that.

---

## 2. Context about the user and hardware

The user is a professional church musician and organ teacher with a production Hauptwerk installation. This is not a toy project — the app will be used for real voicing work.

### Two machines, one portable measurement rig, one app installed on both

The Earthworks M50 mic and Millennia HV-3C preamp are **physically moved between Firefaces** depending on the task. The app is installed on **both** machines and each installation handles a different role.

**The Mac Studio M3 installation — "organ machine" role:**

- Mac Studio M3
- 1× RME Fireface UFXIII audio interface (drives the organ's amplification and speakers for Hauptwerk playback)
- Allen Renaissance Quantum Q325 console (MIDI controller for Hauptwerk)
- Used for **room characterization (Phase 2)**: the M50 + HV-3C are temporarily connected to this Fireface's input so the app can generate a swept sine, route it through the same Fireface output that Hauptwerk uses (through the organ amps and speakers into the room), and capture it synchronously on the mic input. Hauptwerk is stopped during this; the app owns the Fireface for the duration of the sweep.
- Also where the TotalMix FX room-EQ preset is ultimately loaded, since this is the Fireface that feeds the speakers.

**The MacBook Pro installation — "measurement machine" role:**

- MacBook Pro
- 1× RME Fireface UFXIII audio interface (a different physical unit from the Mac Studio's)
- Used for **stop voicing, per-note EQ, and ensemble verification (Phase 1, 3, 4+)**: the M50 + HV-3C are connected here, and the app passively captures Hauptwerk playing from the Mac Studio through the room. No signal is generated; the app only listens.

### Why this split is correct (do not try to consolidate)

Room characterization requires **synchronous playback and capture on a single audio device** — otherwise the sweep generator and the capture drift and impulse response derivation fails. That pins Phase 2 to whichever machine owns the Fireface feeding the speakers, which is the Mac Studio.

Voicing work is the opposite: the app must be a silent observer of the full playback chain (Hauptwerk → Mac Studio Fireface → amps → speakers → room → mic). Running it on the MacBook Pro keeps the measurement chain electrically isolated from the playback chain, which matches how measurement-grade acoustic work is normally done.

### Room profile portability

Room profiles captured on the Mac Studio are saved as a portable `.hwroom` file (a bundle containing RT60 per band, measured frequency response, derived EQ filters, the raw swept-sine WAV, and provenance metadata). The user copies the file to the MacBook Pro (AirDrop, iCloud Drive, USB — user's choice; the app does not do any automatic sync). The MacBook Pro installation imports it; the voicing engine reads it as an input when producing recommendations.

### Lockstep versioning between installations

The two installations must always run the **same app version**. This is an explicit design constraint, not a nice-to-have:

- The `.hwroom` bundle format, the SQLite schema, and the voicing rules schema are all stamped with a single app version number. Cross-version imports are refused with a clear error ("This room profile was captured with version X; this installation is version Y. Update both machines to the same version before continuing.").
- On startup, each installation reads its own version and stamps it into the UI's title bar or status area so the user can trivially compare the two.
- When importing a `.hwroom` file, the import step checks the bundle's version against the running app's version and refuses mismatches outright — no silent migration, no "try to read it anyway" fallback. The user is expected to upgrade the other machine and retry.
- There is **no forward/backward compatibility logic**. Not worth the complexity for a two-machine, single-user deployment. If the schema needs to change, both machines update together.
- Ship both machines from the same build artifact — a single Mac Catalyst `.app` bundle that gets copied to both. Don't build separately per machine; the risk of version skew is too easy to introduce.

### Installation mode

Each installation asks once, on first launch: **"Is this machine the organ machine or the measurement machine?"** The answer changes which workflows are enabled in the UI — the measurement machine hides swept-sine generation (since there's nothing useful to play it through from the MacBook Pro), and the organ machine de-emphasizes the voicing wizard. Both installations share the same codebase, same build, same data model; only the UI emphasis changes.

### Measurement position (standardized)

One-third nave position, seated ear height. Applies to both the Phase 2 sweep capture on the Mac Studio side and the Phase 1/3+ voicing captures on the MacBook Pro side.

### Rooms

The app must work in **any** church the user brings the setup into. Room acoustics vary dramatically between buildings, so the app must measure the room it is currently in rather than assume a profile.

The user's **home church — Church of the Resurrection Ascension, Rego Park, Queens** — has been extensively characterized already and serves as a reference dataset for validating the measurement pipeline:

- RT60 ≈ 2.0 s (measured, not nominal)
- 80–100 ft speaker throw from the organ's speakers to the measurement position
- Coffered ceiling retains 2–4 kHz better than a typical church
- Severe 8 kHz air absorption over the throw distance
- Existing room EQ correction is implemented as notch filters in RME TotalMix FX on the **organ machine's** Fireface, derived from REW swept-sine measurements taken with the measurement chain

These numbers are reference values for validation. Do not hardcode them into the app. Every new room starts from zero and must be characterized through the in-app guided room-measurement workflow.

### Current sample set

Sonus Paradisi Skinner Opus 497 (dry samples, relying on the church's natural acoustics).

---

## 3. The voicing methodology this app must support

The user's established per-rank methodology, in order:

1. **Amplitude per rank** across all its octaves, normalized against the Great Diapason 8' as the anchor stop
2. **Global brightness** across the rank
3. **Swell box modulation**
4. **Celeste tuning**
5. **Tremulant regulation**
6. **Ensemble verification** (stop combinations sound correct together)
7. **Per-note EQ** from chromatic-scale recordings
8. **Wind supply modulation** (last)

**Ranks already voiced using this methodology (validation dataset):** Great Diapason 8', Great Bourdon 16', Great Harmonic Flute 8', Erzähler 8', Octave 4', Fifteenth 2', Mixture III–V, Trumpet 8'. The recommendation engine should reproduce roughly the values the user actually chose for these ranks. If it doesn't, the rules are wrong — not the user's ears.

**Accumulated lessons that must be encoded as rules:**

- Transition frequencies for 4' stops must be set above the stop's harmonic zone (one octave above keyboard pitch); for 2' stops, two octaves above
- A-weighted analysis alone is insufficient for 4' stops
- Mixture stops require C-weighted SPL with a nearly-flat target curve
- SPL profile targets carry ±2 dB tolerance
- High-shelf EQ **Hi Boost** functions as a **cut** for sample sets where harmonics are intrinsically stronger than fundamentals
- Room EQ notch filters in TotalMix FX (verified by REW swept sine) resolve pinging notes more effectively than per-note Hauptwerk amplitude reduction

**The eleven Hauptwerk voicing parameters** the app must recommend per octave:

```
Overall:           Tuning (cents), Amplitude (dB), Brightness (dB)
Wind Supply Mod:   Tuning (pct), Amplitude (pct), Brightness (pct)
Swell Boxes Mod:   Amplitude (pct), Harmonics (pct)          [if applicable]
Lo/Hi EQ:          Transition Frequency (kHz), Transition Width (pct), High Frequency Boost (dB)
```

**Hauptwerk octave layout:**

```
Octave 1: C2–B2
Octave 2: C3–B3
Octave 3: C4–B4
Octave 4: C5–B5
Octave 5: C6–B6
Octave 6: C7–B7   (C7 always; C#7–B7 only on extended stops)
Octave 7: C8     (only on extended stops)
```

---

## 4. Architecture decisions (already made — do not re-litigate)

| Decision | Choice | Why |
|---|---|---|
| Platform | macOS only | User is Mac-only forever |
| Deployment | **Installed on both machines** (Mac Studio M3 and MacBook Pro) from a single shared Mac Catalyst build artifact; each installation picks a role (organ / measurement) on first launch; **versions must always match between the two installations** (see §2, "Lockstep versioning") | Phase 2 room characterization requires synchronous playback + capture on the Fireface feeding the speakers; Phase 1/3+ voicing work is passive capture on the separate measurement Fireface. Shared data (`.hwroom` files, rules) crosses between machines, so version skew is unacceptable. |
| UI framework | **.NET MAUI Blazor Hybrid** targeting Mac Catalyst | User wanted "web MVC" feel; Blazor Hybrid gives Razor/component model in a native shell with in-process audio access |
| Audio layer | Native Swift framework wrapping Core Audio / AVAudioEngine, bridged to C# via P/Invoke over a C-compatible interface | Browser audio APIs can't drive dual Fireface UFXIII units at measurement grade; PortAudio works but adds a dependency when Core Audio is right there |
| DSP | **FftSharp** for FFT, **MathNet.Numerics** for general math | Both are well-maintained, pure managed |
| Rendering | **SkiaSharp** for spectrograph/waterfall canvas components | Real-time drawing performance matters for the spectrograph |
| Persistence | **SQLite** via **Microsoft.Data.Sqlite** | Per-session, per-rank, per-note storage with full provenance |
| Sample rate / bit depth | **96 kHz / 24-bit, locked** | Matches Fireface native; gives harmonic headroom for 2' and Mixture stops; app should refuse to run at 48 kHz |
| Voicing rules representation | **YAML** for target curves and numerical parameters per builder/stop; **C# rule classes** (one per stop family, compiled in) for decision logic | Declarative-only is too constraining; a full DSL is over-engineering |
| Rejected: pure ASP.NET Core web app | — | Browser audio APIs can't meet measurement-grade requirements |
| Rejected: Electron / web-only | — | Same reason |
| Rejected: PortAudio | — | Core Audio directly is simpler for Mac-only |

---

## 5. Proposed solution layout

```
HauptwerkVoicing/
├── HauptwerkVoicing.sln
│
├── src/
│   ├── HauptwerkVoicing.App/              # MAUI Blazor Hybrid shell (Mac Catalyst)
│   │   ├── Platforms/MacCatalyst/
│   │   ├── Components/                    # Razor components
│   │   │   ├── Layout/
│   │   │   ├── SplMeter.razor
│   │   │   ├── Spectrograph.razor
│   │   │   ├── CalibrationWizard.razor
│   │   │   └── VoicingPanel.razor
│   │   ├── Pages/
│   │   ├── wwwroot/
│   │   ├── MauiProgram.cs
│   │   └── App.xaml
│   │
│   ├── HauptwerkVoicing.Core/             # Domain, pure C#, no UI, no platform deps
│   │   ├── Audio/                         # Capture session model, buffer types
│   │   ├── Dsp/                           # FFT wrapper, windowing, SPL weighting (A/C/Z)
│   │   ├── Measurement/                   # Swept sine, impulse response, RT60, noise floor
│   │   ├── Voicing/                       # Rules engine, parameter recommender
│   │   │   ├── Rules/                     # One class per stop family
│   │   │   └── Profiles/                  # Loaded from YAML
│   │   ├── Builders/                      # Skinner, AeolianSkinner, Casavant definitions
│   │   ├── Organs/                        # Organ / Division / Rank / Stop / Note models
│   │   └── Persistence/                   # SQLite repositories
│   │
│   ├── HauptwerkVoicing.Audio.Mac/        # C# wrapper around the Swift framework
│   │   └── HauptwerkAudioInterop.cs
│   │
│   └── HauptwerkVoicing.Shared/           # DTOs shared between Core and App
│
├── native/
│   └── HauptwerkAudio/                    # Swift framework (Xcode project)
│       ├── HauptwerkAudio.xcodeproj
│       └── Sources/
│           ├── HauptwerkAudio.swift       # Public C-compatible API
│           ├── DeviceEnumerator.swift
│           ├── CaptureEngine.swift
│           └── SignalGenerator.swift      # Swept sine, pink noise, reference tones
│
├── rules/                                 # YAML voicing profiles (edit without rebuilding)
│   ├── skinner/
│   │   ├── diapason-8.yaml
│   │   ├── bourdon-16.yaml
│   │   └── ...
│   ├── aeolian-skinner/
│   └── casavant/
│
├── tests/
│   ├── HauptwerkVoicing.Core.Tests/       # Unit tests for DSP, rules engine
│   └── HauptwerkVoicing.Parity.Tests/     # REW parity tests (see §7)
│
└── docs/
    ├── HAUPTWERK_VOICING_APP.md           # This file
    ├── rules-authoring-guide.md
    └── native-audio-interface.md
```

---

## 6. Phase 1 scope (build this first, in this order)

The goal of Phase 1 is **measurement parity with REW** on a single stop, using the MacBook Pro installation in "measurement machine" role (passive capture of Hauptwerk playing through the room). Nothing else matters until this holds.

### Phase 1 deliverables

1. **MAUI Blazor Hybrid project** that launches as a Mac Catalyst app
2. **Swift audio framework** (`HauptwerkAudio.framework`) with these functions exposed via a C-compatible interface:
   - `hw_enumerate_devices()` → list of audio devices with per-channel granularity
   - `hw_open_capture(deviceId, channel, sampleRate, bitDepth)` → handle
   - `hw_start_capture(handle, callback)` / `hw_stop_capture(handle)` — delivers interleaved 24-bit PCM in 10 ms frames
   - `hw_close_capture(handle)`
   - `hw_generate_calibration_tone(frequency, amplitude)` — for playback verification
3. **C# interop layer** (`HauptwerkVoicing.Audio.Mac`) wrapping the native framework
4. **Calibration workflow**:
   - Select device and channel (the Fireface UFXIII connected to the MacBook Pro — the measurement interface)
   - Enter preamp model (Millennia HV-3C) and gain setting
   - Import Earthworks M50 correction file (typical format: frequency-dB pairs, one per line)
   - Run 94 dB / 114 dB pistonphone calibration — capture 10 s, verify steady RMS, store dB-to-dBFS calibration offset
   - Capture 20 s ambient noise; store as session noise floor with per-band breakdown
5. **Real-time SPL meter** Razor component with:
   - A, C, Z weighting selectable
   - Fast/Slow/Impulse integration (IEC 61672 time constants)
   - Peak hold with configurable release
   - Numeric display + bar indicator
6. **FFT spectrograph** Razor component (SkiaSharp):
   - Configurable window size (4096 / 8192 / 16384 / 32768 samples)
   - Hann window for general analysis, flat-top window for steady-tone amplitude accuracy (selectable)
   - 50 % overlap, waterfall scrolling display
   - Log frequency axis (20 Hz – 20 kHz), dBFS magnitude axis
   - Peak markers with harmonic identification relative to the fundamental
7. **WAV export** of any capture at 96 kHz / 24-bit
8. **Minimal persistence**: save a capture session with metadata (device, channel, calibration offset, timestamp, stop identifier — hardcoded to "Skinner Opus 497 Great Diapason 8'" for Phase 1) and the WAV file

### Out of scope for Phase 1

- Voicing rules engine
- Swept sine / impulse response / RT60
- Stop database beyond the one hardcoded entry
- Ensemble verification
- Any UI for navigating between stops, divisions, or ranks

### Phase 1 success criterion

Capture Great Diapason 8' C4 in the church. The app's SPL reading must agree with REW's measurement of the same signal within **±0.5 dB**, and identified harmonic peaks must match REW's within **±2 Hz**. If this criterion doesn't hold, the rest of the app is built on sand — stop and fix Phase 1 first.

---

## 7. Measurement parity testing (critical)

There needs to be an automated test harness that validates DSP correctness independently of live captures.

### Required parity tests

1. **Sine wave at known amplitude** — generate a 1 kHz sine at -20 dBFS, run through the SPL chain with a known calibration offset, assert measured SPL is within 0.1 dB of expected
2. **SPL weighting curves** — feed pink noise, verify A-weighted output differs from Z-weighted by the published IEC 61672 values at each band
3. **FFT bin accuracy** — synthesize a tone at 440 Hz + harmonics, verify the FFT identifies peaks at the correct frequencies with amplitudes within 0.1 dB of synthesis values
4. **Cross-check against REW** — ship a small set of WAV files (1 kHz sine, pink noise, a real Diapason 8' capture) alongside the REW measurement JSON export for the same files. CI asserts the app produces matching numbers.

The parity test project (`HauptwerkVoicing.Parity.Tests`) should run in CI on every push. If parity drifts, the build fails.

---

## 8. Later phases (for context — build Phase 1 first, but keep these in mind so Phase 1 decisions don't paint Phase 2+ into a corner)

- **Phase 2 — Guided room characterization, runs on the Mac Studio installation ("organ machine" role).** This is what makes the app portable to any church, not just the one it was developed in. Phase 2 lives on the Mac Studio because swept-sine characterization requires **synchronous generation and capture on a single audio device** — the app plays the sweep out of the Mac Studio's Fireface (through the organ amps and speakers into the room) and captures it back through the M50 + HV-3C connected to that same Fireface's mic input. Running sweep generation and capture on different machines would drift and break impulse response derivation.
  - The user physically moves the M50 + HV-3C from the MacBook Pro's Fireface to the Mac Studio's Fireface for Phase 2 work, then moves it back for voicing work. The app should remind them to do this with a pre-flight checklist.
  - Hauptwerk must be stopped or fully muted during the sweep — the app temporarily owns the Fireface output. The app should detect if Hauptwerk is running and prompt the user to quit it.
  - The guided workflow walks the user through:
    - Mic positioning (one-third nave, seated ear height — explain why, don't enforce)
    - Level check (play a short burst, confirm headroom at both the preamp and the Fireface input)
    - Swept-sine generation at a user-selectable level, played through the Fireface output that feeds the organ amplification
    - Synchronous capture on the mic input — same Fireface, same clock, no drift
    - Impulse response derivation
    - **RT60 per octave band** (250 Hz, 500 Hz, 1 kHz, 2 kHz, 4 kHz, 8 kHz at minimum) with a waterfall decay plot
    - Room frequency response vs. a user-editable target curve
    - Parametric EQ filter generation from the measurement-vs-target delta
    - **TotalMix FX preset export** that loads directly onto the same Mac Studio's TotalMix — this is exactly where the room EQ should live, since this is the Fireface feeding the speakers
  - Persists the full room profile (RT60 per band, measured FR, derived filters, swept-sine WAV, timestamp, notes) as a named room in local SQLite, and provides an **"Export as .hwroom file"** action that produces a single portable bundle the user can AirDrop or iCloud-share to the MacBook Pro installation for voicing work (see §2, "Room profile portability").
  - Every new church starts from zero — the app ships with no assumed room profile and never silently reuses another room's data.
  - At this point the room-side REW replacement is complete.
- **Phase 3** — Voicing rules engine for Skinner Opus 497 Great Diapason 8', running on the MacBook Pro installation. The engine consumes the current room's Phase 2 profile (imported from the Mac Studio as a `.hwroom` file) as an input, so RT60 per band and measured FR shape the recommendations. Validate engine output against the user's existing voicing decisions on already-completed ranks (see §3).
- **Phase 4** — Full stop database (Skinner), per-rank wizard covering the full 8-step methodology, ensemble verification (combination captures with harmonic-reinforcement analysis).
- **Phase 5** — Aeolian-Skinner and Casavant builder profiles; **.mdat import** so existing REW measurements become part of the app's record (the user has already figured out that .mdat files are Java-serialized binary with byte-by-byte float extraction — reuse that work).

---

## 9. Technical notes and gotchas

- **Each installation sees one Fireface at a time** — the app runs on two machines (Mac Studio M3 and MacBook Pro), each with its own single Fireface UFXIII. Neither machine sees the other's interface. Don't design the device picker around multiple interfaces; design it for a single Fireface whose role depends on the installation mode.
- **Same-device sync requirement for Phase 2** — the native Swift framework must be able to open the Fireface for **simultaneous output and input** on the same device handle, with both streams sharing a clock. The swept-sine generation and the capture must come from the same `AVAudioEngine` (or same Core Audio HAL AggregateDevice / unit graph). Do not use separate input and output handles.
- **Installation-mode awareness** — the app stores its role (organ / measurement) in local settings on first launch and adapts the UI. The role also gates which audio features are exposed: measurement-mode installations hide swept-sine generation, organ-mode installations de-emphasize the voicing wizard. Same build, same codebase; different emphasis.
- **Xcode / MacCatalyst workload version mismatch** — the MacCatalyst workload shipped with .NET SDK 10.0.202 (`Microsoft.MacCatalyst.Sdk.net10.0_26.2`, version `26.2.10233`) expects **Xcode 26.3 exactly**. The user currently has Xcode 26.4.1, which is newer. A strict Xcode version check causes the build to fail even though the SDK surfaces are compatible across this patch-level delta.
  - **Fix applied to both `HauptwerkVoicing.App.csproj` and any future MAUI/MacCatalyst project in this solution** — include the following property so the build does not refuse a newer Xcode:
    ```xml
    <PropertyGroup>
      <ValidateXcodeVersion>false</ValidateXcodeVersion>
    </PropertyGroup>
    ```
  - Revisit this workaround when the .NET workloads add official Xcode 26.4+ support. When updating, bump both machines together (see §2 "Lockstep versioning") and remove the property only after confirming the build works without it on both.
  - If the user ever hits an `ValidateXcodeVersion` surprise again for a *major* version gap (e.g., Xcode 27 vs. workload 26.x), do NOT just silence the check — install the matching Xcode alongside, or wait for the workload to catch up. The workaround is only safe for patch/minor-level skew.
- **Version stamping on all exports** — every `.hwroom` bundle, SQLite database, and voicing session export must embed the app's version string. Imports that find a version mismatch refuse immediately with a user-facing message naming both versions. There is no migration path; the fix is always "update the other machine." Implement this as a shared `ArtifactVersion` value in `HauptwerkVoicing.Shared` so Core, persistence, and export code all reference the same constant.
- **Display the app version prominently** — every installation shows its version in the UI (title bar, status area, or About dialog accessible in one click). The user should be able to glance at both machines and see if they match without digging through menus.
- **Mic correction file format** — Earthworks M50 ships with a text file, one `frequency dB` pair per line. Apply as a real-coefficient frequency-domain correction before SPL integration, not after.
- **SPL weighting filters** — implement as IIR biquads per IEC 61672 Annex E, not as magnitude-only frequency-domain weighting. The phase response matters for impulsive sounds (Phase 2 RT60 work).
- **FFT window selection** — default Hann for general spectrograph use; offer flat-top as a mode for accurate amplitude readings on steady tones (the latter is what voicing work mostly needs).
- **Do not resample** — if the hardware is at 96 kHz, all DSP runs at 96 kHz. Never silently resample to 48 kHz or 44.1 kHz.
- **24-bit PCM handling** — treat samples as signed 24-bit integers in 32-bit containers internally; only convert to float at the FFT boundary, and use double precision for SPL integration to avoid rounding drift over long captures.
- **YAML schema for voicing profiles** — design the schema in Phase 3, not now. But keep the profile directory layout from the solution layout above so Phase 3 has a place to land.
- **No cloud services, no telemetry, no analytics** — this is a local tool. Everything stays on the user's Mac.

---

## 10. Getting started checklist for Claude Code

1. Install prerequisites on **both machines**: **.NET 10 SDK** (the user is on SDK 10.0.202 or newer — verify with `dotnet --version`), Xcode command-line tools, Xcode (for building the Swift framework). Same build is deployed to both.
2. **Pin the SDK version with a `global.json` at the repo root** so both machines and every Claude Code session build against the same SDK and can't accidentally drift:
   ```json
   { "sdk": { "version": "10.0.202", "rollForward": "latestFeature" } }
   ```
   Adjust the version to whatever `dotnet --version` reports on the MacBook Pro at repo creation time.
3. `dotnet new maui-blazor -n HauptwerkVoicing.App` — this will produce a project targeting `net10.0-*` TFMs (including `net10.0-maccatalyst`). Restructure to match §5.
4. **Add `<ValidateXcodeVersion>false</ValidateXcodeVersion>`** to the app's `.csproj` (see §9 gotcha on Xcode / MacCatalyst workload mismatch). The user's Xcode (26.4.1) is newer than the workload's expected Xcode (26.3); the SDKs are compatible but the default build-time check refuses the newer Xcode.
5. Build-verify with `dotnet build -f net10.0-maccatalyst` before touching anything else. Do **not** assume the TFM — check the generated `.csproj` first and use whatever it actually targets.
6. Create the `native/HauptwerkAudio/` Xcode project as a Swift framework targeting macOS 14+
7. Define the C-compatible header in the Swift framework (`HauptwerkAudio.h`) — the interop surface from §6.2. For Phase 1, only the capture-side functions are needed; leave stubs for the output-side (Phase 2) functions.
8. Build the SPL meter and spectrograph Razor components against a mocked audio source first, so UI work can proceed in parallel with the native framework
9. Wire up the native framework, replace the mock
10. Implement the calibration workflow
11. Implement the first-launch "installation mode" picker (organ / measurement) and gate Phase 2 UI behind it (Phase 2 UI stays hidden until the phase is implemented, but the mode check is cheap to add now and avoids refactoring later)
12. Build the parity test harness (§7) — this is the gate for Phase 1 completion
13. Install on the MacBook Pro, set role = measurement, run against REW in the church. If parity holds, Phase 1 is done.

---

## 11. Open questions to raise with the user before Phase 2

These are deliberately deferred; do not answer them unilaterally:

1. `.hwroom` bundle format — proposed as a zip containing a manifest JSON (room name, capture timestamp, mic/preamp/device metadata, RT60 per band, derived filter list) plus the raw swept-sine WAV and the derived impulse response WAV. Confirm contents before implementing.
2. What format should voicing sessions export to, so they can be archived or shared? (SQLite dump? A bundled folder with YAML + WAVs?)
3. Should the app drive Hauptwerk directly (MIDI note-on for the note being measured) or is manual playing preferred? MIDI automation would speed per-note EQ work significantly but adds complexity: on the MacBook Pro measurement machine, the MIDI signal has to reach the Mac Studio over the network or a physical cable.
4. TotalMix FX preset format — the user has existing notch filter configurations on the Mac Studio's TotalMix. Phase 2 should import and export in that format; confirm the file format before implementing.

---

*End of handoff document.*
