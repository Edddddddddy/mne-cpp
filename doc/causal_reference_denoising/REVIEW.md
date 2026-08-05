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

Pending.

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

#### R-APPLY-001 - P2 - In progress

- Location: `causalreferencedenoiser.cpp:328-332`.
- Evidence: finite weights/features can yield nonfinite prediction/residual,
  currently written while returning `Processed`.
- Fix: define a finite-output application fallback before synthetic acceptance.
- Required test: trained finite model plus extreme finite probe never silently
  returns nonfinite `Processed` output.
- Selected policy: atomic sample-wide target pass-through, zero actually-
  subtracted prediction diagnostics, valid Processed block and advancing
  history/transactional learning state; public diagnostics surface unchanged.

#### R-STATUS-001 - P3 - Closed

- Location: `causalreferencedenoiser.cpp:282-286`.
- Evidence: unconfigured module is conflated with invalid shape.
- Fix/test: `NotConfigured` with unchanged block, zero snapshot and NaN RMS.
- Resolution: implemented in `b5f29fff1`; diagnostics lifecycle test passes in
  the 38/0 synchronized report.

No P0 finding. P1 findings must be closed before core completion; all P2
findings will be fixed or explicitly deferred in issue #2.

## Final integration review

Pending.
