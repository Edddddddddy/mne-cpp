# Review Ledger

## Policy

- Review findings use IDs, priority P0-P3, exact file/line, evidence, requested
  correction, and required verification.
- P0/P1 must be fixed before integration completes.
- P2 must be fixed or explicitly deferred in the GitHub issue.
- Reviewers run in dedicated user-visible work conversations using
  `gpt-5.6-sol` with `ultra` reasoning and do not edit code.
- The manager conversation does not spawn internal review subagents or
  continuously poll reviewers. Reviewers send their final structured RESPONSE
  back to the manager.
- Fixes are returned to a new or still-relevant Luna/max or Sol implementation
  conversation through a logged `REQUEST`/`RESPONSE` exchange.

## Queue formal review

- `R-QUEUE-001`: independent visible Sol/ultra review of the concrete
  preallocated SPSC queue, stop/reconfigure lifecycle, public tests and
  realtime constraints. No finding exists until the reviewer returns evidence.
- Manager integration follow-up: native `QSharedPointer<FiffInfo>` ownership,
  first-callback exact-shape bootstrap and row-count transitions are explicit
  caller risks in `SPEC.md`; formal review should classify or leave them to the
  plugin milestone rather than letting the adapter hide callback allocation.
- Manager seam decision: these caller risks are resolved by queue v2 using
  native `QSharedPointer<const FiffInfo>`, maximum-sized preallocation and
  per-slot row/sample extents. Plugin `start()` fixes v1 bounds at 512x2048,
  capacity four; the worker receives row transitions in FIFO order. A RED test,
  Sol/ultra implementation and fresh review are required before plugin code.
- Installed Qt 5.15.2 evidence: same-type `QSharedPointer<T>` copy constructor
  and copy assignment are explicitly `noexcept` in
  `qsharedpointer_impl.h:326-340`; converting copy construction is also
  `noexcept` at line 364. Thus RTMSA `QSharedPointer<FiffInfo>` to queue
  `QSharedPointer<const FiffInfo>` argument conversion and the queue's same-type
  slot assignment can truthfully sit on the callback `noexcept` path without a
  control-block allocation. The later worker-side `constCast` is not part of
  acquisition hot-path evidence.

### Queue-v2 manager pre-integration review

- Reviewed worker commit `90b423e3c` with exact parent `9b526eb14` and only
  `adaptivedenoisingblockqueue.h/.cpp` changed.
- No manager finding. The maximum-dimension/native-owner interface is a deep
  plugin-private seam; configure remains transactional; producer/consumer
  commit points, destination/ring preservation, stop discard semantics and
  fresh-PImpl token isolation are coherent under explicit caller quiescence.
- The fixed scalar hot paths and installed Qt nothrow handle contract support
  the promised no Eigen allocation/resize/retry/explicit-lock/string/FIFF work.
- This is not the formal milestone gate. Require populated Release GREEN and a
  fresh exact-snapshot Sol/ultra concurrency/realtime review after integration.

#### R-QUEUE-QT-SLOTS-001 - P1 - Closed

- Location: integrated `a21e08e00`,
  `adaptivedenoisingblockqueue.cpp:80-89,159,206`.
- Evidence: MSVC focused Release compilation reports C2208/C2059 at the private
  member declaration and every `.reserve`, `.emplace_back` and `[]` use. Qt
  defines `slots` as a keyword macro, so the ordinary member identifier is not
  available in a Qt translation unit.
- Impact: `test_adaptive_denoising_plugin` cannot compile; no queue-v2 behavior
  or plugin target can build.
- Required fix: rename only the private member and all uses to a Qt-safe name;
  preserve public interface and concurrency logic.
- Required verification: populated focused Release compiles/links, complete
  QtTest exits zero, exact source-only mechanical delta. Formal review remains
  after GREEN.
- Fix received: exact-parent delta `33b3eea1d` changes only the five private
  `slots` tokens to `queueSlots`; token/diff/scope/clean proof passes. Finding
  remains open until manager populated Release compiles and all tests pass.
- Resolution: integrated as `c87762e97`. Populated Release compiles/links and
  the full focused executable exits zero on every invocation; the public test
  structure is effective 15/0/0 with no skip/fail escape. The renamed token is
  absent and no concurrency/public behavior changed.

### R-QUEUE-V2-001 formal gate - Hold

- Reviewed exact clean snapshot `e4964aaed` in a fresh visible Sol/ultra
  read-only conversation. Counts: P0=0, P1=1, P2=2, P3=0.
- The queue remains a deep concrete plugin-private module and its ring/order/
  rectangle/metadata logic is statically coherent. The gate is held because
  the Windows dependency invalidates the realtime contract and public tests do
  not yet cover the required concurrency/lifetime states.

#### R-QUEUE-V2-QSEMAPHORE-001 - P1 - Open

- Location: `adaptivedenoisingblockqueue.h:85-89,108-129` and
  `adaptivedenoisingblockqueue.cpp:90-91,147-175,195-237`.
- Evidence: installed Qt 5.15.2 on Windows selects `QtDummyFutex`; its
  `QSemaphore` fallback protects operations with `QMutexLocker`. A zero token
  timeout does not prevent blocking on that mutex, first contention may lazily
  allocate `QMutexPrivate`, and the calls are not declared noexcept.
- Impact: the DirectConnection acquisition callback can block or allocate
  after configure; an allocation exception crossing queue noexcept terminates.
- Required fix: remove `QSemaphore` from the producer path. Use SPSC atomics
  for slot availability/publication and a preconstructed nonallocating,
  nonblocking consumer/stop notification adapter without weakening the frozen
  acquisition contract.
- Selected correction: retain the public seam and use lock-free monotonic SPSC
  sequences plus a configure-created sticky OS wake: Windows auto-reset event/
  `SetEvent`/bounded wait; POSIX nonblocking pipe/one write attempt/bounded
  poll+drain. This avoids producer mutexes, allocation, exceptions and the
  producer-unlocked condition-variable lost-wake window.
- Required verification: Windows overlapping SPSC stress, post-configure
  allocation counting, FIFO metadata/sequence integrity and bounded producer
  latency; platform/source audit proving no QSemaphore/QMutex producer path.
- Production response received: exact-parent two-file `0a8202e405` removes all
  Qt synchronization, uses compile-time lock-free unsigned sequences plus the
  selected Windows event/POSIX pipe wake, and reports unchanged public tests
  19/0/0 on both platforms with five/three repeats. Windows max producer call
  is 4100 ns; POSIX 3996 ns. Finding stays open through manager code review,
  canonical populated build, allocation tracer and independent re-review.
- Manager production review: PASS with no new finding. Transaction, sequence/
  wrap/order, stop linearization, sticky lost-wake closure, public locality and
  producer dependency audit match the selected correction. Integration/runtime/
  allocation/formal gates remain; this P1 is not yet closed.

