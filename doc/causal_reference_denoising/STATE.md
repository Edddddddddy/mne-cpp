# Causal Reference Denoising State

Last updated: 2026-08-07T04:16:11+08:00

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
The actual `mne_rtprocessing` Release project-link probe is now reproducibly
blocked by the existing Qt 5.15.2 `qlist/qvector` use of removed MSVC `stdext`
iterators. `R-CORE-LINK-001` is explicitly environment-deferred per issue #3;
vendor Qt is unchanged and focused direct-source evidence remains authoritative.

The engineering-first processor milestone is implemented and focused GREEN.
The concrete plugin-private processor maps good REF_MEG/MEG rows, disarms old
state on every invalid or missing layout, and resets/relearns on valid layout
changes. The populated Release focused executable exits zero for the mapping,
19 invalid/disarm cases, and valid reconfigure/reset/relearn behavior. The
completed reusable Luna/max test conversation is archived. Independent
Sol/ultra review `R-PROC-001` completed at exact `96ca3eb44`: P0/P1 are zero,
P2 ownership is fixed/verified, and two P3 evidence/locality items are tracked
in final QA. GitHub issue #4 is closed and epic #2 marks it complete.
Final-QA boundary evidence is now integrated as `34a346424`: populated Release
reports 14/0/0 with Ready at taps 32, interval 2048, memory 1/300,
regularization 1 and exact P=256. `R-PROC-BOUNDARY-001` is closed. The processor
locality P3 also closes in comment-only `7f4095a3b`, which preserves all
declaration tokens while moving selection/range/ownership/disarm/reset/
exception/hot-path rules beside the interface. No processor P3 remains open.

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
Dispatch: `https://github.com/Edddddddddy/mne-cpp/issues/7#issuecomment-5195193407`.
It proactively returned exact-base commit `79eff3b3a`, modifying only the
focused example main. Manager timing/scope review passes, but P2 benchmark
integrity finding `R-BENCH-FINITE-001` requires checking all 250 target rows,
not only row 16, before integration and populated Release timing. Discussion:
`https://github.com/Edddddddddy/mne-cpp/issues/7#issuecomment-5195280130`.
The narrow revision proactively returned replacement `b8ce427b8f` with the
same exact parent and one-file scope. It reports all-target/all-sample checks
after the timer while leaving the workload and timing boundaries unchanged;
manager review accepted it and it is integrated as `87ab418bd`. Populated
Release default mode and benchmark both exit zero. Benchmark p50/p95/max are
`2.725/4.317/8.557 ms`, generation/accepted/rejected are `1099/1099/0`, and
the strict p95 <128 ms plus all-target/non-target integrity gates pass.

`R-QUEUE-001` visible Sol/ultra review setup was accepted from exact snapshot
`3d7328683` as `client-new-thread:1030c8cd-96dd-4ad6-a650-927118f3d03f`.
Manager will not poll; reviewer must proactively return its structured gate.
Dispatch: `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5195120967`.
4. Deliver the engineering MVP first: processor, nonblocking queue, plugin
   shell and minimal controls. Do not spend further time tuning algorithm
   quality now that the existing effect gates are GREEN.

The plugin ingress seam is now frozen after read-only integration audit: queue
v2 uses native `QSharedPointer<const FiffInfo>`, preallocated maximum dimensions
with per-slot row/sample extents, and plugin-start bounds 512x2048/capacity four.
This removes callback ownership allocation and transports row-count metadata
transitions in FIFO order. A focused RED test and Sol/ultra implementation are
next; the pending original queue review remains an input and a fresh review is
required before plugin lifecycle integration. Earlier discussion:
`https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5195227638`.

`W-QUEUE-V2-TEST-001` proactively returned exact-base Luna/max commit
`649bbd1ee` from parent `b4299fdaf`. It changes only the focused plugin test,
migrates every queue oracle to native `QSharedPointer` plus row/sample extents,
and adds mixed 2/4/3-row FIFO/deep-copy/tail/metadata acceptance. The response
is integrated as `c646e35d2`. The populated Release build reaches the intended
public-contract RED: the old queue has no `maxChannelCount`/`rowCount` and
still accepts `std::shared_ptr` metadata. No unrelated failure masks it. A new
Sol/ultra queue-v2 production task is the next blocking action; no production
queue change has been accepted yet.

Queue-v2 RED is published at
`https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5200141019`.
`W-QUEUE-V2-GREEN-001` is prepared as the next blocking visible Sol/ultra
implementation task. It owns only the existing queue header/source; the
focused RED test is immutable. Manager will record its exact task base after
the durable request commit and will not poll the task. App worktree setup is
accepted as `client-new-thread:97461215-c1b8-488b-90bd-984aad5d9fd3` from exact
base `9b526eb14`.
Public dispatch:
`https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5200762566`.

The compatible-toolchain follow-up found no usable Visual Studio 2022
installation: the directory exists but contains no MSBuild/compiler, and
`vswhere -all -prerelease -products *` lists only Visual Studio 18.8.1 with
MSVC 14.51. `R-CORE-LINK-001` therefore remains environment-deferred without
vendor or toolchain modification.

Read-only plugin scaffold audit confirms the existing AbstractAlgorithm and
RTMSA connector pattern. The frozen plugin implementation will use one explicit
Qt const-cast only to call the legacy non-const output FiffInfo initializer;
metadata remains logically immutable. Plugin source dispatch waits for queue-v2
GREEN so the worker can compile/audit against a real interface instead of an
imagined one.
DirectConnection lifecycle also requires an atomic accepting-input/in-flight
producer guard: stop must quiesce a callback already inside `tryPush` before a
later start replaces the queue PImpl. The acquisition callback itself remains
wait-free with respect to plugin lifecycle/settings synchronization.
Public lifecycle discussion:
`https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5200841976`.

