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

## Lifecycle and model policy

- Preserve a worker conversation only when the same role, repository state,
  and context remain reusable. Archive one-shot, failed, superseded, or stale
  workers after their request/response/commit evidence is durable.
- The app currently exposes archive but not hard delete. Archive is therefore
  the implemented retirement operation; do not manually delete app-managed
  worktrees. If hard thread deletion becomes supported, it may replace archive.
- Default test/implementation model: `gpt-5.6-luna`, `max` (Luna's highest
  supported effort). Use `gpt-5.6-sol` for manager-selected high-risk math,
  real-time/concurrency, architecture, or review tasks; formal reviewers use
  Sol/ultra.

### RETIREMENT REQUEST CLEANUP-001

- Keep: selected reusable design conversation `W-DESIGN-C`.
- Archive after this record is committed: rejected design A/B and replacement
  attempts; completed one-shot execution workers W-TEST-CORE-001 through 003,
  W-CORE-001, failed W-CORE-002, its successful replacement, and W-CORE-003.
- Evidence remains in this ledger and git history before archival.
- Status: recorded before action.

### RETIREMENT RESPONSE CLEANUP-001

- Result: all 11 requested threads were archived successfully:
  `W-DESIGN-A`, `W-DESIGN-B`, `W-DESIGN-A2`, `W-DESIGN-B2`,
  `W-TEST-CORE-001`, `W-CORE-001`, `W-TEST-CORE-002`, failed `W-CORE-002`,
  `W-CORE-002-R`, `W-TEST-CORE-003`, and `W-CORE-003`.
- Retained: selected reusable `W-DESIGN-C` thread
  `019fcdde-a032-7640-8e86-b71d00375642` and the manager task.
- Worktree policy: no app-owned worktree was manually deleted.
- Status: complete.

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
- Client setup ID: `client-new-thread:8edf70e8-a904-4eb0-aa66-52a9f3b7cb65`.
- Worktree: `C:/Users/lcy/.codex/worktrees/a425/mne-cpp-main`.
- Thread: `019fce1d-7aad-7f12-9781-8497c9f66123`.
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
- Status: dispatched; worktree exists at integration commit `3c338c8f2`.
  Two read-only app thread-list calls timed out and were terminated; the task
  was not dispatched again. The session index resolved the thread, and a
  direct bounded snapshot confirmed it is active.

### SYSTEM FAILURE W-CORE-002

- Thread status: `systemError`; no assistant response or tool marker was
  returned.
- Worktree audit: clean at `3c338c8f2`; no commit, staged file, or unstaged
  change exists.
- Decision: preserve the worktree for evidence and dispatch one replacement
  task with identical scope. No code can be accepted from this failed attempt.

### REQUEST W-CORE-002-R

- From / to: manager / replacement `W-CORE`.
- Client setup ID: `client-new-thread:02f95774-afd9-4ae2-966b-fb94144561a3`.
- Thread: `019fce3e-f589-7a73-b2f4-bb8bad58f90b`.
- Worktree: `C:/Users/lcy/.codex/worktrees/06cd/mne-cpp-main`.
- Model/environment: `gpt-5.6-luna`, `max`, new worktree from the current
  integration branch.
- Blocking: yes.
- Task and scope: identical to `W-CORE-002`; make only the transactional
  configuration test GREEN in the numerical header/source.
- Required response: `RESPONSE W-CORE-002-R` with commit and focused evidence.
- Status: active; direct bounded snapshot confirmed the worker is reading the
  required durable records and skills.

### RESPONSE W-CORE-002-R

- Conclusion: implemented transactional channel/block-bound validation and
  invalid-shape bypass handling.
- Commit: `8d9a6501058d4629ab95338bec13aa7eab8eba59`.
- Changed files: `causalreferencedenoiser.h/.cpp` only.
- Implementation: adds `InvalidConfiguration`/`InvalidShape`; rejects invalid
  bounds before assignment; commits two bounds on success; compares rows and
  positive/max columns in the allocation-free `noexcept` process path without
  writing the block.
- Worker evidence: `git diff --check` passed. Focused build remained unavailable
  in the clean worktree because no build directory/ignored Eigen baseline was
  present; nothing was copied or repaired.
- Suggested next RED: prove `BypassTrackHistory` advances causal lag history
  while preserving output.
- Manager static review: initialization, early return, and assignment order are
  transactional for the tested fields; no unrequested behavior or dependency
  was added. Accepted for cherry-pick and populated-workspace validation.

### MANAGER VALIDATION W-CORE-002-R-GREEN

- Integrated commit: `cc84fd082`.
- Command: focused Release build of `test_causal_reference_denoiser`, followed
  by `out/Release/apps/test_causal_reference_denoiser.exe -txt`.
- Result: build/link succeeded and test executable returned exit code 0.
- Warnings: only the known Eigen/MSVC C4819 code-page warnings.
- Decision: transactionality/shape slice is GREEN. Next RED covers the complete
  configuration invariant matrix, not streaming history or EWLS yet.

### REQUEST W-TEST-CORE-003

- From / to: manager / `W-TEST-CORE`.
- Client setup ID: `client-new-thread:2c0f559b-65c0-4e65-999a-3f6d132c16c8`.
- Thread: `019fce45-615b-7612-9f23-3694f6c581eb`.
- Worktree: `C:/Users/lcy/.codex/worktrees/02c2/mne-cpp-main`.
- Model/environment: `gpt-5.6-luna`, `max`, new worktree from current
  integration branch.