#### R-QUEUE-V2-CONCURRENCY-TEST-001 - P2 - Open

- Location: focused plugin test `:693-778,782-910,915-1157`.
- Evidence: successful push/pop tests are same-thread; the only consumer thread
  marks entry before `waitPop`; no producer/stop race, pending-block discard or
  successful overlapping publication is exercised. Timeout/Stopped paths lack
  complete sentinel preservation assertions.
- Required test: sustained overlapping public SPSC sequence/rectangle/metadata
  traffic with Full accounting; bounded producer/consumer stop races; pending
  discard plus fresh configure; whole destination/extents/metadata preservation
  for Timeout and Stopped.
- Test response received: exact-parent one-file `f367fd75a` adds sustained SPSC,
  active stop-race, deterministic pending-discard/fresh-queue and full sentinel
  preservation coverage. Worker syntax passes; finding remains open until
  manager code review and populated Windows execution against atomic production.
- Manager review: PASS. Thread cleanup is join-safe, the accepted-subsequence
  oracle is sensitive to FIFO/publication errors, and complete destination
  preservation is asserted. Populated Windows Release and three repeat runs
  exit zero; retain open until the same suite passes atomic production review.

#### R-QUEUE-V2-METADATA-LIFETIME-001 - P2 - Open

- Location: focused plugin test `:151-165,730-776,1076-1156`.
- Evidence: raw pointer identity is checked while caller handles remain alive;
  retained native control-block lifetime is not observed.
- Required test: custom-deleter/liveness handles, clear callers immediately
  after push, require no deletion before pop and exactly-once deletion after
  popped handles clear; assert nothrow mutable-to-const conversion.
- Test response received: `f367fd75a` reports exact public custom-deleter/live
  count coverage and mutable-to-const nothrow trait. Finding remains open until
  manager code review and populated Windows execution.
- Manager review: PASS. Caller owners are cleared before pop and exactly-once
  destruction is observed only after the popped native handle is cleared.
  Populated Windows Release and three repeat runs exit zero; final closure waits
  for the atomic implementation and independent re-review.

## Plugin data-lifecycle pre-integration review

#### R-PLUGIN-SPSC-PRODUCER-001 - P1 - Open

- Location: held worker commit `5f4718722`,
  `adaptivedenoising.cpp:101-125,127-150,445-466`.
- Evidence: `tryEnterProducer()` rejects only closed state or count saturation.
  After one callback CASes count zero to one, a second can load one, CAS it to
  two, pass epoch confirmation and concurrently execute `queue.tryPush()`.
  `AdaptiveDenoisingBlockQueue` is explicitly SPSC and its producer index/slot
  writes are not multi-producer safe.
- Impact: concurrent/reentrant DirectConnection notifications can race the
  producer index and slot payload, causing torn, reordered or overwritten
  blocks. The existing count protects lifecycle quiescence, not SPSC admission.
- Required fix: admit only an open epoch whose in-flight count is zero; second
  and later concurrent callbacks return immediately without lock/wait/retry.
  Preserve close/epoch confirmation, count-one leave and stop/restart ordering.
- Required verification: exact one-source-file private-gate delta, proof no
  state path reaches producer count greater than one, moc/syntax where possible,
  and later public concurrent-update/plugin lifecycle coverage after the atomic
  queue is integrated.
- Fix received: exact-parent source-only delta `34a8096f1` rejects every
  nonzero count before the existing one-shot CAS, so an admitted state can only
  publish zero to one. Worker source-contract proof and moc pass. Finding stays
  open until manager delta review, later ordered integration and plugin gate.
- Manager fix review: exact one-file diff and zero-to-one state proof pass with
  no semantic spill. The fix is accepted; the P1 remains administratively open
  until ordered integration and public/plugin formal validation.

### Plugin data manager pre-review decision

- Exact parent/seven-file scope and all non-finding behavior pass. The concrete
  PImpl is a deep adapter at the intended Qt/FIFF-to-data seam; callback,
  worker, exception disarm, output metadata bridge and bounded lifecycle remain
  local and leverage the queue/processor modules.
- Integration is held for the P1 above and the separate atomic queue formal
  gate. A later independent Sol/ultra plugin review remains mandatory.

## Learning guide review

#### R-DOC-QUEUE-V2-001 - P2 - Open

- Location: held worker commit `c558acbf8`, `LEARNING_GUIDE.md:164-168` and
  `:233-249`.
- Evidence: the guide calls 4/128/30/1e-3 numerical defaults although defaults
  live in `AdaptiveDenoisingSettings`; the queue text still says fixed
  `C x Nmax`, leading-column copy, and only sample count. The frozen/public RED
  queue-v2 seam is `maxChannelCount x maxBlockSamples`, accepts variable
  positive top-left rectangles, transports both row/sample extents with native
  `QSharedPointer<const FiffInfo>`, and leaves the destination tail untouched.
- Impact: the primary learning document would teach the superseded adapter
  ownership/shape contract and hide the exact metadata transition mechanism
  the new plugin relies on.
- Required correction: amend the same one-file exact-parent commit with the
  queue-v2 semantics and label the four values adapter defaults. Preserve the
  verified algorithm/Eigen content and recheck all repository-relative links.
- Required verification: manager diff/content review; no runtime test needed
  for a Markdown-only correction. Do not integrate the held commit while open.

#### R-DOC-BENCHMARK-EVIDENCE-002 - P2 - Open

- Location: held worker commit `c558acbf8`, entire `LEARNING_GUIDE.md`.
- Evidence: issue #7 requires the Release benchmark result and development-
  machine context in learning documentation. The guide explains complexity and
  effect/engineering evidence but contains no `--benchmark` command, 100/1000
  warmup/timed configuration or measured p50/p95/max values.
- Impact: the example binary is GREEN, but the promised durable learning path
  cannot reproduce or interpret the accepted performance result from the guide.
- Required correction after the current doc response: add the exact 1000 Hz,
  128-sample, 270-row, 16-reference/250-target, four-tap/P=64 workload; 100
  warmup plus 1000 timed calls; nearest-rank definition; p50/p95/max
  `2.725/4.317/8.557 ms`; strict p95 `<128 ms`; and command
  `ex_causal_reference_denoising --benchmark`. Label it development-machine
  engineering evidence, not a portable effect guarantee.
- Required verification: exact values against durable benchmark record,
  repository-relative links/Markdown, one-file scope and diff-check.

#### R-BENCH-FINITE-001 - P2 - Closed

- Location: worker benchmark commit `79eff3b3a`, example main helper
  `finiteBenchmarkSelectedTarget` and its process verification call.
