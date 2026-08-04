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

### REQUEST W-TEST-CORE-001-REVISE

- From / to: manager / `W-TEST-CORE`
- Evidence: after cherry-pick, focused target generation succeeded, but the
  copied legacy test CMake linked the full MNE dependency set. MSVC 2026 failed
  in unrelated Qt 5.15 / `mne_fiff` code before compiling the tracer test.
- Required change: make this Eigen-only numerical test target link only Qt
  Core/Test and `eigen`, and add only the `src/libraries` include root needed
  for `<rtprocessing/...>`. Do not link `mne_rtprocessing` or other MNE/3D/UI
  libraries in this tracer target. Keep the single test and no production code.
- Required response: a new commit and exact changed CMake evidence, beginning
  `RESPONSE W-TEST-CORE-001-REVISE`.
- Status: completed.

### RESPONSE W-TEST-CORE-001-REVISE

- Commit: `76fbc02ad628ba8e0e2f5a76e305b39e4a50e505`
- Change: reduced Qt dependencies to Core/Test, removed all MNE library links,
  retained `eigen`, and added `${CMAKE_SOURCE_DIR}/libraries` as the only local
  include root.
- Manager review: one-file focused diff; accepted for cherry-pick.

### MANAGER VALIDATION W-TEST-CORE-001-RED

- Integrated commits: `59537fdd6` and `650338f19`.
- Configure: focused MSVC 2026 / Qt 5.15 build generation succeeded.
- RED command: `cmake --build build-causal-reference-denoising --target
  test_causal_reference_denoiser --config Release -- /m:2`.
- Expected failure: MSVC `C1083`, missing
  `rtprocessing/causalreferencedenoiser.h` while compiling the tracer source.
- Result: valid RED accepted; `W-CORE` may begin.

### REQUEST W-CORE-001

- From / to: manager / `W-CORE`.
- Client setup ID: `client-new-thread:719101fb-b9e7-4b6f-99ae-7f77dca31fe6`.
- Thread: `019fce0e-59ad-7ce1-91a3-db5f4fd233e8`.
- Worktree: `C:/Users/lcy/.codex/worktrees/ef9b/mne-cpp-main`.
- Model/environment: `gpt-5.6-luna`, `max`, new worktree from the current
  integration branch.
- Blocking: yes, for every later core behavior.
- Context: the one-slot tracer currently fails only because
  `<rtprocessing/causalreferencedenoiser.h>` is absent.
- Task: add the minimum Eigen/STL production header/source and CMake wiring to
  make the existing valid-config plus `BypassTrackHistory` tracer GREEN.
  Preserve the entire input block and expose the tracer's fixed public status
  names. Do not add a second test or implement the EWLS algorithm yet.
- Required discipline: read the durable records in recovery order; use the
  `codebase-design` and `tdd` skills; keep the focused test isolated from the
  full MNE/FIFF/UI dependency graph; do not repair or vendor the ignored Eigen
  baseline in the clean worktree.
- Required response: `RESPONSE W-CORE-001`, commit SHA, changed files, focused
  command/evidence available in the worktree, limitations, and next suggested
  single RED behavior.
- Status: response received; pending main-workspace validation.

### RESPONSE W-CORE-001

- Conclusion: implemented the minimum first-GREEN production seam.
- Commit: `c3958cde93474c9ed10992987fb604abb7b5a603`.
- Changed files: `causalreferencedenoiser.h/.cpp`, RTPROCESSINGLIB CMake list,
  and focused-test CMake list.
- Design: the public config/mode/method seam matches `SPEC.md`; the tracer
  compiles the numerical source directly while the library also registers it;
  bypass returns the fixed status and does not write the input block.
- Scope held: no second test, EWLS, plugin, or ignored Eigen repair.
- Worker evidence: `git diff --check` passed. Clean-worktree configure stopped
  at the already documented missing ignored Eigen baseline before target
  generation, so the worker did not claim GREEN.
- Suggested next RED: reject one invalid configuration transactionally while
  preserving a prior committed configuration.
- Manager static review: four-file diff is scoped, uses the repository export
  macro, and keeps the numerical test isolated. Accepted for cherry-pick and
  populated-main-workspace validation; runtime GREEN is not yet claimed.