- Blocking: yes, for full configure validation.
- Task: add one data-driven Qt test behavior asserting
  `InvalidConfiguration` for each remaining SPEC invariant: non-finite or
  non-positive sampling frequency; non-positive tap count/update interval;
  non-finite or non-positive memory; regularization outside `[1e-8, 1]` or
  non-finite; empty reference/target sets; duplicate, negative, or out-of-range
  rows; reference/target overlap; and `referenceCount * tapCount > 256`.
- Scope: test source only; no production/CMake change, no streaming behavior.
  Existing channel/max-bound coverage need not be duplicated.
- Required response: `RESPONSE W-TEST-CORE-003`, commit SHA, changed file,
  available evidence, and expected first failing data row.
- Status: active; direct snapshot confirmed required-record reading.

### RESPONSE W-TEST-CORE-003-PATH-CHECK

- From / to: worker / manager.
- Blocking: no; worker reported root-level `STATE.md`-style paths were absent
  and began locating the records.
- Manager evidence: worktree `02c2` is at `652492ff6`, and
  `doc/causal_reference_denoising/STATE.md` exists there.
- Conclusion: use the full repository-relative directory for all five records;
  no checkout, copy, or scope change is needed.

### REQUEST W-TEST-CORE-003-PATH

- From / to: manager / `W-TEST-CORE-003`.
- Blocking: no.
- Instruction: read `doc/causal_reference_denoising/{STATE,SPEC,WORKERS,REVIEW}.md`
  and the tail of `doc/causal_reference_denoising/JOURNAL.md`; continue the
  original test-only task unchanged.
- Status: recorded before forwarding.

### RESPONSE W-TEST-CORE-003

- Conclusion: added one data-driven invalid-configuration behavior.
- Commit: `eb7573efeefb4996e2d417e84d2d224c07c0573b`.
- Changed file: `test_causal_reference_denoiser.cpp` only.
- Coverage: 26 named rows for scalar finiteness/ranges, non-empty row sets,
  uniqueness, sign/range, disjointness, and the 256-feature cap.
- Worker evidence: diff checks passed and worktree was clean. Its focused
  configure stopped before generation due a clean-worktree toolset/dependency
  limitation; no ignored Eigen repair or rt_server run occurred.
- Expected first RED row: `samplingFrequencyHz_zero`.
- Manager static review: each row starts from the valid three-channel config;
  the feature-cap row uses one valid reference with 257 taps, isolating the
  requested cap. Accepted for cherry-pick and runtime RED reproduction.

### MANAGER VALIDATION W-TEST-CORE-003-RED

- Integrated commit: `19a23b9a6`.
- Build: focused Release target succeeded.
- Run: synchronized hidden-window Qt execution with text report under the
  ignored focused build directory.
- Result: 4 existing checks passed, all 26 new rows failed at the intended
  configure assertion, and process exit code was 26. First failure:
  `samplingFrequencyHz_zero`.
- Decision: valid runtime RED; full configure validation may be implemented.

### REQUEST W-CORE-003

- From / to: manager / `W-CORE`.
- Client setup ID: `client-new-thread:014022fe-666d-4dd4-a631-2aea6180d892`.
- Thread: `019fce4c-69f2-7193-b5c7-47bad21bf7c0`.
- Worktree: `C:/Users/lcy/.codex/worktrees/5822/mne-cpp-main`.
- Model/environment: `gpt-5.6-luna`, `max`, new isolated worktree.
- Blocking: yes, before streaming state.
- Task: make the 26-row invariant behavior GREEN in numerical source/header
  only. Validate finite positive fs/memory; positive tap/update; regularization
  in `[1e-8,1]`; non-empty, unique, in-range, disjoint rows; and feature cap
  using overflow-safe arithmetic. Perform all validation before existing state
  assignments so failed configure remains transactional.
- Scope: no tests/CMake, row-state copies, scratch allocation, history, EWLS,
  nonfinite block scan, diagnostics, or plugin code.
- Required response: `RESPONSE W-CORE-003` with commit, files, evidence,
  implementation notes, and next single RED recommendation.
- Status: response received; pending main-workspace validation.

### RESPONSE W-CORE-003

- Conclusion: implemented complete validation for the current configuration
  contract.
- Commit: `f91bab1106907580cdcfd5d8d03288a2403c2a30`.
- Changed file: `causalreferencedenoiser.cpp` only.
- Strategy: `std::isfinite` scalar checks; allocation-free nested loops for row
  range/uniqueness/disjointness; inclusive regularization bounds; division
  comparison for the 256-feature cap. All checks short-circuit before the two
  state assignments.
- Worker evidence: `git diff --check` passed and worktree clean; focused build
  blocked by the known absent ignored Eigen baseline, without repair/copy.
- Suggested next RED: causal lag-history advancement in bypass mode.
- Manager static review: empty reference rejection short-circuits before the
  feature-cap division, tap count is proven positive, helpers do not allocate,
  and all assignments remain after validation. Accepted for cherry-pick.

### MANAGER VALIDATION W-CORE-003-GREEN

- Integrated commit: `e80e02098`.
- Focused Release build: succeeded.
- Synchronized Qt report: 30 passed, 0 failed, exit code 0.
- Decision: configure validation is GREEN. Next RED is one end-to-end causal
  epoch tracer, not another validation case.

