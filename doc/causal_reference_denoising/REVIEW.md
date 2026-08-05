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

#### R-CORE-LINK-001 - P3 - Open

- Location: focused test `CMakeLists.txt:13-16,27-39`; library
  `CMakeLists.txt:25,52,67`.
- Evidence/impact: direct source compilation preserves isolation but cannot
  detect real shared/static library export/client-link regressions.
- Requested later fix: retain isolated test and add a small supported-form
  `mne_rtprocessing` client-link smoke before final integration.

### Formal core gate decision

- P0: zero.
- P1: zero.
- P2: zero open; `R-CORE-FORGET-001` closed by independent analytic evidence.
- P3: `R-CORE-LOCALITY-001` and `R-CORE-LINK-001` tracked for later
  integration.
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

#### R-PROC-LOCALITY-001 - P3 - Open

- Location: `adaptivedenoisingprocessor.h:2-5,32-103`.
- Evidence/impact: worker ownership, inclusive UI ranges, selection, returned-
  failure disarm, Ready reset, allocation/exception behavior and hot-path rules
  are non-local to the public declarations.
- Later correction: compact Doxygen plus explicit copy/move policy. State that
  allocation exceptions propagate while preserving old state so callers must
  fail closed rather than use old ownership for new metadata.
- Verification: header contract review against SPEC and compile-time ownership/
  noexcept checks.
- Tracking: deferred to final QA issue #3 at
  `https://github.com/Edddddddddy/mne-cpp/issues/3#issuecomment-5194141240`.

#### R-PROC-BOUNDARY-001 - P3 - Open

- Location: `adaptivedenoisingprocessor.cpp:49-58,125-135`;
  focused processor test `:133-141,275-404`.
- Evidence/impact: invalid low/high cases and P=288 are covered, but inclusive
  legal maxima and exact P=256 acceptance are not. Source is currently correct,
  yet a narrowing regression could pass the focused suite.
- Later correction/test: Ready rows for taps 32, interval 2048, memory 1/300,
  regularization 1 and P=256, retaining an over-cap disarm case.
- Tracking: deferred to final QA issue #3 in the same comment above.

### Processor formal gate decision

- P0: zero.
- P1: zero.
- P2: zero open; `R-PROC-MOVE-001` closed by explicit ownership and compiled
  C++14 traits.
- P3: `R-PROC-LOCALITY-001` and `R-PROC-BOUNDARY-001` tracked for later
  processor/final-QA follow-up.
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
