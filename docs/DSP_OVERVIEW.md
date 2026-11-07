# DSP overview — v0.1.0

The current signal path is deliberately understandable and bounded:

1. Three-band tone stage: low shelf near 105 Hz, bell near 900 Hz, high shelf near 8.5 kHz.
2. Style voicing: a small fixed low/mid/high tilt and, for selected styles, a restrained drive offset.
3. Nonlinear color: biasable tanh saturation with an added harmonic layer; Analog toggles the saturation blend.
4. Mid/side width: leaves the mono component unchanged and scales the side component with a bounded width ratio.
5. Linked envelope compressor: peak-like detector, fixed attack/release envelope, intensity-dependent ratio and punch-dependent threshold.
6. Output gain, sample-peak soft-knee restraint and dry/wet blend.

This is not a clone of an analog circuit and it does not claim to recreate any named hardware. The parameter names are product-level controls, not promises of exact circuit topology.

## Current metering and reference profile

- Spectrum display: 2048-sample Hann-windowed FFT, 64 log-spaced bands, updated once per FFT frame. FFT storage, window coefficients and log-band bin boundaries are precomputed in `prepareToPlay`; no FFT buffer allocation or window trigonometry occurs in the callback.
- Output meter: smoothed block RMS and decaying sample peak in dBFS. It is not integrated LUFS and not true peak.
- Reference profile: first 45 seconds of the chosen file; approximate unweighted RMS/peak and low (<120 Hz), mid (120 Hz–6 kHz), high (>6 kHz) energy shares. Decoding and analysis run on a single background worker.
- Auto Match: bounded broad tonal correction and conservative intensity/mix starting values. It does not time-align references, match phase, stereo correlation, loudness, crest factor or dynamic spectral balance.

## Planned high-value upgrades

- Oversampled nonlinear path with selectable 2x/4x/8x quality modes and CPU benchmarks.
- Proper EBU R128 / ITU-R BS.1770 loudness and oversampled true-peak metering.
- Reference matching based on perceptually weighted, time-averaged spectral descriptors, loudness-normalized comparisons and stereo/dynamics descriptors.
- More precise crossover or multiband dynamics with controllable attack/release and gain reduction meter.
- Automated regression renders and null/impulse/sweep tests.