### REQUEST W-TEST-CORE-004

- From / to: manager / `W-TEST-CORE`.
- Client setup ID: `client-new-thread:4d463d8d-4f14-4a23-aa69-a84c23e9718b`.
- Thread: `019fd00e-e7ff-7220-96b2-1310acbebf25`.
- Worktree: `C:/Users/lcy/.codex/worktrees/725c/mne-cpp-main`.
- Model/environment: `gpt-5.6-luna`, `max`, new isolated worktree.
- Blocking: yes, before numerical streaming implementation.
- Task: add exactly one test slot for a three-channel stream with reference row
  0, target row 1, preserved row 2, two taps, and a two-sample update interval.
  Feed one warmup sample plus two independent feature samples in
  `ApplyAndLearn`; require a new `DenoiserProcessStatus::Processed`, unchanged
  first block, then feed one post-boundary sample in `ApplyOnly` and require the
  known current+lag reference noise to be removed within `1e-4` while reference
  and preserved rows remain exact. This observes tap order, warmup, causal
  commit ordering, and target-only write in one vertical behavior.
- Scope: test source only; no production/CMake, chunk-equivalence/reset/
  nonfinite/malloc cases yet.
- Required response: `RESPONSE W-TEST-CORE-004`, commit, exact synthetic values,
  expected RED evidence, and limitations.
- Status: response received; pending main-workspace RED reproduction.

### RESPONSE W-TEST-CORE-004

- Commit: `37edd7ef50b2bea69f7515730c6871a48f680edd`.
- Changed file: `test_causal_reference_denoiser.cpp` only.
- Behavior: two taps and a two-eligible-sample epoch; first block uses reference
  `[1,2,3]`, target `[5,7,12]`, and preserved `[100,101,102]`; post-boundary
  ApplyOnly sample is `[5,19,103]`. Requires `Processed`, exact unchanged first
  block, exact non-target rows, and target residual `<=1e-4`.
- Worker evidence: `git diff --check` passed and worktree clean. Configure was
  blocked by missing ignored Eigen files; no repair/copy or rt_server run.
- Expected RED: missing `DenoiserProcessStatus::Processed`.
- Manager static review: the two eligible feature vectors `[2,1]` and `[3,2]`
  are independent and identify weights `[2,3]`; the test observes warmup,
  tap-major order, boundary commit, and target-only write. Accepted.

### MANAGER VALIDATION W-TEST-CORE-004-RED

- Integrated commit: `fb0a42385`.
- Command: focused Release target build.
- Expected failure: MSVC C2838/C2065 at test lines 287 and 298 because
  `DenoiserProcessStatus::Processed` is not declared.
- Result: valid compile RED; streaming implementation may begin.

### RETIREMENT REQUEST CLEANUP-002

- Archive completed one-shot `W-TEST-CORE-004` thread
  `019fd00e-e7ff-7220-96b2-1310acbebf25` after this record is committed.
- Status: complete; app returned `archived: true`.

### REQUEST W-CORE-004

- From / to: manager / high-risk numerical `W-CORE`.
- Client setup ID: `client-new-thread:403e8243-3d5d-453b-ae59-ebcdddb7d7b7`.
- Thread: `019fd018-8939-7e72-9fc8-17085c79806e`.
- Worktree: `C:/Users/lcy/.codex/worktrees/9bd8/mne-cpp-main`.
- Model/environment: `gpt-5.6-sol`, `ultra`, new isolated worktree.
- Model decision: Sol is selected because this slice simultaneously introduces
  causal indexing, recursive EWLS math, Eigen LDLT, transactional preallocation,
  and real-time hot-path constraints.
- Blocking: yes.
- Task: make only the causal epoch tracer GREEN with a general Eigen
  implementation. Add `Processed`; move allocated state behind a private Impl
  constructed transactionally in `configure`; preallocate row copies, lag
  history, feature/raw/prediction vectors, `G`, `H`, committed `W`, candidate
  weights, solve matrix, and sized LDLT. Reset candidate state on configure.
  For each eligible sample build tap-major features, apply committed `W` first,
  learn from raw targets only in `ApplyAndLearn`, exponentially forget per
  sample, and solve at exact update boundaries with scale-aware diagonal
  loading and LDLT (no inverse). Commit only successful finite candidates for
  later samples. Always advance history; warmup consumes `tapCount-1` samples.
- Real-time scope: `process` remains `noexcept` and performs no explicit
  allocation, lock, or string work; configure is the allocation phase.
- Exclusions: no new tests/CMake/plugin; no chunk-equivalence, nonfinite atomic
  pass-through, diagnostics, malloc guard, or benchmark in this slice.
- Required response: `RESPONSE W-CORE-004`, commit SHA, files, state shapes,
  solve/loading definition, validation evidence/limitations, and next RED.
- Status: active; direct snapshot confirmed required skill/record recovery.

### RESPONSE W-CORE-004-DELEGATION-NOTICE

- From / to: `W-CORE-004` / manager.
- Notice: the worker commentary states it started two nested Sol/ultra
  read-only reviewers while inspecting numerical ordering/allocation.
- Conflict: the manager conversation is the sole task dispatcher, and every
  worker request/response must be manager-routed and durable before action.
- Decision: nested reviewer findings are not authorized evidence and must not
  influence the implementation unless separately returned to the manager under
  the protocol.

### REQUEST W-CORE-004-DELEGATION