`W-PLUGIN-DATA-001` is prepared as a non-overlapping visible Sol/ultra task:
add the real `scan_adaptivedenoising` target and default-settings data
lifecycle only. It may code against the frozen queue-v2 interface while the
queue worker owns the two queue files; integration remains ordered queue GREEN
first, plugin data second. UI/settings widgets remain a later Luna task.
Its visible worktree setup is accepted as
`client-new-thread:bb93c4e1-6aa9-4ee4-9d52-de30ff1b0489` from exact requested
base `8c51ea4ce`.
Public dispatch:
`https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5200889469`.
The later UI seam is also frozen: one pending snapshot plus revisions is read
once per worker block; fixed diagnostics cross a queued Qt connection and all
human-readable formatting stays on the GUI thread. No UI task is dispatched
until plugin-data integration/review.
Public UI-seam discussion:
`https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5200910135`.

## Focused verification targets

- `test_causal_reference_denoiser`
- `test_adaptive_denoising_plugin`
- `ex_causal_reference_denoising`

Do not run `mne_rt_server`.

The user requested uninterrupted management progress through engineering MVP
acceptance. Continue across milestones without waiting for routine approval;
stop only for an authority-expanding blocker or final acceptance handoff.

The user reconfirmed completed-task cleanup on 2026-08-06: archive one-shot
visible conversations after clean/content/evidence verification; retain only
same-responsibility implementation conversations that may receive review fixes.

## Current checkpoint override

- Integration/remote HEAD: `e4964aaed`; tracked state is synchronized. The
  integration has queue-v2
  plus the compile-RED evidence committed and pushed;
  only the three preserved user paths are untracked.
  only untracked paths are the three preserved user paths listed in the plan.
- Active blocking task: `W-QUEUE-V2-GREEN-001`, Sol/ultra setup
  `client-new-thread:97461215-c1b8-488b-90bd-984aad5d9fd3`, exact requested
  base `9b526eb14`; queue files only. A single recovery read proved its first
  turn produced no assistant output or file change and the worktree remains
  clean at the exact base. The same reusable Sol/ultra conversation returned
  exact-parent two-file implementation commit `90b423e3c` with a clean
  worktree and full concurrency evidence. Manager review/populated GREEN are
  next. It is integrated as `a21e08e00`, but populated Release compilation
  opens P1 `R-QUEUE-QT-SLOTS-001`: private member name `slots` collides with
  Qt's keyword macro and prevents compilation. Return a source-only rename fix
  to the retained Sol/ultra conversation, then rerun the complete target. The
  existing conversation returned exact-parent one-file delta `33b3eea1d`.
  Manager integrated it as `c87762e97`; populated Release now compiles/links
  and the complete focused executable exits zero, effective 15/0/0. P1 closes;
  GREEN is published and a fresh exact-snapshot Sol/ultra formal review is the
  remaining queue gate. GREEN publication:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5201314365`.
  Earlier RED publication:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5201209614`.
  Recovery publication: issue #5 comment
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5201026440`.
- Active non-overlapping task: `W-PLUGIN-DATA-001`, Sol/ultra setup
  `client-new-thread:bb93c4e1-6aa9-4ee4-9d52-de30ff1b0489`, exact requested
  base `8c51ea4ce`; it proactively returned exact-parent seven-file commit
  `5f4718722` with a clean worktree and complete lifecycle evidence. Manager
  pre-integration review accepts the deep adapter/callback/stop/output seam but
  opens P1 `R-PLUGIN-SPSC-PRODUCER-001`: the packed guard currently permits
  more than one simultaneous callback to enter the SPSC queue. Return a one-
  source-file Sol/ultra fix requiring the in-flight count to be zero before
  admission; integration remains ordered after the atomic queue gate.
  Finding/request publication:
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5201795105`.
  The retained visible thread accepted `W-PLUGIN-DATA-001-FIX-1` with
  Sol/ultra on exact worker parent `5f4718722`; manager will not poll it.
  It has proactively returned exact-parent one-source-file delta `34a8096f1`,
  changing only the private admission predicate/comment so only count zero may
  transition to one. Manager exact-delta review passes with no new finding;
  the fix is accepted but plugin integration remains held behind the atomic
  queue gate and later plugin formal review.
- Existing non-overlapping tasks still await proactive responses without
  polling: `W-QA-CORE-CONTRACT-001` and `W-DOC-001`. Recovery found the complete
  one-file learning-guide response/commit `c558acbf8`, now awaiting manager
  content review. Manager review opened P2 `R-DOC-QUEUE-V2-001`: the guide's
  queue/default paragraph still describes the superseded v1 seam. Return a
  narrow amend request to the same Luna/max doc conversation before integration;
  the existing conversation accepted that revision request. Review dispatch is
  public at issue #7 comment
  `https://github.com/Edddddddddy/mne-cpp/issues/7#issuecomment-5201120187`.
  The core-contract turn had no assistant output; its same
  Luna/max conversation accepted one retry requiring a detach to the original
  exact base before the unchanged two-file task.
  It proactively returned two-file commit `2682034d0` from the actual dispatch
  parent `7eb04cbfa766...`; the originally recorded requested full SHA is not a
  local/remote object. Current-compiler review opens P1
  `R-CORE-CXX14-NOEXCEPT-001`: a `noexcept` member-pointer alias is illegal in
  project C++14. The retained Luna/max conversation accepted a test-only
  portable revision request before integration. Final-QA publication:
  `https://github.com/Edddddddddy/mne-cpp/issues/3#issuecomment-5201485839`.
  The retained Luna/max conversation has now proactively returned test-only
  delta `bc061b834` on exact parent `2682034d0`. It removes the C++14-illegal
  noexcept member-pointer alias, retains ownership/reset traits, and adds
  portable MatrixXd-to-Ref convertibility plus public-call result-type checks.
  Worker MSVC 14.51 `/std:c++14` compilation succeeds. Manager provenance,
  exact scope and portable-trait review pass with no finding; integration and
  populated Release/Debug remain required before P1/P3 closure.
- The older read-only `R-QUEUE-001` reviewed the superseded exact-row/std-owner
  v1 queue and is now app-archived after its clean exact snapshot and durable
  dispatch were verified. Queue-v2 receives a fresh formal review after GREEN.
