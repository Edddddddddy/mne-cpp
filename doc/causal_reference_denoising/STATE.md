# Causal Reference Denoising State

Last updated: 2026-08-06T00:52:00+08:00

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

The engineering-first processor milestone is implemented and focused GREEN.
The concrete plugin-private processor maps good REF_MEG/MEG rows, disarms old
state on every invalid or missing layout, and resets/relearns on valid layout
changes. The populated Release focused executable exits zero for the mapping,
19 invalid/disarm cases, and valid reconfigure/reset/relearn behavior. The
completed reusable Luna/max test conversation is archived. Independent
Sol/ultra review `R-PROC-001` completed at exact `96ca3eb44`: P0/P1 are zero,
one P2 ownership-policy finding blocks issue #4, and two P3 evidence/locality
items are tracked as nonblocking follow-up.

## Completed

- Verified the old feature branch differed from `master` only by seven planning documents.
- Deleted local branch `codex/realtime-adaptive-denoising`.
- Deleted the ignored old-task build directory `build-adaptive-denoise/`.
- Preserved `.codex-build/`, `src/build/`, `doc/mne_scan_technical_document.md`, and `codex/babymeg-stability`.
- Created `codex/causal-reference-denoising` from local `master`.
- Selected the concrete deep-module interface in `SPEC.md`.

## Running workers

- `W-PROC-MOVE-001` is integrated as `4a57c3ca5`. Populated Release compiled
  the five C++14 traits and the focused Qt report is 5 passed, zero failed/
  skipped, exit zero. P2 `R-PROC-MOVE-001` is closed; issue #4 evidence and
  final-QA P3 deferral are published/read back. Worker exact content/clean
  precheck passes and archival is next before issue #4 closure.

## Next actions

1. Archive the completed one-shot
   Luna/max conversation without manually deleting its app worktree.
2. Close issue #4 and update epic #2 after publication/archival evidence is
   durable.
3. Begin issue #5 with a Luna/max queue tracer, followed by a Sol/ultra
   concurrency implementation and independent review.
4. Deliver the engineering MVP first: processor, nonblocking queue, plugin
   shell and minimal controls. Do not spend further time tuning algorithm
   quality now that the existing effect gates are GREEN.

## Focused verification targets

- `test_causal_reference_denoiser`
- `test_adaptive_denoising_plugin`
- `ex_causal_reference_denoising`

Do not run `mne_rt_server`.