- From / to: manager / `W-CORE-004`.
- Blocking: no for the implementation; blocking for use of nested findings.
- Instruction: stop or disregard the two nested reviewers, do not spawn more,
  report their identifiers/status in the final response, and complete the
  original header/source task alone. Formal review remains manager-owned.
- Status: recorded before forwarding.

### RESPONSE W-CORE-004

- Conclusion: implemented and committed the general causal EWLS numerical core.
- Commit: `f3a909941b535abaf5e97bdacce4ba960b059051`.
- Changed files: `causalreferencedenoiser.h/.cpp` only.
- State: PImpl owns row copies, `R x (L-1)` history, `P=R*L` feature,
  `M` raw/prediction vectors, `G(PxP)`, `H(MxP)`, committed `W(MxP)`, solve
  matrix/RHS/candidate scratch, and sized `LDLT<MatrixXd>`.
- Ordering: history column 0 is lag 1; `phi[lR+r]=r_r(t-l)`; first `L-1`
  samples warm history; committed W applies before raw-target EWLS update; an
  accepted boundary candidate affects later samples only.
- Loading: `A = sym(G) + regularization * s * I`, with
  `s=max(epsilon,max_i(abs(sym(G)_ii)))`; solve uses LDLT without inverse and
  commits only finite successful candidates.
- Allocation claim: default construction holds null `unique_ptr`; configure
  builds/sizes a candidate Impl before swap; process contains no explicit
  allocation/lock/string and reuses sized storage. Malloc guard is not yet run.
- Worker evidence: diff check and a numerical oracle passed; clean-worktree
  configure stopped at missing ignored Eigen baseline, so no compiled GREEN was
  claimed.
- Delegation correction: nested threads
  `019fd019-994e-7380-9be9-105d31501639` and
  `019fd019-b14a-7473-95c9-6d18c5cb91ff` were both interrupted; findings were
  disregarded.
- Suggested next RED: selected NaN/Inf must atomically pass through without
  changing history or learning state.
- Manager static review: causal/tap ordering and transaction boundaries match
  the tracer; only target rows are written. Accepted for cherry-pick, with
  Eigen API compilation and runtime still required.

### MANAGER VALIDATION W-CORE-004-GREEN

- Integrated commit: `10e33a1b8`.
- Focused Release build: succeeded, including Eigen LDLT/PImpl APIs.
- Synchronized Qt report: 31 passed, 0 failed, exit code 0.
- Decision: first general causal EWLS tracer is GREEN. Hot-path malloc proof,
  nonfinite atomicity, chunk equivalence, and broader acceptance remain open.

### RETIREMENT REQUEST CLEANUP-003

- Archive completed `W-CORE-004` thread
  `019fd018-8939-7e72-9fc8-17085c79806e` and interrupted nested threads
  `019fd019-994e-7380-9be9-105d31501639` and
  `019fd019-b14a-7473-95c9-6d18c5cb91ff` after this record is committed.
- Status: complete; all three app calls returned `archived: true`.

### REQUEST W-TEST-CORE-005

- From / to: manager / `W-TEST-CORE`.
- Client setup ID: `client-new-thread:659f5b36-a5e8-4984-babb-03d1fc0fd7ce`.
- Thread: `019fd040-bb35-7e83-9dc8-ee2abfbb21d1`.
- Worktree: `C:/Users/lcy/.codex/worktrees/e95c/mne-cpp-main`.
- Model/environment: `gpt-5.6-luna`, `max`, new isolated worktree; Sol is not
  needed because this is a bounded public-contract safety test.
- Blocking: yes.
- Task: add one data-driven behavior for selected-reference NaN and selected-
  target Inf. Prime a test denoiser and identical control through warmup plus
  one eligible learning sample. Only the test denoiser receives the bad block;
  require new status `NonFiniteInput`, exact whole-block pass-through, then feed
  identical continuation and probe blocks to both and require matching outputs,
  proving history/statistics/model state did not advance.
- Scope: test source only; no production/CMake, nonselected-nonfinite, shape,
  chunk, reset, malloc, or diagnostics cases.
- Required response: `RESPONSE W-TEST-CORE-005` with commit, rows, state-proof
  sequence, expected RED, and limitations.
- Status: response received; pending main-workspace RED reproduction.

### RESPONSE W-TEST-CORE-005

- Commit: `9ef2ffe089076def3683b3a0f8d5fdd13cff77a2`.
- Changed file: `test_causal_reference_denoiser.cpp` only.
- Rows: `selected_reference_nan` and
  `selected_target_positive_infinity`.
- Proof: identical control/subject are primed to one eligible epoch sample;
  subject alone rejects a bad block with NaN/Inf-aware exact preservation;
  identical continuation completes the epoch, and an ApplyOnly probe must
  match control within `1e-12` with residual `<=1e-4`.
- Worker evidence: diff check passed; clean configure stopped at ignored Eigen
  baseline before target generation. Expected RED is missing
  `DenoiserProcessStatus::NonFiniteInput`.
- Scope/lifecycle: no production/CMake, rt_server, or subagents.
- Manager static review: the sequence detects history, epoch-statistic, and
  model mutation while preserving explicit nonfinite payload semantics.
  Accepted for cherry-pick.

### MANAGER VALIDATION W-TEST-CORE-005-RED