- Evidence: only target row 16 is checked although target rows 16..265 are all
  selected and timed. Nonfinite output in 249 rows would not fail the benchmark.
- Required correction: validate all configured target rows/samples after every
  call outside timing; retain exact workload/timer. Rebuild/run populated
  Release and require normal example plus benchmark exit zero.
- Resolution: replacement `b8ce427b8f` checks all rows 16..265 outside the
  timer and is integrated as `87ab418bd`. Populated default/benchmark runs exit
  zero; p95 is `4.317 ms` against 128 ms. Full evidence is repeated in the
  final-integration section below.

## Core numerical review

Formal independent review `R-CORE-001` completed on Sol/ultra against exact
commit `f0a0e14d7`, read-only and without subagents. P0 and P1 are zero. The
core gate is held for one P2 test-evidence finding; two P3s remain tracked.

#### R-CORE-FORGET-001 - P2 - Closed

- Location: `SPEC.md:94-100,208-210`;
  `causalreferencedenoiser.cpp:114-115,203-225`;
  `test_causal_reference_denoiser.cpp:75-93,471-593,1279-1444`.
- Evidence: recurrence looks correct, but no public oracle has a lambda-
  dependent analytic expectation. Stationary exact relationships keep
  `H=W*G`, chunk tests compare the same implementation, and the synthetic
  test remains above both gates when lambda is replaced by one.
- Impact: deleted/misplaced within-epoch or cross-epoch decay can pass the
  suite, leaving the explicit forgetting acceptance criterion unmet.
- Required fix/test: no production change indicated. Add `fs=1`,
  `tau=1/log(2)` (`lambda=0.5`), one ref/tap/target, interval two, reference
  `[1,1,1,1]`, target `[0,1,0,0]`; after two accepts require an ApplyOnly
  prediction with `G=1.875`, `H=0.25`,
  `W=H/(G*(1+regularization))`.
- Resolution: test commit `1b542b18a` adds the independent two-epoch public
  oracle. Manager Release report is 44/0/1 and effective Debug report is
  45/0/0; expected and observed future target `-0.1332...` agree within
  `1e-12`. Synthetic metrics and exact chunk equivalence remain GREEN.

#### R-CORE-LOCALITY-001 - P3 - Closed

- Location: `causalreferencedenoiser.h:55-105`.
- Evidence/impact: essential invariants, orientation, modes/epochs, atomic
  errors, ownership, allocation/exception and implicit noncopy/nonmove policy
  are non-local to the public header, increasing caller reconstruction cost.
- Requested later fix: compact Doxygen contract, explicit copy/move policy and
  C++14 trait/noexcept compile checks.
- Fix received with provenance correction: worker commit `2682034d0` has actual
  parent `7eb04cbfa766...`; the longer base recorded in the original request is
  not a resolvable object. The two-file header/trait result and syntax probes
  are reported complete, but manager blob/diff review and populated Release/
  Debug remain required before closure.
- Resolution: accepted header contract and explicit deleted copy/move policy
  are integrated as `47b54efed`; portable traits delta is `0848634a3`.
  Populated Release/Debug compile/link and complete executables exit zero,
  preserving the existing runtime suite and Debug malloc guard.

#### R-CORE-CXX14-NOEXCEPT-001 - P1 - Closed

- Location: held worker commit `2682034d0`, focused numerical test new
  `ProcessMember` alias/static assertion near the top of the file.
- Evidence: MSVC 14.51 with `/std:c++14` rejects `using ProcessMember = ...
  noexcept` as C2279. A minimal matching probe reproduces the error. C++14 does
  not portably include the exception specification in the function type.
- Impact: the focused numerical target cannot compile, and the claimed process
  noexcept check is neither portable nor mutation-sensitive under the project's
  language standard.
- Required fix: keep header contract, remove the illegal alias/cast, retain
  portable ownership/reset traits and assert MatrixXd-lvalue Ref conversion plus
  public call result type. Do not claim the full call expression noexcept because
  by-value Eigen::Ref wrapper construction is outside the member body contract.
- Required verification: MSVC 14.51 C++14 syntax/build, then full populated
  Release/Debug focused runs with unchanged runtime slot count.
- Fix received: exact-parent test-only delta `bc061b834` removes the illegal
  alias/cast and adds portable MatrixXd-to-Ref convertibility plus public-call
  result-type assertions while retaining ownership/reset traits. Worker MSVC
  14.51 `/std:c++14` syntax compilation succeeds. Finding stays open until
  manager delta review, integration and complete populated Release/Debug runs.
- Manager fix review: exact parent and one-test-file scope pass. The word diff
  is limited to removal of the illegal noexcept function type and portable
  callability/result checks; the accepted header and runtime slots are
  unchanged. No new finding; populated Release/Debug remain the only gate.
- Resolution: integrated portable delta `0848634a3`. Both populated Release
  and Debug targets compile/link with MSVC 14.51 `/std:c++14`, and each complete
  executable exits zero. The C2279 path is removed without asserting the
  Eigen::Ref wrapper construction noexcept.

#### R-CORE-LINK-001 - P3 - Environment-deferred

- Location: focused test `CMakeLists.txt:13-16,27-39`; library
  `CMakeLists.txt:25,52,67`.
- Evidence/impact: direct source compilation preserves isolation but cannot
  detect real shared/static library export/client-link regressions.
- Requested later fix: retain isolated test and add a small supported-form
  `mne_rtprocessing` client-link smoke before final integration.
- Current-environment evidence: direct Release build of the generated
  `mne_rtprocessing.vcxproj` with dependency-project rebuild disabled fails in
  installed Qt 5.15.2 `qlist.h:915` / `qvector.h:960` because MSVC 18/14.51 no
  longer provides `stdext::make_checked_array_iterator` (`C2653`, `C3861`).
  Existing `rtaoemeg`, `rtcov`, `rtinvop` and MOC dependency paths trigger the
  error; no new denoiser source is implicated.
- Disposition: explicitly deferred to a compatible toolchain, as allowed by
  final-QA issue #3. Do not patch vendor Qt. Focused direct-source tests,
  example and benchmark remain the local verification seam.

### Formal core gate decision

- P0: zero.
- P1: zero.
- P2: zero open; `R-CORE-FORGET-001` closed by independent analytic evidence.
- P3: `R-CORE-LOCALITY-001` is closed by the integrated public contract;
  `R-CORE-LINK-001` is explicitly environment-deferred with reproducible Qt/
  MSVC evidence.
- Decision: numerical core may proceed to plugin-data TDD.

### Diagnostics seam pre-review

Completed by `/root/r_diagnostics_001` on Sol/ultra against `2f6b6c8d6`.

#### R-DIAG-001 - P1 - Closed

