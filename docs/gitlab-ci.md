# GitLab CI

`.gitlab-ci.yml` builds the project and runs the test suite on every push, on Linux,
Windows and (if you turn it on) macOS. This page covers which runners the jobs need, how
to use GitLab.com's hosted runners or register your own, and how to read the results.

## The jobs

| Job | Runs on | What it does |
|-----|---------|--------------|
| `linux-gcc` | Docker, `ubuntu:24.04`, untagged | GCC 14 build with the GUI, all tests, then runs `audioviz_cli` on a generated WAV and starts `audioviz_gui` for 5 s on a virtual display (Xvfb + Mesa), failing on any OpenGL or GLFW error |
| `linux-clang` | Docker, `ubuntu:24.04`, untagged | Clang 19 build with the GUI, all tests |
| `linux-headless` | Docker, `ubuntu:24.04`, untagged | Release build with `AUDIOVIZ_BUILD_GUI=OFF` and only the compiler installed: proves the library, CLI and tests need no windowing packages |
| `windows-msvc` | Windows, PowerShell, tag `$WINDOWS_RUNNER_TAG` | Visual Studio (MSVC) build of everything, all tests |
| `macos` | macOS, tag `$MACOS_RUNNER_TAG` | Apple Clang build of everything, all tests. **Only runs when `AUDIOVIZ_CI_MACOS` is `"true"`.** |

All jobs are in one stage, so they run in parallel. Each publishes its test results as a
JUnit report, so every individual GoogleTest case shows up in the pipeline's **Tests** tab.

## Variables

Set these under **Settings > CI/CD > Variables** to change them for the whole project,
or when running a pipeline manually (**Build > Pipelines > Run pipeline**) to change them once.

| Variable | Default | Meaning |
|----------|---------|---------|
| `WINDOWS_RUNNER_TAG` | `saas-windows-medium-amd64` | Tag the Windows job asks for. The default is GitLab.com's hosted Windows runner. |
| `MACOS_RUNNER_TAG` | `saas-macos-medium-m1` | Tag the macOS job asks for. The default is GitLab.com's hosted Apple silicon runner. |
| `MACOS_IMAGE` | `macos-15-xcode-16` | macOS image for GitLab.com's hosted runners. Ignored by your own runner with the shell executor. |
| `AUDIOVIZ_CI_MACOS` | `"false"` | Set to `"true"` to run the macOS job. |

## Option 1: GitLab.com's hosted runners

Nothing to install. Check that **Settings > CI/CD > Runners > Instance runners** is
enabled for the project, then push.

- **Linux** jobs are untagged, so GitLab.com's hosted Linux runners pick them up.
- **Windows** uses the hosted Windows runner the default `WINDOWS_RUNNER_TAG` points at.
- **macOS** hosted runners need a Premium or Ultimate plan. If you have one, set
  `AUDIOVIZ_CI_MACOS` to `"true"`. If `MACOS_IMAGE` is no longer offered, pick a current image
  from GitLab's documentation on hosted macOS runners and set the variable to it.

Hosted runners spend your namespace's compute minutes, and Windows and macOS minutes cost
more than Linux ones. New free accounts may have to verify their identity before hosted
runners will take jobs.

If a job sits in **pending** with "no runners with the right tags", no runner matching
that tag is available to the project: either your plan or instance doesn't offer it, or
it is a self-managed GitLab. Register your own runner for that platform (below) and point
the tag variable at it.

## Option 2: your own runners

The flow is the same on every OS:

1. In GitLab, go to **Settings > CI/CD > Runners > New project runner**.
2. Give it a tag (for example `windows` or `macos`). For a Linux runner, tick
   **Run untagged jobs** instead, because the Linux jobs have no tags.
3. Create it, and copy the token it shows (it starts with `glrt-`).
4. Install GitLab Runner on the machine and register it with that token, as below.
   On a self-managed GitLab, use your instance's address instead of `https://gitlab.com`.