- The old queue-v1 implementation conversation is app-archived at exact clean
  worker commit `039b58d1b` because v2 owns its replacement.
- Fresh queue-v2 formal review `R-QUEUE-V2-001` is now creating a visible
  Sol/ultra worktree from exact GREEN snapshot `e4964aaed`; setup ID is
  `client-new-thread:4b85d632-2892-48c3-8044-846ed7c4ad39`. Dispatch is public:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5201388674`.
- Completed queue-v2 tracer conversation is archived after exact clean/blob/
  RED/public evidence. Do not revive it; create a new Luna/max test task if a
  later independent behavior is required.
- Next implementation order after responses: queue-v2 gate, plugin-data gate,
  Luna/max UI/settings/diagnostics, focused lifecycle coverage where the local
  dependency graph permits, learning-guide reconciliation, final Sol/ultra
  review and issue closure.
- UI implementation is prepared but not dispatched: programmatic QWidget only,
  one fixed diagnostics metatype, narrow plugin control slots, one pending
  revision/reset snapshot copied once per worker block and explicit queued GUI
  formatting. It remains behind queue/plugin data integration.

## Latest response checkpoint

- Accepted core-contract commit `47b54efed` plus portable C++14 delta
  `0848634a3` are integrated. Populated Release and Debug targets both compile/
  link and their complete executables exit zero; the unchanged runtime suite
  retains the prior 44/0/1 Release and 45/0/0 Debug structure, with Debug
  exercising the malloc guard. `R-CORE-CXX14-NOEXCEPT-001` and
  `R-CORE-LOCALITY-001` are closed; only the already environment-deferred real
  library-link P3 remains.
  Closure is public at
  `https://github.com/Edddddddddy/mne-cpp/issues/3#issuecomment-5201673779`.
- Fresh queue-v2 formal review `R-QUEUE-V2-001` proactively returned against
  exact clean `e4964aaed` with gate HOLD: P0=0, P1=1, P2=2, P3=0. P1 finds
  that Windows Qt 5.15.2 `QSemaphore` uses a `QMutex` fallback that can block
  and lazily allocate in the producer path, contradicting the frozen realtime
  contract and making queue noexcept unsafe. P2s require real overlapping SPSC/
  stop/preservation tests and observable native metadata lifetime coverage.
- Next blocking queue action: split a Luna/max public-interface test tracer from
  a Sol/ultra atomic-SPSC/nonallocating-wake implementation. Plugin-data
  integration remains held until all queue P0/P1/P2 findings are closed.
  Formal review is public at
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5201624132`.
  Private production correction is frozen without changing the public seam:
  lock-free SPSC sequence counters plus a configure-created sticky OS wake
  (Windows auto-reset event; POSIX nonblocking pipe). This removes QSemaphore/
  QMutex and condition-variable lost-wake risk from the producer path. Dispatch
  the Sol production task only after the active Luna public tests integrate.
  Design publication:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5201877301`.
- `W-QUEUE-ATOMIC-TEST-001` is prepared from exact base `4bb1b2a30` as a
  one-file Luna/max TDD task. It adds overlapping SPSC/FIFO/drop accounting,
  producer/consumer stop and destination-preservation coverage, pending-block
  discard/fresh-reconfigure coverage, and observable native metadata lifetime.
  Production queue/CMake/plugin remain immutable until this response is
  manager-reviewed and integrated.
  The visible Luna/max worktree setup is accepted as
  `client-new-thread:7119c1cd-0c1c-4632-ade6-edbc357dec0b`; no final thread ID
  exists yet. Manager will publish the dispatch and will not poll it.
  Public dispatch:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5201732759`.
  The task proactively returned exact-parent one-test-file commit `f367fd75a`
  with four new public slots plus complete Timeout/Stopped preservation in the
  existing slots. Worker C++14 syntax passes; manager exact code/oracle review
  passes with no finding. It is integrated as `16053ffe6`; populated Windows
  Release compiles/links and the complete executable exits zero on the gate run
  plus three immediate repeat runs. Atomic production replacement is next.
  Public test evidence:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5202622017`.

- Next blocking request: `W-QUEUE-ATOMIC-GREEN-001`, Sol/ultra, exact source
  base `41a2b76fb`; queue header/source only. Replace QSemaphore with lock-free
  SPSC sequences and configure-created sticky OS wake while preserving the
  public four-method seam and the now-GREEN focused test source.
  App worktree creation is accepted as
  `client-new-thread:3fb21cfc-1d5b-40a1-b6ef-fb57b91db17b`; no final thread ID
  exists yet. Exact dispatch is published and read back at
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5202544531`;
  await proactive response without polling.
  GitHub issue #5 body now matches this frozen atomic contract; authenticated
  read-back confirms the stale QSemaphore deliverable is removed.

## Active checkpoint - supersedes earlier checkpoint sections

- Management/integration branch and remote were synchronized at
  `f5235091aec6d2f4ed178470f61ccc7f84b67f8f` immediately before this record.
  The only untracked paths remain the three preserved user paths:
  `.codex-build/`, `src/build/` and `doc/mne_scan_technical_document.md`.
- Completed and accepted: numerical core, processor, example, benchmark,
  processor/core QA, and queue public concurrency/lifetime test integration
  `16053ffe6` with populated Release plus three repeat zero exits.
- Single blocking production task: `W-QUEUE-ATOMIC-GREEN-001`, visible
  Sol/ultra thread `019fd64b-2553-7102-91d6-1f8d35720823` (setup
  `client-new-thread:3fb21cfc-1d5b-40a1-b6ef-fb57b91db17b`),
  exact source base `41a2b76fb`, queue header/source only. Public dispatch:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5202544531`.
  It proactively returned exact-parent two-file commit `0a8202e405` with clean
  status, atomic sequence/native wake proof, Windows 19/0/0 plus five repeats
  and WSL 19/0/0 plus three repeats. Manager provenance/code/race review passes
  with no finding; acceptance is durable. Canonical populated build remains
  after cherry-pick.
