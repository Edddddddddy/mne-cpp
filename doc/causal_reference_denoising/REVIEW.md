# Review Ledger

## Policy

- Review findings use IDs, priority P0-P3, exact file/line, evidence, requested
  correction, and required verification.
- P0/P1 must be fixed before integration completes.
- P2 must be fixed or explicitly deferred in the GitHub issue.
- Reviewers use `gpt-5.6-sol` with `ultra` reasoning and do not edit code.
- Fixes are returned to the owning Luna/max worker through a logged
  `REQUEST`/`RESPONSE` exchange.

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
