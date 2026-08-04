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

### E-007 - Initial push attempt failed

- Actor: manager
- Evidence: `git push -u origin codex/causal-reference-denoising` returned
  `OpenSSL SSL_connect: SSL_ERROR_SYSCALL` before updating the remote.
- Result: local commit `79bf9fed1` is intact; no force operation was attempted.
- Next: perform a read-only remote check and retry the same push after
  connectivity recovers.

### E-008 - Feature branch published

- Actor: manager
- Evidence: remote connectivity and `gh auth status` succeeded; the second
  non-force push created `origin/codex/causal-reference-denoising`.
- Result: local branch now tracks the remote feature branch.
- Next: publish the replacement GitHub issue.

### E-009 - Replacement issue published

- Actor: manager
- Result: created `Edddddddddy/mne-cpp#2`, `[ENH] Add causal block-EWLS
  reference denoising to mne_scan`, with the selected interface, tasks,
  acceptance gates, provenance, and branch-history note.
- URL: https://github.com/Edddddddddy/mne-cpp/issues/2
- Next: link and close issue #1 as superseded.

### E-010 - Old issue closed

- Actor: manager
- Result: added a supersession comment linking issue #2 and closed
  `Edddddddddy/mne-cpp#1` as not planned.
- Next: commit and push the publication record, then dispatch the core TDD
  tracer-bullet worker.

### E-011 - Core tracer-bullet worker dispatched

- Actor: manager
- Request: `W-TEST-CORE-001`.
- Model/environment: Luna/max, separate worktree from the published feature
  branch.
- Scope: one configure-plus-bypass RED tracer bullet and focused test target;
  no production code and no horizontal batch of tests.
- Next: wait for worktree setup and a structured response.

### E-012 - Core tracer worktree ready

- Actor: manager
- Result: resolved worker thread
  `019fcdfb-9c01-7bb2-88a1-7af4a5621a0d` in isolated worktree `76e7`.
- Next: wait for `RESPONSE W-TEST-CORE-001` without modifying the worker tree.

### E-013 - Clean-worktree Eigen baseline gap identified

- Actor: worker and manager
- Evidence: the worker configure stopped before the new target because Eigen
  files ignored by the repository snapshot are absent in a clean worktree;
  the same ignored files exist in the main workspace.
- Decision: do not mutate or vendor the third-party tree. Worker commits only
  its test files; manager validates the intended RED after cherry-pick in the
  main workspace.
- Next: forward `REQUEST W-TEST-CORE-001-INFRA`.

### E-014 - Core tracer response reviewed

- Actor: `W-TEST-CORE` and manager
- Response: `RESPONSE W-TEST-CORE-001`, commit `79cbd1752`.
- Result: accepted a three-file patch containing one public-interface test and
  no production code. Worker correctly did not claim the infrastructure
  failure as TDD RED.
- Next: cherry-pick the commit and reproduce the intended missing-header RED
  in the main workspace.

### E-015 - Tracer target exposed unrelated dependency failure

- Actor: manager
- Evidence: focused configure succeeded, but target build stopped in Qt 5.15
  `qlist.h` while building `mne_fiff` with MSVC 2026; the tracer source was not
  compiled.
- Decision: the pure numerical test must not inherit the legacy full-MNE test
  link template. Return a focused CMake revision request to the original worker.
- Next: forward `REQUEST W-TEST-CORE-001-REVISE` and rerun RED after integration.

### E-016 - Pure numerical test target isolated

- Actor: `W-TEST-CORE` and manager
- Response: commit `76fbc02ad` removes FIFF/display/3D/UI dependencies from
  the tracer target and retains only Qt Core/Test plus Eigen.
- Result: one-file revision reviewed and accepted.
- Next: cherry-pick, regenerate the focused build, and require the missing
  production header as the first valid RED.