- Atomic production is integrated as `3ff146836`. Canonical populated Release
  compiles/links and the main run plus five repeats exit zero. Qt reporter file
  capture failed twice without changing the process result; no further capture
  retry is allowed. Next blocking task is the separate allocation tracer.
  Public integration evidence:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5202971691`.
  Luna/max setup accepted as
  `client-new-thread:c4ec3fcf-866e-41d9-8c7d-9fd4680dc7f2` from exact
  `9b7b1c779`; test source only. Publish issue #5 dispatch, then await proactive
  response without polling.
  Public dispatch/read-back:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5202951958`.
  A single task-list lookup resolved the final visible thread as
  `019fd670-c791-7000-8a5f-4538396a54d6`, worktree `4954`. The request remains
  the sole active queue gate. It proactively returned exact-parent one-file
  commit `ea81256b1` with the complete C++14 global allocation replacement set
  and producer-only counted boundary. Response is durable; manager provenance,
  source review passes with no finding: exact parent/scope/current-base blob,
  replacement completeness, static initialization and counted-boundary cleanup
  are correct. It is integrated as `34aff76b6`; canonical populated MSVC Release
  compiles/links, its full main run exits zero, and five immediate full repeats
  all exit zero. The hard in-slot assertion therefore proves zero counted
  producer allocations. Qt reporter streams remain empty, so no manager total
  is claimed. Evidence is published at
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5203091442`.
  Retained Sol/ultra reviewer thread `019fd5d8-380d-7563-8644-264426bda199`
  will now re-review exact snapshot `9535bf8c7`; queue issue #5 remains open
  until its P0-P3 response is accepted. The request was delivered with explicit
  Sol/ultra settings. One-shot allocation thread `019fd670-c791-7000-8a5f-4538396a54d6`
  is app-archived after exact commit/integration/canonical evidence; its app
  worktree was not manually removed.
  The reviewer returned HOLD on exact `9535bf8c7`: P0=0, P1=0, P2=2, P3=1.
  The old Windows QSemaphore P1 and native-metadata-lifetime P2 are closed.
  Open blockers are POSIX `write(EINTR)` prompt-wake resilience and regression-
  sensitive forced concurrent reuse/blocked-stop tests; SPEC also retains one
  stale semaphore/unspecified-tail paragraph. Queue/plugin integration remains
  blocked while TDD corrections and a fresh retained-reviewer gate run. Findings
  are public at
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5203364178`.
  Next parallel-safe work: new Luna/max public concurrency RED task owns only
  the focused test; a separate Luna/max SPEC-locality task owns only SPEC. The
  POSIX EINTR fault-injection tracer follows the concurrency-test integration
  because it overlaps the same test/CMake seam; Sol/ultra production correction
  follows that RED.
  App setup accepted `W-QUEUE-CONCURRENCY-TEST-002` as
  `client-new-thread:d045832c-271e-4ea6-8e0c-d6e4e0c0980e` and
  `W-QUEUE-SPEC-001` as
  `client-new-thread:69772d64-aeb0-47af-bacc-a1b154974991`, both Luna/max from
  exact `2530c7864`. Public dispatch/read-back:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5203461409`.
  One read-only task-list lookup resolves final threads: concurrency test
  `019fd69f-9196-7a82-9c3f-941076fb84fa` (worktree `a656`) and SPEC
  `019fd69f-9196-7a82-9c3f-94295c3cbeb5` (worktree `17e6`). Await proactive
  responses without polling/overlap. SPEC thread proactively returned clean
  exact-parent one-file commit `96055bf13`; response is durable before manager
  diff/semantic/search review and integration. Manager review passes: all
  remaining semaphore mentions are legacy/rejection context, live atomic/native
  wake and preservation wording match interfaces, and no declaration/code is
  touched. Integrated/pushed as `c56f19269`; exact one-file diff-check and
  targeted search pass. P3 is manager-closed pending final reviewer confirmation,
  with public evidence at
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5203570808`,
  while concurrency test remains active.
- Accepted but deliberately not integrated until the atomic queue gate:
  plugin data commit `5f4718722` plus exact-parent SPSC admission fix
  `34a8096f1`. Retain its Sol/ultra conversation for same-responsibility fixes.
- Prepared next queue gate: a separate Luna/max producer-thread allocation
  test, then the retained Sol/ultra formal reviewer rechecks P0-P2.
- Learning guide remains held in its retained Luna/max conversation. After its
  queue-v2 response, replacement `fb748082f` on exact parent `fa832d51d`
  correctly fixes adapter defaults and the variable-rectangle/native-metadata
  seam. Exact manager review confirms its queue paragraph still names the
  superseded QSemaphore and it contains no measured benchmark evidence. One
  combined narrow Luna/max revision from `fb748082f` must replace that paragraph
  with the accepted atomic/native-wake seam and add the exact benchmark command,
  workload and `2.725/4.317/8.557 ms` results before integration/issue closure.
  The retained visible thread accepted this exact one-file request with explicit
  Luna/max settings. It proactively returned replacement `6333b995d` with the
  original exact parent `fa832d51d` and complete one-guide scope; no QSemaphore
  remains, atomic/native-wake and exact benchmark evidence are reported. Manager
  provenance/content/command review passes: eight relative links resolve, all
  fences balance, queue wording matches production and timing limitations are
  truthful. It is integrated as `b59221e78`. Canonical Release replay builds the
  documented target; default example exits zero/PASS and benchmark exits zero/
  PASS with current p50/p95/max `2.174/2.748/4.130 ms`. The guide's earlier
  recorded `2.725/4.317/8.557 ms` remains valid and is explicitly load-sensitive.
  Both documentation P2 findings close; publish and close issue #7 next.
  Issue #7 is now CLOSED/COMPLETED with final evidence at
  `https://github.com/Edddddddddy/mne-cpp/issues/7#issuecomment-5203274698`.
  The completed reusable Luna/max documentation thread is app-archived without
  manual worktree deletion. Epic #2 should now mark the documentation milestone
  complete. Independent authenticated read-back now confirms epic #2 already
  contains `[x] #7`; no further epic mutation is needed despite the local CLI
  error outputs from the attempted multi-line edits.
