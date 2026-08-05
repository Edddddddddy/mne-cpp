# Causal Reference Denoising State

Last updated: 2026-08-05T20:54:23+08:00

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
- Engineering child issues:
  - `#4` processor mapping and focused tests;
  - `#5` nonblocking queue and worker lifecycle;
  - `#6` plugin shell and minimal teaching controls;
  - `#7` example, benchmark and Eigen guide;
  - `#3` final QA and review closure.

## Current phase

The Eigen numerical core is complete and reviewed. Release reports 44 passes,
zero failures and one intentional Debug-only guard skip; Debug reports 45
passes, zero failures/skips. The measured synthetic result is 58.6541 dB noise
reduction and 0.000113195 clean projection-amplitude error. Formal review has
P0/P1/P2 zero open; two P3 integration items remain tracked.

The corrected engineering-first execution plan is approved. The manager
conversation uses Sol/ultra and does not create internal subagents. Execution
now begins by creating five GitHub child issues under epic #2; the first code
slice will then be dispatched as a minimal REQUEST to a new user-visible
Luna/max work conversation. Dedicated user-visible Sol/ultra review
conversations inspect milestone commits. The manager does not continuously
poll workers; workers return a structured RESPONSE to the manager when done.

## Completed

- Verified the old feature branch differed from `master` only by seven planning documents.
- Deleted local branch `codex/realtime-adaptive-denoising`.
- Deleted the ignored old-task build directory `build-adaptive-denoise/`.
- Preserved `.codex-build/`, `src/build/`, `doc/mne_scan_technical_document.md`, and `codex/babymeg-stability`.
- Created `codex/causal-reference-denoising` from local `master`.
- Selected the concrete deep-module interface in `SPEC.md`.

## Running workers

- None executing. Visible Luna/max `W-PROC-GREEN-001`, thread
  `019fd258-04db-7982-b1f6-c7010727c03c`, proactively returned clean three-file
  commit `46b600f910` from exact base `9a99e80c3`. Response is durable; manager
  exact diff review and populated focused GREEN validation are pending.

## Next actions

1. Review exact worker commit `46b600f910`, then run the populated focused
   target before integration.
2. After GREEN, add the next invalid/missing-layout tracer
   before widening processor behavior.
3. After issue #4 reaches a milestone commit, open a separate visible
   Sol/ultra review conversation before integration.
4. Deliver the engineering MVP first: processor, nonblocking queue, plugin
   shell and minimal controls. Do not spend further time tuning algorithm
   quality now that the existing effect gates are GREEN.

## Focused verification targets

- `test_causal_reference_denoiser`
- `test_adaptive_denoising_plugin`
- `ex_causal_reference_denoising`

Do not run `mne_rt_server`.
