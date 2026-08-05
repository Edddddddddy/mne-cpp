# Causal Reference Denoising State

Last updated: 2026-08-05T20:25:00+08:00

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

The Eigen numerical core is complete and reviewed. Release reports 44 passes,
zero failures and one intentional Debug-only guard skip; Debug reports 45
passes, zero failures/skips. The measured synthetic result is 58.6541 dB noise
reduction and 0.000113195 clean projection-amplitude error. Formal review has
P0/P1/P2 zero open; two P3 integration items remain tracked.

Development is paused at the plugin-data boundary while the execution topology
is corrected. The manager conversation must use Sol/ultra and must not create
internal subagents. Remaining code is organized as GitHub child issues first,
then dispatched as minimal REQUESTs to new user-visible work conversations.
Dedicated user-visible review conversations inspect returned commits. The
manager does not continuously poll worker conversations; workers return a
structured RESPONSE to the manager when complete.

## Completed

- Verified the old feature branch differed from `master` only by seven planning documents.
- Deleted local branch `codex/realtime-adaptive-denoising`.
- Deleted the ignored old-task build directory `build-adaptive-denoise/`.
- Preserved `.codex-build/`, `src/build/`, `doc/mne_scan_technical_document.md`, and `codex/babymeg-stability`.
- Created `codex/causal-reference-denoising` from local `master`.
- Selected the concrete deep-module interface in `SPEC.md`.

## Running workers

- None. Internal subagent `/root/w_test_plugin_data_001` failed with a stream
  disconnect before producing code and is superseded by the visible-work-
  conversation policy. Its detached worktree was verified clean at
  `cdc07b283` before retirement.

## Next actions

1. Review the proposed GitHub child-issue split with the user; keep issue #2 as
   the feature epic.
2. Create the agreed child issues and link their dependency/order from #2.
3. Dispatch the first minimal plugin-processor test task to a new visible work
   conversation using Luna/max or Sol as risk requires.
4. Receive its explicit RESPONSE without polling, then open a separate visible
   Sol/ultra review conversation before integration.
5. Deliver the engineering MVP first: processor, nonblocking queue, plugin
   shell and minimal controls. Do not spend further time tuning algorithm
   quality now that the existing effect gates are GREEN.

## Focused verification targets

- `test_causal_reference_denoiser`
- `test_adaptive_denoising_plugin`
- `ex_causal_reference_denoising`

Do not run `mne_rt_server`.
