# Worker Ledger

The manager owns the integration branch. Implementation workers use separate
worktrees, Luna/max, one minimal responsibility, and return reviewable commits.
Review workers use Sol/ultra and do not implement fixes.

## Message protocol

```text
REQUEST <id>
from / to / blocking / context / question / expected evidence

RESPONSE <id>
from / to / conclusion / evidence / affected files or commit / next action
```

The manager records and forwards all cross-worker messages here before acting.

## Planning workers

### W-DESIGN-A - Minimal interface

- Thread: `019fcdde-694b-70d1-a0fb-4cbf24f41356`
- Model: `gpt-5.6-luna`, `max`
- Initial wait: timed out; completed after one follow-up.
- Response: packed feature/target matrices, only `process` and `reset`.
- Decision: rejected because lag feature construction leaked algorithm knowledge
  into the mne_scan adapter.

### W-DESIGN-B - Extensible interface

- Thread: `019fcdde-8159-7b60-b896-439b127eb431`
- Model: `gpt-5.6-luna`, `max`
- Initial wait: timed out; completed after one follow-up.
- Response: `IStreamAlgorithm`, typed settings, model layer, and registry.
- Decision: rejected as a hypothetical seam with only one v1 algorithm.

### W-DESIGN-C - Caller-first interface

- Thread: `019fcdde-a032-7640-8e86-b71d00375642`
- Model: `gpt-5.6-luna`, `max`
- Response: concrete full-block `CausalReferenceDenoiser` owning lag and EWLS
  state with safe status results.
- Decision: selected, with controls compressed using A's minimal-interface goal.

### Replacement attempts

- Threads: `019fcdeb-3418-7592-9fdf-85d019a04bc7`,
  `019fcdeb-3e5b-7e13-9786-e2822fbfc1e6`
- Reason: dispatched when A/B had exceeded their first follow-up timeout.
- Status: not used; original A/B later completed. They must not influence code.

## Execution queue

1. `W-TEST-CORE`: numerical TDD tracer bullets.
2. `W-CORE`: Eigen implementation.
3. `R-CORE`: Sol/ultra numerical and real-time review.
4. `W-PLUGIN-DATA`: FIFF adapter and adapter tests.
5. `W-PLUGIN-UI`: controls and diagnostics.
6. `W-EXAMPLE-DOC`: example, benchmark, and learning documentation.
7. `R-INTEGRATION`: Sol/ultra final review.

## Execution workers

### REQUEST W-TEST-CORE-001

- From / to: manager / `W-TEST-CORE`
- Thread: `019fcdfb-9c01-7bb2-88a1-7af4a5621a0d`
- Client setup ID: `client-new-thread:2f7a3181-c13c-4df2-a1f1-4e7d6ed1aa4c`
- Model: `gpt-5.6-luna`, `max`
- Worktree base: `codex/causal-reference-denoising`
- Blocking: yes, for `W-CORE`.
- Task: add exactly one public-interface RED tracer bullet for valid configure
  followed by `BypassTrackHistory`, plus the focused CMake target. Do not add
  production code or further behavioral tests.
- Required response: commit SHA, changed files, narrow RED command and failure
  evidence, beginning `RESPONSE W-TEST-CORE-001`.
- Status: active in worktree `C:/Users/lcy/.codex/worktrees/76e7/mne-cpp-main`.

### REQUEST W-TEST-CORE-001-INFRA

- From / to: manager / `W-TEST-CORE`
- Blocking: no; narrows validation responsibility.
- Evidence: the clean worktree lacks Eigen files such as
  `Eigen/src/Core/util/Macros.h` and `scripts/buildtests.in`. They are ignored
  but present in the main workspace; this is not a configured submodule.
- Decision: do not copy, link, stage, or repair the third-party Eigen tree.
  Commit only the requested test/CMake changes and report the infrastructure
  configure failure separately. The manager will cherry-pick the commit and
  reproduce the intended missing-production-header RED in the main workspace.
- Status: forwarded and followed; temporary dependency/build files were removed.

### RESPONSE W-TEST-CORE-001

- Commit: `79cbd1752d2d8501786a007c2e02bedbb7d5057f`
- Changed files: top-level testframe list, focused test CMake, one Qt test source.
- Contract: valid configuration returns `DenoiserStatus::Configured`;
  `BypassTrackHistory` returns `DenoiserProcessStatus::Bypassed` in
  `DenoiserProcessResult::status` and preserves the complete matrix.
- Test count: one Qt slot; no production files added.
- Worker validation: clean-worktree configure failed before target generation
  because of the documented ignored Eigen baseline files. Intended RED not
  claimed.
- Manager review: patch is scoped and matches the repository's current Qt test
  CMake convention. Accepted for cherry-pick; manager must reproduce RED.