5. For Windows or macOS, set `WINDOWS_RUNNER_TAG` or `MACOS_RUNNER_TAG` to the tag from
   step 2 (and `AUDIOVIZ_CI_MACOS` to `"true"` for macOS).

### Linux (Docker executor)

The Linux jobs ask for the `ubuntu:24.04` image and install their own packages, so the
machine needs only Docker and the runner.

```bash
# Debian / Ubuntu. Docker must already be installed.
curl -L "https://packages.gitlab.com/install/repositories/runner/gitlab-runner/script.deb.sh" | sudo bash
sudo apt install gitlab-runner

sudo gitlab-runner register --url https://gitlab.com --token glrt-XXXXXXXX \
    --executor docker --docker-image ubuntu:24.04
```

The runner starts as a system service after installation. `sudo gitlab-runner status`
checks on it.

### Windows (shell executor, PowerShell)

The job finds Visual Studio itself (through `vswhere`) and uses the CMake that ships with
it, so the machine needs Visual Studio with the C++ tools and nothing else. The free
Build Tools edition is enough:

```powershell
winget install Microsoft.VisualStudio.2022.BuildTools --override "--quiet --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

Then install the runner as a service. In an **administrator** PowerShell:

```powershell
mkdir C:\GitLab-Runner; cd C:\GitLab-Runner
Invoke-WebRequest -Uri "https://gitlab-runner-downloads.s3.amazonaws.com/latest/binaries/gitlab-runner-windows-amd64.exe" -OutFile gitlab-runner.exe
.\gitlab-runner.exe install
.\gitlab-runner.exe start
.\gitlab-runner.exe register --url https://gitlab.com --token glrt-XXXXXXXX --executor shell --shell powershell
```

`--shell powershell` uses the Windows PowerShell built into Windows. If you have PowerShell
7 installed, `--shell pwsh` works too; the job's commands run in either.

### macOS (shell executor)

```bash
xcode-select --install          # the compiler, if you have no Xcode tools yet
brew install cmake gitlab-runner

gitlab-runner register --url https://gitlab.com --token glrt-XXXXXXXX --executor shell
brew services start gitlab-runner
```

The job runs as your user, with your Xcode and Homebrew. Set `AUDIOVIZ_CI_MACOS` to `"true"`
and `MACOS_RUNNER_TAG` to the runner's tag.

## Reading the results

- **Pipeline page**: one box per job. Click a job for its full log.
- **Tests tab** (on the pipeline): every GoogleTest case from every job, with the
  failures first and their messages.
- **`linux-gcc` log, at the end**: the CLI's output for the generated chirp, then the
  viewer's smoke test, which ends in `OK: ran 5 s without OpenGL or GLFW errors` when it
  passes. When it fails, the lines just above say why: a `[GL HIGH]` message from the
  driver, a `GLFW error`, or the program exiting early.
- **Configure fails with "standard library is missing C++23 features"**: the image or
  runner has a compiler that is too old. See the compiler table in the README.

## Running the Linux job on your own machine

With Docker installed, this repeats the `linux-headless` job's steps in a throwaway
container, from the repository root. It builds in `build-docker` so it never touches your
own `build` directory:

```bash
docker run --rm -v "$PWD":/src -w /src ubuntu:24.04 bash -c '
  set -e
  apt-get update -qq
  DEBIAN_FRONTEND=noninteractive apt-get install -y -qq --no-install-recommends build-essential cmake g++-14 > /dev/null
  CC=gcc-14 CXX=g++-14 cmake -B build-docker -DCMAKE_BUILD_TYPE=Release -DAUDIOVIZ_BUILD_GUI=OFF
  cmake --build build-docker -j"$(nproc)"
  ctest --test-dir build-docker --output-on-failure'
```

Delete `build-docker` afterwards; it is owned by root because the container ran as root.
