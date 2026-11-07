# Project structure

- `Source/Plugin/PluginProcessor.*` — JUCE plugin lifecycle, APVTS parameters, state, A/B snapshots, reference worker coordination and FFT/meter publication.
- `Source/Plugin/PluginEditor.*` — editor layout, parameter attachments, file selection and timer-driven UI refresh.
- `Source/DSP/Engine.*` — realtime filters, saturation, width, dynamics, output and smoothing.
- `Source/Analysis/ReferenceAnalyzer.*` — bounded WAV/AIFF/FLAC analysis on a background thread.
- `Source/UI/NeumorphicLookAndFeel.*` — custom rotary, vertical fader and button rendering.
- `Assets/Reference/` — user-provided visual reference for the interface.
- `.github/workflows/windows-build.yml` — Windows CI build and VST3 artifact upload.
