# GitHub Actions

`.github/workflows/ci.yml` runs the same checks as `.gitlab-ci.yml` (see
[gitlab-ci.md](gitlab-ci.md)) for a copy of the repository hosted on GitHub: formatting and
clang-tidy, then a build and the test suite on Linux, Windows and macOS. It runs on every
push, on every pull request, and when started by hand. This page covers turning it on, what
it costs, using your own runners, and how to read the results.

## The jobs

| Job | Runs on | What it does |
|-----|---------|--------------|
| `lint` | `ubuntu-24.04`, in an `ubuntu:24.04` container | clang-format 19 and clang-tidy 19 over `src/` and `tests/` (`ci/lint.sh`): fails on unformatted code, naming that breaks the project's style, or a bug-finding check |
| `linux-gcc` | `ubuntu-24.04`, in an `ubuntu:24.04` container | GCC 14 build with the GUI, all tests, then runs `audioviz_cli` on a generated WAV and starts `audioviz_gui` and `audioviz_player` for 5 s each on a virtual display (Xvfb + Mesa), failing on any OpenGL or GLFW error. There is no sound card, so the player's audio goes to miniaudio's silent output. |
| `linux-clang` | `ubuntu-24.04`, in an `ubuntu:24.04` container | Clang 19 build with the GUI, all tests |
| `linux-headless` | `ubuntu-24.04`, in an `ubuntu:24.04` container | Release build with `AUDIOVIZ_BUILD_GUI=OFF` and only the compiler installed: proves the library, CLI and tests need no windowing packages |
| `windows-msvc` | `windows-latest` | Visual Studio (MSVC) build of everything, all tests |
| `macos` | `macos-latest` (Apple silicon) | Apple Clang build of everything, all tests. Skipped when the variable `AUDIOVIZ_CI_MACOS` is `false`. |

The jobs don't depend on each other, so they run in parallel. The Linux jobs run inside a
plain `ubuntu:24.04` container rather than directly on GitHub's Ubuntu image, which comes
with many packages already installed. In the container they get only the packages the
README lists, as the GitLab jobs do, so a package missing from the README makes the
build fail.

As on GitLab, the Linux and macOS builds use `-DAUDIOVIZ_WARNINGS_AS_ERRORS=ON`, so a new
compiler warning fails them. Windows will be added once a run shows that MSVC builds without
warnings. A newer push to the same branch or pull request cancels the run it replaces.

## Turning it on

The workflow starts as soon as `.github/workflows/ci.yml` is pushed to GitHub. If nothing
appears under the repository's **Actions** tab:

- Check **Settings > Actions > General > Actions permissions**. It must allow actions,
  including the GitHub-made `actions/checkout` and `actions/upload-artifact`.
- On a fork, open the **Actions** tab once and click **I understand my workflows, go ahead and
  enable them**. GitHub turns workflows off in new forks.

To start a run by hand, go to **Actions > CI > Run workflow** and pick a branch.

## Variables

Set these under **Settings > Secrets and variables > Actions > Variables**.

| Variable | Default | Meaning |
|----------|---------|---------|
| `AUDIOVIZ_CI_MACOS` | unset (the job runs) | Set to `false` to skip the macOS job, for example to save minutes in a private repository. |

## What it costs

Public repositories pay nothing for GitHub's standard hosted runners. Private repositories
use the account's included Actions minutes, and Windows and macOS minutes use up the
allowance faster than Linux ones (GitHub's billing page has the current rates). Under
**Settings > Billing and plans** you can see the minutes used and set a spending limit. To
save minutes, set `AUDIOVIZ_CI_MACOS` to `false`.

## Your own runners

For a job to run on your own machine, register it as a self-hosted runner. Go to
**Settings > Actions > Runners > New self-hosted runner**, pick the OS and run the commands
it shows. Then change that job's `runs-on` in `ci.yml` to `self-hosted` plus the runner's
labels, for example `runs-on: [self-hosted, windows]`.

What each platform's runner machine needs:

- **Linux**: Docker, because the jobs use `container: ubuntu:24.04` and install their own
  packages. The user the runner service runs as must be allowed to use Docker.
- **Windows**: Visual Studio 2022 17.7+ with the C++ tools, and CMake on `PATH` (the hosted
  image has both). The Build Tools `winget` command in
  [gitlab-ci.md](gitlab-ci.md#windows-shell-executor-powershell) installs the compiler.
  CMake comes from `winget install Kitware.CMake`.
- **macOS**: Xcode or the Command Line Tools, and Homebrew. The job installs CMake through
  Homebrew if it can't find it.

Don't use self-hosted runners on a public repository: a pull request from anyone could
then run its code on your machine.

## Reading the results

- **Actions tab > a run**: one box per job. Click a job for its log, which is split into
  steps. The failing step opens automatically.
- **Pull requests**: each job is listed as a check at the bottom of the pull request. To
  stop a pull request being merged while a check fails, add the job names (`lint`,
  `linux-gcc` and so on) as required checks under **Settings > Branches > Branch protection
  rules**, or in a ruleset.
- **Test results**: GitHub has no built-in page that lists individual tests, unlike
  GitLab's **Tests** tab. The **Test** step's log shows each failing GoogleTest case and its
  message. Each job also uploads its `junit.xml`, which you can download under
  **Artifacts** at the bottom of the run's summary page.
- **`linux-gcc`, step *Run the programs***: the CLI's output for the generated chirp, then
  the viewer's and the player's smoke tests. Each ends in
  `OK: ran 5 s without OpenGL or GLFW errors` when it passes. When one fails, the lines just
  above say why: a `[GL HIGH]` message from the driver, a `GLFW error`, or the program
  exiting early.
- **`lint` fails**: the log names each file and line. "code should be clang-formatted" is
  fixed by running clang-format on that file. Anything else is a clang-tidy check, whose
  name is in brackets at the end of the line. The README's *Code style* section shows how
  to run the same check locally.
- **Configure fails with "standard library is missing C++23 features"**: the runner's
  compiler is too old. See the compiler table in the README.

To repeat a Linux job on your own machine, use the Docker commands at the end of
[gitlab-ci.md](gitlab-ci.md). The two CI systems run the same steps in the same image.
