# Causal Reference Denoising State

Last updated: 2026-08-05T18:45:00+08:00

## Recovery order

After context compaction or a handoff, read this file, then `SPEC.md`,
`WORKERS.md`, `REVIEW.md`, and the tail of `JOURNAL.md` before taking action.

## Repository state

- Integration branch: `codex/causal-reference-denoising`
- Base: local `master` at `a15e27f0a6f0c7022ba5a7b2b8f40e5bcafd35ec`
- Remote history: local `master` and `origin/main` have no common ancestor.
- Pull request: intentionally out of scope; publish the branch and issue only.
- Remote branch: `origin/codex/causal-reference-denoising` published and tracked.
- GitHub issue: `Edddddddddy/mne-cpp#2`.
- Old issue: `Edddddddddy/mne-cpp#1`, closed as superseded by issue #2.

## Current phase

Core numerical contracts and the effective Debug Eigen malloc guard are GREEN.
The focused Debug report has 43 passes, zero failures and zero skips. The
quantitative synthetic acceptance test is now in progress before formal core
review.

## Completed

- Verified the old feature branch differed from `master` only by seven planning documents.
- Deleted local branch `codex/realtime-adaptive-denoising`.
- Deleted the ignored old-task build directory `build-adaptive-denoise/`.
- Preserved `.codex-build/`, `src/build/`, `doc/mne_scan_technical_document.md`, and `codex/babymeg-stability`.
- Created `codex/causal-reference-denoising` from local `master`.
- Selected the concrete deep-module interface in `SPEC.md`.

## Running workers

- `/root/w_test_core_015`: Sol/ultra, one-shot synthetic quantitative
  acceptance test in detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-015` at `ea5870cf2`.

## Next actions

1. Receive and review `W-TEST-CORE-015` without overlapping its test source.
2. Run populated Release/Debug focused validation and publish measured gates.
3. Retire the one-shot worker, then dispatch independent Sol/ultra core review.

## Focused verification targets

- `test_causal_reference_denoiser`
- `test_adaptive_denoising_plugin`
- `ex_causal_reference_denoising`

Do not run `mne_rt_server`.