- After queue review passes: integrate/formally review plugin data, implement
  programmatic minimal UI, add focused block-boundary settings/lifecycle
  evidence, run final targets/review, update and close issues #5/#6/#7/#3/#2.

## Active checkpoint - 2026-08-06 late

- Branch/remote include queue SPEC correction `c56f19269`, POSIX EINTR tracer
  preparation `deefd580f`, and the first bounded concurrency-task progress
  record `e2df78dd8`. No production/test source changed in those records; only
  the three preserved user paths remain untracked.
- `W-QUEUE-CONCURRENCY-TEST-002` remains the sole active code worker. One
  cursor-based completion wait reports that its deterministic sustained reuse
  and stop-race gates are implemented; MSVC 14.51 C++14 syntax passes and the
  worker is performing a temporary focused Release link/run. No commit or
  response is accepted yet, and manager does not inspect the worker worktree.
- The next strict order remains: integrate that public concurrency evidence;
  dispatch the prepared WSL `--wrap=write` EINTR RED tracer; send the retained
  Sol/ultra atomic worker the private bounded-recheck fix; run Windows/WSL
  focused tests; obtain a fresh retained Sol/ultra queue PASS; only then
  integrate the held plugin-data commits and begin the Luna/max teaching UI.

### Completion estimate update

- Engineering progress is approximately 75-80%. Numerical core, processor,
  atomic queue, regression-sensitive Windows concurrency/no-allocation tests,
  example, benchmark and learning guide are complete.
- Remaining critical path: deterministic POSIX EINTR tracer and private bounded
  recheck fix; final Sol queue PASS; held plugin-data integration/review; minimal
  teaching UI and block-boundary tests; final focused Release/Debug/example/
  benchmark review and issue closure.
- Estimated continuous work is 3-5 hours if WSL/Qt adds no new blocker. A
  persistent WSL outage may leave Linux runtime evidence explicitly environment-
  deferred, but does not prevent Windows focused engineering acceptance.

## Active checkpoint - POSIX EINTR tracer review

- `W-QUEUE-POSIX-EINTR-TEST-001` returned exact-parent two-file commit
  `c4b13a220`. Manager provenance, GNU/Clang-only link wrapping, one-shot
  thread-local EINTR injection, wrapper-count, payload/preservation and finite
  cleanup review pass.
- P2 `R-QUEUE-EINTR-BLOCKED-WAIT-001` remains before integration: both prompt
  tests currently require only elapsed `>=0` and `<500 ms`. Because the ready
  marker is published before the public `waitPop`, a pre-call scheduling pause
  can let the push/stop happen first and make the lost-wake implementation look
  prompt. The same Luna/max conversation must add `>=5 ms` lower bounds for
  both complete public wait calls in a test-only delta from `c4b13a220`.
- WSL is still unavailable at service startup. No POSIX RED/GREEN runtime is
  claimed until the environment recovers; Windows focused non-regression and
  source-level evidence remain required after integration.

### EINTR tracer integration result

- Tracer and lower-bound commits are integrated as `0d1d1f949` and
  `b6cf3a13b`. Populated MSVC Release compiles/links; the complete Windows-
  guarded suite reports 19/0/0 and three explicit waited repeats exit 0/0/0.
- `R-QUEUE-EINTR-BLOCKED-WAIT-001` is closed by the named 5 ms lower bounds.
  This does not close production finding `R-QUEUE-ATOMIC-POSIX-EINTR-001`:
  Linux runtime remains unavailable and the retained Sol/ultra atomic worker
  must implement the private bounded consumer recheck before final queue review.

### Next blocking production task

- `W-QUEUE-POSIX-EINTR-GREEN-001` uses the retained atomic implementation
  conversation with Sol/ultra from exact pushed base `f1151d134`.
- It may edit only the queue implementation. On POSIX, each native consumer
  `poll` wait is capped at 25 ms while the existing outer steady-clock deadline
  remains authoritative and rechecks `running`/producer sequence after every
  slice. Windows auto-reset event code is unchanged.
- Producer `tryPush` and `stop()` retain exactly one nonblocking signal attempt:
  no EINTR retry, wait, lock, allocation, public hook or interface change. This
  bounds a lost signal to one consumer slice and preserves acquisition latency.

### Completion estimate - 2026-08-07

- Engineering progress is approximately 82-85%. The private POSIX bounded-
  recheck fix is integrated as `f7c22717e` and is awaiting canonical Windows
  validation plus independent queue review.
- Remaining path: queue PASS; held plugin lifecycle integration/review; minimal
  teaching UI and block-boundary tests; final focused Release/Debug/example/
  benchmark and GitHub issue closure.
- Estimated continuous time to acceptance is 2.5-4 hours absent a new Qt/MSVC
  integration defect. Persistent WSL unavailability may explicitly defer the
  forced-EINTR Linux runtime evidence but does not block Windows engineering
  MVP acceptance.

### Queue validation checkpoint

- Private POSIX correction `f7c22717e` is integrated and pushed. Canonical
  Windows MSVC/Qt focused suite is 19/0/0 plus three repeat zero exits.
- Canonical WSL Ubuntu/GCC/Qt direct build uses Linux/GNU MOC guards and
  `-Wl,--wrap=write`; both forced producer/stop EINTR slots are registered and
  the complete suite is 21/0/0. Current Linux SPSC traffic retains zero counted
  producer allocations.
- Production `R-QUEUE-ATOMIC-POSIX-EINTR-001` is manager-addressed with runtime
  evidence. A retained independent Sol/ultra exact-snapshot review is now the
  sole remaining queue gate before integrating plugin lifecycle.

### Queue formal gate PASS

- Independent Sol/ultra review `R-QUEUE-ATOMIC-003-RETRY-1` completed read-only
  at exact clean/detached SHA `5aa43160bbae27560c4f4fe013728a6476013db0`.