### MANAGER VALIDATION W-CORE-001-GREEN

- Integrated commit: `fc86ff7a6`.
- Build command: `cmake --build build-causal-reference-denoising --target
  test_causal_reference_denoiser --config Release -- /m:2`.
- Build result: success; both the tracer and `causalreferencedenoiser.cpp`
  compiled and linked. Eigen emitted only code-page C4819 warnings.
- Run command: `out/Release/apps/test_causal_reference_denoiser.exe -txt`.
- Run result: exit code 0.
- Decision: first tracer is GREEN. The next task must add only the
  transactional invalid-config RED; no EWLS implementation may begin yet.

### REQUEST W-TEST-CORE-002

- From / to: manager / `W-TEST-CORE`.
- Client setup ID: `client-new-thread:81758647-73c5-4792-b809-9b3be67a7c4c`.
- Thread: `019fce18-2d75-7cf3-ab76-3a8694b4edca`.
- Worktree: `C:/Users/lcy/.codex/worktrees/8a0d/mne-cpp-main`.
- Model/environment: `gpt-5.6-luna`, `max`, new worktree from the current
  integration branch.
- Blocking: yes, for the next production slice.
- Task: add exactly one new Qt test slot proving rejected reconfiguration does
  not replace a prior valid configuration. Configure a valid three-row layout,
  then submit a candidate with four channels and an invalid zero block bound;
  expect `DenoiserStatus::InvalidConfiguration`. Prove the old layout remains
  committed by requiring a four-row bypass block to return
  `DenoiserProcessStatus::InvalidShape` unchanged, then a three-row bypass
  block to return `Bypassed` unchanged.
- Scope: test source only; no production or CMake change, no additional
  validation matrix, no EWLS behavior.
- Required response: `RESPONSE W-TEST-CORE-002`, commit SHA, changed file,
  focused RED command and missing-enum/status evidence, or the documented
  clean-worktree infrastructure limitation.
- Status: response received; pending main-workspace RED reproduction.

### RESPONSE W-TEST-CORE-002

- Conclusion: added exactly one transactional configuration Qt test slot.
- Commit: `6b6ccf5c46234eedfdda2e8e91b3a50379af2f5a`.
- Changed file: `test_causal_reference_denoiser.cpp` only.
- Contract fixed: `DenoiserStatus::InvalidConfiguration` and
  `DenoiserProcessStatus::InvalidShape`; rejected four-channel/zero-block
  candidate must leave the previously committed three-channel layout active,
  and both rejected-shape and valid-shape bypass blocks remain value-identical.
- Worker evidence: `git diff --check` passed. Clean-worktree configure stopped
  at the known ignored Eigen baseline gap, so no RED build was claimed.
- Manager static review: the one-slot test observes transactionality through
  both rejected and accepted process shapes and stays within the requested
  test-only scope. Accepted for cherry-pick and RED reproduction.

### MANAGER VALIDATION W-TEST-CORE-002-RED

- Integrated test commit: `e467f4f54`.
- Command: `cmake --build build-causal-reference-denoising --target
  test_causal_reference_denoiser --config Release -- /m:2`.
- Expected failure: MSVC C2838/C2065 at test lines 97 and 109 because
  `DenoiserStatus::InvalidConfiguration` and
  `DenoiserProcessStatus::InvalidShape` are not yet declared.
- Result: valid RED accepted; production implementation may begin.

### REQUEST W-CORE-002

- From / to: manager / `W-CORE`.
- Model/environment: `gpt-5.6-luna`, `max`, new worktree from the current
  integration branch.
- Blocking: yes, for later core tests.
- Task: make only the transactional test GREEN. Add the two fixed status enum
  values; validate positive `channelCount` and `maxBlockSamples` before
  committing them; a rejected configure must preserve the prior values;
  bypass processing must return `InvalidShape` without mutation when rows do
  not equal the committed channel count or columns are outside the committed
  positive block bound. Keep the original bypass behavior for valid shape.
- Scope: numerical header/source only; no new test, history, EWLS, row picks,
  nonfinite scan, allocation, plugin, or CMake change.
- Required response: `RESPONSE W-CORE-002`, commit SHA, changed files, evidence,
  limitations, and the next recommended single RED behavior.
- Status: recorded before dispatch.