- Location: `causalreferencedenoiser.h:68` and
  `causalreferencedenoiser.cpp:161,183-201,341-352`.
- Evidence: result contains only primary status; update rejection returns
  silently, so accepts/rejects, generation, warmup and RMS are unobservable.
- Fix: selected fixed-size diagnostics in `SPEC.md`; aggregate every boundary.
- Required test: lifecycle plus multiple/mixed boundary event counts.
- Progress: fixed result/lifecycle is GREEN in `b5f29fff1` with test
  `a0fcf0e68`; mixed accepted/rejected boundary proof is integrated as
  `d1ffa0904` and passes after transactional implementation `9142415b7`.
- Resolution: synchronized report has 39 passes, zero failures; a valid block
  reports mixed accepted/rejected events `2/2` and generation two.

#### R-SOLVE-001 - P1 - Closed

- Location: `causalreferencedenoiser.cpp:183-197,334-344`.
- Evidence: finite inputs can overflow in-place outer products; solve returns
  but poisoned `gram/cross` remain infinite and can never recover.
- Fix: committed/pending transactional statistics and failed-epoch discard with
  elapsed forgetting.
- Required test: multiple extreme finite rejected epochs followed by a normal
  accepted epoch in the same call, with finite output and future probe.
- RED evidence: `d1ffa0904`; focused report has 38 passes and the single
  intended accepted-count failure, process exit code one.
- Resolution: `9142415b7` separates committed, pending and candidate stats;
  failed epochs are discarded after aging committed stats. The same report is
  GREEN with 39 passes and zero failures.

#### R-SOLVE-002 - P2 - Closed

- Location: `causalreferencedenoiser.cpp:187-200`.
- Evidence: Eigen `Success` plus finite `D` does not prove strictly positive,
  nonsingular factorization.
- Fix: require positive factorization, every pivot strictly positive, finite RHS
  and finite candidate weights.
- Required test: loaded rank-deficient data accepts; invalid candidate does not
  increment generation.
- Progress: loaded rank-one characterization `d36491643` is GREEN in a 40/0
  report; invalid-candidate rejection/generation is covered by `d1ffa0904`.
  Source hardening `3faa0ca14` adds positive factorization, strict finite
  positive pivots and finite RHS/weights before commit.
- Resolution: synchronized report remains 40/0 GREEN after hardening.

#### R-RMS-001 - P2 - Closed

- Location: result header and `causalreferencedenoiser.cpp:289-352`.
- Evidence: RMS is absent; direct squaring may overflow for valid finite input.
- Fix: fixed scalar scaled-sum-of-squares accumulators.
- Required test: large finite Bypass RMS and analytic ApplyOnly RMS.
- Resolution: scaled accumulation is implemented in `b5f29fff1`; test
  `ada68a56b` passes both required oracles in a 41/0 synchronized report.

#### R-APPLY-001 - P2 - Closed

- Location: `causalreferencedenoiser.cpp:328-332`.
- Evidence: finite weights/features can yield nonfinite prediction/residual,
  currently written while returning `Processed`.
- Fix: define a finite-output application fallback before synthetic acceptance.
- Required test: trained finite model plus extreme finite probe never silently
  returns nonfinite `Processed` output.
- Selected policy: atomic sample-wide target pass-through, zero actually-
  subtracted prediction diagnostics, valid Processed block and advancing
  history/transactional learning state; public diagnostics surface unchanged.
- RED evidence: test `eee23de46`; synchronized report has 41 prior passes and
  the single intended all-finite output failure, exit code one.
- Resolution: source fallback `2aac482d9` validates all target predictions/
  residuals before any write and zeroes actual subtraction on fallback. The
  synchronized report is GREEN with 42 passes and zero failures.

#### R-STATUS-001 - P3 - Closed

- Location: `causalreferencedenoiser.cpp:282-286`.
- Evidence: unconfigured module is conflated with invalid shape.
- Fix/test: `NotConfigured` with unchanged block, zero snapshot and NaN RMS.
- Resolution: implemented in `b5f29fff1`; diagnostics lifecycle test passes in
  the 38/0 synchronized report.

No P0-P2 numerical finding remains open. Hot-path malloc proof and synthetic
acceptance are GREEN; formal independent core review remains before core
completion.

### Quantitative core acceptance evidence

- Integrated test: `a079df377`.
- Release report: 43 passed, zero failed, one intentional Debug-only malloc-
  guard skip, exit code zero.
- Measured gates: 58.6541 dB environmental-noise reduction and `0.000113195`
  clean projection-amplitude error. Both pass with wide margin.
- Existing exact finite-stream chunk equivalence remains zero in the same run.

## Final integration review

Pending.

### Queue producer-allocation acceptance tracer

- Worker commit `ea81256b1` is exact-parent and test-only. Manager source and
  interface review finds no P0-P3: the C++14 global allocation replacement set
  is complete for this target and the thread-local counted region is exactly the
  producer's public `tryPush` call, including by-value metadata construction.
- This is accepted test design, not yet executable evidence. The canonical
  populated MSVC Release target now compiles/links; the full run and five
  immediate repeats all exit zero. Because zero counted producer allocations
  is a hard in-slot assertion, executable evidence is GREEN. The retained
  independent Sol/ultra queue reviewer must still close or replace the original
  P1/P2 findings before issue #5 passes.

### R-QUEUE-ATOMIC-002 - pending independent formal gate

- Retained Sol/ultra reviewer will inspect exact pushed snapshot `9535bf8c7`
  read-only and explicitly re-evaluate the old QSemaphore P1 plus concurrency/
  preservation and metadata-lifetime P2 findings against the atomic queue and
  complete public suite.
- Required result: prioritized P0-P3 with exact evidence and a PASS/HOLD gate.
  Issue #5 and plugin integration remain blocked until the response is durable,
  manager-reviewed and all required P0-P2 closure conditions are met.

#### R-QUEUE-ATOMIC-002 response - HOLD

- Exact `9535bf8c7`, clean/read-only Sol/ultra review: P0=0, P1=0, P2=2,
  P3=1. Original QSemaphore P1 and metadata-lifetime P2 close.
- `R-QUEUE-ATOMIC-POSIX-EINTR-001` P2: POSIX signal discards a recoverable
  pre-transfer EINTR, so push/stop may wait for the complete consumer timeout.
  Fix private wake or bound the consumer recheck slice and add deterministic
  link/private-seam POSIX fault injection for both publication and stop.
- `R-QUEUE-V2-CONCURRENCY-TEST-001` P2 remains: current readiness/attempt markers
  precede public calls and sustained assertions permit only capacity pushes
  followed by a later drain. Require forced slot reuse while both threads are
  live and an actually blocked empty wait before stop, with sensitive payload/
  extents/metadata/tail/sentinel oracles.