- Integrated commit: `1eaa5ba41`.
- Expected failure: focused Release compile reached test line 412 and failed
  C2838/C2065 because `DenoiserProcessStatus::NonFiniteInput` is absent.
- Result: valid compile RED.

### RETIREMENT REQUEST CLEANUP-004

- Archive one-shot `W-TEST-CORE-005` thread
  `019fd040-bb35-7e83-9dc8-ee2abfbb21d1` after this record is committed.
- Status: complete; app returned `archived: true`.

### REQUEST W-CORE-005

- From / to: manager / `W-CORE`.
- Client setup ID: `client-new-thread:f5ea9d28-754d-47f4-8acf-1c126f5c76d3`.
- Thread: `019fd049-160c-7902-b82a-a587ce0a995e`.
- Worktree: `C:/Users/lcy/.codex/worktrees/bb5c/mne-cpp-main`.
- Model/environment: `gpt-5.6-luna`, `max`, new worktree; no subagents.
- Blocking: yes.
- Task: add `NonFiniteInput` and, after shape validation but before any state or
  block mutation, allocation-free loop scans over every selected reference and
  target value in the complete block. On any NaN/Inf return the new status and
  leave block and PImpl state untouched. Nonselected rows are not scanned.
- Scope: numerical header/source only; no tests/CMake, diagnostics, chunk/reset,
  solver, malloc, or plugin changes.
- Required response: `RESPONSE W-CORE-005` with commit, loop ordering, evidence,
  limitations, and next RED; confirm no subagents.
- Status: response received through manager wait API; pending validation.

### RESPONSE W-CORE-005

- Commit: `3040ef33ea12f31c36973e88fe25d51f97ef6f38`.
- Changed files: `causalreferencedenoiser.h/.cpp` only.
- Implementation: adds `NonFiniteInput`; after shape validation it scans
  configured reference rows, then target rows, over ascending samples before
  taking mutable state or writing scratch/block/state. Nonselected rows are not
  scanned.
- Allocation/state argument: scalar index loops only; every nonfinite return is
  before mutation, so block and PImpl state remain unchanged.
- Worker evidence: diff check/worktree clean; clean build unavailable due the
  ignored Eigen baseline; no dependency repair or rt_server.
- Suggested next RED: nonselected NaN/Inf must not be rejected.
- Lifecycle/model: no subagents; Luna/max.
- Manager static review: placement and selected-row scope match the contract;
  accepted for cherry-pick and focused runtime validation.

### RESPONSE DELIVERY W-CORE-005

- User report: thread `019fd049-160c-7902-b82a-a587ce0a995e` indicated a result
  was sent but the manager notice was not visible.
- Resolution: the manager's bounded thread wait returned the completed turn and
  full structured response; commit/diff were verified directly. No re-dispatch
  or duplicate implementation was created.

### MANAGER VALIDATION W-CORE-005-GREEN

- Integrated commit: `b8d865917`.
- Focused Release build: succeeded.
- Synchronized Qt report: 33 passed, 0 failed, exit code 0.
- Decision: selected nonfinite atomicity/state immutability is GREEN.

### RETIREMENT REQUEST CLEANUP-005

- Archive completed `W-CORE-005` thread
  `019fd049-160c-7902-b82a-a587ce0a995e` after this record is committed.
- Status: complete; app returned `archived: true`.

### REQUEST W-TEST-CORE-006

- From / to: manager / `W-TEST-CORE`.
- Client setup ID: `client-new-thread:3edaf3ba-109e-432c-81f0-760ecb78c893`.
- Thread: `019fd0cc-9f37-7dd0-9051-4086072efabe`.
- Worktree: `C:/Users/lcy/.codex/worktrees/639d/mne-cpp-main`.
- Model/environment: `gpt-5.6-luna`, `max`, new worktree; no subagents.
- Blocking: yes for chunk-equivalence acceptance evidence.
- Task: add one test slot processing the same deterministic finite multichannel
  stream with identical configs through (a) one maximum-size block and (b)
  several irregular chunks that cross warmup and multiple adaptation epochs.
  Compare assembled outputs and a common post-stream ApplyOnly probe at relative
  tolerance `<=1e-10`; require all non-target rows exact. Use at least two
  references, two targets, three taps, and update interval four.
- Scope: test source only; no production/CMake. If current implementation passes
  immediately, report GREEN characterization rather than forcing a RED.
- Required response: `RESPONSE W-TEST-CORE-006` with commit, deterministic
  stream/chunks, command/evidence limitation, and whether RED or immediate
  GREEN; confirm no subagents.
- Status: response received; pending manager diff review and main-workspace
  validation.

### RESPONSE W-TEST-CORE-006

- Commit: `8aa84d032fc635bfc8832b6b6fa39941406ef8fc`.
- Changed file: `test_causal_reference_denoiser.cpp` only.
- Stream: 21 samples over five channels; references `{0,1}`, targets `{2,3}`,
  preserved row 4, three taps, interval four, maximum block size 32.
- Boundaries/chunks: four model-update boundaries after samples 5, 9, 13, and
  17; irregular chunk sizes `{1,3,2,5,4,6}`; common five-sample ApplyOnly
  probe follows the stream.
- Signal: targets contain current, one-lag, and two-lag combinations of both
  references plus bounded sinusoidal clean components.
- Worker evidence: `git diff --check` passed. The fresh worktree has no usable
  configured build because its ignored Eigen baseline is absent, so the worker
  correctly classifies the runtime result as infrastructure-limited rather
  than RED or GREEN.
