# Causal Reference Denoising State

Last updated: 2026-08-06T19:31:00+08:00

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
  until its P0-P3 response is accepted.
- Accepted but deliberately not integrated until the atomic queue gate:
  plugin data commit `5f4718722` plus exact-parent SPSC admission fix
  `34a8096f1`. Retain its Sol/ultra conversation for same-responsibility fixes.
- Prepared next queue gate: a separate Luna/max producer-thread allocation
  test, then the retained Sol/ultra formal reviewer rechecks P0-P2.
- Learning guide remains held in its retained Luna/max conversation. After its
  active queue-v2 response, one combined narrow revision must describe the
  final atomic wake and add benchmark command/workload/results
  `2.725/4.317/8.557 ms` before issue #7 can close.
- After queue review passes: integrate/formally review plugin data, implement
  programmatic minimal UI, add focused block-boundary settings/lifecycle
  evidence, run final targets/review, update and close issues #5/#6/#7/#3/#2.