- `R-QUEUE-ATOMIC-SPEC-LOCALITY-001` P3: SPEC `538-550` contradicts current
  atomic wake and untouched-tail contract. Reconcile doc and search for stale
  prescriptive semaphore wording.
- Gate remains HOLD because neither P2 is fixed or explicitly deferred.

`R-QUEUE-ATOMIC-SPEC-LOCALITY-001` fix review: exact one-file commit
`96055bf13` replaces the live contradiction with atomic release/acquire/native
wake and untouched-tail/full-preservation rules. All remaining semaphore terms
are explicitly legacy/rejected/no-semaphore context. Manager accepts the P3
correction. It is integrated/pushed as `c56f19269`; exact committed diff/search
passes. P3 is manager-closed pending final exact-snapshot reviewer confirmation.

#### Correction order

1. Luna/max public deterministic concurrency/blocked-stop tracer, test source
   only, then manager canonical GREEN.
2. Luna/max POSIX EINTR link/private-seam RED tracer after step 1 because the
   same focused test/CMake seam overlaps.
3. Retained Sol/ultra atomic implementation conversation fixes private wake/
   bounded-recheck behavior against the integrated RED.
4. Parallel Luna/max SPEC-only correction closes the locality P3.
5. Retained independent Sol/ultra reviewer rechecks the exact final snapshot.

### R-DOC-QUEUE-V2-001 / R-DOC-BENCHMARK-EVIDENCE-002 - P2 open

- Exact held guide `fb748082f` correctly documents queue-v2 rectangles/extents/
  native ownership but still says producer admission uses QSemaphore. This is
  now false after the reviewed Windows P1 correction to atomic sequences plus
  native event/pipe wake.
- The same guide lacks the completed `--benchmark` command, 270-row/P64/128-
  sample workload and measured Release p50/p95/max `2.725/4.317/8.557 ms`.
- Required correction is one Luna/max guide-only delta from `fb748082f`, keeping
  algorithm/Eigen content intact and clearly separating engineering timing from
  effect evidence.
- Fix review: replacement `6333b995d` has the original exact parent and one-file
  scope. Manager comparison confirms only the requested queue/benchmark content
  changed; QSemaphore count is zero, eight links resolve, fences balance and
  commands/workload/results/limitations match implementation/evidence.
- Status: accepted pending integration plus replay of the documented Release
  commands; no additional content correction is required.
- Resolution: integrated `b59221e78`. Canonical Release default example exits
  zero/PASS; benchmark exits zero/PASS at current p50/p95/max
  `2.174/2.748/4.130 ms`, with exact required dimensions/counts and p95 gate.
  Earlier recorded values remain documented as load-sensitive engineering
  evidence. Both P2 findings are closed; no documentation P0-P3 remains.

### R-BENCH-FINITE-001 - P2 - Closed

- Location: held benchmark commit `79eff3b3a`, focused example main finite
  integrity helper.
- Evidence: the initial helper inspected only target row 16 although the
  configured selected set is rows 16..265.
- Required correction: verify every sample of all 250 selected rows outside
  the measured region without changing workload or timer boundaries.
- Fix review: replacement `b8ce427b8f` has the same exact parent/one-file
  scope. It loops target indices 0..249, maps rows 16..265, checks every column
  and reports the first failing row/sample after the timer; the 100 warmup and
  1000 timed calls share this verifier. Comparison to the held commit is
  confined to that helper/call site.
- Resolution: replacement is integrated as `87ab418bd`. Populated Release
  default mode exits zero with `example invariants: PASS`; benchmark exits zero
  with all integrity checks PASS, p50 `2.725 ms`, p95 `4.317 ms`, max
  `8.557 ms`, generation/accepted/rejected `1099/1099/0`. The p95 gate is well
  below 128 ms and all 250 target rows are now covered.

## Processor milestone review

Formal independent review `R-PROC-001` completed on Sol/ultra against exact
clean snapshot `96ca3eb44`. Code, test and provenance audit found the concrete
data-only processor module suitably deep and the mapping/disarm/reset behavior
consistent with SPEC. P0/P1 are zero; one P2 blocks issue #4 and two P3s remain
tracked.

#### R-PROC-MOVE-001 - P2 - Closed

- Location: `adaptivedenoisingprocessor.h:78-103`;
  `adaptivedenoisingprocessor.cpp:145-162,176-179`.
- Evidence: no special members are declared. C++14 deletes copy through the
  `unique_ptr` but generates noexcept move. Move transfers `m_denoiser` while
  copying the scalar Ready snapshot, leaving the source reporting Ready from
  `configuration()` but NotConfigured from `process()`.
- Impact: a future worker/container refactor can publish Ready while silently
  passing data through, violating ownership consistency and interface locality.
- Required fix: select explicit worker ownership. Prefer an explicit noexcept
  default constructor and deleted copy/move construction/assignment. If moves
  remain, custom moves must transfer model/snapshot together and disarm source.
- Required verification: C++14 type-trait static assertions for the selected
  policy; if moves remain, configured move behavior tests; complete focused
  Release executable remains GREEN.
- Manager decision: select the preferred explicit noexcept default constructor
  with deleted copy/move operations. No real v1 owner needs move support, so a
  custom moved-from interface would add a hypothetical seam. Minimal Luna/max
  header/trait fix is prepared as `W-PROC-MOVE-001`.
- Fix review: exact worker commit `b49f27699` on requested base adds only the
  five special-member declarations and five namespace-scope C++14 traits in
  the two authorized files. Manager interface/diff review found no issue;
  integrated as `4a57c3ca5`.
- Resolution: populated Release compiled the traits and QtTest reports 5/0/0
  with process exit zero. Explicit deleted copy/move prevents construction of
  the contradictory moved-from state; every original runtime behavior remains
  GREEN.

#### R-PROC-LOCALITY-001 - P3 - Closed

- Location: `adaptivedenoisingprocessor.h:2-5,32-103`.
- Evidence/impact: worker ownership, inclusive UI ranges, selection, returned-
  failure disarm, Ready reset, allocation/exception behavior and hot-path rules
  are non-local to the public declarations.
- Later correction: compact Doxygen plus explicit copy/move policy. State that
  allocation exceptions propagate while preserving old state so callers must
  fail closed rather than use old ownership for new metadata.
- Verification: header contract review against SPEC and compile-time ownership/
  noexcept checks.
- Resolution: integrated comment-only `7f4095a3b` places the complete truthful
  mapping/range/ownership/disarm/reset/exception/hot-path contract beside the
  declarations. Stripped declaration-token hashes are identical before/after;
  existing C++14 noncopy/nonmove traits and 14/0/0 focused runtime remain valid.