- Scope/model: test source only; no production/CMake/dependency/rt_server
  changes, no subagents, Luna/max.
- Manager review: accepted. The one-slot public-interface test covers warmup,
  four fixed update epochs, irregular boundaries, future model equivalence,
  and exact preservation of all non-target rows without production coupling.
- Manager validation: cherry-picked as `b6f5b9204`; focused Release build
  succeeded. The synchronized Qt report contains 34 passes, zero failures,
  and exit code 0. Both the assembled stream and common ApplyOnly probe report
  relative difference exactly zero; all configured non-target rows remain
  value-identical.
- Classification: immediate GREEN characterization; no production fix is
  required for this slice.

### RETIREMENT REQUEST CLEANUP-006

- Archive completed one-shot `W-TEST-CORE-006` thread
  `019fd0cc-9f37-7dd0-9051-4086072efabe` after this record is committed and
  published.
- Status: complete; issue milestone published at
  `https://github.com/Edddddddddy/mne-cpp/issues/2#issuecomment-5188904172`
  and the app returned `archived: true`.

### REQUEST W-TEST-CORE-007

- From / to: manager / independent reset-contract test worker.
- Execution: collaboration subagent `/root/w_test_core_007` in detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-007` based on current
  integration HEAD; agent identifier is recorded after dispatch.
- Model/environment: `gpt-5.6-sol`, `ultra`; no nested subagents.
- Model decision: reset correctness spans causal lag history, partial epoch
  statistics, committed weights, and counters, so independent high-risk state
  reasoning is preferred over the default bounded Luna path.
- Blocking: yes for the reset contract.
- Task: add exactly one public-interface test proving `reset()` returns a
  configured denoiser to the same observable state as a freshly configured
  control. First drive the subject through warmup, at least one committed model,
  and a partial next epoch; call reset; then feed identical deterministic
  training/ApplyOnly probe blocks to subject and fresh control and require
  matching statuses and outputs at relative tolerance `<=1e-12`, with all
  non-target rows exact. The sequence must fail if any history, `G/H`, pending
  epoch count, or committed `W` survives reset.
- Scope: test source only; no production/CMake/plugin/dependency edits, no
  rt_server. Immediate GREEN is allowed and must be reported honestly.
- Required response: `RESPONSE W-TEST-CORE-007`, commit SHA, exact priming and
  reset/control sequence, changed files, validation evidence/infra limitation,
  RED or GREEN classification, and confirmation of no nested subagents.
- Status: active. Detached worktree was created at integration commit
  `71aefcd26`; the Sol/ultra agent accepted the no-nested-subagent request.

### REQUEST W-TEST-CORE-007-STATUS

- From / to: manager / `/root/w_test_core_007`.
- Blocking: no; original scope is unchanged.
- Context: bounded waits showed the agent still running and the isolated
  worktree remained clean.
- Question: return a concise progress/blocker note at the next safe boundary.
- Status: forwarded; no implementation assumption is made.

### RESPONSE W-TEST-CORE-007-STATUS

- Conclusion: no blocker; original test-only scope remains active.
- Evidence: the agent read the durable recovery records and full codebase-
  design/TDD instructions, found no `AGENTS.md`, and confirmed the detached
  worktree is clean at `71aefcd26814669a4f5e19700a61c4952988867c`.
- Progress: public seam, current reset implementation, and test conventions are
  inspected; the single observable sequence is being designed to distinguish
  retained lag history, `G/H`, pending count, and committed `W`.
- Lifecycle: no nested subagents.
- Next: continue the same worker; do not duplicate or rescope.

### REQUEST W-TEST-CORE-007-NARROW

- From / to: manager / `/root/w_test_core_007`.
- Blocking: no; accelerates the already authorized single test.
- Guidance: stop further design expansion and implement now. A sufficient
  black-box proof may use a small tap/interval configuration, prime only the
  subject through one model commit plus a partial next epoch, reset it, then
  process an identical post-reset stream on subject/fresh control that crosses
  warmup and at least one new boundary, followed by an identical ApplyOnly
  probe. This ordering detects retained `W/history` immediately and retained
  `G/H/pending-count` at or after the new boundary.
- Scope: unchanged; test source only, one slot, no subagents.
- Status: recorded before forwarding.

### RESPONSE W-TEST-CORE-007-NARROW

- Conclusion: guidance followed; implementation is limited to one Qt slot.
- Sequence: taps three and interval three; subject-only two-sample warmup,
  three eligible samples to commit a model, then one partial-epoch sample;
  reset; identical five-sample post-reset training and two-sample ApplyOnly
  probe against a freshly configured control.
- Scope/lifecycle: test source only; no design expansion and no subagents.
- Status: worker validation and commit are in progress.

### REQUEST W-TEST-CORE-007-FINISH

- From / to: manager / `/root/w_test_core_007`.
- Blocking: no; implementation file is already present and one-file scoped.
- Evidence: manager read-only checks show 110 inserted test lines and a clean
  `git diff --check`; the known clean-worktree Eigen limitation makes further
  build attempts non-actionable.
- Instruction: do not spend more time on unavailable build infrastructure;
  complete final static checks, commit the one test file, and return the
  required structured response now.
- Status: recorded before forwarding.

### RETIREMENT REQUEST W-TEST-CORE-007-INTERRUPT

- From / to: manager / `/root/w_test_core_007`.
- Evidence: the one-file implementation is present and passes diff check, but
  repeated waits after both NARROW and FINISH messages produced no commit or
  final response; worktree HEAD remains `71aefcd26`.
- Decision: interrupt this non-responsive one-shot agent under the user worker
  lifecycle rule. Preserve the isolated modification; manager will review and
  may commit it only if it satisfies the already durable request.
- Replacement: none; do not duplicate an already implemented test.
- Status: recorded before action.

### RECOVERY W-TEST-CORE-007-COMMIT

- Interruption result: previous agent status was `running`; it is now
  `interrupted`.
- Recovered commit: `7640f37501d1f305f345d3fbf39447c7c05a5933`, created just
  before interruption; worktree is clean and detached at that commit.
- Diff: one test file, 110 insertions, no whitespace errors.
- Manager static review: accepted. Prime features commit `[2,-3,4]` then leave
  one partial sample; post-reset features commit a deliberately different
  model and the probe is exactly generated from it. The sequence detects
  retained history/weights before or during training and retained stats/count
  at the new boundary/probe, while checking every non-target row exactly.

### REQUEST W-TEST-CORE-007-FINAL-RESPONSE

- From / to: manager / interrupted `/root/w_test_core_007`.
- Blocking: yes only for protocol completion, not code recovery.
- Instruction: make no edits; return the required structured final response
  for existing commit `7640f3750`, including evidence, limitation,
  classification, and no-subagent confirmation.
- Status: recorded before response-only follow-up.

### RESPONSE W-TEST-CORE-007

- Commit: `7640f37501d1f305f345d3fbf39447c7c05a5933`.
- Changed file: numerical public test source only; one Qt slot and 110 lines.
- Prime: subject-only two-sample warmup, three independent eligible samples
  committing `[2,-3,4]`, then one partial-next-epoch sample observing that model.
- Reset/control: call `subject.reset()`, configure a fresh identical control,
  then give both the same five-sample ApplyAndLearn sequence crossing fresh
  warmup and a complete new epoch for an incompatible model.
- Probe: give both the same two-sample ApplyOnly block; statuses and outputs use
  `1e-12` relative tolerance, while non-target rows `{0,2,3}` are exact.
- Sensitivity: retained history/weights changes training output; retained
  `G/H` or pending count changes the boundary/probe model.
- Evidence: pre/post-stage diff checks passed and the committed worktree is
  clean. Clean-worktree configure failed only on the known absent ignored Eigen
  baseline, so no runtime GREEN is claimed by the worker.
- Classification: static immediate-GREEN expectation, runtime infra-limited;
  manager validation required. No production/CMake/plugin/dependency/rt_server
  changes and no subagents.
- Manager decision: response matches the recovered/reviewed commit; accept for
  cherry-pick and populated-workspace execution.

### MANAGER VALIDATION W-TEST-CORE-007-GREEN

- Integrated commit: `679fc427b`.
- Focused Release build: success, with only known Eigen/MSVC C4819 code-page
  warnings.
- Synchronized runtime: 35 passed, zero failed, exit code 0.
- Reset evidence: post-reset training relative difference zero and future
  ApplyOnly probe relative difference zero against a freshly configured control.
- Decision: reset clears observable causal history, EWLS statistics, pending
  epoch position, and committed weights for this contract; no production change.

### RETIREMENT REQUEST CLEANUP-007

- Retire completed collaboration agent `/root/w_test_core_007`; it is no longer
  reusable after the interrupted/response-recovery lifecycle.
- After this validation is committed/pushed, verify detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-007` is clean at
  `7640f3750` and remove it with Git worktree management. The test is reachable
  on integration as `679fc427b`.
