# Causal Reference Denoising State

Last updated: 2026-08-05T19:10:00+08:00

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

Core numerical contracts, effective Debug Eigen malloc guard and quantitative
synthetic acceptance are GREEN. Release reports 43 passes, zero failures and
one intentional guard skip, with 58.6541 dB noise reduction and 0.000113195
clean projection-amplitude error. Formal independent core review is next.

## Completed

- Verified the old feature branch differed from `master` only by seven planning documents.
- Deleted local branch `codex/realtime-adaptive-denoising`.
- Deleted the ignored old-task build directory `build-adaptive-denoise/`.
- Preserved `.codex-build/`, `src/build/`, `doc/mne_scan_technical_document.md`, and `codex/babymeg-stability`.
- Created `codex/causal-reference-denoising` from local `master`.
- Selected the concrete deep-module interface in `SPEC.md`.

## Running workers

- `/root/r_core_001`: Sol/ultra independent formal core reviewer, read-only in
  `C:/Users/lcy/Desktop/meg/mne-cpp-review-r-core-001` at `f0a0e14d7`; no
  edits or subagents.

## Next actions

1. Receive and durably record `R-CORE-001` findings.
2. Resolve any P0-P2 finding.
3. Begin the plugin-data TDD slice only after the core review gate closes.

## Focused verification targets

- `test_causal_reference_denoiser`
- `test_adaptive_denoising_plugin`
- `ex_causal_reference_denoising`

Do not run `mne_rt_server`.