#### R-PROC-BOUNDARY-001 - P3 - Closed

- Location: `adaptivedenoisingprocessor.cpp:49-58,125-135`;
  focused processor test `:133-141,275-404`.
- Evidence/impact: invalid low/high cases and P=288 are covered, but inclusive
  legal maxima and exact P=256 acceptance are not. Source is currently correct,
  yet a narrowing regression could pass the focused suite.
- Later correction/test: Ready rows for taps 32, interval 2048, memory 1/300,
  regularization 1 and P=256, retaining an over-cap disarm case.
- Resolution: additive test `34a346424` exercises all six legal boundaries
  through fresh public processor instances and exact committed snapshots while
  retaining the existing P=288 invalid/disarm oracle. Populated Release reports
  14/0/0, including one PASS for each boundary row and all prior processor/
  queue slots; process exit is zero.

### Processor formal gate decision

- P0: zero.
- P1: zero.
- P2: zero open; `R-PROC-MOVE-001` closed by explicit ownership and compiled
  C++14 traits.
- P3: both processor locality and boundary findings are closed by final-QA
  documentation plus focused evidence.
- Decision: processor formal gate passes. Track both P3s in final QA issue #3;
  they do not block closing processor issue #4 or beginning queue work.
- GitHub: issue #4 closed as completed; epic #2 marks the processor milestone
  checked.

#### R-PROC-TEST-MEMORY-001 - P2 - Closed

- Location: worker commit `0d8aeafec1`, focused processor test source line 320.
- Evidence: lower memory case uses `0.0`, which cannot distinguish the adapter
  UI requirement `memory >= 1.0` from the numerical core's weaker `memory > 0`.
- Impact: a regression accepting `0.5` would pass the new acceptance suite.
- Required fix/test: use `0.5`, expect InvalidSettings, and retain the full
  learned-model disarm/pass-through oracle. No production change.
- Resolution: replacement test commit `aa75e2520b` uses `0.5` and the
  below-min case name, keeps exact InvalidSettings plus complete learned-model
  disarm/probe assertions, and has the same requested parent. Direct comparison
  to the rejected commit shows only the two requested line changes.

#### R-QUEUE-STOP-WAIT-ELAPSED-001 - P2 - Open

- Snapshot: worker test commit `7aa5360a2`, function
  `queueStopRacesActiveProducerAndConsumer`, around worker lines 2077-2085 and
  2212-2218.
- Evidence: `consumerStopWaitStarted` is published immediately before the
  long-timeout public `waitPop`; main waits for that marker, sleeps 20 ms and
  stops. A scheduling pause after the marker but before the public call lets
  stop happen first, so the later immediate `Stopped` result can satisfy all
  current assertions without the tested call having blocked.
- Impact: this leaves the exact false-green class identified by formal finding
  `R-QUEUE-V2-CONCURRENCY-TEST-001` partially open despite otherwise correct
  forced wrap/reuse and payload/preservation coverage.
- Required correction: measure the complete stop-wait public call on the
  consumer thread, publish the elapsed duration with its `Stopped` result, and
  assert a positive lower bound plus the existing prompt upper bound after
  join. Preserve finite fallback stop/join behavior and all existing oracles.
- Required verification: MSVC Release-style complete focused suite plus repeats;
  exact one-test-file delta and no production/CMake change.
- Resolution: worker response mislabeled its parent, but direct Git object proof
  shows `cee7ed325` has exact parent `8539c8e85` and changes only the focused
  test. Integrated `103d5a5a8` measures the complete stop-phase consumer
  `waitPop`, publishes its elapsed milliseconds and after join requires
  `Stopped`, `>=5 ms` and `<1500 ms`. Populated MSVC Release compiles/links;
  the complete executable and three immediate repeats return `0,0,0,0` with all
  forced wrap/reuse/FIFO/metadata/tail/allocation oracles retained. Closed.

#### R-QUEUE-EINTR-BLOCKED-WAIT-001 - P2 - Open

- Snapshot: worker tracer commit `c4b13a220`, functions
  `queuePopsPromptlyAfterInterruptedProducerSignal` and
  `queueStopsPromptlyAfterInterruptedStopSignal`.
- Evidence: both consumer threads publish `consumerWaiting` immediately before
  the public long-timeout `waitPop`; producer/main waits for that marker and
  sleeps 50 ms. Both elapsed assertions accept zero and only require `<500 ms`.
- Impact: if the consumer is paused after the marker but before entering
  `waitPop`, the producer publication or stop can occur first. The later call
  sees the atomic state immediately and passes even though the injected native
  wake was lost, so the intended POSIX regression can false-green.
- Required correction: in both tests measure the already complete public call
  and require elapsed `>=5 ms` and `<500 ms`, retaining wrapper count one,
  exact Popped payload/tail, complete Stopped destination preservation and all
  finite fallback cleanup.
- Required verification: exact-parent one-test-file delta, diff-check, Windows
  conditional non-regression, and POSIX runtime when WSL is available. An
  unavailable WSL service must be recorded rather than treated as test output.
- Resolution: exact-parent delta `89acb78bd` adds named 5 ms lower bounds to
  both complete public calls and leaves every other tracer oracle unchanged.
  Integrated tracer commits are `0d1d1f949` and `b6cf3a13b`. Populated MSVC
  Release reports 19/0/0, followed by three explicit waited exit-zero repeats.
  The Linux-only slots remain unexecuted because WSL cannot start; this is an
  environment limitation, not a GREEN claim. Test-sensitivity finding closed;
  production `R-QUEUE-ATOMIC-POSIX-EINTR-001` remains open.

#### R-QUEUE-ATOMIC-POSIX-EINTR-001 - P2 - Addressed pending formal review

- Original evidence: one POSIX nonblocking `write` may return pre-transfer
  `EINTR` without leaving a pipe byte, allowing a consumer to remain in a
  full-caller-timeout `poll` after publication or stop.
- Production correction: integration `f7c22717e` caps only POSIX consumer
  `poll` calls to 25 ms. The existing outer deadline and atomic sequence/
  running rechecks are unchanged. Windows wait, `tryPush`, `stop` and POSIX
  signal executable code remain unchanged; producer/stop still make exactly
  one nonblocking signal attempt without retry, wait, lock or allocation.
- Verification: canonical Windows MSVC/Qt reports 19/0/0 plus three repeat
  zero exits. WSL GCC/Qt with GNU `--wrap=write`, explicitly guarded MOC and
  both forced EINTR slots reports 21/0/0. Payload/tail/native metadata and
  complete Stopped preservation gates all pass.
- Status: manager-addressed with source and cross-platform runtime evidence;
  independent Sol/ultra reviewer must confirm closure and the overall queue
  P0-P2 gate.

### Final atomic queue formal gate