- Publish the 35/0 zero-error evidence to issue #2.
- Status: complete. Issue evidence published at
  `https://github.com/Edddddddddy/mne-cpp/issues/2#issuecomment-5189127958`;
  clean worktree content was verified identical to integrated test and Git
  worktree removal succeeded. The collaboration agent is completed.

### REQUEST W-TEST-CORE-008

- From / to: manager / non-learning-mode contract worker.
- Execution: collaboration subagent `/root/w_test_core_008` in detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-008` based on the current
  integration HEAD; identifier recorded after dispatch.
- Model/environment: `gpt-5.6-sol`, `high`; no nested subagents.
- Model decision: this is a bounded analytic state test rather than numerical
  implementation/review, so Sol/high is sufficient and more proportional than
  Sol/ultra; the user-approved model family is preserved.
- Blocking: yes for completion of the three-mode public contract.
- Task: add exactly one data-driven public-interface Qt test with rows for
  `ApplyOnly` and `BypassTrackHistory`. Train a small denoiser on independent
  features to a known two-tap model. Feed an adversarial block of exactly one
  adaptation interval using the selected non-learning mode; assert ApplyOnly
  subtracts the known model while Bypass preserves the entire block. Then feed
  a common analytically generated ApplyOnly probe whose features depend on the
  adversarial block's final references. Require near-zero target residual and
  exact non-target rows. The test must fail if either mode learns or fails to
  advance causal history.
- Scope: one test source only; no production/CMake/plugin/dependency changes,
  no rt_server, immediate GREEN allowed. Do not compare two possibly wrong
  denoisers as the oracle; use the analytic known model/output.
- Required response: `RESPONSE W-TEST-CORE-008`, commit, data rows, exact
  training/adversarial/probe values, tolerances, evidence/infra limitation,
  classification, and no-subagent confirmation.
- Status: active. Detached worktree was created from integration commit
  `b27b328d9`; Sol/high agent accepted the no-subagent request.

### RESPONSE W-TEST-CORE-008

- Worker-reported commit: short SHA `ec69672b6`; the reported full SHA
  `ec69672b6f0281c337123887438dbcb13bcce7ac` does not resolve.
- Manager reconciliation: clean detached worktree HEAD and `git rev-parse HEAD`
  resolve the actual commit as
  `ec69672b6ebda8871de7a8787dabb3f58047e51a`. Use this object for review and
  integration; no duplicate task is needed.
- Data rows: `ApplyOnly` and `BypassTrackHistory`; one public test source only.
- Training: reference `[0,1,0]`, target `[0,2,3]`, preserved row
  `[100,101,102]`; independent features identify analytic model
  `y(t)=2r(t)+3r(t-1)`.
- Adversarial interval: reference `[10,-4]`, target `[500,-400]`, preserved row
  `[200,201]`. ApplyOnly expects `[480,-422]` within `1e-5`; Bypass requires
  exact whole-block preservation.
- Common probe: reference `[7,-6]`, target `[2,9]`, preserved `[300,301]`, with
  features `[7,-4]` and `[-6,7]`; target residual is within `1e-5` and
  non-target rows exact.
- Sensitivity: learning on the two-sample adversarial interval changes the
  future model; failure to track history changes the first probe lag.
- Evidence: staged/committed diff checks passed and worktree is clean. Runtime
  is correctly unclaimed because the detached worktree lacks ignored Eigen
  baseline files; no repair/copy attempted.
- Classification/scope: static immediate-GREEN expectation, runtime infra-
  limited; no production/CMake/plugin/dependency/rt_server edits and no
  subagents.
- Manager status: response is durable; exact diff review is next.

### MANAGER REVIEW W-TEST-CORE-008

- Scope: accepted; exactly one data-driven public-interface slot and one test
  source, with no unrelated change.
- Oracle check: the two eligible training features are `[1,0]` and `[0,1]`;
  target values identify weights `[2,3]`. Adversarial expected outputs are
  `[480,-422]`. After references `[10,-4]`, probe features are `[7,-4]` and
  `[-6,7]`, whose analytic targets are `[2,9]`.
- Sensitivity check: regularization/forgetting perturbations remain well below
  `1e-5`; any adversarial learning or stale history produces order-one errors.
- Decision: accept reconciled commit `ec69672b6ebda8871de7a8787dabb3f58047e51a`
  for cherry-pick and populated-workspace runtime validation.

### MANAGER VALIDATION W-TEST-CORE-008-GREEN

- Integrated commit: `58c4646ae`.
- Focused Release build: success, with only known Eigen/MSVC C4819 warnings.
- Synchronized report: 37 passed, zero failed, process exit code 0.
- Both `ApplyOnly` and `BypassTrackHistory` analytic rows pass, proving the
  committed model is unchanged, history advances, ApplyOnly subtracts it, and
  Bypass preserves the block for this contract.
- Decision: immediate GREEN; no production fix.

### RETIREMENT REQUEST CLEANUP-008

- After committing/pushing validation, publish mode evidence to issue #2.
- Verify detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-008` is clean at actual
  commit `ec69672b6ebda8871de7a8787dabb3f58047e51a` and content-matches
  integrated `58c4646ae`, then remove it via Git worktree management.
