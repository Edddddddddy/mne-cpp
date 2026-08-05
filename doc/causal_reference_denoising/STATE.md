# Causal Reference Denoising State

Last updated: 2026-08-06T02:10:00+08:00

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
P2 ownership is fixed/verified, and two P3 evidence/locality items are tracked
in final QA. GitHub issue #4 is closed and epic #2 marks it complete.

## Completed

- Verified the old feature branch differed from `master` only by seven planning documents.
- Deleted local branch `codex/realtime-adaptive-denoising`.
- Deleted the ignored old-task build directory `build-adaptive-denoise/`.
- Preserved `.codex-build/`, `src/build/`, `doc/mne_scan_technical_document.md`, and `codex/babymeg-stability`.
- Created `codex/causal-reference-denoising` from local `master`.
- Selected the concrete deep-module interface in `SPEC.md`.
- Completed/closed issue #4 processor mapping, focused tests and formal review.

## Running workers

- `W-QUEUE-TEST-001` is integrated as `b3ff9afc1`. Populated Release build
  reaches the intended C1083 missing `adaptivedenoisingblockqueue.h`; valid RED
  is accepted and published. Worker clean/content precheck passes; archival
  completed without manual worktree deletion. `W-QUEUE-GREEN-001` was created
  as a new visible Sol/ultra implementation task from exact base `930480bf3`;
  setup ID is `client-new-thread:e551ef42-5b43-48d0-a0e1-ddb5863854d4`.

## Next actions

1. Await only the proactive `RESPONSE W-QUEUE-GREEN-001`; do not poll or edit
   the authorized queue/CMake scope concurrently.
2. On response, first record/push it, then inspect the exact commit and run the
   populated focused Release target before integrating.
3. Deliver the engineering MVP first: processor, nonblocking queue, plugin
   shell and minimal controls. Do not spend further time tuning algorithm
   quality now that the existing effect gates are GREEN.

## Focused verification targets

- `test_causal_reference_denoiser`
- `test_adaptive_denoising_plugin`
- `ex_causal_reference_denoising`

Do not run `mne_rt_server`.
