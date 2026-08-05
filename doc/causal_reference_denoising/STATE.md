# Causal Reference Denoising State

Last updated: 2026-08-06T03:10:00+08:00

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
  Its proactive response returned exact-base commit `039b58d1b`; manager
  provenance/diff/interface review passes. It is integrated as `73f05da14`;
  populated focused Release reports 6/0/0 with process exit zero.
- `W-QUEUE-TEST-002` proactively returned exact-base Luna/max commit
  `da315d81e`, adding one focused public lifecycle slot for AlreadyRunning,
  bounded stop wake, idempotent stop, stopped statuses and fresh reconfigure.
  Manager review passes; it is integrated as `90f5cdbf3`. Populated Release
  reports 7/0/0, process exit zero and a 63 ms stop wake versus the 3000 ms
  natural timeout. GREEN is published at
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5194971349`.

## Next actions

1. Dispatch independent Sol/ultra queue concurrency/realtime review from the
   current exact integration snapshot. Retain the implementation conversation
   for a narrow fix until that gate passes.
2. Create the already-durable `W-DOC-001` visible Luna/max task while example
   work remains non-overlapping; do not poll either task.
3. Run a separate Luna/max `W-BENCH-001` task that edits only the focused
   example main to add the required BabyMEG-scale Release timing mode.

`W-QUEUE-TEST-002` completed from exact base `f77bf44ce` as worker commit
`da315d81e`; its one-shot conversation may be reused for the immediately
adjacent queue-validation slice only.

`W-QUEUE-TEST-003` was dispatched to that retained Luna/max conversation
`019fd2e0-188f-7a62-a89c-7ceefa15f6cd` from exact base `701f5bed9`.
Manager will not poll or overlap its focused test-source edit. Public dispatch:
`https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5195009989`.
It proactively returned exact-base commit `cff55a01e`, adding one focused slot
for invalid configuration/block/destination state preservation. Manager review
passes; it is integrated as `422c5264c`. Populated Release reports 8/0/0 and
exit zero, including a 62 ms stop wake. GREEN is published at
`https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5195069942`.

Non-overlapping issue #7 work is prepared as `W-EXAMPLE-001`: a focused
Eigen/streaming example target that directly exercises the completed numerical
interface without FIFF, full mne_scan or mne_rt_server.
Its corrected Luna/max setup was accepted from exact base `473d9eeed` as
`client-new-thread:2e3283a1-8aa7-4872-a44f-c36a8a686808`.
Dispatch: `https://github.com/Edddddddddy/mne-cpp/issues/7#issuecomment-5194881491`.

`W-DOC-001` is prepared as a separate Luna/max task adding one learning guide;
it is disjoint from active queue-test and example files. Its visible worktree
setup was accepted from exact base `fa832d51d` as
`client-new-thread:7c8c1a8c-3952-4f9e-9229-d9f7257a2ccb`.
Dispatch: `https://github.com/Edddddddddy/mne-cpp/issues/7#issuecomment-5195139614`.
`W-EXAMPLE-001` proactively returned exact-base Luna/max commit `708db5425`
from visible thread `019fd2e5-43ca-79c3-822c-20ef795608f1`; it adds only the
registered focused example CMake/main files. Manager provenance/dependency/
behavior review passes; it is integrated as `26ca3b73e`. Populated Release
build/run exits zero with `example invariants: PASS`; learning reaches model
generation four, freeze emits zero updates and reset restores generation zero.
GREEN is published at
`https://github.com/Edddddddddy/mne-cpp/issues/7#issuecomment-5195038508`.
Queue-test and example worker scoped blobs exactly match integration; both
one-shot conversations were app-archived without worktree deletion.

`W-BENCH-001` visible Luna/max setup was accepted from exact base `60b394ab2`
as `client-new-thread:70b6de3a-9f69-407d-b46d-2d17ac2da778`.

`R-QUEUE-001` visible Sol/ultra review setup was accepted from exact snapshot
`3d7328683` as `client-new-thread:1030c8cd-96dd-4ad6-a650-927118f3d03f`.
Manager will not poll; reviewer must proactively return its structured gate.
Dispatch: `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5195120967`.
4. Deliver the engineering MVP first: processor, nonblocking queue, plugin
   shell and minimal controls. Do not spend further time tuning algorithm
   quality now that the existing effect gates are GREEN.

## Focused verification targets

- `test_causal_reference_denoiser`
- `test_adaptive_denoising_plugin`
- `ex_causal_reference_denoising`

Do not run `mne_rt_server`.

The user requested uninterrupted management progress through engineering MVP
acceptance. Continue across milestones without waiting for routine approval;
stop only for an authority-expanding blocker or final acceptance handoff.
