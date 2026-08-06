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

## Queue formal review pending

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

#### R-CORE-LOCALITY-001 - P3 - Open

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

#### R-CORE-CXX14-NOEXCEPT-001 - P1 - Open

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
- P3: `R-CORE-LOCALITY-001` is under final-QA correction;
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
