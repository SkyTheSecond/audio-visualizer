# shazam

Audio fingerprinting from scratch: FFT, STFT/spectrogram, and an OpenGL viewer.

| Program        | What it does |
|----------------|--------------|
| `shazam_cli`   | `shazam_cli <file.wav> [out.pgm]` prints the file's details and writes its spectrogram as a greyscale image |
| `shazam_gui`   | `shazam_gui <file.wav>` shows the spectrogram in a window, in colour, with zoom |
| `shazam_tests` | the GoogleTest suite, run through `ctest` |

All of them are built on `shazam`, the DSP library, which has no GUI dependencies.

## Requirements

Every dependency (GoogleTest, GLFW, Dear ImGui, GLAD) is vendored in `third_party/`, so all
you install is CMake 3.25+ and a compiler whose standard library has C++23's `<expected>`,
`<format>` and `<print>`:

| Platform | Compiler |
|----------|----------|
| Linux    | GCC 14+, or Clang 19+. Clang 18 only works with `-stdlib=libc++`: with GCC's standard library it has no `std::expected`. |
| macOS    | Apple Clang from a current Xcode or Command Line Tools |
| Windows  | Visual Studio 2022 17.7+ (MSVC), or GCC 14+ from MSYS2 |

If the compiler falls short, `cmake` stops at configure time and says so. After switching
compilers, delete the `build` directory: CMake remembers the first compiler it saw.

The repository's `.envrc` (for direnv) selects `clang`. On Ubuntu 24.04 that is Clang 18,
so there use `CC=gcc-14 CXX=g++-14` or `CC=clang-19 CXX=clang++-19` instead.

## Linux

Install the compiler and the X11/Wayland headers that GLFW compiles against:

```bash
# Ubuntu 24.04 / Debian (this is what CI runs; the default g++ there is 13, too old)
sudo apt install build-essential cmake pkg-config xorg-dev libwayland-dev libxkbcommon-dev wayland-protocols g++-14
# Fedora
sudo dnf install gcc-c++ cmake libXcursor-devel libXi-devel libXinerama-devel libXrandr-devel wayland-devel libxkbcommon-devel wayland-protocols-devel
# Arch
sudo pacman -S --needed base-devel cmake libx11 libxrandr libxinerama libxcursor libxi wayland wayland-protocols libxkbcommon
```

Build and test (on Ubuntu 24.04, prefix the first command with `CC=gcc-14 CXX=g++-14`):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Run:

```bash
./build/src/cli/shazam_cli song.wav spectrogram.pgm
./build/src/gui/shazam_gui song.wav
```

The viewer works on X11 and Wayland desktops alike; GLFW picks whichever is running.

## macOS

```bash
xcode-select --install        # once, if you have no Xcode tools yet
brew install cmake

cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure

./build/src/cli/shazam_cli song.wav spectrogram.pgm
./build/src/gui/shazam_gui song.wav
```

## Windows

**Visual Studio.** Install Visual Studio 2022 (Community is fine) with the
*Desktop development with C++* workload, which includes CMake. Then, in a
**Developer PowerShell for VS 2022** (from the Start menu, so `cmake` and the compiler are
on `PATH`):

```powershell
cmake -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure

.\build\src\cli\Debug\shazam_cli.exe song.wav spectrogram.pgm
.\build\src\gui\Debug\shazam_gui.exe song.wav
```

Visual Studio's generator builds several configurations from one `build` directory, so
the configuration is picked when building (`--config`) and testing (`-C`), and each gets
its own folder. Use `Release` in all three places for an optimised build. Alternatively,
*File > Open > Folder* in Visual Studio opens the project directly through its CMake support.

**MSYS2.** In the *UCRT64* shell:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/src/gui/shazam_gui.exe song.wav
```

File names are passed in the system's legacy code page, so a song whose name uses characters
outside it (Japanese on an English-locale PC, say) will not open. Rename it, or keep it to
plain ASCII.

## Getting a WAV file

The programs read uncompressed WAV (16-bit PCM or 32-bit float). Convert anything else with
ffmpeg (`sudo apt install ffmpeg`, `brew install ffmpeg`, `winget install Gyan.FFmpeg`):

```bash
ffmpeg -i song.mp3 -ac 1 -ar 11025 -c:a pcm_s16le song.wav
```

Mono at 11025 Hz is what the analysis wants, and it keeps the spectrogram small enough for
the GPU: textures are commonly limited to 16384 pixels wide, about 12 minutes of audio at
that rate. Without ffmpeg, `python3 ci/make_test_wav.py chirp.wav` writes a 3 second test
sweep that draws a clean rising line.

To look at the `.pgm` the CLI writes: Preview on macOS, `eog`/`feh`/GIMP on Linux, GIMP or
IrfanView on Windows.

## Build options

| Option | Default | Effect |
|--------|---------|--------|
| `-DSHAZAM_BUILD_GUI=OFF` | `ON` | Build only the library, CLI and tests. No windowing packages needed: for servers and headless CI. |
| `-DCMAKE_BUILD_TYPE=Release` | none | Optimised build (single-config generators). Debug builds are several times slower on long files. |

## OpenGL

The viewer asks for the newest core-profile context the driver offers, trying 4.6, 4.5,
4.3, 4.1 and 3.3 in that order, and prints the one it got. Two features depend on it:

- **Direct State Access** (4.5+) is used to create textures; older contexts use the
  classic bind-to-edit calls.
- **Debug output** (4.3+, Debug builds only) prints driver errors as `[GL ...]` lines.

macOS stops at OpenGL 4.1, so there you get 4.1 without either feature. Linux (Mesa,
NVIDIA) and Windows drivers normally provide 4.5 or 4.6.

## Continuous integration

`.gitlab-ci.yml` builds and tests on Linux (GCC 14, Clang 19, and a headless build),
on Windows with MSVC, and optionally on macOS. The Linux GCC job also runs the CLI and starts
the viewer on a virtual display. Runner setup and the variables you can set are in
[docs/gitlab-ci.md](docs/gitlab-ci.md).
