# LAB 1898 — QA and release checklist

The prototype should not be called release-ready until all of these checks pass on the actual target DAWs and Windows machines.

## Functional

- [ ] VST3 scan succeeds in FL Studio on Windows 10 and 11.
- [ ] Mono and stereo sessions instantiate correctly; unsupported layouts are rejected safely.
- [ ] Host automation moves every parameter and state recall restores every parameter.
- [ ] Both style selectors remain synchronized and all eight style voicings are audible but restrained.
- [ ] A/B slots capture, recall and survive save/reopen.
- [ ] Reference loading supports representative WAV, AIFF and FLAC files; invalid/silent files report a clear status.
- [ ] Closing the editor while the asynchronous file chooser or reference analysis is active is safe.
- [ ] Bypass and Power produce a sample-identical dry path (apart from the analyzer's read-only metering).

## Audio / DSP

- [ ] Render silence, -90 dBFS tone, -18 dBFS sine, full-scale sine, impulses, pink noise and dense commercial masters.
- [ ] Check for NaN/Inf output, denormals, clicks, DC offset and unexpected channel imbalance.
- [ ] Test at 44.1, 48, 88.2, 96, 176.4 and 192 kHz.
- [ ] Test 32, 64, 128, 256, 512 and 1024 sample host buffers.
- [ ] Verify filter stability at every parameter boundary and during fast automation sweeps.
- [ ] Confirm that the sample-peak restraint is not marketed as a true-peak limiter.
- [ ] Evaluate nonlinear aliasing; add oversampling before any commercial release if needed.
- [ ] Loudness-match bypass and processed renders before subjective comparisons.

## Performance / realtime

- [ ] Profile plugin CPU in FL Studio with one and multiple instances.
- [ ] Confirm no allocations, file access, mutex acquisition or blocking waits in `processBlock()`.
- [ ] Confirm the reference worker exits promptly when the processor is destroyed.
- [ ] Test under high CPU load, low buffer sizes and repeated editor open/close.
- [ ] Run a long-duration soak test (at least 2 hours).

## Product polish

- [ ] Accessibility and keyboard focus review.
- [ ] Retina / Windows scaling review at 100%, 125%, 150% and 200%.
- [ ] Replace prototype font assumptions with licensed bundled font resources if desired.
- [ ] Add proper loudness metering (EBU R128 / BS.1770) and oversampled true-peak metering before displaying LUFS/TP labels.
- [ ] Run independent listening tests on multiple genres and playback systems.