- Gate is PASS with P0/P1/P2/P3 all zero. All seven prior queue findings close;
  no deferral remains. Manager Windows 19/0/0 plus repeats and WSL 21/0/0 are
  audited but correctly distinguished from independent source evidence.
- Next: integrate held plugin lifecycle commits `5f4718722` + `34a8096f1`, run
  available focused/static checks and dispatch formal plugin lifecycle review.
  Issue #5 remains open until that worker lifecycle is integrated/reviewed.

### Plugin lifecycle integrated

- Held worker commits integrate conflict-free as `85b40cb9e` and `3099a4292`.
  New target `scan_adaptivedenoising`, concrete plugin shell, DirectConnection
  ingress, queue/worker lifecycle, FIFF mapping/output and corrected zero-to-one
  SPSC admission are now on the integration branch.
- Queue/processor/core files remain unchanged by the plugin series. Next run
  target-local MOC/CMake/static checks, then send the exact snapshot to an
  independent Sol/ultra plugin lifecycle reviewer before UI work.

### Plugin target environment boundary

- Dedicated CMake configure succeeds with only mne_scan enabled and
  `BUILD_MNE_RT_SERVER=OFF`; the `scan_adaptivedenoising` project is generated.
- Building that target stops in existing `mne_fiff` before plugin compilation:
  Qt 5.15.2 `qlist.h` references removed MSVC 14.51 `stdext` checked iterators.
  Vendor Qt remains unchanged. Standalone plugin MOC and ingress audit pass.
- Treat real target compile/link as the already documented toolchain environment
  deferral. Proceed with exact-source Sol/ultra lifecycle review and focused
  processor/queue evidence; do not run the full app/server.

### Plugin lifecycle review active

- Independent visible Sol/ultra review `R-PLUGIN-DATA-001` is creating a clean
  worktree from exact pushed snapshot
  `fd33dc9cd1a2ce90a30fdedd1ffde71c8514ec56`; setup ID is
  `client-new-thread:3696dfeb-1800-4b9a-a8cf-a0d70d7eb2f0`.
- The review is read-only and owns the plugin data/lifecycle gate. Teaching UI
  is intentionally the next milestone and its absence is not a finding here.
- Await the proactive structured response without continuous polling. UI work
  remains held until this gate returns PASS or its findings are corrected.

### Plugin lifecycle review HOLD

- Independent `R-PLUGIN-DATA-001` completed read-only at exact clean SHA
  `fd33dc9cd1a2ce90a30fdedd1ffde71c8514ec56`: P0=0, P1=0, P2=4, P3=1,
  recommendation HOLD. The prior SPSC-producer P1 is confirmed closed.
- Immediate source corrections are admission-overlap drop accounting, C++14
  lock-free guards for callback atomics, and static plugin link/import wiring.
- Terminal destructor waiting and the complete real-plugin lifecycle harness are
  deferred candidates: returning from destruction with a live QThread/callback
  is unsafe, while the current Qt/MSVC baseline cannot build the real target.
  They require an explicit follow-up issue and final-QA/#6 linkage before gate
  retry; no silent deferral is allowed.
- Teaching UI remains held. After the three immediate fixes plus durable
  deferral, rerun an independent Sol/ultra lifecycle gate. Only PASS permits
  dispatching `W-PLUGIN-UI-TEST-001`.
- Follow-up issue #8 now durably owns terminal teardown and the supported
  real-plugin lifecycle harness. Issue #3 and #6 comments link the deferral;
  both findings remain explicit residual work rather than being marked fixed.
- Active fixes from exact pushed base `48672b02eb35de300a046079028e96ec6d1b25b6`:
  the spent admission thread is archived after zero-mutation setup failure;
  replacement Sol/ultra setup
  `client-new-thread:58ed8491-eb34-43fc-8382-7d6a691c81ce` owns only admission/
  drop/atomic/destructor-contract source changes. Luna/max setup
  `client-new-thread:8153b471-f290-4f15-8d7a-ac87cfeee593` owns only static
  mne_scan CMake/main registration. Their files are disjoint.
- Static registration is integrated as `78d443144`: static mne_scan now links
  `scan_adaptivedenoising` and imports `AdaptiveDenoising`, with no shared-build
  or qrc change. Admission/drop/atomic source correction remains active.
- Admission/drop/atomic worker returns exact-parent two-file commit
  `2b571453da200af02ee437bbcac0a545021786cb`; manager review/integration is next.
  Worker MOC and MSVC 14.29 C++14 syntax checks pass; no lifecycle runtime claim.
- Manager direct state-machine/diff review accepts the response and integrates
  it conflict-free as `6ab7326ac`. Populated MOC/source/compile verification and
  fresh Sol/ultra lifecycle review remain before the gate can pass.
- Populated verification now passes: Qt MOC, deterministic callback oracle and
  MSVC 14.29 C++14 `/Zs` on the exact integrated source. Immediate code findings
  are addressed; the lifecycle gate now needs a fresh exact-snapshot review of
  these fixes plus the explicit issue #8 deferrals.
- Retained independent Sol/ultra reviewer is now re-auditing exact pushed SHA
  `3dca8628eb43029ea35980e869da1a44d21b053a` as `R-PLUGIN-DATA-002`.
  The dispatch is published and read back at
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5208534174`.
  One bounded wait shows the review active: the reviewer confirms the intended
  three immediate closures/two issue-#8 deferrals and is checking exact current
  lines plus unchanged queue/processor contracts. UI remains blocked until its
  proactive PASS/HOLD response; do not poll again.
- Read-only UI preflight confirms the blank current `setupWidget()` and a
  FIFF-free focused Widgets/Test seam. Exact control/label object names and
  typed widget signals are now frozen in SPEC; no UI code is dispatched before
  lifecycle PASS. Issues #2/#3/#5/#6/#8 remain open without checkbox changes.
- `R-PLUGIN-DATA-002` returns PASS at exact clean/detached SHA
  `3dca8628eb43029ea35980e869da1a44d21b053a`: new P0/P1/P2/P3 are all zero.
  The SPSC producer, Busy drop accounting, atomic lock-free and static
  registration findings are closed. Destructor boundedness and the real-plugin
  lifecycle harness remain explicitly deferred, not fixed, under open issue #8.
  The teaching UI gate is now released. PASS is published on issues #5/#6;
  issue #5 is CLOSED/COMPLETED and epic #2 checks #5. The next blocking task is
  `W-PLUGIN-UI-TEST-001`, a new visible Luna/max TDD RED tracer from exact code
  base `4cec85cbe6ed53bb07e5ebdf8f6de96675e7c204`.
- App worktree setup for `W-PLUGIN-UI-TEST-001` is accepted as
  `client-new-thread:978a2856-8c82-4b58-b5f3-8d11562ac0cc` on local host. Await
  Final visible thread is `019fd892-ff55-7903-ab3e-25890ccb10b5`; dispatch is
  published at
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5208612705`.
  Await the proactive response without polling; production UI files remain
  untouched.