- Collaboration agent `/root/w_test_core_008` is one-shot and completed; do
  not reuse it.
- Status: complete. Issue comment
  `https://github.com/Edddddddddy/mne-cpp/issues/2#issuecomment-5189222318`
  published 37/0 evidence; clean worktree content matched integrated test and
  Git worktree removal succeeded. The completed agent will not be reused.

### REQUEST R-DIAGNOSTICS-001

- From / to: manager / independent diagnostics seam reviewer.
- Model/environment: `gpt-5.6-sol`, `ultra`; read-only current integration
  snapshot, no worktree edits and no nested subagents.
- Blocking: yes for the next public-interface RED.
- Context: numerical core currently returns only primary process status. The
  approved spec also requires fixed-size warmup/model/RMS diagnostics and an
  observable rejected candidate solve while retaining the committed model.
- Review questions:
  1. What is the smallest C++14 fixed-size `DenoiserProcessResult`/diagnostics
     surface that serves the adapter UI without Qt/FIFF/strings or a strategy
     seam?
  2. How should a block report normal processing plus zero/one/multiple model
     accepts/rejections without misclassifying it as an atomic input error?
  3. Define exact reset, bypass, ApplyOnly, ApplyAndLearn, shape/nonfinite, and
     failed-solve semantics for counters, warmup, model generation, and RMS.
  4. Should invalid finite EWLS updates/failed factorization roll back, discard
     an epoch, or retain finite statistics, while keeping future recovery and
     hot-path no-allocation?
  5. Identify current P0-P3 findings relevant to this seam and the minimum
     focused tests needed before malloc guard/synthetic acceptance.
- Output: `RESPONSE R-DIAGNOSTICS-001` with recommended declaration sketch,
  semantics table in prose, findings with priority/file/line/evidence/fix/test,
  explicitly rejected alternatives, and no-edit/no-subagent confirmation.
- Status: recorded before dispatch.