- Review: `R-QUEUE-ATOMIC-003-RETRY-1` at exact SHA
  `5aa43160bbae27560c4f4fe013728a6476013db0`.
- Decision: PASS, P0=0, P1=0, P2=0, P3=0; no new finding.
- `R-QUEUE-V2-QSEMAPHORE-001`, `R-QUEUE-V2-CONCURRENCY-TEST-001`,
  `R-QUEUE-V2-METADATA-LIFETIME-001`,
  `R-QUEUE-ATOMIC-POSIX-EINTR-001`,
  `R-QUEUE-ATOMIC-SPEC-LOCALITY-001`,
  `R-QUEUE-STOP-WAIT-ELAPSED-001` and
  `R-QUEUE-EINTR-BLOCKED-WAIT-001` are formally closed.
- Independent evidence is exact-SHA source/test/SPEC/provenance audit and clean
  state; runtime remains manager evidence: Windows 19/0/0 plus three repeats,
  WSL GNU wrapper 21/0/0 including both forced EINTR cases.
- No queue P2 deferral remains. Proceed to held plugin lifecycle integration.

### Plugin data/lifecycle formal review - R-PLUGIN-DATA-001

- Snapshot: `fd33dc9cd1a2ce90a30fdedd1ffde71c8514ec56`.
- Clean/read-only proof: exact detached HEAD before/after; status, staged and
  unstaged diffs empty.
- Decision: HOLD, P0=0, P1=0, P2=4, P3=1.
- Prior `R-PLUGIN-SPSC-PRODUCER-001` is closed by zero-to-one admission and
  quiescent replacement; no two-producer state path was found.

#### R-PLUGIN-DESTRUCTOR-BOUND-001 - P2 - Deferred candidate

- Location: `adaptivedenoising.cpp:154-167,295-312,392-412`.
- Evidence: public `stop()` is bounded, but destructor fallback uses unbounded
  `wait()` and `waitForProducerQuiescence(-1)` with 1 ms polling.
- Impact: a stuck worker/upstream callback can hang terminal destruction.
- Disposition: do not replace memory-safe waiting with a bounded return while
  QThread/callback ownership is live. Create a durable follow-up documenting
  the host quiescence precondition/risk and requiring timeout/retry/destruction
  tests on a supported real-plugin harness.
- Durable deferral: issue #8,
  `https://github.com/Edddddddddy/mne-cpp/issues/8`; linked from final QA #3
  and plugin task #6.

#### R-PLUGIN-ADMISSION-DROP-001 - P2 - Open

- Location: `adaptivedenoising.cpp:101-117,447-466`.
- Evidence: Busy and Closed admission both return false; Busy notifications
  return before matrix enumeration and the sole dropped-block increment.
- Impact: concurrent/reentrant data loss is silently undercounted.
- Required fix/test: typed Busy/Closed result; prompt Busy rejection that never
  enters `tryPush`, but counts every matrix exactly and preserves admitted FIFO.
- Correction response: worker commit `2b571453da2` adds translation-unit-private
  Closed/Busy/Entered admission. Busy counts exact RTMSA matrix cardinality with
  no metadata/queue access; Entered retains the frozen path; Closed is silent.
  MOC/C++14 syntax/source oracles pass. Addressed pending manager integration and
  fresh formal review; runtime harness remains explicitly deferred to #8.
- Manager integration/verification: integrated as `6ab7326ac`; MOC, exact
  callback branch oracle and MSVC 14.29 C++14 `/Zs` pass. Mark addressed pending
  formal re-review; no runtime-harness claim is made.

#### R-PLUGIN-LIFECYCLE-TEST-001 - P2 - Deferred candidate

- Location: focused plugin test CMake `:13-18` and slots `:395-417`.
- Evidence: target compiles queue/processor/core only, not the actual plugin;
  no admission/epoch/stop/restart/output/metadata lifecycle test is registered.
- Impact: caller/lifecycle regressions can leave all focused tests GREEN.
- Disposition: create a dedicated supported-toolchain real-plugin harness task;
  explicitly carry the current Qt/MSVC FIFF block and residual risk in #3/#6.
- Durable deferral: issue #8 owns the complete harness acceptance and is linked
  from #3/#6. This finding is deferred, not closed.

#### R-PLUGIN-STATIC-REGISTRATION-001 - P2 - Open

- Location: plugin CMake `:53-55`; omissions in mne_scan CMake `:75-88` and
  `main.cpp:84-115`.
- Evidence: QT_STATICPLUGIN is defined but static mne_scan neither links the
  target nor calls `Q_IMPORT_PLUGIN(AdaptiveDenoising)`.
- Impact: supported static builds cannot discover the new plugin.
- Required fix/test: add guarded static link/import wiring and static discovery
  configure/source evidence; real build may retain the known environment block.
- Correction integrated: `78d443144` adds only static-guarded
  `scan_adaptivedenoising` linkage and `Q_IMPORT_PLUGIN(AdaptiveDenoising)`;
  no qrc initializer or shared-build change. Status is addressed pending
  populated configure/source verification and fresh formal review.
- Populated verification: static CMake configure succeeds with mne_scan ON and
  mne_rt_server OFF; generated mne_scan project explicitly references/links the
  generated Adaptive Denoising project. Mark addressed pending formal review;
  target execution remains environment-deferred.

#### R-PLUGIN-ATOMIC-LOCKFREE-001 - P3 - Open

- Location: `adaptivedenoising.cpp:101-145,280-281,463-465`.
- Evidence: callback uint32/uint64 atomics lack C++14 compile-time lock-free
  guards, although they are lock-free on reviewed MSVC x64.
- Impact: a future architecture may silently introduce a library lock.
- Required fix/test: architecture-sensitive C++14 type/macro assertions compiled
  with the real source.
- Correction response: `2b571453da2` maps actual uint32/uint64 aliases through
  C++14 standard atomic lock-free macros and requires value two at compile time;
  MSVC 14.29 C++14 `/Zs` accepts the exact source. Addressed pending integration/
  formal review.
- Manager integration/verification: `6ab7326ac`; exact source compiles under
  MSVC 14.29 C++14 `/Zs`, so both C++14 guards are active. Mark addressed pending
  formal re-review.

### Plugin lifecycle action decision

- Fix `R-PLUGIN-ADMISSION-DROP-001`, `R-PLUGIN-STATIC-REGISTRATION-001` and
  `R-PLUGIN-ATOMIC-LOCKFREE-001` immediately in minimal worker scopes.
- Explicitly defer the destructor/harness pair to one linked follow-up issue;
  their shared prerequisite is a supported host/toolchain lifecycle harness.
- Re-review a fresh exact snapshot. PASS still requires P0/P1 zero and every P2
  either closed or durably deferred; UI remains out of scope until then.

