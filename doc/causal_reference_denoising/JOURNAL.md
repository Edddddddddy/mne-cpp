# Causal Reference Denoising Journal

Append-only structured event log. Visible decisions and evidence are recorded;
hidden reasoning is not.

## 2026-08-05

### E-001 - Requirements locked

- Actor: user and manager
- Result: generic FIFF reference-channel scope; one v1 algorithm; block-recursive
  least squares; minimal teaching UI; structured per-event logging; local
  `master` baseline; balanced quantitative gates.
- Next: compare interfaces and prepare a clean execution baseline.

### E-002 - Design It Twice completed

- Actor: W-DESIGN-A, W-DESIGN-B, W-DESIGN-C, manager
- Evidence: worker entries in `WORKERS.md`.
- Result: selected a C+A hybrid. The concrete module accepts a full Eigen block
  and resolved row indices, owns lag/EWLS state, and exposes configure/process/
  reset. Rejected packed-feature leakage and a premature runtime registry.
- Next: clean old task state and start a fresh branch.

### E-003 - Old task audited

- Actor: manager
- Evidence: `git diff master..codex/realtime-adaptive-denoising` showed only
  seven files under `doc/realtime_adaptive_denoising/` and 392 insertions.
- Result: authorized cleanup target confirmed.
- Next: switch to master, delete old branch and old build artifact.

### E-004 - Clean branch created

- Actor: manager
- Result: deleted local `codex/realtime-adaptive-denoising` at `33a0581b1`,
  removed ignored `build-adaptive-denoise/`, and created
  `codex/causal-reference-denoising` from local `master`.
- Preserved: `.codex-build/`, `src/build/`,
  `doc/mne_scan_technical_document.md`, `codex/babymeg-stability`.
- Next: commit durable specification and publish the GitHub issue.

### E-005 - Development skills activated

- Actor: manager
- Result: `codebase-design` governs the deep module seam, `tdd` requires
  vertical red/green tracer bullets through public interfaces, and `github`
  governs issue publication and milestone reporting.
- Next: initial documentation commit.

### E-006 - Durable record initialized

- Actor: manager
- Result: created `STATE.md`, `SPEC.md`, `JOURNAL.md`, `WORKERS.md`, and
  `REVIEW.md` on the new branch; verified that no unrelated untracked path was
  included.
- Next: commit and push only `doc/causal_reference_denoising/`.