- Current canonical Release replay after the lifecycle gate is GREEN:
  numerical core `45/0/0`, processor/queue `19/0/0`, teaching example PASS and
  benchmark PASS at p50/p95/max `2.227/2.643/3.787 ms`. No full app/server is
  used. These are the current final-QA baseline while UI work proceeds.
- Current numerical Debug configuration also rebuilds and reports `45/0/0`;
  chunk differences remain zero and quantitative values remain 58.6541 dB /
  0.000113195. The project intentionally places the Debug executable in the
  shared `out/Release/apps` path, which the corrected harness follows.
- With issue #5 and both formal gates complete, the spent atomic implementation,
  queue review and plugin lifecycle review conversations are app-archived. The
  only active code task is UI tracer thread
  `019fd892-ff55-7903-ab3e-25890ccb10b5`; app worktrees are not manually deleted.
- One bounded wait shows the UI tracer active and still performing its mandated
  recovery/contract read. It has not reported RED, a commit or a blocker. Do not
  poll again; await its proactive response while preserving the three-file scope.
- `W-PLUGIN-UI-TEST-001` now returns exact-parent, exact-three-path commit
  `57310185f31d5095471d6664271148ad8f56f830`. It covers frozen controls/signals,
  fixed diagnostics and 14 labels with a Qt/Widgets/Test/Eigen-only target.
  Isolated CMake is blocked by the known absent Eigen baseline before target
  generation, so no compile RED/GREEN is claimed. Manager provenance/diff
  review, integration and populated intended-RED reproduction are next.
- Manager accepts and integrates the tracer as `c5eef427d`. Populated CMake
  reaches the intended RED: generation fails only because
  `adaptivedenoisingsetupwidget.cpp` is absent. The Eigen baseline configures,
  so no unrelated dependency masks the test. Next blocking task is a separate
  Luna/max `W-PLUGIN-UI-GREEN-001` production implementation from this pushed
  RED base; the tracer remains immutable.
- RED evidence is published at
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5208691454`;
  the completed Luna/max tracer task is archived and its local-only base ref is
  deleted. TDD GREEN is now split: Luna/max owns diagnostics/widget/CMake only;
  a later Sol/ultra task owns plugin h/cpp block-boundary wiring.
- `W-PLUGIN-UI-GREEN-001` app setup is accepted as
  `client-new-thread:b4a1fa59-623e-40cf-b6a8-f00057a9b97e` from exact RED base
  `7da5d498d113f9adc710dd426f2270619142f4da`, using Luna/max. Await proactive
  response; immutable plugin h/cpp and tracer files must remain untouched.
- Final visible widget thread is `019fd8a2-5324-7342-818b-7836cd1a0379` in app
  worktree `0443`; dispatch is published at
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5208713549`.
- One bounded wait shows Luna/max active after completing contract recovery and
  beginning convention/source inspection; there is no commit, blocker or input
  request yet. Do not poll again. The later Sol/ultra two-source wiring request
  is prepared below but remains undispatched until widget GREEN is integrated.
- Later event waits show the exact four-file implementation in place with clean
  diff; the worker is attempting only an isolated focused UI build and will
  fall back to MOC/C++14 probes if its app worktree lacks a usable generated
  build. No commit/final response is accepted yet; stop status polling now.
- Isolated configure confirms only the known missing Eigen baseline; worker is
  finishing direct Qt MOC/C++14 checks. A single noninterrupting finalize
  request is prepared: commit/respond after those checks without new validation
  branches or scope expansion.
- `W-PLUGIN-UI-GREEN-001` returns exact-parent four-file commit
  `aa7ca542c702596c19de1f87931819f29e0362c7`. Widget/diagnostics/CMake behavior,
  MOC and C++14 syntax are reported complete; isolated Eigen prevents an
  executable claim. Manager exact diff review, integration and populated
  focused GREEN are next before Sol wiring dispatch.

### Teaching widget focused gate GREEN

- Manager exact-parent/scope/interface review accepts worker commit
  `aa7ca542c702596c19de1f87931819f29e0362c7`; conflict-free integration is
  `56fcc5a5ef8453ef6f360edcb965b790719d62ee` and changes only the plugin CMake,
  fixed diagnostics header and setup-widget header/source.
- Populated Release CMake builds and links `test_adaptive_denoising_ui`.
  Two offscreen executions both report `3 passed, 0 failed/skipped/blacklisted`;
  the explicit rerun exits zero in 7 ms. Qt's missing optional font-directory
  warnings do not affect the tested control/signal/diagnostics contract.
- The next blocking task is `W-PLUGIN-UI-WIRE-001`: a new visible Sol/ultra
  worker editing only `adaptivedenoising.h/.cpp` for the pending UI snapshot,
  worker-block-boundary configuration/reset/mode order and queued diagnostics.
