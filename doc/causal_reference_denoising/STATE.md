# Causal Reference Denoising State

Last updated: 2026-08-05T21:48:00+08:00

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

Core behavior, fixed diagnostics lifecycle and transactional finite-overflow
epoch rejection/recovery are GREEN. Both P1 findings are closed; focused
runtime has 39 passes and zero failures.

## Completed

- Verified the old feature branch differed from `master` only by seven planning documents.
- Deleted local branch `codex/realtime-adaptive-denoising`.
- Deleted the ignored old-task build directory `build-adaptive-denoise/`.
- Preserved `.codex-build/`, `src/build/`, `doc/mne_scan_technical_document.md`, and `codex/babymeg-stability`.
- Created `codex/causal-reference-denoising` from local `master`.
- Selected the concrete deep-module interface in `SPEC.md`.

## Running workers

`W-TEST-CORE-013` returned one-test commit `a5f37e5bd`; manager exact review,
integration and runtime RED reproduction are pending.

## Next actions

1. Review exact commit `a5f37e5bd` and mixed-target oracle.
2. Integrate only the one-slot contract and reproduce runtime RED.
3. Dispatch source fallback implementation only after RED evidence.

## Focused verification targets

- `test_causal_reference_denoiser`
- `test_adaptive_denoising_plugin`
- `ex_causal_reference_denoising`

Do not run `mne_rt_server`.
