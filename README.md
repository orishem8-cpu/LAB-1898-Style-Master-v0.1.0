# LAB 1898 — Style Master

A C++17 / JUCE 8 VST3 + Standalone mastering and tone-shaping prototype. The interface is based on the supplied three-panel neumorphic reference: style/intensity on the left, spectrum/EQ in the center, and color/drive/output controls on the right.

> **Project status: engineering prototype (v0.1.0).** This is working source code and a buildable project scaffold, not a finished commercial mastering product. The DSP is intentionally conservative and transparent. It still needs listening tests, gain-staging validation, plugin validation in FL Studio, and measurement against a defined reference corpus before it can be called release-ready.

## Implemented in this archive

- VST3 and Standalone targets, CMake-based and GitHub Actions Windows build.
- Neumorphic three-panel UI, responsive resizing, orange accent, live 64-band FFT spectrum display.
- Host-automatable parameters for style, intensity, color, harmonics, bass, punch, mid, air, width, drive, output, bias, tone, mix, analog, warmth, bypass, and power.
- DSP: low shelf / bell / high shelf EQ, controlled nonlinear saturation, harmonic layer, linked envelope compression, mid/side width, wet/dry blend, output trim and soft-knee peak restraint.
- Reference file analysis for WAV/AIFF/FLAC on a background worker thread. Analysis is bounded to the first 45 seconds and reports RMS/peak and low/mid/high energy shares.
- Auto Match applies a restrained tonal starting point from the measured band balance; it is not a full perceptual or phase-aware reference-matching system.
- APVTS state persistence and simple A/B snapshot slots.

## Build on Windows 10/11 x64

Prerequisites:

- Visual Studio 2022 or newer with **Desktop development with C++** workload.
- CMake 3.22 or newer.
- Git available in `PATH`.
- Internet access on the first configure: CMake fetches JUCE `8.0.17-security` from the official JUCE repository.

From a Developer PowerShell or terminal:

```powershell
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target LAB1898StyleMaster_VST3
cmake --build build --config Release --target LAB1898StyleMaster_Standalone
```

The VST3 bundle is normally created under:

```text
build/LAB1898StyleMaster_artefacts/Release/VST3/LAB 1898 Style Master.vst3
```

Copy the `.vst3` bundle to your VST3 folder if it is not automatically installed, then rescan plugins in FL Studio. The exact path may vary with the generator and CMake configuration.

## GitHub

1. Create an empty repository on GitHub.
2. Extract this archive and put its contents at the repository root.
3. Commit and push to `main`.
4. Open **Actions**. The included workflow configures and builds the Windows VST3/Standalone targets and publishes the VST3 bundle as an artifact.

Example:

```bash
git init
git add .
git commit -m "Initial LAB 1898 Style Master project"
git branch -M main
git remote add origin https://github.com/YOUR_ACCOUNT/YOUR_REPOSITORY.git
git push -u origin main
```

## Architecture

```text
Source/
  Analysis/   Background reference-file analysis
  DSP/        Realtime audio engine and filters
  Plugin/     JUCE AudioProcessor, parameter/state model, editor
  UI/         Look-and-feel drawing code
```

### Realtime and performance decisions

- The audio callback performs no file I/O, network access, mutex locking, UI work, or dynamic object construction.
- Parameter atomics are cached during processor construction and read with relaxed loads.
- Filter state, FFT input/work arrays, Hann window, log-band bin ranges, and display spectrum atomics are prepared before playback.
- FFT window and band mapping are precomputed outside the audio callback; the FFT runs on the audio thread once per 2048 samples.
- EQ coefficients are recalculated only when their smoothed targets change, not blindly on every audio block.
- Reference-file decoding and analysis run on a single background worker.
- Control smoothing is applied to audible parameters to reduce zipper noise.
- The processor supports mono and stereo layouts and clears any extra output channels defensively.

## Important engineering notes before release

1. **Listen and measure.** Test at 44.1/48/88.2/96/192 kHz, mono/stereo, 32/64/128/512/1024 sample buffers, and with silence, impulses, pink noise, dense masters and hot transients.
2. **True-peak safety.** The output stage is a sample-peak soft-knee restraint, not an oversampled true-peak limiter. Do not advertise true-peak limiting until oversampling and inter-sample peak validation have been added.
3. **Reference matching.** The current profile compares broad low/mid/high energy and RMS/peak only. It does not align loudness, phase, stereo image, dynamics, crest factor or time-varying spectral balance.
4. **A/B.** The initial click on an empty slot captures that slot; subsequent clicks recall its stored state. A/B is currently an in-memory snapshot saved with the plugin state.
5. **DSP review.** The included chain is a starting point. A release version should add automated null tests, sweep/impulse tests, denormal checks, oversampled nonlinear stages, parameter-boundary tests and independent listening review.

## Dependency and licensing

JUCE is fetched from `juce-framework/JUCE` at tag `8.0.17-security`. JUCE remains under its own license terms; review the current JUCE licensing options before distributing a commercial binary. Project code in this repository is offered under the MIT license in `LICENSE`.