- Widget GREEN is published at
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5208840455`.
  The completed Luna/max widget task is app-archived and the merged local-only
  `codex/worker-ui-green-base` ref is deleted after detached/merged proof.
- `W-PLUGIN-UI-WIRE-001` is durably requested from exact pushed code base
  `60fdf633f83b41bb2a960c1b83ab3d6c5d608b5a`; create a new visible Sol/ultra
  app worktree only after this request record is committed and pushed.
- Sol/ultra setup is accepted as
  `client-new-thread:0b5832f3-60f5-4d75-8cad-24ee7dba22fc`; the visible task is
  `019fd8b9-bf4c-7ad2-a647-786634476d30` in app worktree `9355`, renamed
  `W-PLUGIN-UI-WIRE-001`. Await its proactive response without polling.
- Dispatch is public at
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5208859667`.
  One bounded event wait times out with the worker active in recovery-path
  discovery; no blocker, commit or input request exists. Do not poll again.
- Issue #6's body is reconciled to the current lifecycle PASS, widget GREEN,
  active Sol wiring task and unchanged Qt/MSVC environment boundary. The issue
  remains OPEN and all goal/contract/acceptance text is preserved.
- Manager freezes the pre-wiring acquisition callback segment at 1218 characters
  with SHA-256 `eaf16dc13bc5036da3e6d33f901885622b2337e018c8baf5a504a34e57d3b0a4`;
  it contains one metadata `info()` and one `tryPush`, with no mutex/wait token.
- Current widget-GREEN/pre-wiring focused Release baseline is fully GREEN:
  numerical `45/0/0`, processor/queue `19/0/0`, UI `3/0/0`, example PASS and
  benchmark PASS at p50/p95/max `2.163/2.767/3.683 ms` with 1099 accepted and
  zero rejected epochs. This is the comparison baseline for the wiring commit.
- A compatible real-target path is now proven: VS18 with `-T v142` selects
  installed MSVC 19.29/14.29, configures mne_scan with `mne_rt_server` disabled,
  and compiles/links the current real `scan_adaptivedenoising.dll` plus
  `mne_rtprocessing`, FIFF, Qt and scan-library dependencies. The MSVC14.51/Qt
  failure remains a default-toolset boundary, but no longer blocks compatible-
  toolchain target evidence. Repeat after final wiring before closing QA.
- Pre-wiring compatible-link evidence is published at QA issue #3 comment
  `https://github.com/Edddddddddy/mne-cpp/issues/3#issuecomment-5208984447`
  and cross-linked from plugin issue #6 comment
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5208984539`;
  both issues remain OPEN pending exact final wiring/review.
- A later 60-second event wait after the focused/link work shows the Sol wiring
  worker active with edited translation unit and both MOC probes passing; it is
  running only the planned MSVC14.29 C++14 syntax check. A noninterrupting
  finalize request limits the remainder to that check, exact proofs and response.
- `W-PLUGIN-UI-WIRE-001` proactively returns exact-parent, exact-two-file commit
  `3b9aa6b1edc604729942c1f21f339f8cfbc14acf`. It reports the frozen pending-
  snapshot/block-order/queued-diagnostics seam, plugin/widget MOC, MSVC14.29
  C++14 syntax and immutable UI `3/0/0` GREEN. Manager provenance/diff/semantic
  review and integration are next; no response claim is yet relabeled manager
  evidence.
- Manager exact-scope/deep-seam review accepts the response with no P0-P3
  finding and integrates it as
  `36fa8058915840a5317debf1bb0201f0781fd249`. Parent/child acquisition callback
  segments are byte-equal under the same reconstruction, with one info/push and
  zero UI/mutex/wait tokens; block order is snapshot/configure/reset/mode/process.
- Exact integrated v142 real target builds and links
  `scan_adaptivedenoising.dll`. Final focused Release is core `45/0/0`,
  processor/queue `19/0/0`, UI `3/0/0`, example PASS and benchmark PASS at
  p50/p95/max `2.174/2.992/4.684 ms`; Debug numerical is `45/0/0`, chunk
  differences zero and quantitative result `58.6541 dB / 0.000113195`.
- Next blocking gate: persist/push/publish this integration, archive the completed
  wiring worker and dispatch a new independent visible Sol/ultra UI review.
- Exact integration evidence is published at issue #6 comment
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5209032275`
  and final-QA #3 comment
  `https://github.com/Edddddddddy/mne-cpp/issues/3#issuecomment-5209032379`;
  both stay OPEN for formal review.
- Completed Sol/ultra wiring task is app-archived and its merged local-only base
  ref is deleted after detached/merged proof. New read-only `R-PLUGIN-UI-001`
  is durably requested from exact pushed snapshot
  `7a33d537b403c2be0d7ee04bcb49f376c83673b7`.
- Review setup is accepted as
  `client-new-thread:7beffd28-3d95-4504-9a9d-8ecbafd1240e`; visible read-only
  Sol/ultra task `019fd8d8-c239-7941-b57b-ce57c8f55269` runs in app worktree
  `c70c`, titled `R-PLUGIN-UI-001`. Publish dispatch, then await proactive gate.
- Exact review dispatch is published at
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5209050760`;
  issue #6 remains OPEN and no final checkbox/closure occurs before PASS.
- One 60-second event wait times out with reviewer active and following the
  mandated recovery sequence; STATE/SPEC are complete and WORKERS is next. It
  reports no finding, blocker or input request. Do not poll again.
- Follow-up issue #8 remains OPEN and now explicitly includes runtime regression
  for UI settings/freeze/reset application only at dequeued block boundaries and
  fixed queued diagnostics. This makes the real-plugin test deferral complete;
  no current finding is relabeled fixed.
- #8's environment rationale is also current: default MSVC14.51/Qt remains
  blocked, compatible v142 real target now links, and that compile/link evidence
  is explicitly not the missing lifecycle-runtime harness.
- Durable issue-#8 reconciliation is committed/pushed as `e6e507736`. Final
  read-only UI reviewer `R-PLUGIN-UI-001` remains active at exact snapshot
  `7a33d537b`; its latest bounded event report confirms the intended deep
  view/adapter seam and exact two-file blob stability with no finding or blocker.
  Do not poll again; await the proactive gate response before closing #6/#3/#2.
