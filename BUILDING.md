# Build notes

This repository intentionally uses CMake FetchContent rather than committing JUCE sources into the project. The first configure requires network access and may take several minutes while JUCE is fetched and its helper tools are built.

For Visual Studio 2022/2026 on Windows:

```powershell
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target LAB1898StyleMaster_VST3
```

For a quick local iteration build, replace `Release` with `Debug`, but use Release for audio performance checks. Do not commit the `build/` directory.
