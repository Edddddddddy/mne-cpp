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

`R-DIAGNOSTICS-001` is prepared for Sol/ultra. It will define the minimum
fixed-size process diagnostics and numerical-solve rejection semantics before
the next test, without reviewing or changing plugin/UI code.

## Final integration review

Pending.