### Plugin data/lifecycle formal re-review - R-PLUGIN-DATA-002

- Snapshot: exact clean/detached
  `3dca8628eb43029ea35980e869da1a44d21b053a`.
- Decision: PASS. New findings P0=0, P1=0, P2=0, P3=0; gate-relevant open
  P0/P1/P2 counts are all zero.
- `R-PLUGIN-SPSC-PRODUCER-001`: CLOSED. Zero-to-one admission, provisional
  invalidation release and stop/restart quiescence preserve the queue SPSC seam.
- `R-PLUGIN-ADMISSION-DROP-001`: CLOSED. Busy counts exact matrix cardinality
  without metadata or queue work; Closed stays silent; Entered retains one
  metadata snapshot and one push attempt per matrix.
- `R-PLUGIN-ATOMIC-LOCKFREE-001`: CLOSED. C++14 actual-width atomic guards are
  compiled and cover the callback admission/drop member types.
- `R-PLUGIN-STATIC-REGISTRATION-001`: CLOSED. Static-only link/import and the
  populated generated project reference are coherent; shared behavior and qrc
  handling are unchanged.
- `R-PLUGIN-DESTRUCTOR-BOUND-001`: explicitly DEFERRED, not fixed, to open issue
  #8. The local bounded-stop contract plus memory-safe unbounded destructor
  fallback is acceptable for this MVP; the residual hang risk remains visible.
- `R-PLUGIN-LIFECYCLE-TEST-001`: explicitly DEFERRED, not fixed, to open issue
  #8, which owns the supported-toolchain real-plugin lifecycle/teardown matrix.
- Regression audit finds no change to queue/processor/test blobs, no stale FIFF
  ownership path, no additional dependency seam and no late-output/restart
  regression. Teaching UI is later scope and its absence is not a finding.
- Evidence attribution: independent review is source/provenance only. Manager
  MOC, callback oracle, MSVC14.29 syntax and static configure remain manager
  evidence; the existing MSVC14.51 Qt/mne_fiff pre-plugin block remains an
  environment boundary rather than runtime evidence.
- Gate consequence: issue #5 may close with issue #8 linked, and the teaching UI
  TDD sequence may begin.

### Teaching widget implementation review - W-PLUGIN-UI-GREEN-001

- Snapshot: integrated commit
  `56fcc5a5ef8453ef6f360edcb965b790719d62ee`; worker source commit
  `aa7ca542c702596c19de1f87931819f29e0362c7` has the exact requested parent and
  four-file scope.
- Decision: ACCEPT. Manager findings P0=0, P1=0, P2=0, P3=0 for the standalone
  widget/diagnostics seam.
- Deep-module assessment: the widget is a narrow view module. It owns controls
  and human-readable formatting, while the fixed scalar diagnostics value is
  the only cross-thread data seam. It owns no plugin, queue, FIFF or numerical
  model state and therefore keeps later lifecycle wiring local to the adapter.
- Test evidence: populated MSVC/Qt Release build/link succeeds; two offscreen
  QtTest executions report 3/0/0 and the explicit rerun exits zero. This proves
  only the public widget contract; plugin pending-state/block-boundary wiring is
  intentionally the next Sol/ultra slice and receives a separate formal review.

### Compatible-toolchain real plugin link evidence - pre-wiring checkpoint

- Configuration: Visual Studio 18 generator with explicit `-T v142` selects
  installed MSVC 19.29.30159/14.29.30133, Qt 5.15.2, mne_scan enabled and
  `BUILD_MNE_RT_SERVER=OFF`.
- Result: the real `scan_adaptivedenoising` target compiles and links to
  `out/Release/apps/mne_scan_plugins/scan_adaptivedenoising.dll`. Its dependency
  graph also builds/links `mne_rtprocessing`, `mne_fiff`, `scShared`, `scMeas`
  and the required project libraries. Vendor/dependency source is unchanged.
- Evidence boundary: this is target compile/link, not a full mne_scan runtime or
  plugin lifecycle test. It proves a compatible local toolchain exists and
  removes the earlier absolute environment blocker for link verification.
- Gate status: pre-wiring snapshot only. Rebuild the exact final wiring snapshot
  with the same v142 configuration before closing `R-CORE-LINK-001` and QA #3.

### Manager UI wiring review - W-PLUGIN-UI-WIRE-001

- Snapshot: worker `3b9aa6b1edc604729942c1f21f339f8cfbc14acf`, integrated as
  `36fa8058915840a5317debf1bb0201f0781fd249`.
- Decision: ACCEPT; manager P0=0, P1=0, P2=0, P3=0. Final independent review is
  still mandatory and this manager decision is not that gate.
- Depth/locality: the public plugin adds only seven narrow control slots and one
  fixed diagnostics signal. Pending state, worker-applied markers, configuration,
  failure handling and diagnostics remain behind the existing PImpl; the
  standalone widget owns only controls/string formatting.
- Realtime seam: parent/child acquisition callback is exact-content identical,
  retains one metadata snapshot and one queue push site, and contains no pending
  snapshot, mutex or wait access.
- Serialization: one mutex-protected snapshot is copied only after successful
  dequeue. Settings revision configures before reset; reset precedes whole-block
  mode/process. UI changes cannot split a block.
- Failure/output: invalid structural metadata disarms and suppresses output;
  allocation/output exceptions disarm and diagnose without stale-model use;
  output pass-through requires valid initialized metadata. Stop emits Stopped
  only after the existing bounded quiescence sequence.
- Verification: exact final v142 real plugin MOC/compile/link exits zero; focused
  Release core/plugin/UI/example/benchmark and Debug core all pass. Issue #8 is
  unchanged and remains the only real-plugin lifecycle/terminal-teardown gap.

### R-CORE-LINK-001 - Addressed pending final independent review

- The exact integrated real plugin target now compiles and links on the local
  compatible v142/MSVC14.29 + Qt5.15.2 configuration, including the shared
  `mne_rtprocessing` client boundary. The former default MSVC14.51 failure is a
  toolset mismatch, not the only available environment.
- No vendor patch or full app/server run is involved. Mark manager-addressed;
  final Sol/ultra review and final-QA publication close the finding.

### Issue-#8 UI lifecycle deferral reconciliation

- Open follow-up #8 already owns the real plugin lifecycle/destructor harness.
  Its required-work matrix now explicitly includes enabled/frozen/numerical-
  settings coalescence, reset only at dequeued block boundaries, and fixed
  diagnostics through the queued GUI connection (including exceptions/drops).
- This is a durable test-coverage deferral, not a code fix. The current source
  still has manager source/order/target evidence but no real plugin lifecycle
  runtime. #8 remains OPEN after MVP acceptance.
