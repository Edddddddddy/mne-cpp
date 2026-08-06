# Worker Ledger

The user-visible manager conversation owns direction, GitHub issues and the
integration branch. It runs on Sol/ultra and does not create internal
subagents. Implementation work conversations use separate worktrees,
Luna/max or manager-selected Sol, one minimal responsibility, and return
reviewable commits. Dedicated visible review conversations use Sol/ultra and
do not implement fixes.

## Message protocol

```text
REQUEST <id>
from / to / blocking / context / question / expected evidence

RESPONSE <id>
from / to / conclusion / evidence / affected files or commit / next action
```

The manager records and forwards all cross-conversation messages here before
acting. Workers send a structured RESPONSE back when complete; the manager
does not continuously poll them.

## Lifecycle and model policy

- User-confirmed hard rule: every newly created work conversation uses either
  `gpt-5.6-luna` at `max` (Luna's highest available effort) or
  `gpt-5.6-sol`; concurrency, realtime safety, architecture and formal review
  use Sol/ultra. Do not dispatch this project to Terra or another model.
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
- Status: active as collaboration agent `/root/r_diagnostics_001`; Sol/ultra
  accepted the read-only/no-subagent scope.

### REQUEST R-DIAGNOSTICS-001-STATUS

- From / to: manager / `/root/r_diagnostics_001`.
- Blocking: no; review scope remains unchanged.
- Context: repeated bounded waits show the read-only reviewer still running,
  with no repository modification.
- Question: report concise progress/blocker state at the next safe boundary.
- Status: recorded before forwarding.

### RESPONSE R-DIAGNOSTICS-001-STATUS

- Conclusion: no blocker; full review remains active.
- Evidence: durable recovery records and full codebase-design guidance were
  read; public header/source and all focused tests were inspected at review
  snapshot `2f6b6c8d6`.
- Progress: reviewer is resolving the exact fixed-size fields and transactional
  EWLS rejection policy.
- Scope/lifecycle: strictly read-only, no subagents, no rt_server.
- Next: continue the same review without duplicate dispatch.

### REQUEST R-DIAGNOSTICS-001-FINISH

- From / to: manager / `/root/r_diagnostics_001`.
- Blocking: no; scope and evidence set are unchanged.
- Context: reviewer reported no blocker and completed source/test inspection;
  multiple later bounded waits produced no additional request.
- Instruction: conclude the design choice now and return the required structured
  final response; do not expand into plugin design or implementation.
- Status: recorded before forwarding.

### RESPONSE R-DIAGNOSTICS-001

- Conclusion: keep primary block validity orthogonal to fixed-size model-update
  events. Valid blocks remain `Processed`; per-call accepted/rejected counts and
  post-call generation preserve mixed/multiple boundary information.
- Selected fields: configured reference/target/feature counts, post-call warmup,
  cumulative model generation, per-call accepted/rejected model updates, and
  input/output/actually-subtracted-noise RMS.
- Error/mode semantics: `NotConfigured` is distinct; invalid shape/nonfinite are
  atomic and report quiet-NaN RMS; Bypass/ApplyOnly advance history but pause
  partial learning epochs; reset clears generation and numerical state.
- Rejection transaction: separate committed finite `G/H` from preallocated
  pending epoch statistics. Poisoned/failed epochs are discarded; committed
  stats are aged by elapsed decay and remain recoverable; output/history are
  not rolled back.
- Findings: P1 absent diagnostics/observable update outcomes; P1 poisoned
  in-place `G/H` is permanently unrecoverable; P2 LDLT acceptance is too weak;
  P2 RMS absent/naive squaring would overflow; P2 application overflow policy
  is undefined; P3 unconfigured conflates invalid shape.
- Rejected alternatives: numerical primary failure for solve rejection, last-
  solve boolean, cumulative-only counters, dynamic events/strings, diagnostics
  getter, retaining poisoned stats, clearing all committed stats, history/output
  rollback, and a strategy registry.
- Scope/evidence: reviewed snapshot `2f6b6c8d6`; read-only, no edits/commit/tests,
  no rt_server and no subagents.
- Manager decision: accept the fixed-size surface and rejection transaction;
  implement test-first in the reviewer's minimum order.

### REQUEST W-TEST-CORE-009

- From / to: manager / diagnostics lifecycle test worker.
- Execution: collaboration subagent `/root/w_test_core_009` in detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-009`; identifier recorded
  after dispatch.
- Model/environment: `gpt-5.6-sol`, `high`; no nested subagents.
- Blocking: yes for diagnostics production work.
- Task: add exactly one public-interface Qt slot that first processes a finite
  block before configure and requires `NotConfigured`, unchanged block, zero
  snapshot/count fields and quiet-NaN RMS. Configure one ref/one target/three
  taps/update interval two, then exercise one-sample Bypass warmup, one-sample
  ApplyOnly warmup completion, a two-sample ApplyAndLearn accepted boundary,
  and reset followed by Bypass. Check configured counts, post-call warmup,
  generation, per-call events, statuses, exact block preservation where
  required, and analytic input/output/noise RMS. Old zero model means the first
  learning epoch output remains unchanged while generation becomes one.
- Scope: test source only; no production/CMake/plugin/dependency/rt_server,
  exactly one slot, expected compile RED on missing diagnostics/NotConfigured.
- Required response: `RESPONSE W-TEST-CORE-009` with commit, exact sequence and
  RMS oracle, expected compile failure, evidence/infra limitation, and no-
  subagent confirmation.
- Status: active. Detached worktree created from `1d7729ab3`; Sol/high accepted
  one-slot/no-subagent scope.

### RESPONSE W-TEST-CORE-009

- Commit: `bdfca52d37804bcb8a9e4f4f83ed6f5c6ec2588f`.
- Changed file: focused numerical test source only; exactly one Qt slot.
- Sequence/oracle: unconfigured `3x2` block requires NotConfigured/unchanged/
  zero snapshot/NaN RMS; Bypass target 3 gives warmup1 and RMS `3/3/0`;
  ApplyOnly target -4 gives warmup0 and `4/4/0`; two learning targets `[5,-12]`
  keep old-zero-model output, accept/reject `1/0`, generation1, and input/output
  RMS `sqrt(84.5)=9.19238815542512`, noise0; reset plus Bypass target9 restores
  generation0/warmup1 and `9/9/0`.
- Expected RED: missing NotConfigured, diagnostics type/result member and fields.
- Evidence: diff check passed and committed worktree is clean. Configure stopped
  at known ignored Eigen baseline absence; no dependency repair/runtime claim.
- Scope/lifecycle: no production/CMake/plugin/dependency/rt_server changes and
  no subagents.
- Manager status: response is durable; exact diff/oracle review is next.

### MANAGER REVIEW W-TEST-CORE-009

- Scope: accepted; one Qt slot/test source only.
- Oracle: `sqrt((5^2+(-12)^2)/2)=sqrt(84.5)`; all single-sample RMS values are
  absolute target magnitudes. Old zero weights preserve the first learning
  epoch while the boundary can still increment generation.
- Lifecycle: post-call warmup transitions `2 -> 1 -> 0`; reset plus one bypass
  correctly expects warmup1 and generation0.
- Decision: accept `bdfca52d3` for cherry-pick and intended compile RED.

### MANAGER VALIDATION W-TEST-CORE-009-RED

- Integrated test commit: `a0fcf0e68`.
- Focused Release compile reaches the new slot and fails on exactly the selected
  absent interface: unknown `DenoiserProcessDiagnostics`, missing
  `DenoiserProcessStatus::NotConfigured`, and no `DenoiserProcessResult::diagnostics`.
- Known Eigen C4819 warnings are non-fatal; no rt_server run.
- Decision: valid compile RED; fixed-size lifecycle implementation may begin.

### RETIREMENT REQUEST CLEANUP-009

- After this RED/request record is committed, verify detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-009` is clean at
  `bdfca52d3` and content-matches integrated `a0fcf0e68`, then remove it via Git
  worktree management. Completed one-shot agent is not reusable.
- Status: recorded before cleanup.

### REQUEST W-CORE-006

- From / to: manager / diagnostics lifecycle implementation worker.
- Execution: collaboration subagent `/root/w_core_006` in detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-core-006`; identifier recorded after
  dispatch.
- Model/environment: `gpt-5.6-sol`, `high`; no nested subagents.
- Model decision: bounded header/source implementation, but RMS and result
  aggregation execute on the real-time hot path, so Sol/high is selected.
- Blocking: yes.
- Task: make only the existing diagnostics lifecycle slot GREEN. Add
  `NotConfigured`, fixed `DenoiserProcessDiagnostics`, and result member exactly
  as selected in `SPEC.md`. Track cumulative generation in Impl/reset. Make the
  existing boundary solve return accepted/rejected outcome so each valid call
  aggregates per-call counters and increments generation only on acceptance.
  Return zero/NaN unconfigured diagnostics and committed snapshot plus NaN RMS
  for shape/nonfinite errors. For valid calls, compute selected-target input,
  returned-output, and actually-subtracted-prediction RMS with allocation-free
  scaled sum-of-squares; report post-call warmup. Bypass/ApplyOnly behavior and
  every existing algorithm result must remain unchanged.
- Scope: numerical header/source only; no test/CMake/plugin changes. Do not add
  transactional pending `G/H`, large-application fallback, malloc guard, or new
  tests in this slice. No explicit allocation/lock/string work in process.
- Required response: `RESPONSE W-CORE-006`, commit, declaration and aggregation
  details, stable RMS algorithm, solve outcome/generation semantics, evidence/
  infra limitation, next RED, and no-subagent confirmation.
- Status: active. Test worker was clean/content-matched and removed; new
  implementation worktree was created from `8637bb118`; Sol/high accepted the
  header/source-only and no-subagent scope.

### RESPONSE W-CORE-006-ERROR

- Result: agent turn failed before structured completion because the Codex
  response stream disconnected while sending to the backend.
- Classification: external worker transport failure, not a compile/test/code
  finding. No commit or scope claim is accepted yet.
- Decision: record before inspecting the isolated worktree. If work is not a
  complete clean commit, retire the non-reusable agent and use a fresh
  replacement under the same durable request.

### RECOVERY/RETIREMENT W-CORE-006

- Inspection: detached worktree remains clean at base `8637bb118`; there is no
  diff and no implementation commit to recover.
- Decision: do not reuse `/root/w_core_006`. Remove its verified clean worktree
  and create a fresh replacement; no code is lost.
- Status: recorded before cleanup.

### REQUEST W-CORE-006-R

- From / to: manager / fresh diagnostics lifecycle implementation replacement.
- Execution: collaboration subagent in detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-core-006-r`; identifier recorded
  after dispatch.
- Model/environment: `gpt-5.6-sol`, `high`; no nested subagents.
- Blocking/scope: identical to `REQUEST W-CORE-006`. Implement only the fixed
  diagnostics lifecycle GREEN in numerical header/source: NotConfigured,
  fixed snapshot, generation/boundary outcome counters, post-call warmup and
  allocation-free scaled RMS. Do not add transactional pending stats,
  application fallback, tests/CMake/plugin changes, or dependency repairs.
- Required response: `RESPONSE W-CORE-006-R` with a complete clean commit and
  the same evidence fields as the original request.
- Status: active as `/root/w_core_006_r`. The failed clean worktree was removed;
  replacement worktree was created from `810e11d65`; Sol/high accepted the
  unchanged header/source-only and no-subagent scope.

### REQUEST W-CORE-006-R-STATUS

- From / to: manager / `/root/w_core_006_r`.
- Blocking: no; implementation scope is unchanged.
- Context: repeated bounded waits show the replacement running while its
  isolated worktree remains clean.
- Question: report concise progress/blocker state at the next safe boundary.
- Status: recorded before forwarding.

### RESPONSE W-CORE-006-R-STATUS

- Conclusion: no blocker; implementation is complete and diff-check clean.
- Progress: required records/skills and exact lifecycle RED were inspected;
  numerical header/source are in static/compile-oriented final review.
- Limitation: detached worktree has the known absent ignored Eigen baseline and
  no focused build directory, so runtime GREEN remains manager-owned.
- Lifecycle: no subagents.
- Next: finish static review, commit, and return structured response.

### RESPONSE W-CORE-006-R-ERROR

- Result: final response transport disconnected before structured completion.
- Difference from the first failure: numerical header/source implementation is
  present in the isolated worktree, exactly two files and diff-check clean.
- Classification: external response transport failure; implementation is not
  yet accepted or discarded.
- Decision: do not duplicate or delete. Manager will inspect the entire diff;
  if it satisfies the durable request, create a recovery commit in that
  worktree and validate it in the populated workspace.

### MANAGER RECOVERY REVIEW W-CORE-006-R

- Scope: accepted; only `causalreferencedenoiser.h/.cpp`, 134 insertions and 21
  deletions, diff-check clean.
- Interface/early returns: exact selected fixed struct and NotConfigured;
  unconfigured uses zero snapshot/NaN RMS, while shape/nonfinite use committed
  snapshot, zero events and NaN RMS before mutation.
- Valid aggregation: input targets accumulate before modification, outputs
  after processing, and prediction RMS includes only actually subtracted
  predictions with implicit zeros for warmup/bypass.
- Stable RMS: scale plus normalized sum-of-squares avoids direct squaring and
  returns zero for an all-zero prediction stream.
- State: solve returns bool; accepted/rejected counts are call-local and only
  accepted boundaries increment cumulative generation; reset clears it.
- Real-time review: stack scalars/aggregate results and existing preallocated
  Eigen state only; no explicit allocation, locks or strings were added.
- Known open scope: poisoned in-place `G/H`, strict-positive LDLT and application
  overflow remain the recorded later findings.
- Decision: create a manager recovery commit in the isolated worktree without
  further edits, then cherry-pick and validate in the populated workspace.

### RESPONSE W-CORE-006-R-RECOVERY

- Recovery commit: `eb342faf5c8e91fbcef29944bd1b9764a52e2bb4`.
- Author/action: manager committed the already reviewed two-file worktree diff
  after both implementation agents' response transports failed; no code edit
  was made during recovery.
- Worktree state: clean detached HEAD after commit.
- Scope/evidence: numerical header/source only, diff-check clean; implementation
  evidence and known limitations are captured in the recovery review above.
- Next: integrate and run populated-workspace compile/runtime; no worker GREEN
  is claimed.

### MANAGER VALIDATION W-CORE-006-R-GREEN

- Integrated production commit: `b5f29fff1`.
- Focused Release build: success, including diagnostics header/source and test;
  only known Eigen/MSVC C4819 warnings.
- Synchronized report: 38 passed, zero failed, exit code 0.
- Lifecycle/RMS slot passes exactly; all prior core tests remain GREEN.
- Decision: fixed diagnostics lifecycle is GREEN. Transactional `G/H` and
  application overflow findings remain open by design.

### RETIREMENT REQUEST CLEANUP-010

- After committing/pushing GREEN evidence, publish it to issue #2.
- Verify recovery worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-core-006-r` is clean at
  `eb342faf5`, content-matches integrated `b5f29fff1`, and remove via Git
  worktree management. Both transport-failed implementation agents are not
  reusable.
- Status: complete. Diagnostics evidence published at
  `https://github.com/Edddddddddy/mne-cpp/issues/2#issuecomment-5189807403`;
  clean recovery worktree content matched integration and was removed.

### REQUEST W-TEST-CORE-010

- From / to: manager / transactional solve-rejection test worker.
- Execution: collaboration subagent `/root/w_test_core_010` in detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-010`; identifier recorded
  after dispatch.
- Model/environment: `gpt-5.6-sol`, `ultra`; no nested subagents.
- Model decision: extreme finite arithmetic, mixed boundary diagnostics, epoch
  forgetting and recovery make this a high-risk numerical contract.
- Blocking: yes for P1 `R-SOLVE-001` and completion of P1 `R-DIAG-001`.
- Task: add exactly one public-interface Qt slot using one reference, one target,
  one tap, interval two and a single eight-sample ApplyAndLearn block. Epoch 1
  uses refs `[0,0]`, targets `[0,0]` and must accept a zero model. Epochs 2/3
  use finite refs `[DBL_MAX/2, DBL_MAX/2]` with zero targets and must each reject
  their poisoned candidates. Epoch 4 uses refs `[1,2]`, targets `[2,4]` and must
  recover/accept. Require status Processed, per-call accepted/rejected `2/2`,
  generation2, exact reference preservation, finite target output and analytic
  target RMS. Then ApplyOnly probe ref3/target6 must be near zero with generation2
  and zero call-local events. This is a runtime RED against poisoned in-place
  `G/H`, not a compile RED.
- Scope: test source only; no production/CMake/plugin/dependency/rt_server.
- Required response: `RESPONSE W-TEST-CORE-010`, commit, exact stream/oracle,
  expected current RED, evidence/infra limitation, and no-subagent confirmation.
- Status: active. Worktree created from `ff0fc018d`; Sol/ultra accepted the
  single-slot/no-subagent extreme arithmetic scope.

### RESPONSE W-TEST-CORE-010

- Commit: `3d38866c537706ea9072811cc7bd7e7a1126ca4a`.
- Changed file: focused numerical test source only; one Qt slot, 68 insertions.
- Oracle: eight-sample learning block has zero epoch accepted, two
  `H=DBL_MAX/2` epochs rejected, and final `[1,2] -> [2,4]` epoch accepted.
  Require Processed, events `2/2`, generation2, exact refs, finite unchanged
  targets and RMS `sqrt(2.5)/sqrt(2.5)/0`.
- Probe: ApplyOnly ref3/target6 must have finite residual `<=1e-5`, generation2,
  zero per-call events and exact ref.
- Expected current RED: poisoned in-place stats produce events `1/3`,
  generation1, zero weights and probe residual6.
- Evidence: committed diff check passes and worktree is clean. Detached runtime
  is unavailable only due missing ignored Eigen/build baseline; no repair.
- Scope/lifecycle: no production/CMake/plugin/dependency/rt_server changes and
  no subagents.
- Manager status: response is durable; exact diff review follows.

### MANAGER REVIEW W-TEST-CORE-010

- Scope: accepted; commit `3d38866c5` changes only the focused numerical test
  source and adds exactly one public-interface slot.
- Epoch oracle: interval two yields four boundaries. The initial zero epoch is
  a valid zero-model accept; each `DBL_MAX/2` epoch overflows its outer product
  despite finite selected input and must reject; the final finite `[1,2]` to
  `[2,4]` epoch must recover and accept for call-local events `2/2` and
  cumulative generation two.
- Output/RMS oracle: the committed zero model is applied throughout the
  learning block, so targets remain exact and selected-target RMS is
  `sqrt((2^2 + 4^2) / 8) = sqrt(2.5)` with zero subtracted-noise RMS. The
  post-boundary ApplyOnly probe observes the recovered approximately-2 model.
- Sensitivity: current in-place poisoned `G/H` cannot recover and is expected
  to report `1/3`, generation one and a probe residual of six.
- Decision: accept for cherry-pick and populated-workspace runtime RED. No
  production change is authorized before that RED is captured.

### MANAGER VALIDATION W-TEST-CORE-010-RED

- Integrated test commit: `d1ffa0904`.
- Focused Release build: succeeded; only the known Eigen/MSVC C4819 code-page
  warnings were emitted.
- Synchronized Qt report: 38 passed, one failed, process exit code one. The
  unique failure is the new `modelUpdatesAccepted == 2` assertion at the
  transactional recovery slot; all prior tests remain GREEN.
- Interpretation: the current implementation cannot accept the final normal
  epoch after finite extreme epochs poison in-place `G/H`. This is the intended
  runtime RED for `R-SOLVE-001`.
- Decision: production work may now begin. It must keep committed finite stats
  separate from preallocated pending-epoch/candidate storage and change only
  numerical header/source.

### RETIREMENT REQUEST CLEANUP-011

- Before dispatching production work, verify detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-010` is clean at
  `3d38866c5`, and that its test content matches integrated `d1ffa0904`.
- Then remove it through Git worktree management. Completed agent
  `/root/w_test_core_010` is one-shot and must not be reused.
- Status: complete. The worktree was clean at `3d38866c5`, its test content
  exactly matched integrated `d1ffa0904`, and Git worktree removal succeeded.
  The completed collaboration agent remains retained only as immutable tool
  history because this runtime exposes no hard-delete operation; it is not
  reusable.

### REQUEST W-CORE-007

- From / to: manager / fresh transactional EWLS implementation worker.
- Planned execution: detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-core-007` from current integration
  HEAD; collaboration agent identifier is recorded after dispatch.
- Model/environment: `gpt-5.6-sol`, `ultra`; no nested subagents.
- Model decision: this slice changes recursive EWLS state ownership, finite
  overflow recovery, boundary transactions and the no-allocation hot path, so
  Sol/ultra is selected.
- Blocking: yes for P1 `R-SOLVE-001` and closure of mixed event diagnostics.
- Task: make only the integrated `rejectsPoisonedEpochsAndRecoversWithinBlock`
  runtime RED GREEN. Keep committed finite `G/H` separate from preallocated
  pending-epoch statistics and candidate scratch. Accumulate pending stats and
  elapsed forgetting per eligible learning sample. At a boundary, solve from
  a finite candidate composed from aged committed plus pending stats; on
  success atomically commit candidate `G/H/W`, and on poison/solve failure
  discard pending stats, age only committed `G/H`, retain committed `W`, and
  increment the existing rejection event. Clear the pending epoch after every
  boundary so later epochs in the same block can recover.
- Required invariants: configure remains the only allocation phase; reset
  clears all committed/pending/candidate state; process stays noexcept with no
  explicit allocation/lock/string; causal apply-before-learn ordering, chunk
  equivalence, modes, diagnostics and target-only writes remain unchanged.
- Scope: numerical header/source only; no tests/CMake/plugin/dependency or
  rt_server. Do not broaden into strict-pivot `R-SOLVE-002`, application
  overflow `R-APPLY-001`, malloc guard, synthetic acceptance or plugin code.
- Required response: `RESPONSE W-CORE-007` with commit SHA, exact added state
  shapes, recurrence/composition/discard equations, boundary ordering,
  diff/evidence limitation, remaining findings and no-subagent confirmation.
- Status: active as collaboration agent `/root/w_core_007`. Detached worktree
  created clean at `5eff9abb9`; Sol/ultra accepted the exact bounded request.

### REQUEST W-CORE-007-STATUS

- From / to: manager / `/root/w_core_007`.
- Blocking: no; implementation scope remains unchanged.
- Context: several bounded waits show the worker still running while its
  detached worktree remains clean at the request base.
- Question: at the next safe boundary, report concise progress, current design
  choice and any blocker; do not broaden scope or spawn subagents.
- Status: recorded before forwarding.

### RESPONSE W-CORE-007-STATUS

- Conclusion: no blocker; required records and codebase-design/TDD skills were
  fully read and the detached base was clean before edits.
- Current patch: committed `G(PxP)/H(MxP)` are separate from pending
  `Gp(PxP)/Hp(MxP)` and candidate `Gc(PxP)/Hc(MxP)`, plus scalar epoch decay
  `d`.
- Recurrence: each eligible sample applies `d <- lambda*d`,
  `Gp <- lambda*Gp + phi*phi^T`, and
  `Hp <- lambda*Hp + y*phi^T`. A boundary composes
  `Gc=d*G+Gp`, `Hc=d*H+Hp`.
- Transaction: a finite successful LDLT commits `G/H/W`; rejection ages only
  committed `G/H` by `d`, retains weights/generation, and both paths clear the
  pending matrices/count/decay.
- Scope/lifecycle: source-only implementation, no public header change and no
  subagents. Static review/available validation is in progress; detached Eigen
  baseline may prevent runtime.
- Next: await the complete clean commit response; no duplicate or rescope.

### REQUEST W-CORE-007-FINISH

- From / to: manager / `/root/w_core_007`.
- Blocking: no; implementation contract is unchanged.
- Evidence: manager read-only inspection sees one source file, 67 insertions
  and 16 deletions, clean diff check, exact committed/pending/candidate state,
  and no public/CMake/test change. The detached dependency limitation is known.
- Instruction: finish static checks now, do not spend time repairing unavailable
  build infrastructure, commit the scoped source diff, and return the required
  structured response. No extra finding or feature may be implemented.
- Status: recorded before forwarding.

### RESPONSE W-CORE-007

- Commit: `0e55194179d465d10bb1413e95a6593e3877df98`.
- Changed file: `src/libraries/rtprocessing/causalreferencedenoiser.cpp` only;
  public header/interface unchanged.
- Configure-time state: committed `G(PxP)/H(MxP)/W(MxP)`, pending
  `Gp(PxP)/Hp(MxP)`, candidate `Gc(PxP)/Hc(MxP)` and scalar epoch decay `d`.
- Recurrence/composition: `d<-lambda*d`, `Gp<-lambda*Gp+phi*phi^T`,
  `Hp<-lambda*Hp+y*phi^T`; boundary uses `Gc=d*G+Gp`, `Hc=d*H+Hp` and the
  existing loaded LDLT solve.
- Transaction: finite successful solve commits `G/H/W`; rejection performs
  only `G<-d*G`, `H<-d*H`, retaining `W` and generation. Both outcomes clear
  pending matrices/count and reset `d=1`, enabling a later same-call recovery.
- Ordering: raw target capture, apply committed model, accumulate pending,
  transact boundary, then advance history; a new model remains future-only.
- Evidence: clean exact base, pre-stage/staged/committed diff checks and clean
  detached HEAD. Scalar oracle recovered weight `1.99999998`, probe residual
  about `6e-8`, and confirmed finite extreme outer-product overflow.
- Limitation: detached worktree lacks the known ignored Eigen/build baseline;
  manager owns compiled/runtime validation. No repair was attempted.
- Remaining untouched: strict-pivot P2, application-overflow P2, malloc guard,
  synthetic acceptance, plugin/CMake/dependency/test and rt_server work.
- Lifecycle: Sol/ultra; no subagents. Manager commit-level review is next.

### MANAGER REVIEW W-CORE-007

- Scope/base: accepted. Exact commit `0e5519417` is clean on request base
  `5eff9abb9`, changes one numerical source file (71 insertions, 16 deletions),
  and has no public header, test, CMake, plugin or dependency edit.
- Recurrence: pending matrices initialized at zero and scalar `d=1` reproduce
  the per-sample EWLS recurrence; `d*committed + pending` is the correct full
  candidate at every fixed boundary, including partial epochs across calls.
- Atomicity: candidate composition and every existing finite/LDLT/weight check
  occur before committed state writes. Success commits `G/H/W` together;
  failure only ages finite committed `G/H`, retains `W/generation`, then clears
  pending state. Candidate scratch may be nonfinite but is never committed.
- Streaming: ApplyOnly/Bypass leave a partial learning epoch paused; reset
  clears all new matrices/count/decay; apply-before-learn and future-only model
  activation are preserved.
- Real-time: all dynamic matrices are sized in `Impl` construction; process
  adds only in-place operations and scalar branches, with no explicit
  allocation, lock or string work. Formal malloc guard remains a later test.
- Known exclusions remain correctly untouched: strict LDLT pivots and
  application overflow.
- Decision: accept for cherry-pick and populated-workspace compiled/runtime
  validation. GREEN is not claimed until the synchronized 39-check report.

### MANAGER VALIDATION W-CORE-007-GREEN

- Integrated production commit: `9142415b7`.
- Focused Release build: success; only known Eigen/MSVC C4819 code-page
  warnings.
- Synchronized Qt report: 39 passed, zero failed, process exit code zero.
- Recovery evidence: the mixed block now accepts/rejects `2/2`, reaches
  generation two, retains finite unchanged learning output and applies the
  recovered model to the future probe. Every prior behavior remains GREEN.
- Decision: close P1 `R-SOLVE-001` and P1 `R-DIAG-001`. P2 strict pivots,
  stable extreme RMS/application fallback and hot-path malloc proof remain.

### RETIREMENT REQUEST CLEANUP-012

- After committing/pushing GREEN evidence, publish it to issue #2.
- Verify detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-core-007` is clean at
  `0e5519417`, and its numerical source content matches integrated `9142415b7`.
- Remove it through Git worktree management. Completed agent
  `/root/w_core_007` is one-shot and must not be reused.
- Status: complete. GREEN was published at
  `https://github.com/Edddddddddy/mne-cpp/issues/2#issuecomment-5190070518`.
  The worktree was clean at `0e5519417`, its source exactly matched integrated
  `9142415b7`, and Git worktree removal succeeded. The completed agent is not
  reusable; only immutable tool history remains because hard delete is absent.

### REQUEST W-TEST-CORE-011

- From / to: manager / strict-positive LDLT characterization test worker.
- Planned execution: detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-011` from current
  integration HEAD; agent identifier recorded after dispatch.
- Model/environment: `gpt-5.6-sol`, `high`; no nested subagents.
- Model decision: this is one bounded analytic public-contract test. Sol/high
  is proportional; the later implementation/review can use Sol/ultra.
- Blocking: yes for P2 `R-SOLVE-002` hardening.
- Task: add exactly one public-interface Qt slot proving a rank-deficient EWLS
  epoch remains acceptable because relative diagonal loading makes the normal
  equations positive definite. Use two perfectly collinear references, two
  taps, one target, one warmup sample and four eligible samples so the `P=4`
  feature matrix has rank one. Require the training block to remain unchanged
  under the old zero model, one accepted update, zero rejected updates and
  generation one. Then an analytic ApplyOnly probe must be finite, preserve
  both references and reduce the known target residual to `<=1e-5`.
- Scope: test source only; one slot, no production/CMake/plugin/dependency or
  rt_server. Do not duplicate the already GREEN extreme-candidate rejection
  test. Honest immediate GREEN characterization is allowed and expected.
- Required response: `RESPONSE W-TEST-CORE-011` with commit, exact stream and
  rank argument, analytic probe/tolerance, available evidence and GREEN/RED
  classification, plus no-subagent confirmation.
- Status: active as collaboration agent `/root/w_test_core_011`. Detached
  worktree created clean at `aebe2fafb`; Sol/high accepted the one-slot scope.

### RESPONSE W-TEST-CORE-011

- Commit: `148870f873bef105f5a7dae6193191a32916a3bb`.
- Changed file: focused numerical test source only; exactly one Qt slot.
- Stream/rank: references `[1,2,4,8,16]` and twice that row; target `3*r0`.
  After one warmup the `P=4` features are `v,2v,4v,8v` for
  `v=[2,4,1,2]^T`, so the feature span has rank one.
- Training oracle: old zero model preserves the complete block; events `1/0`,
  generation one and exact reference rows.
- Probe: refs `[32,64]` give feature `16v` and analytic target 96; finite
  residual `<=1e-5`, generation one, zero events and exact refs. With loading
  `1e-8`, expected ridge residual is about `6.144e-7`.
- Evidence: all diff checks pass, clean detached HEAD, test source only.
  Runtime is correctly unclaimed due the known detached dependency/build gap.
- Classification/lifecycle: immediate GREEN characterization expected; no
  production/build/plugin/dependency/rt_server change and no subagents.
- Manager status: response durable; exact diff/oracle review follows.

### MANAGER REVIEW W-TEST-CORE-011

- Scope/base: accepted; exact commit `148870f87` is clean on `aebe2fafb` and
  changes only the focused test source with one slot.
- Rank proof: after warmup, tap-major vectors are exactly
  `[2s,4s,s,2s]^T = s*v` for `s={1,2,4,8}`, so the four-dimensional normal
  matrix has rank one before loading.
- Oracle: `y=3*r0=6s`; the next probe feature is `16v` and target 96. Relative
  loading `1e-8` gives a ridge residual about `6.144e-7`, safely inside the
  nontrivial `1e-5` tolerance while rejecting a zero/unlearned model.
- State/rows: one warmup plus four eligible samples produces exactly one
  boundary; old model keeps training exact, diagnostics require `1/0` and
  generation one, and both reference rows are value-preserved.
- Decision: accept for cherry-pick and populated-workspace immediate-GREEN
  characterization. No production change is inferred from worker evidence.

### MANAGER VALIDATION W-TEST-CORE-011-GREEN

- Integrated test commit: `d36491643`.
- Focused Release build: success; only known Eigen/MSVC C4819 warnings.
- Synchronized Qt report: 40 passed, zero failed, exit code zero.
- Rank-deficient slot accepts exactly one loaded boundary and its analytic
  future probe passes; all prior numerical contracts remain GREEN.
- Classification: immediate GREEN characterization. It guards the next
  source-only strict positivity hardening from rejecting valid rank-one data.

### RETIREMENT REQUEST CLEANUP-013

- Verify detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-011` is clean at
  `148870f87` and its focused test content matches integrated `d36491643`.
- Remove it through Git worktree management. Completed agent
  `/root/w_test_core_011` is one-shot and must not be reused.
- Status: complete. Worktree was clean at `148870f87`, its focused test exactly
  matched integrated `d36491643`, and Git worktree removal succeeded. The
  completed one-shot agent will not be reused.

### REQUEST W-CORE-008

- From / to: manager / strict-positive LDLT hardening worker.
- Planned execution: detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-core-008` from current integration
  HEAD; agent identifier recorded after dispatch.
- Model/environment: `gpt-5.6-sol`, `ultra`; no nested subagents.
- Model decision: factorization acceptance sits on a numerical transaction
  boundary and must preserve rank-deficient loaded behavior, so Sol/ultra is
  selected.
- Blocking: yes for P2 `R-SOLVE-002`.
- Task: harden only `solveAndCommitPendingEpoch()` so a candidate is accepted
  only when Eigen LDLT reports success and positive factorization, every `D`
  pivot is finite and strictly greater than zero, the solve RHS is finite, and
  candidate weights are finite. Any failed condition must return false before
  committed `G/H/W` writes, using the existing rejection transaction and
  diagnostics. Preserve loading, recurrence and public interface.
- Scope: numerical source only; no test/header/CMake/plugin/dependency or
  rt_server. Do not change tolerance/loading, implement application fallback,
  malloc guard, synthetic acceptance or plugin work.
- Required response: `RESPONSE W-CORE-008` with commit, exact predicates and
  ordering, changed files, diff/available validation, 40-test expected
  characterization status, remaining findings and no-subagent confirmation.
- Status: active as collaboration agent `/root/w_core_008`. Detached worktree
  created clean at `230697fed`; Sol/ultra accepted the source-only scope.

### REQUEST W-CORE-008-STATUS

- From / to: manager / `/root/w_core_008`.
- Blocking: no; the strict-acceptance scope is unchanged.
- Context: repeated bounded waits show the agent running while the detached
  worktree remains clean at its base.
- Question: report concise progress, selected Eigen predicates/API confidence
  and any blocker at the next safe boundary; no rescope or subagents.
- Status: recorded before forwarding.

### RESPONSE W-CORE-008-STATUS

- Conclusion: no blocker; required records/skills and exact clean base were
  verified. One-source patch is present and diff-check clean.
- Predicate order: retain existing finite solve matrix check; after LDLT
  compute require `info()==Success`, then `isPositive()`, then every `vectorD`
  pivot finite and strictly greater than zero. Materialize the transposed
  candidate cross matrix into preallocated RHS and require it all finite;
  solve, require Success and finite candidate weights, then commit `G/H/W`.
- API confidence: high; `isPositive`, `vectorD`, `info` and dense `allFinite`
  are stable Eigen APIs adjacent to current code. Compile/runtime remains
  manager-owned due the known detached baseline gap.
- Scope/lifecycle: 40/0 rank-one behavior expected unchanged; no subagents.
- Next: worker is finishing static checks/commit; do not duplicate or rescope.

### RESPONSE W-CORE-008

- Commit: `e09e3da5c641c6c48e4e74211458e746b2f4c9b6`.
- Changed file: numerical source only.
- Acceptance order: finite solve matrix; LDLT compute; Success; positive
  factorization; every `D` pivot finite and strictly positive; materialized RHS
  finite; solve; solve Success; finite candidate weights; only then commit
  candidate `G/H/W`.
- Transaction: every failure returns false before committed writes, reusing the
  existing rejection diagnostics and pending cleanup. Loading, recurrence,
  tolerances and public interface are unchanged.
- Evidence: exact clean base `230697fed`, pre-stage/staged/committed diff checks,
  clean detached HEAD and one source file. Detached compile/runtime remains
  unavailable due the known ignored dependency/build gap.
- Expected characterization: immediate 40/0 GREEN; loaded rank-one equations
  remain positive after loading and poisoned candidates remain rejected.
- Remaining untouched: RMS coverage, application fallback, malloc guard,
  synthetic/plugin/dependency/rt_server work. No subagents.
- Manager status: response durable; exact commit review follows.

### MANAGER REVIEW W-CORE-008

- Scope/base: accepted. Exact commit `e09e3da5c` is clean on `230697fed` and
  changes one numerical source file by 12 insertions/one deletion.
- Predicate review: `solveMatrix` finiteness still gates compute; LDLT Success
  and `isPositive()` precede an explicit fixed-size loop requiring each `D`
  pivot finite and `>0`; the preallocated RHS is materialized and checked
  finite; solve Success and finite candidate weights remain final gates.
- Transaction: all new returns occur before existing committed `G/H/W`
  assignments. Rejection therefore preserves the selected pending-epoch
  discard/aging/generation semantics.
- Real-time/API: scalar loop and preallocated dense check add no explicit
  allocation, lock or string. Eigen methods are used consistently with the
  existing sized LDLT object.
- Decision: accept for cherry-pick and populated compile/runtime. P2 closes
  only if rank-one and poisoned-recovery cases remain GREEN in the 40-check run.

### MANAGER VALIDATION W-CORE-008-GREEN

- Integrated production commit: `3faa0ca14`.
- Focused Release build: success; only known Eigen/MSVC C4819 warnings.
- Synchronized Qt report: 40 passed, zero failed, process exit code zero.
- Both loaded rank-one acceptance and poisoned-candidate rejection/recovery
  remain GREEN under strict positivity/pivot gates.
- Decision: close P2 `R-SOLVE-002`.

### RETIREMENT REQUEST CLEANUP-014

- Publish 40/0 strict-solver evidence to issue #2.
- Verify detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-core-008` is clean at
  `e09e3da5c` and its source content matches integrated `3faa0ca14`; remove it
  through Git worktree management. The one-shot agent is not reusable.
- Status: complete. Solver evidence published at
  `https://github.com/Edddddddddy/mne-cpp/issues/2#issuecomment-5190277167`.
  Worktree was clean at `e09e3da5c`, source content exactly matched integrated
  `3faa0ca14`, and Git worktree removal succeeded. The agent is not reusable.

### REQUEST W-TEST-CORE-012

- From / to: manager / stable RMS contract test worker.
- Planned execution: detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-012` from current
  integration HEAD; identifier recorded after dispatch.
- Model/environment: `gpt-5.6-sol`, `high`; no nested subagents.
- Blocking: yes for P2 `R-RMS-001`.
- Task: add exactly one public-interface Qt slot with two sequential sections.
  First, Bypass a finite block whose selected target values are alternating
  `+/- DBL_MAX/4`; require exact block preservation, finite input/output RMS
  equal to `DBL_MAX/4` within a relative tolerance, and zero noise RMS. Second,
  train a one-tap one-reference model `y=2r` over one two-sample epoch with
  regularization `1e-8`; ApplyOnly probe refs `[3,4]`, raw targets `[10,14]`.
  Using `w=2/(1+1e-8)`, require diagnostics to match analytic input,
  output-residual and actually-subtracted-noise RMS, finite output, exact ref,
  generation one and zero probe update events.
- Scope: test source only; exactly one slot, no production/header/CMake/plugin/
  dependency or rt_server. Honest immediate GREEN is expected; do not force RED
  or change the existing scaled-sum implementation.
- Required response: `RESPONSE W-TEST-CORE-012` with commit, exact matrices and
  RMS equations/tolerances, evidence/classification and no-subagent confirmation.
- Status: active as collaboration agent `/root/w_test_core_012`. Detached
  worktree created clean at `a7bb413ca`; Sol/high accepted one-slot scope.

### REQUEST W-TEST-CORE-012-STATUS

- From / to: manager / `/root/w_test_core_012`.
- Blocking: no; one-slot RMS scope remains unchanged.
- Context: repeated bounded waits show the worker running while its detached
  worktree remains clean at the request base.
- Question: report concise progress, chosen overflow-safe relative comparison,
  analytic tolerance and any blocker; no rescope or subagents.
- Status: recorded before forwarding.

### RESPONSE W-TEST-CORE-012-STATUS

- Conclusion: no blocker; required records/skills read and the one-slot,
  two-section test-only patch is implemented.
- Large RMS: selected target `[+M,-M,+M,-M]`, `M=DBL_MAX/4`, exact block
  preservation and overflow-safe `abs(actual/M - 1) <= 8*epsilon`, avoiding
  direct large subtraction or squaring.
- Analytic RMS: `W=2/(1+1e-8)`; expected input `sqrt(148)`, output residual RMS
  from `[10-3W,14-4W]`, and noise `W*sqrt(12.5)`. Combined `1e-12` absolute and
  relative tolerance covers diagnostics and target samples.
- Scope/lifecycle: final diff review/available validation/commit pending; no
  subagents and no scope change.
- Next: await clean commit response; do not duplicate.

### RESPONSE W-TEST-CORE-012

- Commit: `c302fbfff6d1b057161597390c98ce3b9aa98dff`.
- Changed file: focused numerical test source only; one slot with two sections.
- Bypass oracle: reference `[1,-2,3,-4]`, target alternating
  `+/- DBL_MAX/4`; require Bypassed, exact block, finite input/output RMS and
  zero noise. Compare with `abs(actual/M-1) <= 8*epsilon` to avoid overflow.
- Training/probe oracle: train `[1,2] -> [2,4]`, one tap/interval two/reg
  `1e-8`, so `W=2/(1+1e-8)`. Probe refs `[3,4]`, raw target `[10,14]` yields
  residuals `[10-3W,14-4W]`, input `sqrt(148)`, output residual RMS, and noise
  `W*sqrt(12.5)`; combined absolute/relative `1e-12` tolerance.
- State/rows: training accepts once/generation one; probe has zero events,
  finite output and exact reference preservation.
- Evidence: one-file/one-slot clean commit and independent oracle; detached
  runtime unavailable only due known ignored baseline/build gap.
- Classification/lifecycle: immediate GREEN expected, no forced RED and no
  subagents. Manager exact review follows.

### MANAGER REVIEW W-TEST-CORE-012

- Scope/base: accepted; exact commit `c302fbfff` is clean on `a7bb413ca` and
  adds exactly one test slot/source with no production/build change.
- Overflow oracle: true RMS of four alternating `+/-M` samples is exactly `M`;
  `actual/M-1` is the correct bounded comparison and avoids overflow from
  direct large subtraction/squaring. `8*epsilon` is tight but realistic.
- ApplyOnly oracle: recursive forgetting preserves `H=2G`; relative loading
  produces `W=2/(1+reg)` up to rounding. Input, output residual and actually
  subtracted prediction RMS equations match the selected diagnostics semantics;
  combined `1e-12` tolerances are materially tighter than the signal scale.
- State/sensitivity: the training boundary must accept; an unlearned or wrong
  prediction fails sample and RMS checks; reference preservation is explicit.
- Decision: accept for cherry-pick and populated immediate-GREEN execution.

### MANAGER VALIDATION W-TEST-CORE-012-GREEN

- Integrated test commit: `ada68a56b`.
- Focused Release build: success; only known Eigen/MSVC C4819 warnings.
- Synchronized Qt report: 41 passed, zero failed, exit code zero.
- Large finite Bypass RMS and analytic ApplyOnly input/output/noise RMS all
  pass; all prior contracts remain GREEN.
- Decision: close P2 `R-RMS-001`.

### RETIREMENT REQUEST CLEANUP-015

- Publish 41/0 stable-RMS evidence to issue #2.
- Verify detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-012` is clean at
  `c302fbfff` and its test content matches integrated `ada68a56b`; remove it
  through Git worktree management. The one-shot agent is not reusable.
- Status: complete. Stable-RMS evidence published at
  `https://github.com/Edddddddddy/mne-cpp/issues/2#issuecomment-5190370219`.
  Worktree was clean at `c302fbfff`, its test exactly matched integrated
  `ada68a56b`, and Git worktree removal succeeded. The agent is not reusable.

### REQUEST W-TEST-CORE-013

- From / to: manager / finite application-overflow safety test worker.
- Planned execution: detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-013` from current
  integration HEAD; identifier recorded after dispatch.
- Model/environment: `gpt-5.6-sol`, `ultra`; no nested subagents.
- Model decision: the test fixes multi-target atomicity, floating-point overflow
  and real-time state semantics, so Sol/ultra is selected.
- Blocking: yes for P2 `R-APPLY-001`.
- Task: add exactly one public-interface Qt slot. Configure one reference, two
  targets, one preserved row, one tap and interval two. Train one boundary on
  refs `[1,2]`, targets `2r` and `0.5r`, producing finite committed weights.
  ApplyOnly a one-sample probe with ref `DBL_MAX`, raw targets `[7,11]` and a
  preserved sentinel. The first prediction must overflow while the second is
  finite; require sample-wide fallback: Processed status, entire probe exactly
  unchanged and finite, generation unchanged, zero events, input/output RMS
  `sqrt(85)` and estimated-noise RMS zero. Then a normal ApplyOnly probe
  `[r=3, targets=6,1.5]` must still denoise both targets near zero, proving the
  committed model was retained.
- Scope: test source only; exactly one slot, no production/header/CMake/plugin/
  dependency or rt_server. This must be an honest runtime RED against current
  partial/nonfinite writes; do not implement fallback in the test task.
- Required response: `RESPONSE W-TEST-CORE-013` with commit, exact matrices,
  overflow and atomicity argument, RMS/state oracle, expected RED, evidence and
  no-subagent confirmation.
- Status: active as collaboration agent `/root/w_test_core_013`. Detached
  worktree created clean at `d991e1e9a`; Sol/ultra accepted the one-slot scope.

### REQUEST W-TEST-CORE-013-STATUS

- From / to: manager / `/root/w_test_core_013`.
- Blocking: no; fallback test scope remains unchanged.
- Context: bounded waits show the worker running while the detached worktree
  remains clean at its request base.
- Question: report concise progress, mixed-target overflow oracle and any
  blocker at the next safe boundary; no rescope or subagents.
- Status: recorded before forwarding.

### RESPONSE W-TEST-CORE-013-STATUS

- Conclusion: no blocker; one-source/one-slot patch is implemented and final
  diff/available runtime checks are next.
- Training: ref `[1,2]`, targets `[2,4]` and `[0.5,1]`, preserved `[100,101]`;
  one accepted boundary/generation one with weights near two and one-half.
- Mixed probe: `[DBL_MAX,7,11,102]^T`; first prediction overflows while second
  stays finite. Require Processed, exact sample-wide finite pass-through,
  generation one, zero events, input/output `sqrt(85)` and noise zero.
- Integrity probe: `[3,6,1.5,103]^T`; exact non-target rows and both target
  residuals `<=1e-5`.
- Scope/lifecycle: no expansion and no subagents; final commit pending.
- Next: continue the same worker only.

### RESPONSE W-TEST-CORE-013

- Commit: `a5f37e5bda129bf29d3bf497e9912e8fa71893f1` on base
  `d991e1e9a`; one focused test slot/source, 88 insertions.
- Matrices: training rows `[[1,2],[2,4],[0.5,1],[100,101]]`;
  overflow probe `[[DBL_MAX],[7],[11],[102]]`; normal probe
  `[[3],[6],[1.5],[103]]`.
- Model: one accepted boundary/generation one; weights near `1.99999998` and
  `0.499999995`. Overflow predictions are infinity and about `8.988e307`.
- Fallback oracle: Processed, exact all-finite complete probe, generation one,
  zero events, input/output `sqrt(85)` and noise zero. Normal probe later has
  exact non-targets and residuals below `1e-5`.
- Expected current RED: existing loop writes `-Inf` to target one and about
  `-8.988e307` to target two; first failure should be `overflowProbe.allFinite()`.
- Evidence: clean one-file commit/diff; detached runtime unavailable due known
  ignored dependency/build gap; no repair and no subagents.
- Manager status: response durable; exact review follows.

### MANAGER REVIEW W-TEST-CORE-013

- Scope/base: accepted. Exact commit `a5f37e5bd` is clean on `d991e1e9a` and
  changes one test source/slot only.
- Overflow proof: target-one weight near two times `DBL_MAX` must overflow;
  target-two weight near one-half remains finite. Requiring the complete probe
  exact distinguishes selected sample-wide atomic fallback from partial per-
  target subtraction.
- RMS/state: raw target RMS is `sqrt((7^2+11^2)/2)=sqrt(85)`; exact passthrough
  makes output equal and actually-subtracted noise zero. Generation/events and
  later analytic normal probe prove no committed-model corruption.
- Sensitivity: current implementation necessarily violates all-finite/exact
  output before later assertions; this is a genuine public-interface runtime RED.
- Decision: accept for cherry-pick and focused RED reproduction; no production
  edit is authorized before failure evidence is captured.

### MANAGER VALIDATION W-TEST-CORE-013-RED

- Integrated test commit: `eee23de46`.
- Focused Release build: success; only known Eigen/MSVC C4819 warnings.
- Synchronized Qt report: 41 passed, one failed, exit code one. The unique
  failure is `overflowProbe.allFinite()` in the new slot; all prior contracts
  remain GREEN.
- Decision: valid runtime RED for P2 `R-APPLY-001`; production fallback may
  begin under the selected SPEC semantics.

### RETIREMENT REQUEST CLEANUP-016

- Publish the 41/1 RED evidence to issue #2.
- Verify detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-013` is clean at
  `a5f37e5bd` and its test content matches integrated `eee23de46`; remove it
  through Git worktree management. The one-shot agent is not reusable.
- Status: complete. RED evidence published at
  `https://github.com/Edddddddddy/mne-cpp/issues/2#issuecomment-5190474486`.
  Worktree was clean at `a5f37e5bd`, its test exactly matched integrated
  `eee23de46`, and Git worktree removal succeeded. The agent is not reusable.

### REQUEST W-CORE-009

- From / to: manager / application-overflow fallback implementation worker.
- Planned execution: detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-core-009` from current integration
  HEAD; identifier recorded after dispatch.
- Model/environment: `gpt-5.6-sol`, `ultra`; no nested subagents.
- Blocking: yes for P2 `R-APPLY-001` and synthetic acceptance.
- Task: make only `fallsBackAtomicallyWhenPredictionOverflows` GREEN. After
  computing the preallocated prediction vector, validate all target predictions
  and raw-target-minus-prediction residuals before any target write or noise-RMS
  accumulation. If any is nonfinite, set the preallocated prediction vector to
  zero and leave every target at that sample raw; otherwise subtract all target
  predictions. Then accumulate the prediction actually subtracted. Preserve raw
  target learning, transactional epoch rejection, history advancement and
  Processed status exactly as selected in SPEC.
- Scope: numerical source only; no tests/header/CMake/plugin/dependency or
  rt_server. Do not add a public status/diagnostic field, saturate/clamp values,
  change solver/RMS logic, malloc guard or synthetic/plugin work.
- Required response: `RESPONSE W-CORE-009` with commit, exact validation/write/
  learn/history ordering, atomicity and no-allocation argument, evidence/infra
  limitation, remaining work and no-subagent confirmation.
- Status: active as collaboration agent `/root/w_core_009`. Detached worktree
  created clean at `1cfae4696`; Sol/ultra accepted source-only scope.

### REQUEST W-CORE-009-STATUS

- From / to: manager / `/root/w_core_009`.
- Blocking: no; source-only fallback scope is unchanged.
- Context: bounded waits show the worker running while its detached worktree is
  still clean at the request base.
- Question: report concise progress, exact validation/write ordering and any
  blocker at the next safe boundary; no rescope or subagents.
- Status: recorded before forwarding.

### RESPONSE W-CORE-009-STATUS

- Conclusion: no implementation blocker; required records/skills and exact
  clean base are verified; integrated RED and process loop inspected.
- Planned order: input RMS/current refs; feature; raw targets; preallocated
  prediction; scalar validation of all prediction/residual pairs before any
  write/noise accumulation; on failure zero prediction and leave targets raw,
  otherwise write every residual; then accumulate actually-subtracted
  prediction; learn transactionally from raw targets; advance history.
- Bypass/warmup remain history-only. Detached compile/runtime unavailable only
  because of the known ignored dependency/build gap; static/diff checks remain.
- Scope/lifecycle: source-only semantics unchanged and no subagents.
- Next: await implemented clean commit; no duplicate/rescope.

### RESPONSE W-CORE-009

- Commit: `ae8824219b33056b66dc50c6576a11c573aea701`.
- Changed file: numerical source only.
- Order: capture raw targets/compute preallocated prediction; validate every
  prediction and residual before writes/noise; on any nonfinite zero prediction
  and leave all targets raw, otherwise write all residuals; accumulate actual
  prediction; learn from raw targets; advance history.
- Atomicity/real-time: no rollback buffer because validation precedes every
  target write; only stack bool/double, scalar loops and in-place zero on
  existing storage. `process` stays noexcept without explicit allocation,
  lock or string.
- Preserved: Processed status, history, generation/events, solver transaction,
  scaled RMS, valid subtraction and non-target rows.
- Evidence: exact clean base, pre-stage/staged/committed diff checks and clean
  detached HEAD. Compiled/runtime remains manager-owned due known ignored
  dependency/build gap.
- Remaining: malloc guard, synthetic acceptance and plugin work. No subagents.
- Manager status: response durable; exact commit review follows.

### MANAGER REVIEW W-CORE-009

- Scope/base: accepted. Exact commit `ae8824219` is clean on `1cfae4696` and
  changes one numerical source file by 18 insertions/two deletions.
- Atomicity: raw targets already reside in preallocated scratch; a complete
  scalar validation pass occurs before any block write. Failure zeros the whole
  prediction vector and writes nothing, so all targets at the sample remain raw.
- Diagnostics/learning: noise accumulation follows the decision and therefore
  records exactly what was subtracted. ApplyAndLearn continues from raw targets
  and the same feature; existing transactional boundary rejection contains any
  extreme candidate. History ordering is unchanged.
- Real-time: stack scalar state and in-place vector zero only; no dynamic
  container, exception, lock, string or rollback buffer. Valid fast-path writes
  all targets only after validation.
- Decision: accept for cherry-pick and populated build/runtime; close P2 only
  after the 42/0 report.

### MANAGER VALIDATION W-CORE-009-GREEN

- Integrated production commit: `2aac482d9`.
- Focused Release build: success; only known Eigen/MSVC C4819 warnings.
- Synchronized Qt report: 42 passed, zero failed, exit code zero.
- Mixed overflow/finite prediction sample falls back atomically; RMS/state and
  future normal-model integrity pass with every prior contract.
- Decision: close P2 `R-APPLY-001`; no P0-P2 numerical review finding remains.

### RETIREMENT REQUEST CLEANUP-017

- Publish 42/0 application-fallback evidence to issue #2.
- Verify detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-core-009` is clean at
  `ae8824219` and its source content matches integrated `2aac482d9`; remove it
  through Git worktree management. The one-shot agent is not reusable.
- Status: complete. GREEN evidence published at
  `https://github.com/Edddddddddy/mne-cpp/issues/2#issuecomment-5190579108`.
  Worktree was clean at `ae8824219`, its source exactly matched integrated
  `2aac482d9`, and Git worktree removal succeeded. The agent is not reusable.

### REQUEST W-TEST-CORE-014

- From / to: manager / Eigen hot-path malloc-guard test worker.
- Planned execution: detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-014` from current
  integration HEAD; identifier recorded after dispatch.
- Model/environment: `gpt-5.6-sol`, `ultra`; no nested subagents.
- Model decision: runtime allocation assertions around dynamic Eigen LDLT and
  matrix expressions are a real-time safety gate, so Sol/ultra is selected.
- Blocking: yes for core real-time acceptance and formal core review.
- Task: add `EIGEN_RUNTIME_NO_MALLOC` to the focused test target and exactly one
  Qt slot proving configured `process()` does not allocate. Allocate/configure
  every matrix before the guard. With Eigen assertions active, disable malloc,
  call ApplyAndLearn on a finite stream that crosses multi-tap warmup and one
  full update boundary (therefore exercising feature products, candidate
  composition, LDLT solve, diagnostics and history), restore malloc immediately,
  then assert Processed/one acceptance/generation, finite output and exact
  non-target rows. Also guard a preallocated ApplyOnly probe using committed
  weights. The slot may explicitly skip when `EIGEN_NO_DEBUG` is defined; the
  manager will build/run Debug for effective guard evidence.
- Scope: focused test source and its CMake only; no production/header/plugin/
  dependency or rt_server. Do not change denoiser code, vendor Eigen, add a
  global allocator hook, benchmark or synthetic acceptance.
- Required response: `RESPONSE W-TEST-CORE-014` with commit, exact config/data
  and guarded regions, compile-definition/debug assertion explanation,
  evidence/infra limitation and no-subagent confirmation.
- Status: active as collaboration agent `/root/w_test_core_014`. Detached
  worktree created clean at `7c29b4f55`; Sol/ultra accepted scoped request.

### REQUEST W-TEST-CORE-014-STATUS

- From / to: manager / `/root/w_test_core_014`.
- Blocking: no; malloc-guard scope is unchanged.
- Context: bounded waits show the worker running while detached worktree remains
  clean at the request base.
- Question: report concise progress, Debug/Eigen assertion handling, guarded
  call coverage and any blocker; no rescope or subagents.
- Status: recorded before forwarding.

### RESPONSE W-TEST-CORE-014-STATUS

- Conclusion: no blocker; required records/skills/base and focused CMake/test
  are fully inspected.
- Planned slot: one configured five-row denoiser, refs `{0,1}`, targets `{2,3}`,
  preserved row four, four taps (`P=8`), interval four. Guard one seven-sample
  ApplyAndLearn call (three warmup plus four eligible/LDLT), restore immediately,
  then separately guard a preallocated committed-model ApplyOnly probe.
- Allocation discipline: every block/copy/state allocated/configured before
  guard; all Qt assertions after restoration. Target defines
  `EIGEN_RUNTIME_NO_MALLOC`.
- Debug effectiveness: `EIGEN_NO_DEBUG` branch QSKIPs before setup so Release
  cannot masquerade as evidence; manager Debug run is authoritative.
- Scope/lifecycle: numeric data finalization remains; no subagents or rescope.
- Next: continue same worker only.

### RESPONSE W-TEST-CORE-014-GUARD-CORRECTION

- Correction: Eigen 3.4 `set_is_malloc_allowed(bool)` returns the newly stored
  value, not the previous state.
- Implemented sequence: snapshot `is_malloc_allowed()` first; disable; call
  process; immediately restore the snapshot. Both guarded regions use this
  sequence.
- Decision: accepted correction; no scope change. Await complete commit.

### RESPONSE W-TEST-CORE-014

- Commit: `ecd252fbaae7c452fe39d3a9461fdb7a0ad10814`.
- Changed files: focused test CMake and test source only; exactly one slot.
- Config/data: 1000 Hz, five rows, refs `{0,1}`, targets `{2,3}`, preserved row
  four, four taps (`P=8`), interval four. Seven-sample constant relationship
  covers three warmup plus four eligible samples and a loaded LDLT boundary.
- Guard: snapshot `is_malloc_allowed`, disable, call ApplyAndLearn, restore
  immediately; separately guard preallocated ApplyOnly. All buffers/copies/state
  pre-exist and all Qt assertions follow restoration.
- Assertions: one accepted generation, finite diagnostics/output, exact
  non-target rows; ApplyOnly target residuals below `1e-6`, positive noise RMS
  and exact non-targets.
- Build semantics: target defines `EIGEN_RUNTIME_NO_MALLOC`; Debug assertions
  abort on allocation. `EIGEN_NO_DEBUG` explicitly QSKIPs so Release cannot be
  misreported as effective evidence.
- Evidence: clean scoped commit/diff; analytic probe residuals about `6e-9` and
  `-8e-9`. Detached runtime unavailable due known baseline/build gap; no repair.
- Lifecycle: no subagents. Manager exact review/Debug execution follow.

### MANAGER REVIEW W-TEST-CORE-014

- Scope/base: accepted. Exact commit `ecd252fba` is clean on `7c29b4f55` and
  changes only focused test CMake plus one test slot.
- Effectiveness: `EIGEN_RUNTIME_NO_MALLOC` covers both directly compiled test
  and numerical source. Under Debug, Eigen's `eigen_assert` checks the runtime
  flag; Release/NDEBUG is explicitly skipped and cannot be used as proof.
- Coverage: all allocations/copies/configure occur before disabling. The guarded
  learning call includes warmup, `P=8` feature products, pending/candidate
  matrices, loaded LDLT solve, diagnostics and history; a second guarded call
  exercises committed model application.
- Restoration: explicit pre-call `is_malloc_allowed()` snapshot avoids Eigen's
  setter-return trap and is restored before any Qt assertion.
- Decision: accept for cherry-pick and populated Debug build/run. A process
  abort would be valid failure evidence; success must produce the effective
  guard slot pass, not a skip.

### MANAGER VALIDATION W-TEST-CORE-014-GREEN

- Integrated commit: `b719d7ccd`.
- Debug build: success; Qt report explicitly identifies a Debug build with
  Eigen assertions active. Only known Eigen/MSVC C4819 warnings.
- Synchronized report: 43 passed, zero failed, zero skipped, exit code zero.
- Guard slot crossed `P=8` warmup/full boundary/LDLT and committed ApplyOnly
  without process abort, proving no Eigen heap allocation after configure for
  these hot paths.
- Decision: core malloc-guard acceptance is satisfied.

### RETIREMENT REQUEST CLEANUP-018

- Publish effective Debug 43/0 malloc-guard evidence to issue #2.
- Verify detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-014` is clean at
  `ecd252fba` and its focused CMake/test content matches integrated `b719d7ccd`;
  remove it through Git worktree management. Agent is not reusable.
- Status: complete. Effective guard evidence published at
  `https://github.com/Edddddddddy/mne-cpp/issues/2#issuecomment-5190727524`.
  Worktree was clean at `ecd252fba`, its CMake/test content exactly matched
  integrated `b719d7ccd`, and Git worktree removal succeeded. Agent not reused.

### REQUEST W-TEST-CORE-015

- From / to: manager / quantitative synthetic acceptance test worker.
- Planned execution: detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-015` from current
  integration HEAD; identifier recorded after dispatch.
- Model/environment: `gpt-5.6-sol`, `ultra`; no nested subagents.
- Model decision: deterministic streaming signal construction, causal lag
  alignment and quantitative dB/amplitude gates require high-risk numerical
  reasoning.
- Blocking: yes for core numerical acceptance and formal review.
- Task: add exactly one public-interface Qt slot using two deterministic rich
  reference streams, four taps (`P=8`), one target, 128-sample blocks/update
  interval, 30-second memory and `1e-3` regularization. Precompute a continuous
  finite reference stream, construct target environmental noise from known
  tap-major weights with zero prehistory, and train over enough blocks for
  steady state using ApplyAndLearn. Evaluate one continuing ApplyOnly block
  whose raw target is known noise plus a deterministic clean sinusoid. Require
  exact reference preservation/finite output, environmental-noise reduction
  `20*log10(rms(noise)/rms(output-clean)) >= 10 dB`, and clean projection
  amplitude error `abs(dot(output,clean)/dot(clean,clean)-1) <= 0.02`.
- Determinism/sensitivity: fixed integer PRNG or explicit deterministic formula,
  no external data/random_device; log measured values with `qInfo`. Use enough
  training epochs for a robust margin, not a threshold-tuned borderline case.
- Scope: test source only; exactly one slot, no production/header/CMake/plugin/
  dependency or rt_server. Honest immediate GREEN is allowed; do not change the
  algorithm or add benchmark/example code in this task.
- Required response: `RESPONSE W-TEST-CORE-015` with commit, generator/weights/
  block counts and causal construction, exact metric equations, expected margin,
  evidence/classification and no-subagent confirmation.
- Status: active as collaboration agent `/root/w_test_core_015`. Detached
  worktree is clean at exact integration base `ea5870cf2`; Sol/ultra accepted
  the one-slot, test-source-only scope and the nested-subagent prohibition.

### REQUEST W-TEST-CORE-015-STATUS

- From / to: manager / `/root/w_test_core_015`.
- Blocking: no; the single synthetic acceptance slot remains unchanged.
- Context: repeated bounded waits report the agent running while its detached
  worktree remains clean at the exact request base.
- Question: at the next safe boundary, report concise progress, chosen
  deterministic generator/known weights/training duration, expected metric
  margins and any blocker. Do not rescope, edit production code or use
  subagents.
- Status: recorded before forwarding.

### RESPONSE W-TEST-CORE-015-STATUS

- Conclusion: no blocker; exact base, durable records, codebase-design/TDD
  skills and current public seam are verified.
- Generator: one uninterrupted 98,432-sample two-reference stream from fixed
  xorshift32 seeds, independent AR mixes `0.35` and `-0.25`, and deterministic
  multisine terms.
- Causal model: tap-major weights
  `[1.4,-1.1,0.8,0.65,-0.55,0.45,0.35,-0.25]` with explicit zero prehistory.
- Training/evaluation: 768 full 128-sample ApplyAndLearn blocks (98.304 s,
  more than three 30-second memory constants), then one continuing 128-sample
  ApplyOnly block with a 0.65-amplitude 62.5 Hz clean sinusoid. Expected 767
  accepted epochs after the three-sample warmup.
- Independent oracle: about 58.65 dB noise reduction and `1.13e-4` clean
  projection-amplitude error, wide versus the 10 dB and 0.02 gates.
- Scope/lifecycle: source-only single slot, no production change or subagents;
  patch, validation and commit remain in progress.

### RESPONSE W-TEST-CORE-015

- Commit: `bae282c347cfb578d6a78109eeb92dfb885125ae` on exact requested
  base `ea5870cf2`.
- Changed file/scope: focused numerical test source only; one public-interface
  Qt slot, no production/header/CMake/plugin/dependency/benchmark/example or
  rt_server change.
- Generator: uninterrupted 98,432-sample stream from xorshift32 seeds
  `0xA341316C` and `0xC8013EA4`, AR coefficients `0.35/-0.25`, and
  deterministic multisine terms.
- Causal noise: known tap-major weights
  `[1.40,-1.10,0.80,0.65,-0.55,0.45,0.35,-0.25]`, exact current-to-three-lag
  ordering and explicit zero prehistory. The first three samples only warm up.
- Training/state: 768 128-sample ApplyAndLearn blocks; expect 767 accepted
  complete epochs, zero rejected and 125 pending samples. Evaluate one
  continuing ApplyOnly block with a 0.65-amplitude 62.5 Hz clean sinusoid.
- Metrics: `rms(x)=sqrt(sum(x^2)/128)`, reduction is
  `20*log10(rms(noise)/rms(output-clean))`, and amplitude error is
  `abs(dot(output,clean)/dot(clean,clean)-1)`.
- Independent oracle: noise RMS `1.11545669742232`, residual RMS
  `0.00130240402120637`, reduction `58.654139739579` dB and amplitude error
  `1.13194967407138e-4`; margins are +48.654 dB and 0.0198868.
- Evidence/classification: exact reference preservation, finite output and zero
  rejected updates are asserted; metrics use qInfo. Static diff checks and
  clean detached HEAD pass. Runtime is manager-owned due the known ignored
  detached dependencies; immediate GREEN is analytically expected.
- Lifecycle: no subagents. Manager exact review and populated execution follow.

### MANAGER REVIEW W-TEST-CORE-015

- Scope/base: accepted. Exact commit `bae282c347` is clean on requested base
  `ea5870cf2` and adds one slot/test source only (172 lines including one
  standard-library include).
- Causality/counts: target noise uses the module's exact tap-major current-to-
  lag-three ordering with zero prehistory. After three warmup samples,
  `98,301 / 128` gives 767 accepted epochs and 125 pending samples, matching
  the asserted generation and aggregate event count.
- Signal quality: two independent seeded broadband AR/multisine references make
  the eight-feature system well excited. More than three memory constants of
  pure known-noise training separates estimator convergence from the clean
  evaluation sinusoid and leaves wide analytic gate margins.
- Sensitivity: exact reference equality guards row selection; finite output,
  zero rejects/generation, residual-noise reduction and clean projection all
  fail materially for an unlearned, mis-lagged or corrupt model.
- Decision: accept for cherry-pick and populated Release execution. Runtime
  metrics, not the independent oracle alone, determine quantitative GREEN.

### MANAGER VALIDATION W-TEST-CORE-015-GREEN

- Integrated commit: `a079df377`.
- Release build: focused target succeeds with only the known Eigen/MSVC C4819
  code-page warnings.
- Synchronized report: 43 passed, zero failed, one intentional Release skip for
  the Debug-only Eigen malloc guard; process exit code zero.
- Quantitative metrics: 58.6541 dB environmental-noise reduction and
  `0.000113195` clean projection-amplitude error, versus the >=10 dB and <=0.02
  hard gates. Chunk equivalence remains exactly zero in the same report.
- Decision: quantitative core acceptance is GREEN. The earlier Debug 43/0/0
  report remains the effective malloc-allocation proof.

### RETIREMENT REQUEST CLEANUP-019

- Publish the measured Release metrics and 43/0/1 report to issue #2.
- Verify detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-015` is clean at
  `bae282c347` and its focused test content matches integrated `a079df377`;
  remove it through Git worktree management. The one-shot Sol/ultra agent is
  not reusable.
- Status: recorded before action.
- Publish attempt 1: `gh issue comment` failed with GitHub GraphQL EOF; branch
  and evidence commits were already pushed successfully. Record before retry;
  no code or issue state change is claimed. Worktree verification remains.
- Worktree cleanup: complete. The detached tree was clean at exact
  `bae282c347`; worker/integration focused-test hashes both equal
  `8e649c188532cf3e1b105b4d29d232ea6017f737`; verified Git worktree removal
  succeeded. The one-shot agent will not be reused.
- Publish status: issue comment still pending retry; the failure-record commit
  push also encountered a transient GitHub SSL connection error.
- Final status: complete. Branch retry succeeded; quantitative evidence was
  published at
  `https://github.com/Edddddddddy/mne-cpp/issues/2#issuecomment-5190907402`.

### REQUEST R-CORE-001

- From / to: manager / independent formal core reviewer.
- Planned execution: detached read-only worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-review-r-core-001` at the exact integration
  HEAD recorded after request commit; agent identifier recorded after dispatch.
- Model/environment: `gpt-5.6-sol`, `ultra`; no edits and no subagents.
- Blocking: yes for starting plugin-data implementation.
- Review scope: public header, numerical source, focused CMake/test source and
  durable SPEC/evidence. Independently inspect deep-module interface/locality,
  config transactionality, causal tap/history/update-boundary semantics, EWLS
  forgetting/loading/LDLT acceptance, pending/committed state transactions,
  modes, nonfinite/overflow behavior, diagnostics, chunk invariance, target-only
  writes and configured hot-path real-time safety.
- Evidence audit: distinguish code proof from tests; validate the Release
  43/0/1 quantitative metrics, Debug 43/0/0 effective malloc guard and exact
  chunk equivalence recorded in the ledgers. Do not run `mne_rt_server`.
- Finding format: P0-P3, stable ID, exact file/line, evidence, impact, requested
  fix and required test. Explicitly state when no finding exists at a priority.
  P0/P1 must be zero; P2 must be fixed or explicitly deferred before core close.
- Required response: `RESPONSE R-CORE-001` with reviewed commit, inspection/
  validation performed, prioritized findings, residual risks, overall gate
  recommendation and no-edit/no-subagent confirmation.
- Status: active as collaboration agent `/root/r_core_001`. Detached worktree
  is clean at exact reviewed commit `f0a0e14d7`; Sol/ultra accepted the
  read-only/no-edit/no-subagent scope.

### RESPONSE R-CORE-001-START

- Reviewer confirmed the exact `f0a0e14` read-only review has begun.
- Method: use codebase-design to assess interface depth/locality; inspect only,
  with no edit, stage, rt_server execution or nested subagent.
- Status: no finding or gate conclusion yet; formal response remains pending.

### REQUEST R-CORE-001-STATUS

- From / to: manager / `/root/r_core_001`.
- Blocking: no; exact commit and read-only formal scope remain unchanged.
- Context: repeated bounded waits show the reviewer running and its detached
  worktree remains clean.
- Question: at the next safe boundary, report reviewed areas, any provisional
  P0-P2 candidate or absence so far, remaining work and blocker. Do not finalize
  early, edit, run rt_server or spawn subagents.
- Status: recorded before forwarding.

### RESPONSE R-CORE-001-STATUS

- Exact review state: detached `f0a0e14d7` remains clean/read-only; all durable
  recovery records, full codebase-design skill, public header/source, relevant
  CMake and all 1,546 focused test lines are inspected.
- Provisional priorities: no P0 and no P1. One provisional P2 evidence gap:
  SPEC requires forgetting coverage, but every current oracle is stationary or
  same-implementation equivalence; a missing/wrong lambda recurrence could pass
  without a lambda-dependent analytic expectation.
- Implementation view so far: recurrence and state transactions are internally
  consistent; no production defect claimed yet.
- Provisional P3 areas: public-contract locality/documentation and narrow
  allocation/evidence provenance.
- Remaining: edge cases, noexcept/integer boundaries, CMake and evidence audit.
  No blocker, edits, rt_server or subagents; reviewer continues.

### RESPONSE R-CORE-001

- Reviewed commit: exact clean detached `f0a0e14d7`; no edits, staging,
  comments, rt_server or subagents.
- Inspections: durable records/codebase-design, public header, complete source,
  library/focused CMake and all 1,546 test lines; C++14, deep-interface locality,
  transactions, causality, EWLS/LDLT, modes, finite safety, diagnostics, reset,
  chunking, target writes, integer/noexcept and hot-path behavior.
- P0/P1: none.
- P2 `R-CORE-FORGET-001`: no public lambda-dependent analytic forgetting
  expectation. Stationary relationships and same-implementation equivalence
  can pass if decay is missing/wrong; even the synthetic gate still passes with
  lambda forced to one. Required test: `fs=1`, `tau=1/log(2)`, one ref/tap/
  target, interval two, refs `[1,1,1,1]`, targets `[0,1,0,0]`; after two
  accepts require future prediction from `G=1.875`, `H=0.25`,
  `W=H/(G*(1+regularization))`.
- P3 `R-CORE-LOCALITY-001`: essential config/orientation/mode/error/ownership/
  allocation/copy-move semantics live in SPEC rather than compact public-header
  Doxygen. Add contract text and explicit copy/move policy with trait/noexcept
  compile checks later.
- P3 `R-CORE-LINK-001`: isolated test compiles source directly and cannot catch
  actual shared/static `mne_rtprocessing` client-link/export regressions. Retain
  isolation but add/schedule a client-link smoke before final integration.
- Evidence audit: independent synthetic oracle reproduced 58.6541397394941 dB
  and `0.000113194967413`; existing matching Release executable exited zero.
  Debug malloc and exact chunk-zero counts remain durable ledger evidence, not
  independently rerun. No machine-readable report is committed.
- Residual risks: guard is P=8 not P=256/rejection/overflow; allocation failure
  lacks fault injection; extreme `fs*tau` rounding to zero/one uncharacterized.
- Gate recommendation: hold only for the sole P2 analytic test; then core may
  proceed while tracking the two P3s.

### RETIREMENT RESPONSE CLEANUP-020

- Review worktree `C:/Users/lcy/Desktop/meg/mne-cpp-review-r-core-001` was
  clean at exact reviewed `f0a0e14d7`; verified Git worktree removal succeeded.
- Reviewer made no code/GitHub mutations and is a completed one-shot role; it
  will not be reused. Durable findings remain in this ledger and REVIEW.md.

### REQUEST W-TEST-CORE-016

- From / to: manager / analytic forgetting acceptance test worker.
- Planned execution: detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-016` from the exact
  integration HEAD recorded after request commit.
- Model/environment: `gpt-5.6-sol`, `high`; no nested subagents.
- Model decision: this is a bounded one-slot public analytic oracle with fully
  specified scalar equations; Sol/high is sufficient while formal review used
  Sol/ultra.
- Blocking: yes for P2 `R-CORE-FORGET-001` and core gate closure.
- Task: add exactly one public-interface Qt test slot. Configure one reference,
  one target, one tap, two-sample max block/update interval, `fs=1`,
  `memoryTimeSeconds=1/log(2)` so independently `lambda=0.5`, and a supported
  fixed regularization. Process two sequential ApplyAndLearn blocks whose full
  stream has references `[1,1,1,1]` and raw targets `[0,1,0,0]`. Require one
  accepted update per block, zero rejects, generation two and exact references.
  Then ApplyOnly a future `r=1`, raw-target-zero sample and require output equal
  `-W` with `G=1.875`, `H=0.25`,
  `W=H/(G*(1+regularization))` at a tight analytic tolerance. Log expected and
  observed values. This must distinguish lambda=1, missing within-epoch decay
  and missing committed-stat aging.
- Scope: focused test source only, exactly one slot; no production/header/CMake/
  plugin/dependency/benchmark/example/rt_server. Honest immediate GREEN is
  expected; do not force RED or modify the recurrence.
- Required response: `RESPONSE W-TEST-CORE-016` with commit, exact matrices,
  step-by-step analytic recurrence/loading, sensitivity alternatives,
  evidence/classification, changed file/clean status and no-subagent confirmation.
- Status: active as collaboration agent `/root/w_test_core_016`. Detached
  worktree is clean at exact base `12ab4e621`; Sol/high accepted the one-slot,
  test-source-only scope and nested-subagent prohibition.

### REQUEST W-TEST-CORE-016-STATUS

- From / to: manager / `/root/w_test_core_016`.
- Blocking: no; exact one-slot analytic scope remains unchanged.
- Context: repeated bounded waits show the worker running while its detached
  worktree remains clean.
- Question: at the next safe boundary report exact recurrence/tolerance,
  sensitivity alternatives, progress and blocker. Do not rescope, edit
  production code or spawn subagents.
- Status: recorded before forwarding.

### RESPONSE W-TEST-CORE-016-STATUS

- Base/scope: exact clean detached `12ab4e621`; one public slot in focused test
  source only, with no production/header/CMake edits or subagents.
- Matrices: first block `[[1,1],[0,1]]`, second `[[1,1],[0,0]]`, future probe
  `[[1],[0]]`.
- Oracle: lambda 0.5 gives epoch-one pending `G=1.5,H=1`; epoch-two pending
  `G=1.5,H=0`; committed aging by `lambda^2=0.25` gives final
  `G=1.875,H=0.25`; scalar loading yields
  `W=H/(G*(1+1e-3))` and probe target `-W`, tolerance `1e-12`.
- Sensitivity: lambda one gives `1/(4*(1+r))`; missing within-epoch decay gives
  `0.25/(2.5*(1+r))`; missing committed aging gives `1/(3*(1+r))`, all
  materially different from the selected oracle.
- Validation: diff check GREEN. Detached compile stopped before test compilation
  because ignored Eigen/Core is absent; nothing was copied/repaired. Worker is
  doing only a read-only include reference to the populated dependency and will
  not edit either worktree.
- Status: commit/complete response pending.

### RESPONSE W-TEST-CORE-016

- Commit/base: `6c39f16db34ef1aeaf0a0ebcca9a9216051db5d0` on exact
  `12ab4e621`; clean detached HEAD and no divergence.
- Scope: one focused test slot/source only; no production/header/CMake/plugin/
  dependency/benchmark/example/rt_server change and no subagents.
- Matrices/config: blocks `[[1,1],[0,1]]`, `[[1,1],[0,0]]`, probe
  `[[1],[0]]`; fs one, tau `1/log(2)`, lambda 0.5, one ref/target/tap,
  interval/block two, regularization `1e-3`.
- Recurrence: epoch one `G=1.5,H=1`; epoch-two pending `G=1.5,H=0`;
  committed decay `0.25`; final `G=1.875,H=0.25`; loaded
  `W=0.133200133200133`, so probe target is `-W` within `1e-12`.
- Sensitivity: lambda one `0.24975024975025`; missing within-epoch decay
  `0.0999000999000999`; missing committed aging `0.333000333000333`.
- Assertions: one accept/zero reject per block, generations one then two, exact
  references, finite output and analytic probe equality.
- Worker validation: immediate GREEN; reported Release 44/0/1 and Debug
  45/0/0, plus diff check. Manager must independently verify these populated
  results before closing P2.
- Durable publish: local response-record commit succeeded; its first branch
  push failed with transient GitHub `SSL_ERROR_SYSCALL`. No remote state or
  code retry is claimed yet.

### MANAGER REVIEW W-TEST-CORE-016

- Scope/base: accepted. Exact `6c39f16db3` is clean on requested
  `12ab4e621` and adds one slot/test source only (87 lines).
- Recurrence: two separate interval-two blocks independently exercise within-
  epoch decay and cross-epoch committed aging; scalar relative-loading math and
  final `-0.133200133200133` probe target match the final review requirement.
- Sensitivity: `1e-12` absolute tolerance is far tighter than differences from
  lambda one, missing pending decay or missing committed aging. Generation/
  events, exact reference rows and finite output guard state/selection.
- Decision: accept for cherry-pick. Manager populated Release and Debug reports
  determine P2 closure; worker-reported counts are supporting evidence only.

### MANAGER VALIDATION W-TEST-CORE-016-GREEN

- Integrated commit: `1b542b18a`.
- Release: focused build succeeds; synchronized report has 44 passes, zero
  failures, one intentional Debug-only malloc-guard skip and exit code zero.
- Debug: focused build succeeds; synchronized report explicitly identifies
  Debug and has 45 passes, zero failures/skips and exit code zero, so the Eigen
  runtime guard remains effective.
- Oracle: expected/observed forgetting probe target is `-0.1332...`; one accept
  per training block, generations one/two, exact references and zero rejects
  all pass. Synthetic metrics remain 58.6541 dB/0.000113195 and chunk
  differences remain zero in both reports.
- Decision: close `R-CORE-FORGET-001`. Formal core gate passes with P0/P1 zero,
  no open P2 and two tracked P3s.

### RETIREMENT REQUEST CLEANUP-021

- Publish formal core review and analytic P2 GREEN evidence to issue #2.
- Verify detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-016` is clean at
  `6c39f16db3` and its focused test content matches integrated `1b542b18a`;
  remove through Git worktree management. One-shot agent is not reusable.
- Status: recorded before action.
- Final status: complete. Formal review/P2 evidence published at
  `https://github.com/Edddddddddy/mne-cpp/issues/2#issuecomment-5191259854`.
  Worktree was clean at exact `6c39f16db3`; worker/integration focused-test
  hashes both equal `471533e32d0824c57509f8c411db9a72e5d329c3`; verified Git
  worktree removal succeeded. One-shot agent will not be reused.

### REQUEST W-TEST-PLUGIN-DATA-001

- From / to: manager / first plugin-data TDD tracer worker.
- Planned execution: detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-plugin-data-001` from the exact
  integration HEAD recorded after request commit.
- Model/environment: `gpt-5.6-sol`, `high`; no nested subagents.
- Blocking: yes for `W-PLUGIN-DATA-001`.
- Task: add focused target `test_adaptive_denoising_plugin` plus exactly one
  behavioral public/private seam RED slot for the SPEC processor interface.
  Use six rows: good REF_MEG, bad REF_MEG, good MEG, bad MEG, STIM and misc,
  expressed with real FIFF constants and data-only descriptors. Configure one
  tap, interval/max block two, 30-second memory and `1e-8` regularization; require
  Ready counts 1/1/1. Train two samples where good target is `2*goodRef`, then
  ApplyOnly a future sample and require that good target residual is `<=1e-5`.
  At both stages require exact preservation of good/bad reference, bad MEG,
  STIM and misc rows, finite output, one accepted generation and zero rejects.
- Initial target boundary: compile test plus numerical source directly with Qt
  Core/Test and Eigen; include library/plugin roots, do not link mne_fiff,
  mne_rtprocessing or mne_scan. Initial RED should be missing
  `adaptivedenoising/adaptivedenoisingprocessor.h`; production source is not
  authorized in this task.
- Files: new test source/CMake and top-level testframe registration only. No
  production/plugin header/source, queue, UI, vendor/toolchain fix, dependency,
  benchmark/example or rt_server.
- Required response: `RESPONSE W-TEST-PLUGIN-DATA-001` with commit, exact test
  matrices/oracles, CMake dependency proof, RED evidence or honest infrastructure
  limit, changed files/clean status and no-subagent confirmation.
- Status: active as collaboration agent `/root/w_test_plugin_data_001`.
  Detached worktree is clean at exact `cdc07b283`; Sol/high accepted the
  test/CMake-only RED scope and nested-subagent prohibition.

### REQUEST W-TEST-PLUGIN-DATA-001-CORRECTION

- From / to: manager / `/root/w_test_plugin_data_001`.
- Blocking: yes; corrects a manager inconsistency before commit.
- Evidence: SPEC/UI requires adaptation interval 16-2048, but the initial
  tracer requested interval/max block two while the frozen processor says it
  validates UI ranges.
- Correction: use max block and interval 16. Training block is 6x16 with good
  reference `1..16`, good target exactly twice it, and distinct 16-sample
  sequences for bad reference, bad MEG, STIM and misc. Retain one future probe
  (`goodRef=17`, `goodTarget=34`) and the same Ready 1/1/1, one acceptance/
  generation, exact non-target and `<=1e-5` residual oracles.
- Other scope/CMake/dependency/RED/no-subagent rules are unchanged.
- Status: recorded before forwarding.

### RESPONSE W-TEST-PLUGIN-DATA-001-CORRECTION

- Worker confirmed the correction before commit: max block/interval 16,
  6x16 training and future good-ref/target probe 17/34.
- Original one-slot, isolated dependency, intended RED, no-production and
  no-subagent constraints remain accepted.

### SYSTEM FAILURE W-TEST-PLUGIN-DATA-001

- Transport used: internal collaboration subagent `/root/w_test_plugin_data_001`.
- Result: stream disconnected before completion; no RESPONSE, commit, staged
  change or worktree modification exists.
- Audit: detached worktree
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-plugin-data-001` is clean at
  exact `cdc07b283`.
- Decision: the request is superseded, not retried. More importantly, this
  transport is invalid for the user-required topology. The manager will not
  create further internal subagents.

### USER DIRECTION TOPOLOGY-001

- Manager: this visible conversation, Sol/ultra, no internal subagents.
- Planning order: discuss the work breakdown, create multiple linked GitHub
  issues, then issue minimal logged REQUESTs to newly created visible work
  conversations using Luna/max or Sol according to risk.
- Review: separate visible Sol/ultra conversations; reviewers do not fix.
- Coordination: worker completion/question is pushed back as RESPONSE; no
  continuous manager polling.
- Priority: engineering MVP and rapid end-to-end delivery first. Existing
  algorithm-quality results remain regression evidence, not a reason for more
  numerical tuning before user acceptance.
- Status: effective immediately; no new worker is active.

### RETIREMENT RESPONSE W-TEST-PLUGIN-DATA-001

- Pre-removal proof: registered detached worktree was clean at exact
  `cdc07b28363422abddc8c11f6131d57ca8ae1d3b`.
- Action: removed through `git worktree remove` using the verified absolute
  target `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-plugin-data-001`.
- Verification: the path no longer exists and `git worktree list` contains
  only the manager integration worktree.
- Recoverability: no code or uncommitted change was removed; the failed request
  and correction remain fully recorded in this ledger and Git history.

### REQUEST W-PROC-TEST-001

- From / to: manager thread
  `019fcdc3-4a1e-76d1-8140-1bd521219297` / new visible processor-test work
  conversation.
- GitHub issue: `Edddddddddy/mne-cpp#4`.
- Model/environment: `gpt-5.6-luna`, `max`, saved Git project worktree starting
  from `codex/causal-reference-denoising`; no internal or nested subagents.
- Blocking: yes for every processor production change.
- Task: add focused target `test_adaptive_denoising_plugin` and exactly one Qt
  public/private-seam behavioral RED slot. Use six descriptor rows: good
  REF_MEG, bad REF_MEG, good MEG, bad MEG, STIM and misc, expressed with real
  FIFF constants. Configure fs 1000 Hz, max block/update interval 16, one tap,
  30-second memory and `1e-8` regularization. Train a 6x16 block whose good
  reference is `1..16`, good target is twice it and every non-target row has a
  distinct sentinel sequence. Require Ready counts `1/1/1`, exactly one
  accepted generation, zero rejects and value-identical non-target rows. Then
  ApplyOnly probe good reference/target `17/34`; require finite output, exact
  non-target values and good-target residual `<=1e-5`.
- CMake seam: register the new testframe directory; compile the test plus the
  existing numerical `causalreferencedenoiser.cpp` directly; include library
  and plugin roots; link only Qt Core/Test and Eigen. Do not link mne_fiff,
  mne_rtprocessing, scShared, scMeas or mne_scan.
- Authorized files: `src/testframes/CMakeLists.txt` and new
  `src/testframes/test_adaptive_denoising_plugin/{CMakeLists.txt,
  test_adaptive_denoising_plugin.cpp}` only. Do not add processor production
  header/source, queue, plugin, UI, example, benchmark, dependency or vendor fix.
- Expected RED: missing
  `<adaptivedenoising/adaptivedenoisingprocessor.h>`. If clean-worktree ignored
  Eigen files block configuration first, report that honestly and do not copy,
  repair or stage dependencies; the manager owns populated-workspace RED.
- Required response: commit SHA/base, exact matrices/oracles, changed files,
  dependency proof, available RED evidence or infrastructure limitation, clean
  status and no-subagent confirmation. Begin `RESPONSE W-PROC-TEST-001`, then
  actively send it to the manager thread via `send_message_to_thread`; do not
  wait for or poll the manager.
- Status: request recorded before visible work conversation creation.

### SYSTEM FAILURE W-PROC-TEST-001-CREATE-1

- Result: the app rejected the first `create_thread` call as invalid arguments
  because `projectId` was supplied at the top level rather than inside the
  project target object.
- Side effects: none; no thread, worktree, branch or code change was created.
- Decision: preserve the same single request and retry once with the declared
  `target: {type: project, projectId, environment}` schema. This is not a
  duplicate task dispatch.

### RESPONSE W-PROC-TEST-001-CREATE

- Result: corrected visible-thread creation was accepted.
- Client setup ID: `client-new-thread:6b3b6649-8bc8-4b37-ba95-1cc2e6e25068`.
- Host/project: local saved Git project `mne-cpp-main`, isolated app-managed
  worktree starting from branch `codex/causal-reference-denoising` at exact
  manager HEAD `eef0621925c4c08f2c8e942b110ab636887e9b71`.
- Model: `gpt-5.6-luna`, `max`.
- Notification contract: initial prompt includes manager thread
  `019fcdc3-4a1e-76d1-8140-1bd521219297` and requires worker-initiated final
  `send_message_to_thread`; no manager polling or internal subagent is used.
- Status: setup accepted asynchronously; final thread ID was not returned and
  will not be polled. Await only the worker-pushed RESPONSE.
- GitHub dispatch evidence:
  `https://github.com/Edddddddddy/mne-cpp/issues/4#issuecomment-5192068833`.

### RESPONSE W-PROC-TEST-001

- Visible thread: `019fd204-2ae1-7153-bc52-d33d952b5598`.
- Base/commit: exact parent
  `eef0621925c4c08f2c8e942b110ab636887e9b71`; returned commit
  `095633f159397130b13baffc4450b5d0cf87aa1d`.
- Scope: exactly the top testframe registration plus new focused test CMake and
  one Qt slot `mapsRowsTrainsAndAppliesOnly`; no production file.
- Train oracle: six rows, 16 samples, good reference `1..16`, good target twice
  it, distinct bad-reference/bad-MEG/STIM/misc sentinels; Ready counts `1/1/1`,
  Processed, generation/accepted/rejected `1/1/0`, exact non-target rows.
- Probe oracle: a 6x16 ApplyOnly block with good reference/target `17/34`,
  distinct non-target sentinels, all-finite output, exact non-target rows and
  every good-target absolute residual `<=1e-5`.
- Dependency evidence: focused target compiles the existing numerical source
  directly, includes library/plugin roots and links only Qt Core/Test + Eigen;
  no mne_fiff/mne_rtprocessing/scShared/scMeas/mne_scan.
- RED evidence: direct MSVC compilation reaches expected C1083 missing
  `adaptivedenoising/adaptivedenoisingprocessor.h`. Clean-worktree CMake first
  encounters the known ignored Eigen `Macros.h`/`buildtests.in` baseline; no
  dependency repair/copy/stage or rt_server run occurred.
- Integrity: only three authorized files changed; cached and commit diff checks
  passed; worktree is clean/detached; no subagents and no manager polling.
- Status: response recorded before manager inspects the worker diff.

### MANAGER REVIEW W-PROC-TEST-001

- Provenance: exact clean worker HEAD `095633f159` has parent `eef062192`; diff
  contains only the three authorized test/CMake files and passes diff check.
- Interface behavior: one Qt slot crosses the frozen processor seam and checks
  Ready row/feature counts, actual numerical learning/application diagnostics,
  exact preservation of five non-target rows and finite near-zero target output.
- Oracle sensitivity: with one scalar feature and relative regularization
  `1e-8`, the learned coefficient is approximately `2/(1+1e-8)`; a raw target
  of 34 leaves residual about `3.4e-7`, safely inside `1e-5`. Missing/wrong row
  selection, bad/STIM leakage or an uncommitted model produces a material fail.
- Dependency/static review: CMake mirrors the existing isolated core-test
  pattern, adds only the plugin include root and does not link any forbidden
  MNE/FIFF/scan library. Real FIFF constants are header-only in this target.
- Finding: none. Scope and TDD tracer quality accepted.
- Decision: record/push this review, cherry-pick the worker commit, then the
  populated manager build must produce the intended missing-processor-header
  RED before any production implementation request.

### MANAGER VALIDATION W-PROC-TEST-001-RED

- Integrated test commit: `daae515ed` (cherry-pick of exact worker
  `095633f159`).
- Configure: `cmake -S src -B build-causal-reference-denoising` succeeded and
  generated the new focused target without mne_fiff/mne_scan linkage.
- RED command: `cmake --build build-causal-reference-denoising --target
  test_adaptive_denoising_plugin --config Release -- /m:2`.
- Result: exit code 1 at the intended first test-source failure, MSVC C1083 on
  missing `adaptivedenoising/adaptivedenoisingprocessor.h` at line 11. The
  numerical source compiled; only known Eigen/MSVC C4819 code-page warnings
  accompanied the failure.
- Decision: valid behavioral RED accepted. A production worker may implement
  only the frozen processor seam required to make this tracer GREEN.

### RETIREMENT REQUEST W-PROC-TEST-001

- Verify visible worker worktree is clean at exact `095633f159` and the three
  test/CMake file contents match integrated `daae515ed`.
- Publish RED evidence to issue #4, then archive thread
  `019fd204-2ae1-7153-bc52-d33d952b5598` through the app. Do not manually
  delete its app-owned worktree.
- Status: recorded before publication/archival.

### RETIREMENT PRECHECK W-PROC-TEST-001

- GitHub RED evidence:
  `https://github.com/Edddddddddy/mne-cpp/issues/4#issuecomment-5193087026`;
  read-back matches worker/integration commits, exact command/error and
  dependency isolation.
- Worker worktree remains clean/detached at exact `095633f159`.
- Content identity: worker and integration hashes match for all three files:
  top CMake `284eb5a3...`, focused CMake `f52cf0cd...`, test source
  `efca6b30...`.
- Decision: thread `019fd204-2ae1-7153-bc52-d33d952b5598` is a completed
  one-shot role and may now be archived through the app. Its app-owned worktree
  is not manually removed.

### RETIREMENT RESPONSE W-PROC-TEST-001

- App result: visible thread `019fd204-2ae1-7153-bc52-d33d952b5598` archived
  successfully.
- Worktree policy: the app-owned path was not manually deleted or modified.
- Durable evidence: response, exact review, integrated RED, content hashes and
  issue #4 comment all precede archival.
- Status: complete; this one-shot conversation will not be reused.

### REQUEST W-PROC-GREEN-001

- From / to: manager thread
  `019fcdc3-4a1e-76d1-8140-1bd521219297` / new visible processor implementation
  work conversation.
- GitHub issue: `Edddddddddy/mne-cpp#4`.
- Model/environment: `gpt-5.6-luna`, `max`, saved Git project worktree from the
  current integration branch; no internal/nested subagents.
- Blocking: yes for further processor tests and all queue/plugin work.
- Task: make only the existing `mapsRowsTrainsAndAppliesOnly` tracer GREEN by
  adding the frozen concrete plugin-private `AdaptiveDenoisingProcessor` header
  and source and adding that source to the focused test target.
- Required implementation: expose the exact SPEC/test data-only descriptors,
  settings defaults, configure statuses/counts and `configure/process/reset/
  configuration` methods in namespace `ADAPTIVEDENOISINGPLUGIN`; select good
  REF_MEG/MEG rows by FIFF kind and bad flag; configure/reset/delegate to
  `CausalReferenceDenoiser`; return fixed NotConfigured/NaN diagnostics and
  preserve the whole block while no numerical model is armed. Configuration is
  worker-thread only; process remains `noexcept`, allocation/string/lock-free.
- Configuration safety: construct a candidate numerical model and commit it only
  on Ready; every invalid/missing attempt disarms the old model so stale weights
  cannot be applied. Implement the fixed status classification needed by the
  method, but add no new public seam, algorithm registry or speculative plugin
  behavior.
- Authorized files: new
  `src/applications/mne_scan/plugins/adaptivedenoising/
  adaptivedenoisingprocessor.{h,cpp}` and the focused test CMake only. No test
  source change, queue, plugin class, UI, global CMake registration, example,
  benchmark, core change, vendor/dependency repair or rt_server.
- Verification: build/run `test_adaptive_denoising_plugin` if the worktree
  dependencies permit; otherwise report the known clean-worktree Eigen baseline
  and manager will validate in the populated workspace. Run diff checks and
  commit only authorized files.
- Required response: begin `RESPONSE W-PROC-GREEN-001`; include base/commit,
  interface/ownership and disarm semantics, changed files, build/test evidence
  or honest infrastructure limit, remaining untested behavior, clean status and
  no-subagent confirmation. Actively send it to the manager thread using
  `send_message_to_thread`, then stop without polling.
- Status: request recorded before visible work conversation creation.

### RESPONSE W-PROC-GREEN-001-CREATE

- Result: visible saved-project worktree creation accepted asynchronously.
- Client setup ID: `client-new-thread:aa50ed99-db64-4f34-addb-ea3b71e91ba2`.
- Model/host: `gpt-5.6-luna`, `max`, local.
- Exact branch base at dispatch:
  `9a99e80c39d731cbc4f9ed552df9632c754c923d`.
- Prompt integrity: includes exact three-file authorization, RED contract,
  candidate/disarm/pass-through safety, recovery/skill reads, no-subagent and
  no-rt_server rules, plus manager thread for active RESPONSE notification.
- Status: final thread ID is setup-owned and will not be polled. Await only the
  worker-pushed `RESPONSE W-PROC-GREEN-001`.
- GitHub dispatch evidence:
  `https://github.com/Edddddddddy/mne-cpp/issues/4#issuecomment-5193135833`.

### RESPONSE W-PROC-GREEN-001

- Visible thread: `019fd258-04db-7982-b1f6-c7010727c03c`.
- Base/commit: exact parent
  `9a99e80c39d731cbc4f9ed552df9632c754c923d`; returned commit
  `46b600f910a7638791f6bc6b5dccb9af6813a933`.
- Interface: adds the frozen data-only descriptors/settings defaults, configure
  statuses/result and final concrete processor in namespace
  `ADAPTIVEDENOISINGPLUGIN`.
- Ownership/safety: configure validates metadata/UI ranges, selects good
  REF_MEG/MEG rows, configures a candidate numerical model and swaps only after
  success. Every invalid/missing/core-rejected attempt clears old ownership and
  publishes zero counts, preventing stale-layout weights.
- Process: armed calls delegate directly; unarmed calls preserve the complete
  block and report NotConfigured with zero snapshot/events and quiet-NaN RMS.
  Reset delegates only while armed; configuration returns the fixed snapshot.
- Scope: exactly processor header/source plus focused test CMake; no test/core/
  queue/plugin/UI/example/benchmark/dependency/rt_server change.
- Worker verification: commit diff check and clean status pass. Runtime focused
  GREEN was not claimed because the clean worktree retains the known missing
  ignored Eigen `Macros.h`/`buildtests.in` baseline; no dependency repair/copy.
- Realtime claim: adapter process adds no allocation, lock, string, Qt/FiffInfo,
  registry or strategy seam beyond numerical delegation.
- Model/process policy: Luna/max, no subagents, no manager polling, no
  mne_rt_server.
- Status: response recorded before manager inspects the worker diff.

### MANAGER REVIEW W-PROC-GREEN-001

- Provenance/scope: exact clean worker HEAD `46b600f910` has requested parent
  `9a99e80c3`; diff is only processor header/source plus focused test CMake and
  passes diff check.
- Deep-module seam: concrete processor exposes only fixed descriptor/settings/
  result data and configure/process/reset/configuration. No Qt/FiffInfo,
  registry, strategy or caller-side numerical state leaks into the header.
- Mapping/ownership: source selects only non-bad REF_MEG and MEG rows, builds
  Eigen row vectors, configures an isolated candidate and swaps ownership only
  after `DenoiserStatus::Configured`.
- Failure safety: every validation/missing/core-reject path destroys prior
  ownership and zeroes the adapter snapshot. Unarmed process is write-free and
  returns the exact numerical NotConfigured shape with quiet-NaN RMS.
- Hot path: processor process performs only a unique_ptr branch and numerical
  delegation or fixed aggregate construction; no allocation, lock or string.
- Finding: none for the authorized GREEN slice. Invalid classification and
  stale-model disarm remain behaviorally untested and are the next tracer, not
  a reason to widen this implementation commit.
- Decision: accept for cherry-pick and populated Release build/run; runtime
  GREEN is manager-owned and not yet claimed.

### MANAGER VALIDATION W-PROC-GREEN-001-GREEN

- Integrated production commit: `a602bdff4` (cherry-pick of exact worker
  `46b600f910`).
- Configure/build: regenerated `build-causal-reference-denoising` and built
  Release target `test_adaptive_denoising_plugin`; processor, numerical source,
  test and moc compiled and linked successfully. Only known Eigen/MSVC C4819
  code-page warnings occurred.
- Run: `out/Release/apps/test_adaptive_denoising_plugin.exe -txt` returned
  process exit code zero.
- Behavioral result: Ready counts, good/bad FIFF-kind selection, one accepted
  generation, numerical ApplyOnly denoising, finite output and exact five-row
  preservation all pass through the frozen processor interface.
- Decision: first processor tracer is GREEN. Next RED must cover invalid/
  missing configure classification and prove old model ownership is disarmed.

### RETIREMENT REQUEST W-PROC-GREEN-001

- Publish the focused GREEN evidence to issue #4.
- Verify visible worker worktree is clean at exact `46b600f910` and all three
  authorized file contents match integrated `a602bdff4`, then archive thread
  `019fd258-04db-7982-b1f6-c7010727c03c` through the app.
- Do not manually delete the app-owned worktree.
- Status: recorded before publication/archival.

### RETIREMENT PRECHECK W-PROC-GREEN-001

- GitHub GREEN evidence:
  `https://github.com/Edddddddddy/mne-cpp/issues/4#issuecomment-5193285229`;
  read-back matches commits, focused exit-zero evidence and next tracer.
- Visible worker worktree remains clean/detached at exact `46b600f910`.
- Content identity: worker/integration hashes match for processor header
  `5f91b46b...`, processor source `1ea6b059...` and focused CMake
  `29228c07...`.
- Decision: completed one-shot thread
  `019fd258-04db-7982-b1f6-c7010727c03c` may be archived through the app; its
  app-owned worktree remains untouched by the manager.

### RETIREMENT RESPONSE W-PROC-GREEN-001

- App result: visible thread `019fd258-04db-7982-b1f6-c7010727c03c` archived
  successfully after all evidence and identity checks were durable.
- App-owned worktree was not manually deleted or modified.
- Status: complete; this implementation conversation will not be reused.

### REQUEST W-PROC-TEST-002

- From / to: manager thread
  `019fcdc3-4a1e-76d1-8140-1bd521219297` / new visible reusable processor-test
  conversation.
- GitHub issue: `Edddddddddy/mne-cpp#4`.
- Model/environment: `gpt-5.6-luna`, `max`, saved Git project worktree from the
  current integration branch; no internal/nested subagents.
- Blocking: yes for processor milestone review.
- Behavior: one public-interface test slot proves every invalid/missing
  configure attempt disarms a previously learned model. For each case, create a
  fresh processor, Ready-configure/train one ref and one target through one
  accepted interval, submit the invalid candidate, require the exact adapter
  status and zero returned/configuration counts, then ApplyOnly a finite probe
  and require NotConfigured, complete matrix identity, zero diagnostic snapshot/
  events and NaN RMS.
- Required cases in the one behavior: invalid metadata (nonfinite/nonpositive
  fs, empty channels, nonpositive max block); MissingReferences; MissingTargets;
  settings outside every UI bound/nonfinite (taps, interval, memory,
  regularization); and a UI-valid reference-count/tap combination exceeding the
  core 256-feature cap, classified InvalidSettings.
- TDD classification: the production implementation already expresses this
  approved contract, so honest immediate GREEN is expected. Do not fabricate a
  RED or edit production when the public behavior passes.
- Authorized file: focused test source only. Add no CMake/processor/core/queue/
  plugin/UI/example/benchmark/dependency change and do not run rt_server.
- Verification: build/run focused Release if dependencies permit, otherwise
  report the known ignored Eigen baseline; manager owns populated execution.
  Diff check and commit the single authorized file.
- Reuse policy: keep this conversation/worktree after response because the same
  processor-test responsibility will receive a later separate valid-
  reconfigure-reset tracer if this behavior is accepted.
- Required response: begin `RESPONSE W-PROC-TEST-002`; include exact base/
  commit, case table, priming/disarm/probe oracle, evidence/classification,
  changed file, clean status and no-subagent confirmation. Actively send to the
  manager thread and stop without polling.
- Status: request recorded before visible work conversation creation.

### RESPONSE W-PROC-TEST-002-CREATE

- Result: visible saved-project worktree creation accepted asynchronously.
- Client setup ID: `client-new-thread:9b5ed006-e485-4b72-94f5-c37db3fb1d76`.
- Model/host: `gpt-5.6-luna`, `max`, local; satisfies the user hard model rule.
- Exact branch base: `a868914ca6a41cba17762c35bfb602e36feb5f5e`.
- Prompt integrity: one test-source behavior, complete invalid/missing/feature-
  cap cases, prime/disarm/probe oracle, immediate-GREEN classification, no
  production/dependency/rt_server/subagent and manager active-notification rule.
- Reuse: keep the conversation after this response for the separate valid-
  reconfigure-reset tracer if clean and relevant.
- Status: final thread ID is setup-owned and will not be polled. Await only the
  worker-pushed `RESPONSE W-PROC-TEST-002`.
- GitHub dispatch evidence:
  `https://github.com/Edddddddddy/mne-cpp/issues/4#issuecomment-5193332420`.

### RESPONSE W-PROC-TEST-002

- Visible reusable thread: `019fd266-902d-77e1-b40a-a754eaac6222`.
- Base/commit: exact parent
  `a868914ca6a41cba17762c35bfb602e36feb5f5e`; returned commit
  `0d8aeafec1471e7d38323ba308f88def8375e294`.
- Scope: focused test source only; preserves the original mapping/training slot
  and adds one public invalid/missing-disarm behavior.
- Cases/status: four InvalidMetadata (zero/NaN fs, empty channels, zero max
  block); two MissingReferences; two MissingTargets; ten InvalidSettings across
  tap/interval/memory/regularization low/high/nonfinite bounds; and UI-valid
  nine-reference/tap-32 feature cap `P=288` mapped to InvalidSettings.
- Prime oracle: every case uses a fresh Ready 2-row processor and one 16-sample
  ApplyAndLearn epoch with generation/accepted/rejected `1/1/0`.
- Disarm/probe oracle: invalid configure and configuration snapshot have exact
  status and zero counts; subsequent finite ApplyOnly is NotConfigured, keeps
  the complete 2x16 matrix exact, reports zero snapshot/events and NaN RMS.
- Worker evidence: single-file diff/clean checks pass. Focused runtime is not
  claimed because the reusable app worktree retains the known ignored Eigen
  baseline gap; no dependency repair/copy/stage, rt_server, subagent or manager
  polling occurred.
- TDD classification: expected immediate GREEN acceptance of an already
  implemented contract; no artificial RED or production edit.
- Status: response recorded before manager inspects the exact diff. Conversation
  remains unarchived for planned reuse after acceptance.

### MANAGER REVIEW W-PROC-TEST-002

- Provenance/scope: exact clean `0d8aeafec1` has requested parent `a868914ca`,
  changes only the focused test source and covers the reported 19 cases.
- Prime/disarm/probe structure is strong and observes stale ownership entirely
  through the public processor interface.
- P2 `R-PROC-TEST-MEMORY-001`: at test source line 320, memory lower-bound case
  uses `0.0`. This proves only the numerical/core positive-memory invariant and
  would pass an incorrect adapter accepting `0 < memory < 1`, so it does not
  protect the specified UI minimum `1.0`.
- Required correction: replace the lower-bound value/name with `0.5` and keep
  expected `InvalidSettings`; no other case, production or scope change.
- Decision: hold integration for an amended one-file commit with the same exact
  parent. Reuse the active Luna/max conversation; do not create a replacement.

### REQUEST W-PROC-TEST-002-REVISE

- From / to: manager / reusable visible thread
  `019fd266-902d-77e1-b40a-a754eaac6222`.
- Blocking: yes for integration.
- Task: in the single authorized test source, change the lower memory case from
  `0.0`/`memory-zero` to `0.5`/a below-min name. Keep all other 18 cases and
  prime/disarm/probe assertions identical.
- Commit shape: amend `0d8aeafec1` so the replacement commit still has exact
  parent `a868914ca`; run diff checks and available focused evidence.
- Required response: `RESPONSE W-PROC-TEST-002-REVISE` with replacement SHA,
  line/value/name evidence, unchanged scope, clean status and proactive manager
  notification. No production/dependency/rt_server/subagent/polling.
- Status: forwarded successfully to the same visible Luna/max thread; no model
  override, new conversation or polling.

### RESPONSE W-PROC-TEST-002-REVISE

- Reusable thread: `019fd266-902d-77e1-b40a-a754eaac6222`, still Luna/max.
- Replacement commit: `aa75e2520b659c922c5db61de743b1ff27cec006`,
  exact parent `a868914ca6a41cba17762c35bfb602e36feb5f5e`; prior
  `0d8aeafec1` was amended rather than stacked.
- Exact correction: focused test source line 320 changes memory from `0.0` to
  `0.5`; line 322 renames the case from `memory-zero` to
  `memory-below-min`; expected InvalidSettings remains.
- Unchanged: all other 18 cases, prime/disarm/probe oracle, original mapping
  tracer and every non-test file.
- Evidence: replacement and old-to-new diff checks pass; old-to-new delta is
  exactly two requested lines; commit is one authorized test source and the
  worktree is clean/detached.
- Runtime: still unavailable only because of the known ignored Eigen worktree
  baseline; no dependency repair/copy, rt_server, subagent or manager polling.
- Reuse: conversation/worktree remains available for the following valid-
  reconfigure-reset tracer.
- Status: response recorded before manager inspects the replacement commit.

### MANAGER REVIEW W-PROC-TEST-002-REVISE

- Provenance: exact clean replacement `aa75e2520b` has requested parent
  `a868914ca` and one authorized test-source diff.
- Old-to-new audit: compared directly with rejected `0d8aeafec1`; the only
  changes are memory `0.0 -> 0.5` and case name `memory-zero ->
  memory-below-min`.
- Sensitivity: `0.5` is finite/positive, so it passes the numerical core's
  weaker positivity invariant but violates the adapter UI minimum `1.0`.
  Therefore the test now fails if adapter lower-bound enforcement is removed or
  weakened to `>0`.
- Finding resolution: close P2 `R-PROC-TEST-MEMORY-001`. No production change
  or other test revision is required.
- Decision: accept replacement for cherry-pick and populated focused execution;
  keep the reusable conversation active for the later reset tracer.

### MANAGER VALIDATION W-PROC-TEST-002-GREEN

- Integrated test commit: `6a721991c` (cherry-pick of amended
  `aa75e2520b`).
- Build: focused Release target recompiled and linked successfully; only known
  Eigen/MSVC C4819 code-page warnings occurred.
- Run: `out/Release/apps/test_adaptive_denoising_plugin.exe -txt` returned
  process exit code zero.
- Behavioral result: original mapping/train/probe tracer and all 19 fresh-
  processor prime/invalid-disarm/NotConfigured-probe cases pass. Coverage
  includes adapter-specific memory `0.5` lower-bound sensitivity and the
  `P=288` numerical feature-cap rejection.
- Review state: P2 `R-PROC-TEST-MEMORY-001` is closed; no production fix was
  required.
- Lifecycle: keep reusable thread
  `019fd266-902d-77e1-b40a-a754eaac6222` active for a separate valid-
  reconfigure-reset test request; do not archive or recreate it yet.

### REQUEST W-PROC-TEST-003

- From / to: manager / existing reusable visible Luna/max processor-test thread
  `019fd266-902d-77e1-b40a-a754eaac6222`.
- GitHub issue: `Edddddddddy/mne-cpp#4`; blocking processor milestone review.
- Behavior: add one public-interface slot proving a Ready valid reconfiguration
  to a changed row layout resets the old learned model and can relearn the new
  mapping.
- Oracle: train initial two-row ref0/target1 model `target=2*ref` through one
  accepted generation. Ready-reconfigure to three rows `{misc, good REF_MEG,
  good MEG}` with the same valid settings; require Ready counts `1/1/1` and
  configuration snapshot. Immediately ApplyOnly a 3x16 block with distinct misc
  sentinel, ref 17 and target 34; require Processed, generation/events zero and
  complete matrix identity, proving old weights/reset state do not cross layout.
  Then ApplyAndLearn one interval with new target `3*ref`, require generation/
  accepted/rejected `1/1/0` and exact misc/reference rows; a future ApplyOnly
  probe ref/target `17/51` must leave misc/reference exact and target residual
  `<=1e-5`.
- TDD classification: acceptance of existing configured-reset contract;
  immediate GREEN expected. Do not manufacture RED or edit production.
- Authorized file: focused test source only. Preserve both existing slots; no
  CMake/processor/core/queue/plugin/UI/example/dependency/rt_server.
- Commit: add a new delta commit on top of reusable worker HEAD `aa75e2520b`;
  do not amend the accepted disarm commit. Run diff/clean checks and available
  focused evidence.
- Required response: `RESPONSE W-PROC-TEST-003` with parent/delta SHA, exact
  matrices/layout/generation oracle, evidence, changed file and no-subagent/
  no-polling confirmation; proactively notify manager then stop.
- Lifecycle: after acceptance this processor-test responsibility is complete;
  manager will verify and archive the conversation before formal review.
- Status: forwarded successfully to the existing thread with no model override;
  it remains Luna/max. No new conversation or polling.

### RESPONSE W-PROC-TEST-003

- Reusable Luna/max thread: `019fd266-902d-77e1-b40a-a754eaac6222`.
- Delta parent/commit: accepted worker commit `aa75e2520b`; new delta
  `2cbca78eb56b9d50b6d8dcec649e7600b8470350`.
- Scope: one new slot `validReconfigureResetsAndRelearnsModel` in the focused
  test source; two existing behaviors and all 19 invalid cases are unchanged.
- Oracle: train two-row `target=2*ref` generation one; Ready-reconfigure same
  processor to `{MISC, REF_MEG, MEG}`; immediate ApplyOnly 3x16 probe is complete
  matrix identity with generation/events zero; learn new `target=3*ref` mapping
  to generation/accepted/rejected `1/1/0`; future `17/51` probe is finite, keeps
  misc/reference exact and target residual `<=1e-5`.
- Evidence: one authorized test-source commit, diff/clean checks pass. Runtime
  not claimed because the app worktree retains the known ignored Eigen gap; no
  dependency repair/copy, production/CMake/core change, rt_server, subagent or
  manager polling.
- TDD classification: expected immediate-GREEN public-interface acceptance of
  existing reconfigure/reset behavior.
- Status: response recorded before manager exact delta review. Conversation is
  retained until acceptance/archival.

### MANAGER REVIEW W-PROC-TEST-003

- Provenance/scope: exact clean delta `2cbca78eb5` has required parent
  `aa75e2520b`, changes only the focused test source and passes diff check.
- Behavior: one 98-line slot observes valid reconfigure solely through the
  public processor interface: changed layout, configuration snapshot, zero-
  generation whole-block reset probe, new mapping acceptance and future
  residual.
- Sensitivity: stale old row picks/weights modify the reset probe or wrong rows;
  failure to reset generation violates zero; failure to relearn generation one
  or the `3*ref` model leaves a material 51-amplitude future residual.
- Locality: misc/reference row identity verifies only the selected target may
  change after both reset and relearning. Existing two behaviors remain intact.
- Finding: none. Expected immediate GREEN acceptance test is scoped and
  behavior-sensitive.
- Decision: accept for cherry-pick and populated Release execution. After GREEN
  archive the completed reusable tester and start independent Sol/ultra
  processor milestone review.

### MANAGER VALIDATION W-PROC-TEST-003-GREEN

- Integrated delta: `d977e98ed` (cherry-pick of `2cbca78eb5`).
- Build/run: focused Release target recompiles/links successfully and
  `test_adaptive_denoising_plugin.exe -txt` returns exit zero; only known
  Eigen/MSVC C4819 warnings occur.
- Behavioral result: changed three-row Ready reconfigure resets old model and
  generation, immediate ApplyOnly block is complete matrix identity, new
  `target=3*ref` generation is accepted and future residual/row-preservation
  gates pass. Original mapping and 19 disarm cases remain GREEN in the same run.
- Decision: issue #4 implementation/test behavior is ready for independent
  Sol/ultra milestone review after worker retirement.

### RETIREMENT REQUEST W-PROC-TEST-COVERAGE

- Publish the final coverage GREEN to issue #4.
- Verify reusable worker worktree is clean at exact `2cbca78eb5` and the focused
  test source equals integrated `d977e98ed`; then archive thread
  `019fd266-902d-77e1-b40a-a754eaac6222` through the app.
- Do not manually delete its app-owned worktree.
- Status: recorded before publication/archival.

### RETIREMENT PRECHECK W-PROC-TEST-COVERAGE

- GitHub final coverage evidence:
  `https://github.com/Edddddddddy/mne-cpp/issues/4#issuecomment-5193676575`;
  read-back matches delta/integration commits, reset/relearn oracle and formal-
  review readiness.
- Reusable worker worktree is clean/detached at exact `2cbca78eb5`.
- Focused test source hash is identical between worker and integration:
  `8d41bbefcd6f64431eb822aa9466fbaa21143023`.
- Decision: thread `019fd266-902d-77e1-b40a-a754eaac6222` completed its reusable
  processor-test responsibility and may be archived. App-owned worktree remains
  manager-untouched.

### RETIREMENT RESPONSE W-PROC-TEST-COVERAGE

- App result: reusable Luna/max thread
  `019fd266-902d-77e1-b40a-a754eaac6222` archived successfully.
- Worktree policy: the app-owned worktree was not manually deleted or modified.
- Durable evidence: all worker responses, exact reviews, integrated GREEN runs,
  source identity and issue #4 publications precede archival.
- Status: complete; this processor-test conversation will not be reused.

### REQUEST R-PROC-001

- From / to: manager thread
  `019fcdc3-4a1e-76d1-8140-1bd521219297` / new visible processor milestone
  review conversation.
- GitHub issue: `Edddddddddy/mne-cpp#4`.
- Model/environment: `gpt-5.6-sol`, `ultra`, saved Git project worktree from
  `codex/causal-reference-denoising`; no internal or nested subagents.
- Blocking: yes for closing issue #4 and beginning queue implementation.
- Review snapshot: exact integration HEAD after this REQUEST commit; confirm
  the SHA and keep the review worktree clean/read-only.
- Review scope: processor header/source, focused test CMake/source, the frozen
  processor interface and issue #4 evidence. Inspect the concrete deep-module
  seam, interface depth/locality, C++14 ownership/noexcept traits, good
  REF_MEG/MEG mapping, bad/STIM/misc preservation, metadata/settings status
  classification, invalid/missing stale-model disarm, valid reconfigure reset
  and relearn, unarmed diagnostics/pass-through, configure/process allocation
  and exception behavior, and focused dependency isolation.
- Evidence audit: distinguish code proof from tests and durable manager runs;
  confirm the populated Release executable exits zero for mapping/train/probe,
  all 19 invalid/missing disarm cases, and changed-layout reset/relearn. Do not
  run full `mne_scan` or `mne_rt_server`.
- Finding format: P0-P3 with stable ID, exact file/line, evidence, impact,
  requested correction and required test. Explicitly state zero findings at
  each empty priority. P0/P1 must be zero; every P2 must be fixed or explicitly
  deferred before issue #4 closes.
- Mutation policy: read-only; no edit, stage, commit, push, GitHub comment,
  issue change, dependency/vendor repair or subagent.
- Required response: begin `RESPONSE R-PROC-001`; include reviewed SHA,
  inspections/evidence, prioritized findings, residual risks, gate
  recommendation, clean status and no-edit/no-subagent confirmation. Actively
  send it to the manager thread using `send_message_to_thread`, then stop
  without polling the manager.
- Status: recorded before visible review conversation creation.

### RESPONSE R-PROC-001-CREATE

- Result: visible saved-project worktree creation accepted asynchronously.
- Client setup ID:
  `client-new-thread:fade23aa-de07-4aff-964c-e41e113aa385`.
- Host/model: local, `gpt-5.6-sol`, `ultra`.
- Exact requested review snapshot:
  `96ca3eb44ab448aefc1cfa0048064e926df6340a`.
- Prompt integrity: read-only exact-SHA audit includes processor depth/locality,
  mapping, disarm/reset, real-time/exception properties, dependency isolation,
  focused evidence, P0-P3 format, no edits/subagents/rt_server and proactive
  manager RESPONSE notification.
- Coordination: setup did not return a final thread ID. The manager will not
  poll setup or review; it awaits only the reviewer-pushed RESPONSE.

### PUBLISH R-PROC-001-START

- GitHub issue #4 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/4#issuecomment-5193766737`.
- Read-back: exact snapshot `96ca3eb44`, Sol/ultra read-only scope, P0-P3
  format, no-mutation/no-rt_server constraints and proactive RESPONSE contract
  all match the durable request.
- Status: review dispatch is public and complete; issue #4 remains open while
  the manager awaits only the reviewer-pushed result.

### RESPONSE R-PROC-001

- Visible reviewer thread: `019fd288-1f93-7df1-8e55-78bc930ad73a`;
  `gpt-5.6-sol`, `ultra`, read-only.
- Reviewed exact snapshot:
  `96ca3eb44ab448aefc1cfa0048064e926df6340a`; initial/final worktree status
  clean and detached.
- Inspection: full processor header/source, focused CMake/test, relevant
  numerical ownership/delegation, frozen SPEC, worker provenance and all three
  issue #4 GREEN comments. No independent build was claimed because the clean
  review worktree has the known ignored Eigen gap.
- Architecture result: concrete data-only module is suitably deep; the small
  interface centralizes mapping, classification, disarm/reset, diagnostics and
  numerical delegation with no Qt/FiffInfo/strategy/registry leak. Mapping,
  invalid disarm, valid reset/relearn and armed/unarmed hot paths match SPEC.
- Priorities: P0 zero; P1 zero; one blocking P2 `R-PROC-MOVE-001`; two
  nonblocking P3s `R-PROC-LOCALITY-001` and `R-PROC-BOUNDARY-001`.
- P2 evidence: implicit C++14 move transfers `m_denoiser` but copies the Ready
  scalar snapshot, so a moved-from processor can report Ready while process is
  NotConfigured pass-through. Requested policy is explicit deletion of copy and
  move, or a custom consistent move; add C++14 type-trait verification and run
  the complete focused Release executable.
- P3 summary: compact header-local contract/exception documentation and exact
  accepted upper-bound/P=256 tests are missing; current source behavior is
  correct and these do not block the processor gate.
- Gate: HOLD issue #4 until P2 is fixed and focused Release rerun. Do not reopen
  numerical algorithm review.
- Integrity verification by manager: app worktree
  `C:/Users/lcy/.codex/worktrees/f01a/mne-cpp-main` is clean at exact reviewed
  SHA and all four scoped file hashes equal integration content. No reviewer
  edit, stage, commit, push, GitHub mutation, dependency repair, subagent,
  full mne_scan or rt_server run occurred.

### RETIREMENT RESPONSE R-PROC-001

- App result: one-shot Sol/ultra reviewer thread
  `019fd288-1f93-7df1-8e55-78bc930ad73a` archived successfully after its full
  RESPONSE and independent worktree/content verification were durable.
- Worktree policy: app-owned path
  `C:/Users/lcy/.codex/worktrees/f01a/mne-cpp-main` was not manually deleted or
  modified by the manager.
- Status: complete; findings remain in `REVIEW.md` and this conversation will
  not be reused for implementation.

### REQUEST W-PROC-MOVE-001

- From / to: manager thread
  `019fcdc3-4a1e-76d1-8140-1bd521219297` / new visible processor ownership-fix
  work conversation.
- GitHub issue/finding: `Edddddddddy/mne-cpp#4`, P2
  `R-PROC-MOVE-001`.
- Model/environment: `gpt-5.6-luna`, `max`, new saved-project worktree from
  `codex/causal-reference-denoising`; no internal/nested subagents.
- Blocking: yes for processor issue #4 closure and queue work.
- Selected interface policy: `AdaptiveDenoisingProcessor` is a worker-owned
  single-stream module. Add an explicit noexcept default constructor and
  explicitly delete copy construction/assignment and move construction/
  assignment. Do not add custom move support or another ownership interface.
- TDD verification: in the focused public test source add `<type_traits>` and
  namespace-scope C++14 static assertions proving nothrow default construction,
  noncopyability and nonmovability. If available, observe that the pre-fix
  move-trait assertion fails before applying the header change; otherwise
  report the known clean-worktree dependency limitation honestly.
- Authorized files: processor header and focused processor test source only.
  Preserve all three runtime slots and every production behavior. No source/
  CMake/core/queue/plugin/UI/example/dependency/vendor or rt_server change.
- Required validation: diff checks, exact two-file scope, available focused
  Release build/run or known ignored-Eigen limitation, and clean commit.
- Required response: begin `RESPONSE W-PROC-MOVE-001`; include exact base/
  commit, declarations and trait assertions, RED/GREEN or infrastructure
  evidence, changed files, limitations, clean status and no-subagent
  confirmation. Actively send it to the manager thread using
  `send_message_to_thread`, then stop without polling.
- Lifecycle: one-shot; manager archives after exact integration/content and
  populated Release verification.
- Status: recorded before visible work conversation creation.

### RESPONSE W-PROC-MOVE-001-CREATE

- Result: visible saved-project worktree creation accepted asynchronously.
- Client setup ID:
  `client-new-thread:a72b97b8-3a57-4239-aae3-bdc670aa52ad`.
- Host/model: local, `gpt-5.6-luna`, `max`; satisfies the project model rule.
- Exact branch base at dispatch:
  `46a413609b0d691ad3d66503c04967df00b22d41`.
- Prompt integrity: exact two-file special-member/trait slice, codebase-design
  and TDD reads, no source/CMake/core/plugin expansion, no dependency repair,
  no subagents/rt_server/polling and proactive manager RESPONSE.
- Coordination: final thread ID is setup-owned and was not returned. Manager
  will not poll; await only the worker-pushed result.

### PUBLISH W-PROC-MOVE-001-START

- GitHub issue #4 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/4#issuecomment-5194029032`.
- Read-back: exact formal-review SHA and P0/P1/P2/P3 state, implicit-move
  contradiction, selected deleted-copy/move policy, Luna/max base/scope and
  no-polling/no-rt_server rules all match durable records.
- Status: P2 fix dispatch is public; issue #4 remains open pending worker
  response, manager integration and complete focused Release GREEN.

### RESPONSE W-PROC-MOVE-001

- Visible Luna/max thread: `019fd29c-2487-77a3-bfee-512054491941`.
- Exact base/commit:
  `46a413609b0d691ad3d66503c04967df00b22d41` /
  `b49f27699c75868a70c43909149464d076c5129f`.
- Header change: explicit noexcept default constructor; deleted copy
  constructor/assignment and move constructor/assignment.
- Focused test change: adds `<type_traits>` and five namespace-scope C++14
  static assertions for nothrow default construction, noncopyability and
  nonmovability. Existing three Qt runtime slots remain unchanged.
- Worker evidence: exactly the two authorized files; unstaged/staged/committed
  diff checks and clean detached status pass.
- Infrastructure: clean worktree configuration stopped before test compilation
  on the known ignored Eigen `Macros.h`/`buildtests.in` gap. No pre-fix RED or
  post-fix GREEN is claimed, and no dependency was copied/repaired/staged.
- Restrictions/lifecycle: no source/CMake/core/plugin change, full mne_scan,
  rt_server, subagent or manager polling. Manager owns exact diff review and
  populated Release validation before P2 closure.
- Status: response recorded before manager commit inspection.

### MANAGER REVIEW W-PROC-MOVE-001

- Provenance/scope: exact commit `b49f27699` has requested parent
  `46a413609`, changes only the processor header and focused test source, and
  passes commit diff check.
- Interface: explicit noexcept default construction plus deleted copy/move
  operations encode the selected single-worker ownership without a new move
  seam or moved-from state contract.
- Traits: five namespace-scope C++14 assertions observe the public type policy;
  an implicit move regression becomes a compile failure. Existing three Qt
  runtime behaviors are byte/content unchanged.
- Noexcept: default construction only initializes `unique_ptr` and the fixed
  scalar snapshot, so the declaration is truthful under the current members.
- Finding: none. This is the minimal correction requested by formal P2
  `R-PROC-MOVE-001`.
- Decision: accept for cherry-pick and populated focused Release build/run.
  P2 remains open until compiled traits and all three runtime slots are GREEN.

### MANAGER VALIDATION W-PROC-MOVE-001-GREEN

- Integrated commit: `4a57c3ca5` (cherry-pick of exact worker
  `b49f27699`).
- Build: populated Release target `test_adaptive_denoising_plugin` compiled and
  linked successfully. The five C++14 type traits compiled under the project
  MSVC/Qt configuration; only known Eigen C4819 code-page warnings occurred.
- Runtime report: QtTest 5.15.2 reports 5 passed, zero failed/skipped/
  blacklisted; all three behavior slots plus init/cleanup pass in 1 ms and the
  process exits zero.
- Behavioral preservation: mapping/train/ApplyOnly, all 19 invalid-disarm
  cases and valid reconfigure/reset/relearn remain GREEN.
- Decision: close P2 `R-PROC-MOVE-001`. The selected interface is now
  compile-time noncopyable/nonmovable and no contradictory moved-from state can
  be formed. Issue #4 is ready for publication/retirement closure.
- No full mne_scan or rt_server was run; user untracked paths remain untouched.

### PUBLISH/RETIREMENT PRECHECK W-PROC-MOVE-001

- Issue #4 GREEN/review closure comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/4#issuecomment-5194141013`;
  read-back matches commits, 5/0/0 report, P0/P1/P2 zero and no-rt_server
  evidence.
- Final QA P3 deferral comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/3#issuecomment-5194141240`;
  read-back tracks `R-PROC-LOCALITY-001` and `R-PROC-BOUNDARY-001` with exact
  requested follow-up.
- Worker worktree is clean/detached at exact `b49f27699`.
- Worker/integration content hashes match for both authorized files: header
  `ec7a273b4200b1d4178b2cc7802ff0573cece115`, focused test
  `14cfc9a0db4e64ae4ed6b0b96d0db7cbe2fefdb4`.
- Decision: one-shot Luna/max thread
  `019fd29c-2487-77a3-bfee-512054491941` may be archived through the app. Do
  not manually delete or modify its app-owned worktree.

### RETIREMENT RESPONSE W-PROC-MOVE-001

- App result: one-shot Luna/max thread
  `019fd29c-2487-77a3-bfee-512054491941` archived successfully after exact
  integration/content and populated GREEN evidence were durable.
- Worktree policy: app-owned path
  `C:/Users/lcy/.codex/worktrees/9da4/mne-cpp-main` was not manually deleted or
  modified by the manager.
- Status: complete; the ownership-fix conversation will not be reused.

### GITHUB RESPONSE PROCESSOR-MILESTONE-CLOSE

- Issue #4 was closed with state reason `completed` after its formal gate
  reached P0/P1/P2 zero and the fix worker was archived.
- Epic #2 body now marks `#4 — AdaptiveDenoisingProcessor mapping and focused
  tests` checked and records the 5/0/0 processor review result.
- Remaining child order is #5 queue/lifecycle, #6 plugin/UI, #7 example/docs,
  then #3 final QA. No conventional PR is created because histories remain
  unrelated.

### REQUEST W-QUEUE-TEST-001

- From / to: manager thread
  `019fcdc3-4a1e-76d1-8140-1bd521219297` / new visible queue tracer work
  conversation.
- GitHub issue: `Edddddddddy/mne-cpp#5`.
- Model/environment: `gpt-5.6-luna`, `max`, new saved-project worktree from
  `codex/causal-reference-denoising`; no internal/nested subagents.
- Blocking: yes for queue implementation.
- TDD behavior: add exactly one new focused Qt slot through the frozen
  `AdaptiveDenoisingBlockQueue` interface. Configure two rows, max four samples
  and capacity two. Push distinct 2x3 block A and 2x4 block B with distinct
  shared metadata control blocks, mutate the caller matrices afterward, then
  require a 2x2 block C push to return Full. Pop into a preallocated 2x4
  destination with timeout zero: A then B must preserve original deep-copied
  valid columns, exact sample counts and metadata ownership order. Empty pop
  returns Timeout; then C can be pushed/popped, proving Full did not advance or
  overwrite the ring.
- Metadata test seam: queue header forward-declares `FIFFLIB::FiffInfo`. The
  test creates aliasing `shared_ptr<const FiffInfo>` handles with null element
  pointers but distinct live owner control blocks and compares ownership using
  `owner_before`; it never constructs/dereferences FIFF or links mne_fiff.
- Authorized file: focused processor test source only. Preserve all three
  existing processor slots. Do not add queue production/CMake/core/plugin/UI/
  dependency/vendor/example changes or run rt_server.
- Expected RED: missing
  `<adaptivedenoising/adaptivedenoisingblockqueue.h>`. If the clean worktree
  Eigen baseline blocks CMake first, report honestly and do not copy/repair it;
  manager owns populated RED reproduction.
- Required response: `RESPONSE W-QUEUE-TEST-001` with exact base/commit,
  matrices/status/ownership oracle, expected RED or infrastructure evidence,
  changed file/clean status and no-subagent/no-polling confirmation. Actively
  notify the manager thread, then stop.
- Lifecycle: one-shot; archive after accepted/integrated RED and content proof.
- Status: recorded before visible task creation.

### RESPONSE W-QUEUE-TEST-001-CREATE

- Result: visible saved-project worktree creation accepted asynchronously.
- Client setup ID:
  `client-new-thread:424b1c1f-6d6a-4c9a-a380-39b3ce73abf2`.
- Host/model: local, `gpt-5.6-luna`, `max`.
- Exact branch base: `734d9951514274d9c795c45c1e179e3259659534`.
- Prompt integrity: one test-source slot, exact A/B/C drop-newest/FIFO/deep-
  copy/metadata-owner oracle, no FIFF construction/link, no production/CMake/
  dependency/rt_server/subagent/polling and proactive manager RESPONSE.
- Coordination: final thread ID was not returned and will not be polled; await
  only the worker-pushed result.

### PUBLISH W-QUEUE-TEST-001-START

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5194234831`.
- Read-back: exact base/model/scope, A/B/C deep-copy/FIFO/metadata/Full/Timeout
  contract, expected missing-header RED, isolated FIFF handle and no-polling/
  no-rt_server rules match the durable request.
- Status: tracer dispatch is public; manager awaits only the worker-pushed
  RESPONSE and does not edit the same focused test source concurrently.

### RESPONSE W-QUEUE-TEST-001

- Visible Luna/max thread: `019fd2ad-308f-7282-bdaa-0b321de8df06`.
- Exact base/commit: `734d9951514274d9c795c45c1e179e3259659534` /
  `ad3e54bfebfbfc62bcf3c4e6ea5e4af594122f21`.
- Slot: `queuePreservesFifoDropNewestAndMetadata` in the focused processor test
  source only.
- Matrices: A=`[[1,2,3],[11,12,13]]`, B=`[[21,22,23,24],
  [31,32,33,34]]`, C=`[[41,42],[51,52]]`; A/B are mutated after push and
  pops compare saved originals, proving deep copy.
- Metadata: three distinct `shared_ptr<int>` owners create aliasing null
  `shared_ptr<const FIFFLIB::FiffInfo>` handles. Ownership equivalence uses
  symmetric `owner_before`; no FIFF object is constructed or dereferenced.
- Status oracle: configure Ready; A/B Pushed; C Full; FIFO pops A/B with sample
  counts 3/4 and paired owners; empty pop Timeout; C then Pushed/Popped with
  sample count two, proving Full did not advance/overwrite the ring.
- Worker RED evidence: queue header absence is confirmed, but clean-worktree
  CMake stopped earlier on the known ignored Eigen `Macros.h`/`buildtests.in`
  gap. No compiler RED or GREEN is claimed and no dependency was repaired.
- Integrity: exactly one authorized test source; diff checks and clean detached
  status pass; no production/CMake/full scan/rt_server/subagent/polling.
- Status: response recorded before manager commit/diff inspection.

### MANAGER REVIEW W-QUEUE-TEST-001

- Provenance/scope: exact `ad3e54bf` has requested parent `734d99515`, changes
  one authorized focused test source by 96 insertions and passes diff check.
- Public-interface sensitivity: post-push A/B mutation distinguishes deep copy;
  capacity-two C Full plus later C success distinguishes drop-newest from
  overwrite/index advance; sequential pops and distinct control-block owners
  distinguish FIFO data/metadata pairing.
- Test locality: only valid leading columns are compared; unused preallocated
  tail remains unspecified. Metadata handles are never dereferenced and the
  test adds no mne_fiff link or implementation seam.
- Finding: none. One vertical behavior is accepted for populated compile RED.
- Decision: cherry-pick and require C1083 missing queue header before any
  production implementation request.

### MANAGER VALIDATION W-QUEUE-TEST-001-RED

- Integrated commit: `b3ff9afc1` (cherry-pick of exact worker `ad3e54bf`).
- Command: populated Release build of `test_adaptive_denoising_plugin`.
- Expected/observed failure: MSVC C1083 at focused test line 12, missing
  `adaptivedenoising/adaptivedenoisingblockqueue.h`. Compilation reaches the
  new tracer after normal target generation; only known Eigen C4819 warnings
  accompany the failure.
- Classification: valid public-interface compile RED. No queue production file
  exists and no unrelated dependency/link/full-scan/rt_server failure masks it.
- Decision: Sol/ultra production implementation may begin after RED publication
  and one-shot tracer retirement.

### PUBLISH/RETIREMENT PRECHECK W-QUEUE-TEST-001

- GitHub RED comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5194354446`;
  read-back matches worker/integration commits, exact C1083 and behavior scope.
- Worker worktree `C:/Users/lcy/.codex/worktrees/6cbb/mne-cpp-main` is clean and
  detached at exact `ad3e54bf`.
- Focused test content hash matches integration:
  `c0e6b3808a7569b9af990cc15190b8a5cc4b57cd`.
- Decision: one-shot thread `019fd2ad-308f-7282-bdaa-0b321de8df06` may be
  archived through the app. Do not manually delete or alter its worktree.

### RETIREMENT RESPONSE W-QUEUE-TEST-001

- App result: one-shot Luna/max tracer thread
  `019fd2ad-308f-7282-bdaa-0b321de8df06` archived successfully after valid RED,
  exact content and clean worktree evidence were durable.
- Worktree policy: app-owned path
  `C:/Users/lcy/.codex/worktrees/6cbb/mne-cpp-main` was not manually deleted or
  modified by the manager.
- Status: complete; the tracer conversation will not be reused.

### REQUEST W-QUEUE-GREEN-001

- From / to: manager thread
  `019fcdc3-4a1e-76d1-8140-1bd521219297` / new visible queue implementation
  conversation.
- GitHub issue: `Edddddddddy/mne-cpp#5`.
- Model/environment: `gpt-5.6-sol`, `ultra`, new saved-project worktree from
  `codex/causal-reference-denoising`; no internal/nested subagents.
- Model decision: preallocated SPSC ownership, semaphore ordering, stop wakeup
  and no-wait producer safety make this a concurrency/realtime slice.
- Blocking: yes for queue lifecycle tests and plugin work.
- Task: make only the integrated
  `queuePreservesFifoDropNewestAndMetadata` tracer GREEN by implementing the
  frozen `AdaptiveDenoisingBlockQueue` header/source and adding its source to
  the focused test CMake. Do not edit the test.
- Implementation contract: concrete noncopyable/nonmovable PImpl queue;
  configure validates positive rows/max samples/capacity fitting QSemaphore's
  integer count, allocates all fixed matrix slots transactionally and starts an
  empty running queue; already-running configure returns `AlreadyRunning`.
  Producer checks running/shape, performs exactly one
  `QSemaphore::tryAcquire(1,0)`, returns Full without index/slot change, deep-
  copies only valid columns, retains metadata and publishes once. Consumer
  requires a preallocated exact max-size destination, uses one timed acquire,
  copies valid columns plus sampleCount/metadata in FIFO order, releases the
  slot and leaves destination tail unspecified.
- Stop/lifecycle: atomic running flag; `stop()` is noexcept/idempotent, rejects
  future pushes and releases one filled semaphore token to wake a timed waiter.
  A woken/stopped consumer returns Stopped without reading a slot. Reconfigure
  after stop creates fresh state; caller must quiesce producer/consumer before
  reconfigure/destruction. Pending blocks may be discarded on stop.
- Realtime: push/pop/stop perform no heap allocation, retry, busy wait, lock,
  string or FIFF dereference. Shared metadata may be null and is transported
  by ownership. Configure is the only allocation phase and may propagate
  allocation exceptions while retaining old stopped state.
- Authorized files: new
  `adaptivedenoising/adaptivedenoisingblockqueue.{h,cpp}` and focused test
  CMake only. No test/processor/core/plugin/UI/global CMake/dependency/vendor/
  example or rt_server change.
- Verification: exact diff/scope checks; build/run focused Release if isolated
  dependencies permit, otherwise report known Eigen limitation. Do not repair
  dependencies. Manager owns populated GREEN.
- Required response: `RESPONSE W-QUEUE-GREEN-001` with exact base/commit,
  interface and state/slot shapes, semaphore/index/order and stop semantics,
  no-allocation argument, changed files, evidence/limitations, clean status and
  no-subagent/no-polling confirmation. Proactively notify manager, then stop.
- Lifecycle: retain until manager accepts GREEN because a focused follow-up may
  be useful only if the same implementation context remains current.
- Status: recorded before visible task creation.

### RESPONSE W-QUEUE-GREEN-001-CREATE

- Result: visible saved-project worktree creation was accepted asynchronously.
- Client setup ID:
  `client-new-thread:e551ef42-5b43-48d0-a0e1-ddb5863854d4`.
- Host/model: local, `gpt-5.6-sol`, `ultra`; the saved project is a Git
  repository and the task uses an app-managed worktree.
- Exact branch base at dispatch:
  `930480bf301a05a51e12f7c24be3b93a29ddf9bb`.
- Prompt integrity: exact three-file authorization, immutable integrated RED,
  semaphore/index/stop/reconfigure contract, recovery and skill reads,
  no-dependency-repair/no-subagent/no-rt_server/no-polling restrictions and
  proactive `RESPONSE W-QUEUE-GREEN-001` notification to the manager.
- Coordination: setup returned no final thread ID and will not be polled. The
  manager awaits only the worker-pushed RESPONSE after issue dispatch is
  published.

### PUBLISH W-QUEUE-GREEN-001-START

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5194438436`.
- Read-back: exact request base, Sol/ultra model decision, three-file scope,
  immutable RED tracer, PImpl/SPSC/semaphore/stop/reconfigure/hot-path gates,
  setup ID and no-polling/no-rt_server contract all match the durable request.
- Status: implementation dispatch is public. The manager will not poll or edit
  the same queue/CMake scope and awaits only the worker-pushed RESPONSE.

### RESPONSE W-QUEUE-GREEN-001

- Visible Sol/ultra thread: `019fd2bd-b720-7b30-8d65-780a0b500924`.
- Exact base/commit:
  `930480bf301a05a51e12f7c24be3b93a29ddf9bb` /
  `039b58d1b1cb2b7d79b8d8db27839a3c9f7ad81b`; worker reports a clean
  detached HEAD whose parent is the requested base.
- Scope: new queue header/source plus focused test CMake only; the integrated
  RED test and processor/core/plugin/UI/global CMake remain unchanged.
- Interface/ownership: concrete final PImpl queue, explicit noexcept default
  construction, out-of-line destruction and deleted copy/move. Header only
  forward-declares `FIFFLIB::FiffInfo`.
- State: immutable configured dimensions/capacity, pre-sized matrix slots with
  sample count/metadata ownership, free/filled semaphores, atomic running flag
  and single-owner producer/consumer indices.
- Configure: rejects invalid dimensions/count and active reconfigure; builds a
  fully allocated candidate and publishes it only after successful construction,
  preserving prior stopped ownership if allocation throws.
- Producer: validates shape/running, performs one zero-time free-slot acquire,
  returns Full without mutation, copies only valid columns/metadata, advances
  once and publishes one filled token.
- Consumer: validates exact preallocated destination, performs one timed filled
  acquire, returns Stopped after a stop wake without reading a slot, otherwise
  copies valid columns/sample count/metadata in FIFO order, advances once and
  releases one free token. Unused destination tail is untouched.
- Stop/reconfigure: one acq_rel running exchange makes stop idempotent and the
  first transition releases one wake token. Caller quiescence is documented
  before reconfigure/destruction; a later configure installs a fresh queue and
  pending stopped blocks may be discarded.
- Realtime claim: all matrices/container/semaphores allocate at configure;
  hot paths use scalar copies, shared ownership operations, indices, atomics
  and semaphore calls without resize/growth/retry/busy wait/explicit locks/
  strings/FIFF access.
- Worker validation: exact scope/diff checks pass. Isolated CMake stops before
  target generation at the known ignored Eigen `Macros.h`/`buildtests.in` gap;
  no dependency repair or local runtime GREEN is claimed. No subagents,
  manager polling, full mne_scan or rt_server.
- Status: response is durable before manager commit inspection. The visible
  conversation remains reusable only for a narrow same-context follow-up.

### MANAGER REVIEW W-QUEUE-GREEN-001

- Provenance/scope: exact commit `039b58d1b` has requested parent `930480bf3`,
  passes diff check and changes exactly the two new queue files plus one focused
  CMake source registration. The integrated public tracer is unchanged.
- Deep-module seam: accepted. Four public operations hide slot allocation,
  ring ownership, semaphore capacity/publication, stop wake and metadata
  pairing. No strategy/registry/FIFF definition or caller-side ring state leaks
  through the interface.
- Transaction: invalid/active checks precede allocation; a fully constructed
  candidate owns all slots/semaphores and becomes running before swap. Any
  construction exception leaves the previous stopped PImpl untouched; success
  discards it only after committing a fresh empty queue.
- SPSC order: producer/consumer indices are single-thread-owned. A successful
  producer acquires one free token, writes the complete slot, advances once and
  releases one filled token; Full has no write/index change. Consumer acquires
  one filled token, copies/clears one slot, advances once and releases one free
  token. Scalar copy order matches Eigen's column-major layout.
- Stop safety: atomic active-to-stopped exchange is idempotent and contributes
  one wake token. A blocked empty consumer wakes, observes stopped and reads no
  slot; stopped queue state is intentionally disposable. In-flight operations
  may finish, while caller quiescence is required before replacement/destruction.
- Hot path: no resize/container growth/retry/busy wait/string/FIFF access or
  explicit lock. Shared-pointer ownership transfer/clear performs no control-
  block allocation; destination-owner destruction occurs on the worker side.
- Residual coverage assigned to the next vertical slice: invalid dimensions,
  AlreadyRunning, invalid block/destination, stop wake/idempotence, restart,
  FIFO after overflow and producer-drop accounting are not all observed by the
  first tracer. Formal Sol/ultra concurrency review still follows the completed
  queue milestone.
- Finding: none in the authorized GREEN implementation slice. Accept for
  cherry-pick and populated Release compile/runtime; no GREEN is claimed yet.

### MANAGER VALIDATION W-QUEUE-GREEN-001-GREEN

- Integrated production commit: `73f05da14` (cherry-pick of exact worker
  `039b58d1b`).
- Configure/build: populated focused CMake generation succeeded; Release target
  compiled and linked the queue, processor, numerical core, test and Qt moc.
  Only the known Eigen/MSVC C4819 code-page warnings occurred.
- Runtime: hidden synchronized QtTest run wrote a durable text report and
  returned process exit code zero.
- Report: 6 passed, zero failed/skipped/blacklisted. The new
  `queuePreservesFifoDropNewestAndMetadata` tracer passes with all three existing
  processor behaviors plus init/cleanup.
- Behavioral conclusion: configured capacity-two slots deep-copy caller data,
  preserve FIFO matrix/metadata ownership pairing, drop newest on Full without
  corrupting indices, return Timeout when drained and accept the later C block.
- Decision: first queue vertical slice is genuinely GREEN. Publish evidence,
  then add the next lifecycle behavior before formal concurrency review.
- Restrictions: no full mne_scan or rt_server; user untracked paths remain
  untouched.

### PUBLISH W-QUEUE-GREEN-001-GREEN

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5194795653`.
- Read-back: exact worker/integration commits, three-file scope, populated
  Release build, 6/0/0 report, FIFO/drop-newest/deep-copy/metadata conclusions,
  restrictions and next lifecycle slice all match durable evidence.

### REQUEST W-QUEUE-TEST-002

- From / to: manager thread
  `019fcdc3-4a1e-76d1-8140-1bd521219297` / new visible queue-lifecycle test
  conversation.
- GitHub issue: `Edddddddddy/mne-cpp#5`.
- Model/environment: `gpt-5.6-luna`, `max`, new saved-project worktree from the
  current integration branch; no internal/nested subagents.
- Blocking: yes for queue lifecycle completion and formal concurrency review.
- TDD behavior: add exactly one public-interface slot
  `queueStopWakesWaiterAndReconfigureStartsFresh`. Configure an empty queue,
  require a second active configure returns AlreadyRunning, then start one
  consumer waiting with a bounded multi-second timeout into an exact
  preallocated destination. After the consumer has entered the call and a
  short scheduling allowance, call stop twice; require the consumer joins well
  before its timeout and returns Stopped. Subsequent push/pop while stopped
  must return Stopped.
- Fresh-state oracle: configure again after the stopped consumer is joined;
  require Ready, push/pop one distinct short matrix plus aliasing metadata owner
  and prove exact leading columns/sample count/owner. An immediate second pop
  must Timeout, proving neither the old stop wake token nor pending state leaked
  into the fresh queue.
- Test robustness: use a finite consumer timeout so a broken wake cannot hang
  the suite; synchronize only test setup, not queue internals; allow reasonable
  Windows scheduling margin while still distinguishing wake from timeout.
- TDD classification: expected immediate GREEN characterization of the newly
  integrated interface. Do not manufacture RED or edit production when it
  passes; if it fails, return exact timing/status evidence for a separate Sol
  fix request.
- Authorized file: focused plugin test source only. Preserve existing slots;
  no queue/processor/core/CMake/plugin/UI/dependency/vendor/example/rt_server.
- Required response: `RESPONSE W-QUEUE-TEST-002` with exact base/commit,
  synchronization/timing oracle, stop/status/reconfigure matrices and metadata
  evidence, RED or GREEN/infra classification, changed file/clean status and
  no-subagent/no-polling confirmation. Proactively notify manager then stop.
- Lifecycle: one-shot unless manager explicitly reuses it for one immediately
  adjacent queue-validation test after acceptance.
- Status: recorded before visible task creation.

### RESPONSE W-QUEUE-TEST-002-CREATE

- Result: new visible saved-project worktree creation accepted asynchronously.
- Client setup ID:
  `client-new-thread:0b964bc3-a775-4fbf-88c3-657beb2570a5`.
- Host/model: local, `gpt-5.6-luna`, `max`.
- Exact branch base: `f77bf44cee08b54445b672119bccd3789b67f663`.
- Prompt integrity: one focused test-source slot, public-only waiter timing,
  main-thread assertions, guaranteed join discipline, idempotent stop/stopped
  statuses/fresh reconfigure/metadata owner oracle, no production/CMake/vendor/
  rt_server/subagent/polling and proactive RESPONSE notification.
- Coordination: no final thread ID was returned and setup will not be polled;
  manager awaits only the worker-pushed response after public dispatch.

### PUBLISH W-QUEUE-TEST-002-START

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5194824674`.
- Read-back: exact base/model/setup ID, one test-source scope, active configure,
  bounded stop wake/idempotence/stopped statuses/fresh reconfigure oracle and
  no-subagent/no-polling/no-rt_server restrictions match the durable request.
- Status: public dispatch complete. Manager will not poll or overlap the test
  source and awaits only the worker-pushed RESPONSE.

### RESPONSE W-QUEUE-TEST-002

- From / to: visible Luna/max thread
  `019fd2e0-188f-7a62-a89c-7ceefa15f6cd` / manager.
- Exact base/commit: `f77bf44cee08b54445b672119bccd3789b67f663` /
  `da315d81ecfbf43f9e27f2013970abe3fa38715e`; parent is the requested base.
- Scope: one focused test-source file only, adding
  `queueStopWakesWaiterAndReconfigureStartsFresh`; production queue unchanged.
- Synchronization: configure 2x4/capacity-one, reject active reconfigure, start
  one finite 3000 ms public waiter into a preallocated destination, record only
  atomics in the consumer, then stop twice and join before all Qt assertions.
  The oracle requires `Stopped` in less than 1500 ms and logs entry/wait/timing
  values to distinguish wakeup from natural timeout.
- Lifecycle oracle: push/pop return Stopped after stop; a joined reconfigure is
  Ready, transports a distinct 2x2 matrix and aliasing metadata owner exactly,
  then returns Timeout on the fresh empty queue, proving no stale token/data.
- Worker evidence: one-file commit, clean status and diff checks pass. Isolated
  execution is honestly unclaimed because ignored Eigen baseline files are
  absent; no vendor repair, full scan, rt_server, subagent or manager polling.
- Status: response recorded before manager diff review or cherry-pick.

### MANAGER REVIEW W-QUEUE-TEST-002

- Provenance: exact commit `da315d81e` has parent `f77bf44ce`; its diff is
  clean and contains exactly one authorized focused test-source edit with 134
  additions. Queue production, CMake and all existing slots are unchanged.
- Safety: the destination is fully preallocated before thread creation; the
  consumer performs one public finite wait and stores only atomic observations.
  The main thread uses a bounded entry wait, calls idempotent stop, always joins,
  and performs every Qt assertion afterward, so no failing assertion strands a
  joinable thread.
- Sensitivity: the 1500 ms upper bound is below the 3000 ms natural timeout;
  `consumerWasWaitingBeforeStop`, exact Stopped statuses and a fresh queue
  matrix/metadata/Timeout oracle jointly detect failed wake, active-configure
  mutation and stale stop/data tokens. The 50 ms allowance is not an internal
  synchronization dependency and remains a conservative Windows margin.
- Finding: none. Accept for cherry-pick; runtime GREEN remains required in the
  populated Release workspace before this lifecycle slice closes.

### VALIDATION W-QUEUE-TEST-002

- Integration: worker `da315d81e` cherry-picked as `90f5cdbf3`.
- Focused build: populated Visual Studio Release target
  `test_adaptive_denoising_plugin` compiles and links successfully; output is
  `out/Release/apps/test_adaptive_denoising_plugin.exe`. Only the known Eigen
  code-page C4819 warnings are present.
- Synchronized hidden run: process exit zero; QtTest reports 7 passed, zero
  failed/skipped/blacklisted. The lifecycle slot logs `stopDelayMs 64`,
  `observedElapsedMs 63`, `enteredBeforeStop true`, `waitingBeforeStop true`
  against a 3000 ms waiter timeout and 1500 ms wake bound.
- Regression: all three processor slots and the original queue FIFO/drop-newest
  slot remain GREEN. No full mne_scan or rt_server was run.
- Decision: lifecycle slice GREEN. One adjacent validation slice plus formal
  Sol/ultra concurrency review remain before closing queue issue #5.

### PUBLISH W-QUEUE-TEST-002-GREEN

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5194971349`.
- First write defect: PowerShell interpreted Markdown backticks preceding the
  base SHA and target name as escape sequences, so read-back exposed control
  characters in those two fields. No code/test/repository state was affected.
- Correction: the same comment was patched in place using plain SHA/target
  text; a second API read-back exactly matches commits, 7/0/0 result, 63 ms
  wake evidence, lifecycle oracle, restrictions and remaining review work.

### REQUEST W-QUEUE-TEST-003

- From / to: manager / reusable visible Luna/max queue-test conversation
  `019fd2e0-188f-7a62-a89c-7ceefa15f6cd`.
- GitHub issue: `Edddddddddy/mne-cpp#5`; model remains `gpt-5.6-luna`, `max`.
- Blocking: yes for focused queue coverage before formal concurrency review.
- Task: add exactly one public-interface QtTest slot
  `queueRejectsInvalidInputsWithoutConsumingState` to the focused test source;
  production queue and CMake remain immutable.
- Configuration rows: on fresh/stopped queues require InvalidConfiguration for
  channelCount zero, maxBlockSamples zero, capacity zero and capacity one above
  `int` maximum, without allocation. Then configure 2x4/capacity-one Ready.
- Producer rows: exact invalid row-count, zero-column and five-column matrices
  each return InvalidBlock and do not consume capacity. A valid distinct 2x3
  matrix with null metadata must then push successfully; a second valid block
  returns Full while the first remains pending.
- Consumer rows: wrong-row and wrong-column preallocated destinations each
  return InvalidDestination without consuming the pending block or mutating
  their matrix/sampleCount/metadata sentinels. A valid exact 2x4 destination
  then pops the original 2x3 matrix, reports sampleCount three and transports
  null metadata; a zero-time second pop returns Timeout.
- Test design: compare only valid leading columns in the successful pop, but
  compare complete invalid destinations against sentinels. Preserve all seven
  existing passes. Do not inspect queue internals, use timing, or edit any
  production/CMake/core/plugin/example/docs/vendor/dependency file.
- Verification: attempt focused Release if dependencies permit; otherwise
  report the known clean-worktree Eigen limitation honestly. Return exact
  base/commit, matrices/status/state-preservation oracle, file/clean checks and
  no-subagent/no-polling/no-full-scan/no-rt_server confirmation as
  `RESPONSE W-QUEUE-TEST-003`, proactively notifying manager.
- Lifecycle: final reuse of this test conversation for queue issue #5; archive
  after manager integration/GREEN/content verification.
- Status: recorded and will be committed/pushed before follow-up dispatch.

### RESPONSE W-QUEUE-TEST-003-DISPATCH

- Result: follow-up accepted by retained visible task
  `019fd2e0-188f-7a62-a89c-7ceefa15f6cd`.
- Model: explicit `gpt-5.6-luna`, `max` as required by the user.
- Exact requested base: `701f5bed955fde2050438e6e4d8440f6fbe1766b`;
  worker must first prove its task worktree clean, fetch if required, and detach
  at that integration commit before editing.
- Prompt integrity: one test slot/file, exact configuration/producer/consumer
  invalid-state oracles, valid null-metadata capacity-one recovery, no timing,
  production/CMake/vendor/full-scan/rt_server/subagent/polling restrictions and
  proactive RESPONSE are all present.
- Coordination: manager will not poll or edit the focused test source. Formal
  review starts only after this response is inspected, integrated and GREEN.

### PUBLISH W-QUEUE-TEST-003-START

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5195009989`.
- GitHub connector write plus CLI API read-back exactly match the Luna/max
  thread, base `701f5bed9`, one test-file scope, invalid state-preservation and
  valid null-metadata recovery oracles, proactive response and restrictions.
- Status: public dispatch complete; manager does not poll or overlap the file.

### RESPONSE W-QUEUE-TEST-003

- From / to: retained visible Luna/max queue-test thread
  `019fd2e0-188f-7a62-a89c-7ceefa15f6cd` / manager.
- Exact base/commit: `701f5bed955fde2050438e6e4d8440f6fbe1766b` /
  `cff55a01e785e0b7d44bd333e1fc3a024c06717e`; exact requested parent.
- Scope: 131 insertions in the focused test source only; production/CMake and
  all existing slots unchanged; initial/final worker state clean.
- Configuration: fresh and stopped queues reject zero channel/block/capacity
  and capacity 2147483648, then a 2x4/capacity-one configuration is Ready.
- Producer: 3x3, 2x0 and 2x5 inputs return InvalidBlock; a distinct 2x3 null-
  metadata block then enters, and a second valid block returns Full.
- Consumer: wrong 3x4 and 2x3 destinations return InvalidDestination without
  changing complete matrix/sampleCount/alias-owner sentinels or consuming the
  pending block. Exact 2x4 pop returns the original leading 2x3 values/count
  and null metadata, followed by Timeout.
- Evidence: diff/one-file/clean checks pass; isolated configure is blocked by
  the known ignored Eigen gap without repair or runtime claim. Luna/max, no
  subagent, polling, full scan or rt_server.
- Status: response durable before manager diff review or cherry-pick.

### MANAGER REVIEW W-QUEUE-TEST-003

- Provenance: exact `cff55a01e` on requested `701f5bed9`; clean diff with 131
  insertions in exactly the focused test source. Queue/CMake and seven prior
  passes remain unchanged.
- Configuration sensitivity: fresh and stopped invalid candidates exercise all
  public bounds including `size_t(INT_MAX)+1`, and Ready after them proves the
  failed candidates neither allocate/publish nor poison future configuration.
- State sensitivity: invalid producer shapes precede a successful capacity-one
  push; Full on the next valid block proves capacity is occupied only by the
  first valid payload. Two invalid destinations preserve full matrix, count and
  alias-owner sentinels, while a later valid pop returns the original first
  block/null metadata and then Timeout.
- Finding: none. Accept for cherry-pick; populated Release GREEN remains
  required before queue coverage is sent to formal Sol/ultra review.

### VALIDATION W-QUEUE-TEST-003

- Integration: worker `cff55a01e` cherry-picked as `422c5264c`.
- Focused build: populated Visual Studio Release target compiles and links;
  only known Eigen C4819 warnings occur.
- Runtime: synchronized hidden process exits zero; QtTest reports 8 passed,
  zero failed/skipped/blacklisted. Lifecycle wake remains bounded at 62 ms
  after a 63 ms stop delay against the 3000 ms natural timeout.
- New oracle: every invalid configuration/input/destination assertion passes,
  followed by exact null-metadata payload recovery and Timeout. All processor,
  FIFO/drop-newest and lifecycle regressions remain GREEN.
- Restrictions: no full mne_scan, rt_server or vendor repair.
- Decision: focused queue coverage complete; publish GREEN and start formal
  Sol/ultra concurrency/realtime review after worker retirement evidence.

### REQUEST W-EXAMPLE-001

- From / to: manager thread
  `019fcdc3-4a1e-76d1-8140-1bd521219297` / new visible example work
  conversation.
- GitHub issue: `Edddddddddy/mne-cpp#7`.
- Model/environment: `gpt-5.6-luna`, `max`, new saved-project worktree from the
  current branch; no internal/nested subagents.
- Independence: modifies only examples registration and a new example
  directory, so it may run while `W-QUEUE-TEST-002` owns the focused test
  source. It does not consume the queue/plugin seam.
- Task: add focused target `ex_causal_reference_denoising` with a deterministic
  C++14/Eigen stream that uses the public `CausalReferenceDenoiser` interface.
  Demonstrate channel-by-time matrices, two reference rows, one target, one
  preserved row, four tap-major lags, 128-sample blocks, warmup, multiple
  ApplyAndLearn blocks, ApplyOnly freeze, reset and BypassTrackHistory.
- Observable teaching output: print config dimensions (`R/M/P`), each stage's
  status/warmup/generation/accepted/rejected counts and input/output/estimated-
  noise RMS. Print and enforce exact preserved reference/unrelated rows, finite
  target output, generation growth during learning, zero call-local updates in
  freeze and generation reset after reset. Exit nonzero on a violated invariant.
- Synthetic data: explicit deterministic reference formulas and known
  tap-major environmental weights plus a clean sinusoid; maintain one
  continuous causal reference stream across block boundaries. Algorithm effect
  is illustrative here; do not duplicate or retune the quantitative unit-test
  gate.
- Focused dependency seam: add one examples top-level registration, new example
  CMake and `main.cpp`. Compile `causalreferencedenoiser.cpp` directly, include
  `${CMAKE_SOURCE_DIR}/libraries`, define `MNE_RTPROCESSING_LIBRARY`, and link
  only Qt Core and Eigen. Do not link mne_rtprocessing/mne_fiff/mne_scan or
  repair the known toolchain baseline; actual client-link remains final QA P3.
- Authorized files: `src/examples/CMakeLists.txt` and new
  `src/examples/ex_causal_reference_denoising/{CMakeLists.txt,main.cpp}` only.
  No numerical/test/plugin/queue/docs/vendor/dependency/rt_server changes.
- Required verification: focused Release build/run if dependencies permit,
  complete console/report output and exit code; otherwise honest ignored Eigen
  limitation. Exact diff/clean commit checks.
- Required response: `RESPONSE W-EXAMPLE-001` with base/commit, matrices/
  generator/weights/stage sequence, output invariants, CMake dependency proof,
  build/run evidence or limitation, files/clean status and no-subagent/no-poll
  confirmation. Proactively notify manager and stop.
- Lifecycle: one-shot; benchmark and guide remain separate later tasks.
- Status: recorded before visible task creation.

### SYSTEM FAILURE W-EXAMPLE-001-CREATE-1

- Result: the local create-thread orchestration script failed before calling
  the app because an unescaped CMake `${...}` token in its JavaScript template
  prompt was interpreted as a JavaScript variable.
- Side effects: none. No thread, worktree, code, GitHub or dependency change
  was created.
- Decision: preserve the exact request and retry once with a plain/escaped
  prompt string. This is not a duplicate task dispatch.

### RESPONSE W-EXAMPLE-001-CREATE

- Result: the corrected visible saved-project worktree creation was accepted
  asynchronously.
- Client setup ID:
  `client-new-thread:2e3283a1-8aa7-4872-a44f-c36a8a686808`.
- Host/model: local, `gpt-5.6-luna`, `max`.
- Exact corrected branch base:
  `473d9eeeda55ab280771b24b2a0b2b7b4e803995`.
- Prompt integrity: exact examples-only three-file scope, deterministic public-
  interface stream/stage/runtime invariants, direct numerical-source Qt Core/
  Eigen dependency seam, no test/plugin/core/docs/vendor/rt_server/subagent/
  polling and proactive response notification.
- Coordination: no final thread ID was returned; manager will not poll and
  awaits only the worker-pushed RESPONSE after issue #7 dispatch.

### PUBLISH W-EXAMPLE-001-START

- GitHub issue #7 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/7#issuecomment-5194881491`.
- Read-back: exact base/model/setup ID, examples-only three-file scope,
  deterministic channel-time/tap-major stages, fixed runtime invariants,
  isolated Qt Core/Eigen build and no-polling/no-rt_server restrictions match
  the durable request.
- Status: public dispatch complete; manager does not poll either active worker.

### REQUEST W-DOC-001

- From / to: manager / new visible learning-guide conversation.
- GitHub issue: `Edddddddddy/mne-cpp#7`.
- Model/environment: `gpt-5.6-luna`, `max`, current integration branch;
  no internal/nested subagents or manager polling.
- Independence: add one new documentation file only, so it does not overlap
  queue tests, example CMake/main or plugin planning.
- Task: create
  `doc/causal_reference_denoising/LEARNING_GUIDE.md` as a concise but complete
  engineering guide to the implemented v1. Explain channel-by-time Eigen
  shapes, reference/target/preserved rows, tap-major feature ordering, causal
  history across arbitrary block boundaries, warmup and future-only model
  commits.
- Mathematics/code mapping: define `R/M/L/P`, `G(PxP)`, `H(MxP)`, `W(MxP)`,
  per-sample forgetting, pending/committed candidate transaction, relative
  diagonal loading and LDLT solve without inverse. Explain what each actual
  `noalias()` assignment/product buys and where aliasing would be unsafe.
- Streaming/engineering: modes, reset/freeze behavior, latency components,
  fixed update epochs, no-allocation configure/process split, scaled RMS,
  solve/application fallback, target-only writes, queue drop-newest seam and
  complexity `O(N(P^2+MP)) + O(P^3/K)` with memory shapes.
- Learning aids: include one small four-row/two-reference/two-tap numeric shape
  walkthrough and one block-boundary timeline; distinguish algorithm effect
  evidence from plugin engineering verification.
- Research/license: cite the TSR/TSPCA paper, Fast-LMS MEG precedent, BSD
  MEEGkit behavior reference and GPL FieldTrip research-only reference already
  frozen in SPEC. Explicitly state independent equation-based implementation,
  no copied/linked GPL code and v1 non-goals.
- Accuracy: read current numerical header/source and SPEC; do not invent a
  runtime registry, tSSS/HFC support or guarantees not present in tests. Link
  local module/example/test paths using repository-relative Markdown.
- Authorized file: new `LEARNING_GUIDE.md` only. Do not edit durable ledgers,
  source/test/CMake/plugin/example/vendor/dependency files or run rt_server.
- Verification/response: Markdown structure/link/path/static review, diff
  check, one-file clean commit. Return `RESPONSE W-DOC-001` with base/commit,
  outline, exact formulas/example/timeline, sources/license statement, file and
  limitations; proactively notify manager. No subagents/polling/rt_server.
- Lifecycle: one-shot; benchmark results may later be appended by a different
  task after manager review.
- Status: recorded before visible task creation.

### RESPONSE W-DOC-001-CREATE

- Result: new visible Luna/max worktree setup accepted asynchronously.
- Client setup ID:
  `client-new-thread:7c8c1a8c-3952-4f9e-9229-d9f7257a2ccb`; host `local`.
- Exact base: `fa832d51d3b9aa463490cf3afbc2037f71843f41`.
- Prompt integrity: one new guide file, actual numerical/example/test reading,
  exact Eigen/EWLS/noalias/LDLT/streaming/queue/complexity/walkthrough/timeline/
  provenance scope, repository links and non-goals; no other file, subagent,
  polling, full scan or rt_server.
- Coordination: no final thread ID yet; manager will not poll and awaits only
  the proactive structured RESPONSE after public dispatch.

### PUBLISH W-DOC-001-START

- GitHub issue #7 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/7#issuecomment-5195139614`.
- Connector write and API read-back exactly match Luna/max setup/base, one-file
  guide scope, implemented Eigen/EWLS/queue/provenance topics, path accuracy,
  non-goals and restrictions.
- Status: public dispatch complete; manager will not poll or edit the new guide.

### REQUEST W-BENCH-001

- From / to: manager / new visible benchmark conversation.
- GitHub issue: `Edddddddddy/mne-cpp#7`.
- Model/environment: `gpt-5.6-luna`, `max`, new saved-project worktree from the
  current integration branch; no internal/nested subagents or manager polling.
- Scope: modify only
  `src/examples/ex_causal_reference_denoising/main.cpp`; retain the current
  default teaching example exactly and add a `--benchmark` execution mode.
- Scale/config: 1000 Hz, 128 samples/block, 270 total rows, 16 reference rows,
  250 target rows and four preserved rows; four taps (`P=64`), update interval
  128, 30 s memory and 1e-3 loading. Generate deterministic finite reference/
  target/preserved data outside the timed region.
- Timing: configure once, run 100 untimed warmup blocks, then 1000 timed
  ApplyAndLearn calls. Restore/repopulate the already-sized input block outside
  each timer; include the full `process` call and its epoch solve inside. Do no
  per-block I/O or allocation in the timed region.
- Report: print dimensions/config, sample/block duration 128 ms, p50/p95 in ms,
  max/generation/accepted/rejected summary and explicit PASS/FAIL for `p95 <
  128 ms`. Use a documented deterministic percentile rule and exit nonzero on
  configure/process/nonfinite/preservation/performance failure.
- Integrity: prove reference/preserved rows remain exact and selected targets
  stay finite on representative post-call blocks. Benchmark is engineering
  evidence, not a unit test or new algorithm-quality gate.
- Verification: focused populated build is manager-owned; worker should attempt
  Release if its ignored Eigen baseline permits, otherwise report limitation
  without dependency repair. One-file commit/diff/clean checks.
- Required response: `RESPONSE W-BENCH-001` with exact base/commit, dimensions,
  generator/timing boundaries, percentile definition, worker build/run evidence
  or limitation, file/clean status and no-subagent/no-poll/no-full-scan/no-
  rt_server confirmation. Proactively notify manager then stop.
- Lifecycle: one-shot; benchmark-result documentation follows after manager run.
- Status: recorded and will be committed/pushed before visible task creation.

### RESPONSE W-BENCH-001-CREATE

- Result: new visible Luna/max benchmark worktree setup accepted asynchronously.
- Client setup ID:
  `client-new-thread:70b6de3a-9f69-407d-b46d-2d17ac2da778`; host `local`.
- Exact base: `60b394ab2b30509e46b11c16ee3121d99488414b`.
- Prompt integrity: one example-main edit, preserved default mode, exact
  270/16/250/4/P64 workload, 100+1000 timing boundary, percentile/report/
  performance/integrity gates and restrictions are present.
- Coordination: no final thread ID yet; manager will not poll or edit example
  main and awaits the proactive response after public dispatch.

### PUBLISH W-BENCH-001-START

- GitHub issue #7 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/7#issuecomment-5195193407`.
- Connector write and API read-back exactly match Luna/max setup/base, one-file
  preserved-default scope, 270-row/P64/100+1000 workload, timing/report/gate and
  restrictions.
- Status: public dispatch complete; manager will not poll or edit example main.

### RESPONSE W-BENCH-001

- From / to: visible Luna/max benchmark thread
  `019fd300-a9e2-7f93-8063-32450ff1d543` / manager.
- Exact base/commit: `60b394ab2b30509e46b11c16ee3121d99488414b` /
  `79eff3b3ac6119955c6288bd50030d515736f1a5`; exact parent, clean detached
  initial/final worktree and one authorized example-main file.
- Default mode: existing teaching output/invariants are preserved; only
  `--benchmark` dispatch is added.
- Workload: 270 rows = 16 references + 250 targets + four preserved, P=64,
  1000 Hz/128 samples, four taps, one epoch per block, 30 s/1e-3 settings.
  Deterministic finite xorshift/AR/multisine-style data is prepared before timing.
- Timing: one configure, exactly 100 untimed and 1000 timed ApplyAndLearn calls.
  Same-sized work data is restored with raw contiguous copy before each timer;
  the interval contains clocks, complete process/solve and preallocated timing
  write only, with no per-block I/O/container growth.
- Oracle: every call Processed; representative selected target finite; all
  references/preserved rows exact. Nearest-rank percentile is
  `rank=ceil(q*N)`, value `sorted[rank-1]`; report p50/p95/max, generations and
  events, and exit failure unless p95 is strictly below 128 ms.
- Worker evidence: isolated VS18 configure is blocked by the known ignored
  Eigen gap; a VS2022 attempt also lacks v143. No vendor repair or runtime
  percentile claim. Generated temporary/build outputs were cleaned. Luna/max,
  no subagent, polling, full scan or rt_server.
- Status: response durable before manager provenance/code/timing review.

### MANAGER REVIEW W-BENCH-001

- Provenance/scope: exact `79eff3b3a` on requested `60b394ab2`; clean one-file
  323-addition/one-line-change diff. Default teaching path remains behind the
  no-argument branch and is textually unchanged.
- Timing: matrices/timing vector reach final sizes before loops; raw restore is
  before clocks; timed interval contains one complete `process` call only.
  Nearest-rank sorting/report and strict p95 gate match the request.
- `R-BENCH-FINITE-001` - P2: helper
  `finiteBenchmarkSelectedTarget` checks only row 16 even though the configured
  selected set is rows 16..265. A nonfinite output in any other selected target
  would let the integrity gate and performance PASS stand.
- Required correction: check every sample of all 250 configured target rows on
  every warmup/timed result (outside the measured interval), report the failing
  row/sample if practical, and keep timer/default/scope unchanged. Amend/replace
  the commit with the same exact parent; populated execution remains manager-
  owned after response.

### REQUEST W-BENCH-001-REVISE

- To: retained visible Luna/max benchmark thread
  `019fd300-a9e2-7f93-8063-32450ff1d543`.
- Blocking: yes for benchmark integration.
- Scope: same one example-main file/parent. Replace row-16-only finite helper
  with all selected target rows 16..265; no timer/generator/config/default path
  or other-file change.
- Required response: amended replacement commit, exact comparison to rejected
  `79eff3b3a`, all-target oracle, diff/clean evidence and unchanged restrictions.
- Status: recorded and will be committed/pushed before follow-up dispatch.

### RESPONSE W-BENCH-001-REVISE-DISPATCH

- Result: retained visible thread
  `019fd300-a9e2-7f93-8063-32450ff1d543` accepted the follow-up with explicit
  Luna/max.
- Prompt preserves exact parent/one-file scope and changes only the finite
  helper/call to rows 16..265 after the timer. Generator/workload/default/
  timing/percentile/gate remain immutable.
- Coordination: manager will not poll or edit example main; await proactive
  replacement response.

### PUBLISH W-BENCH-001-REVISE

- GitHub issue #7 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/7#issuecomment-5195280130`.
- Connector write/API read-back exactly record P2 all-target gap, HOLD, narrow
  Luna/max correction and immutable timer/workload/default/scope constraints.

### RESPONSE W-BENCH-001-REVISE

- From / to: retained visible Luna/max benchmark thread
  `019fd300-a9e2-7f93-8063-32450ff1d543` / manager.
- Replacement: `b8ce427b8f58e213b5aab80f267fcce4a9b19bba` with exact
  parent `60b394ab2b30509e46b11c16ee3121d99488414b`; it replaces
  held commit `79eff3b3ac6119955c6288bd50030d515736f1a5`.
- Scope: only `src/examples/ex_causal_reference_denoising/main.cpp`; worker
  reports a clean detached worktree and passing committed diff checks.
- Correction: the post-timer integrity helper now scans every sample of every
  configured target row 16..265 and reports the first failing row/sample. It is
  used for all 100 warmup and 1000 timed results. Timer boundaries, generator,
  dimensions, configuration, counts, percentile rule, default teaching path
  and strict p95 gate are unchanged.
- Worker limitation: isolated Release execution remains blocked by the known
  ignored Eigen baseline gap; no vendor repair or runtime percentile is
  claimed. Luna/max, no subagent, polling, full scan or rt_server.
- Status: response is durable before manager provenance/diff review,
  integration or populated Release execution.

### MANAGER REVIEW W-BENCH-001-REVISE

- Provenance/scope: replacement `b8ce427b8f` has exact requested parent
  `60b394ab2b`, modifies only the focused example main and passes committed
  diff checks. Its delta from held `79eff3b3a` is 16 insertions/eight deletions
  confined to the finite-target helper and its diagnostic call site.
- Oracle: target indices 0..249 map exactly to rows 16..265; every column is
  checked after `steady_clock::now()` captures the process finish time. The
  first failing row/sample is surfaced, and the same verifier runs for all 100
  warmup plus 1000 timed calls.
- Frozen behavior: source generator, 270-row/P=64 configuration, iteration
  counts, preallocation, restore/timer boundaries, nearest-rank percentile,
  strict p95 gate and no-argument teaching path are unchanged.
- Decision: no remaining source finding. Accept for cherry-pick; populated
  Release build, default example and `--benchmark` execution remain mandatory
  before closing `R-BENCH-FINITE-001` or publishing GREEN.

### VALIDATION W-BENCH-001-REVISE

- Integration: replacement `b8ce427b8f` cherry-picked as `87ab418bd`; only
  the existing focused example main changes.
- Build/default: populated Visual Studio Release target
  `ex_causal_reference_denoising` recompiles/links. The no-argument teaching
  path exits zero with `example invariants: PASS`; its R/M/P, learning/freeze/
  reset and exact row-preservation behavior remains GREEN.
- Benchmark: an independent `Start-Process -Wait` invocation exits zero and
  reports p50 `2.725 ms`, p95 `4.317 ms`, max `8.557 ms`, final generation
  `1099`, accepted `1099`, rejected `0`, and `benchmark: PASS`.
- Gate: nearest-rank p95 is below the 128 ms acquisition duration by a wide
  margin. All 250 target rows are finite after every call, and every reference
  plus preserved row passes exact equality checks.
- Diagnostic note: two direct PowerShell pipeline invocations returned empty
  captured output; a child-process invocation isolated this as shell capture,
  not an application failure. No code change or performance claim was based on
  the empty capture.
- Restrictions: no full mne_scan, FIFF client link, vendor repair or rt_server.
- Decision: benchmark GREEN; `R-BENCH-FINITE-001` is closed and public issue #7
  evidence may be published after this record is committed/pushed.

### PUBLISH W-BENCH-001-GREEN

- GitHub issue #7 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/7#issuecomment-5195718600`.
- Connector write and CLI API read-back exactly record worker/integration/
  evidence commits, all-target integrity, Release default PASS, p50/p95/max,
  generation/update counts, strict gate PASS and execution restrictions.

### RETIREMENT PRECHECK W-BENCH-001

- Worker reported a clean detached exact replacement `b8ce427b8f`; manager
  verifies its sole example-main blob `86d1bc04f` exactly equals integrated
  HEAD.
- Review, populated Release default/benchmark runs and GitHub evidence are
  durable. This one-shot benchmark conversation may be app-archived without
  deleting or mutating its worktree.

### RESPONSE W-BENCH-001-RETIRE

- App archived completed visible thread
  `019fd300-a9e2-7f93-8063-32450ff1d543` successfully after exact content and
  GREEN prechecks.
- No manual deletion or mutation of the app-owned worktree occurred. Any later
  example/benchmark correction must use a new minimal Luna/max task.

### REQUEST W-QA-PROC-BOUNDARY-001

- From / to: manager / new visible Luna/max focused-test conversation.
- GitHub issue: `Edddddddddy/mne-cpp#3`; finding
  `R-PROC-BOUNDARY-001` (P3).
- Exact base: `8030221b51c5ba38d2f6d4c4e7c46c6420c055e7` on
  `codex/causal-reference-denoising`; separate Codex worktree.
- Authorized edit: only
  `src/testframes/test_adaptive_denoising_plugin/test_adaptive_denoising_plugin.cpp`.
  Do not edit production, CMake, queue, plugin, docs, dependencies or builds.
- Test task: add one data-driven public-interface QtTest slot proving Ready at
  each inclusive legal adapter boundary: taps 32, interval 2048, memory 1,
  memory 300, regularization 1, and exact feature count P=256. Each row uses a
  fresh processor, valid metadata/settings and exact expected R/M/P snapshot.
- P=256 oracle: eight good REF_MEG rows, one good MEG target, taps 32, valid
  remaining settings; expect Ready counts R/M/P = 8/1/256. Retain all existing
  P=288 invalid/disarm coverage unchanged.
- Prefer minimal accepted rows that isolate one boundary at a time. No large
  training run is required; configure/configuration behavior is the seam.
- Verification: compile/run the full populated focused target if dependencies
  exist in the worker; otherwise report the existing clean-worktree Eigen gap
  honestly. Run diff checks and return exact base/commit/file/oracles/results.
- Restrictions: gpt-5.6-luna/max, no nested subagents, manager polling, full
  repository scan, full mne_scan, vendor repair or rt_server. Proactively send
  `RESPONSE W-QA-PROC-BOUNDARY-001` and stop.
- Status: request durable before task creation.

### RESPONSE W-QA-PROC-BOUNDARY-001-CREATE

- Result: new visible Luna/max worktree setup accepted asynchronously as
  `client-new-thread:f247e307-5cb2-4444-ac8b-eb38e28233d9`; host `local`.
- Prompt requires realignment to exact worker base `8030221b5`, one focused
  test-source edit, all six inclusive Ready oracles, exact P=256 counts and
  preservation of existing over-cap/disarm cases.
- Coordination: manager will not poll or overlap the focused test source.
  Await only proactive structured response after public issue #3 dispatch.

### PUBLISH W-QA-PROC-BOUNDARY-001-START

- GitHub issue #3 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/3#issuecomment-5195752602`.
- Connector write and CLI API read-back exactly record the Luna/max visible
  worker, exact base/scope, six Ready boundaries, P=256 counts, preserved P=288
  disarm coverage and no-overlap/no-server restrictions.

### REQUEST W-QA-CORE-CONTRACT-001

- From / to: manager / new visible Luna/max core-contract conversation.
- GitHub issue: `Edddddddddy/mne-cpp#3`; finding
  `R-CORE-LOCALITY-001` (P3).
- Exact base: `7eb04cbfa6aa9cbd09594c6aa2d6fe6aebcc43db` on
  `codex/causal-reference-denoising`; separate Codex worktree.
- Authorized files only:
  `src/libraries/rtprocessing/causalreferencedenoiser.h` and
  `src/testframes/test_causal_reference_denoiser/test_causal_reference_denoiser.cpp`.
- Interface task: add compact Doxygen beside the public declarations covering
  rows-by-time shapes; unique/disjoint selected rows and P<=256; transactional
  allocation/exception behavior; only targets mutable; three mode semantics;
  state-preserving invalid/nonfinite calls; post-configure single-owner hot
  path with no allocation/lock/string/Qt/FIFF; reset semantics and fixed result.
- Ownership policy: explicitly declare noexcept default construction and delete
  copy/move construction/assignment. Do not invent move support, registry,
  getter, strategy or new public method.
- Compile contract: add namespace-scope C++14 `static_assert`s for nothrow
  default construction, non-copy/non-move traits, noexcept `reset()`, and
  noexcept `process()` using a valid `Eigen::MatrixXd`/Ref expression without
  runtime test or implementation edit.
- Verification: run complete focused Release/Debug if available, otherwise
  report the known isolated Eigen limitation honestly. Run diff/scope checks,
  commit exact-parent changes and return contract/traits/results.
- Restrictions: read full codebase-design and TDD skills; gpt-5.6-luna/max;
  no nested subagent, manager polling, implementation/CMake/dependency/docs/
  plugin/queue edit, full scan, full mne_scan, vendor repair or rt_server.
  Proactively send `RESPONSE W-QA-CORE-CONTRACT-001` and stop.
- Status: request durable before task creation.

### RESPONSE W-QA-CORE-CONTRACT-001-CREATE

- Result: new visible Luna/max worktree setup accepted asynchronously as
  `client-new-thread:77046610-99af-4ce6-a7de-2522cded7ccf`; host `local`.
- Prompt requires exact base `7eb04cbfa`, two authorized core header/test
  files, compact truthful contract, explicit noncopy/nonmove ownership and
  namespace-scope C++14 traits/noexcept checks with no runtime behavior change.
- Coordination: manager will not poll or overlap either authorized core file;
  await only the proactive structured response after public issue dispatch.

### RESPONSE W-QA-CORE-CONTRACT-001-PUBLISH-RETRY

- The previously available GitHub connector method was not callable in the
  current tool isolate (`TypeError: ... is not a function`) before any remote
  mutation occurred.
- Decision: record the failure, then use authenticated `gh issue comment` as
  the permitted fallback and read back the resulting issue comment before
  treating the dispatch as public.

### RESPONSE W-QA-PROC-BOUNDARY-001

- From / to: visible Luna/max processor-boundary worker / manager; source
  thread `019fd333-c569-79b1-b2c5-2d5343388357`.
- Exact base/commit: `8030221b51c5ba38d2f6d4c4e7c46c6420c055e7` /
  `203e33f7f0eb0a11d46056ab2bf61111aeb0d116`; parent is exact and the
  detached worktree is reported clean.
- Scope: only the focused plugin test source; one data-driven public-interface
  slot plus provider, insert-only 73 lines. Existing slots and all 19 invalid/
  missing/P=288 learned-model disarm cases remain unchanged.
- Ready rows: taps 32 -> R/M/P 1/1/32; interval 2048, memory 1, memory 300 and
  regularization 1 -> 1/1/4; exact P=256 uses eight good REF_MEG rows, one good
  MEG target and taps 32 -> 8/1/256. Every row uses a fresh processor and checks
  both configure result and committed snapshot.
- Worker verification: diff/scope checks pass. Isolated build is honestly not
  claimed because the ignored Eigen baseline is absent; no vendor repair.
  Luna/max, no subagent, polling, full scan/full mne_scan or rt_server.
- Status: response durable before manager provenance/code review, integration
  or populated focused execution.

### PUBLISH W-QA-CORE-CONTRACT-001-START

- GitHub issue #3 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/3#issuecomment-5199950147`.
- Authenticated `gh` fallback write and API read-back exactly match the visible
  Luna/max worker, exact base/two-file scope, compact contract, explicit
  ownership/traits and no-runtime/no-server restrictions.

### MANAGER REVIEW W-QA-PROC-BOUNDARY-001

- Provenance/scope: exact `203e33f7f` on requested `8030221b5`, one authorized
  focused-test file and 73 insertions. Committed diff check is clean; no
  production/CMake/dependency or existing-test deletion exists.
- Sensitivity: data rows isolate each inclusive UI maximum/minimum. The helper
  constructs exactly the requested number of good reference channels followed
  by one target; the P=256 row therefore exercises the numerical cap rather
  than a synthetic expected count. Both returned and committed snapshots must
  be Ready with exact R/M/P.
- Regression preservation: the new slot is additive; the prior P=288 invalid
  case and all learned-model disarm, queue and processor runtime slots remain.
- Decision: no finding. Accept for cherry-pick; complete populated Release
  focused execution remains required before closing the P3.

### VALIDATION W-QA-PROC-BOUNDARY-001

- Integration: worker `203e33f7f` cherry-picked as `34a346424`; only the
  focused plugin-test source changes.
- Populated Release: target `test_adaptive_denoising_plugin` recompiles/links;
  QTest text report exits zero with 14 passed, zero failed/skipped/blacklisted.
- Boundary evidence: separate PASS rows exist for taps 32, interval 2048,
  memory 1, memory 300, regularization 1 and P=256. All three prior processor
  behavior slots and all three queue slots remain PASS; stop wake is 61 ms
  against the 3000 ms timeout.
- Decision: `R-PROC-BOUNDARY-001` is closed. Publish issue #3 evidence after
  this validation is committed/pushed; no full scan, vendor repair or server.

### PUBLISH W-QA-PROC-BOUNDARY-001-GREEN

- GitHub issue #3 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/3#issuecomment-5199965770`.
- Authenticated `gh` write/API read-back exactly record commits, single-file
  scope, six boundary PASS rows, complete 14/0/0 Release result, prior coverage
  preservation, stop-wake evidence and restrictions.

### RETIREMENT PRECHECK W-QA-PROC-BOUNDARY-001

- Worker reported clean detached `203e33f7f`; its sole focused-test blob
  `dfe51c414` exactly equals integrated HEAD.
- Review, populated Release result and GitHub evidence are durable. The
  one-shot processor boundary thread may be app-archived without deleting or
  mutating its worktree.

### RESPONSE W-QA-PROC-BOUNDARY-001-RETIRE

- App archived completed visible thread
  `019fd333-c569-79b1-b2c5-2d5343388357` successfully after exact content and
  GREEN prechecks.
- No manual worktree deletion or mutation occurred; any later processor test
  edit must use a new minimal Luna/max task.

### REQUEST W-QA-PROC-LOCALITY-001

- From / to: manager / new visible Luna/max processor-contract conversation.
- GitHub issue: `Edddddddddy/mne-cpp#3`; finding
  `R-PROC-LOCALITY-001` (P3).
- Exact base: `16c9938724dbb455098a1bb69349d327b7f4817f` on the integration
  branch; separate Codex worktree.
- Authorized edit: only
  `src/applications/mne_scan/plugins/adaptivedenoising/adaptivedenoisingprocessor.h`.
- Add compact Doxygen local to descriptors/settings/results/class and methods:
  good REF_MEG/good MEG selection; inclusive ranges/defaults; single worker-
  thread ownership; Ready always resets; every returned invalid/missing/core
  rejection disarms and zeroes counts; configure may allocate/throw, with
  exceptions preserving prior state so caller must fail closed for new layout;
  armed noexcept process delegates and may only modify targets; disarmed process
  is exact pass-through NotConfigured; reset/configuration behavior and hot-path
  no allocation/lock/string/Qt/FIFF claims.
- Keep all declarations, defaults, enum values, special members and includes
  behaviorally unchanged. Do not add interface, implementation or test code.
- Verification: compare contract against SPEC/source and existing compiled
  C++14 ownership traits; run diff/scope checks. A build is optional because
  this is comments-only; do not claim runtime evidence not run.
- Restrictions: read full codebase-design skill; gpt-5.6-luna/max; no nested
  subagent, manager polling, other-file/build/dependency edit, full scan, full
  mne_scan, vendor repair or rt_server. Proactively send
  `RESPONSE W-QA-PROC-LOCALITY-001` and stop.
- Status: request durable before task creation.

### RESPONSE W-QA-PROC-LOCALITY-001-CREATE

- Result: new visible Luna/max worktree setup accepted asynchronously as
  `client-new-thread:134a7b33-95e7-45f2-9c34-dc184ec9cb1e`; host `local`.
- Prompt requires exact base `16c993872`, one header/comment-only scope,
  complete processor contract/locality audit and declaration-token preservation.
- Coordination: manager will not poll or overlap the processor header; await
  only proactive structured response after public issue #3 dispatch.

### PUBLISH W-QA-PROC-LOCALITY-001-START

- GitHub issue #3 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/3#issuecomment-5199978887`.
- Authenticated `gh` write/API read-back exactly record the comments-only
  Luna/max worker, base/scope, required local contract, declaration preservation
  and no-runtime/no-server restrictions.

### RESPONSE W-QA-PROC-LOCALITY-001

- From / to: visible Luna/max processor-locality worker / manager; source
  thread `019fd512-20ce-75d2-8bcc-892695e67352`.
- Exact base/commit: `16c9938724dbb455098a1bb69349d327b7f4817f` /
  `1d60ed4f761b998ef882e11ca5df731a11dcedfa`; parent is exact and the
  detached worktree is reported clean.
- Scope: only `adaptivedenoisingprocessor.h`, 68 comment insertions and no
  deletion. Worker reports no declaration/include/formatting or other-file edit.
- Contract locations cover the data-only mapping seam, defaults/ranges/P cap,
  fixed statuses/counts, worker ownership, returned-failure disarm, Ready reset,
  allocation-exception preservation plus caller fail-closed rule, armed/
  disarmed process semantics, target-only mutation, hot-path constraints,
  reset and committed snapshot.
- Declaration proof: base/commit header after comment+whitespace stripping have
  identical SHA-256 `25a643579ca9fa3cb4be10bb924c9e5b12326ef583fc34f7e595703ecb6add35`
  and normalized length 1922. Diff/scope checks pass; comments-only build was
  not claimed. Luna/max, no subagent/poll/full scan/full mne_scan/vendor/server.
- Status: response durable before manager contract/source review or integration.

### VALIDATION R-CORE-LINK-001-ATTEMPT-1

- Manager invoked only the populated Release `mne_rtprocessing` library target;
  no full application or server target was requested.
- Result: the wrapper timed out after 124 seconds with no captured build output;
  child `cmake`, `MSBuild` and `cl` processes continued and were stopped by
  exact PID to avoid an orphaned background build.
- Classification: inconclusive infrastructure attempt, neither GREEN nor the
  previously expected Qt/FIFF failure evidence. A later retry must use a narrow
  verbose/logged client-link target or bounded MSBuild invocation.

### VALIDATION R-CORE-LINK-001-ATTEMPT-2

- Manager directly invoked the generated Release
  `mne_rtprocessing.vcxproj` with `BuildProjectReferences=false`, one MSBuild
  worker and minimal logging; no full application/server target ran.
- Result: deterministic compile failure in installed Qt 5.15.2 headers under
  MSVC 18/14.51. `QtCore/qlist.h:915` and `qvector.h:960` reference missing
  `stdext::make_checked_array_iterator` (`C2653`, `C3861`).
- Triggering existing sources include `rtaoemeg.cpp`, `rtcov.cpp`,
  `rtinvop.cpp` and generated MOC code through existing FIFF/MNE/inverse Qt
  container equality instantiations. The new denoiser is not in the diagnostic
  chain; Eigen produces only known C4819 warnings.
- Classification: confirmed pre-existing toolchain/vendor incompatibility.
  Per plan, do not patch Qt/vendor or widen this feature. Retain focused direct-
  source Release/Debug/example/benchmark evidence and defer actual shared/static
  client-link smoke to a compatible Qt/MSVC environment.

### PUBLISH R-CORE-LINK-001-DEFER

- GitHub issue #3 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/3#issuecomment-5200077990`.
- Authenticated `gh` write was immediately corrected from a placeholder head to
  exact evidence commit `e6f4e48d9`; PATCH read-back confirms the final body.
- The public record exactly identifies the narrow project command, Qt/MSVC
  failure, unrelated existing instantiation paths, compatible-toolchain deferral
  and no-vendor/no-server decision.

### PUBLISH EPIC-MILESTONE-002

- GitHub epic #2 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/2#issuecomment-5200090627`.
- Authenticated `gh` write/API read-back update the public summary to focused
  14/0/0, example/benchmark numbers, closed processor P3s, environment-deferred
  core link, frozen queue-v2 seam and current active work.
- Child checkboxes remain intentionally open until complete issue-level gates;
  no partial milestone is presented as finished.

### DISCUSSION PLUGIN-INGRESS-002

- Evidence: production input owns `QSharedPointer<FiffInfo>`; `info()` returns a
  native handle, while first rows/columns appear only in DirectConnection data.
  Exact-row queue configuration cannot carry a row transition to the worker.
- Frozen seam: replace queue `std::shared_ptr` with
  `QSharedPointer<const FiffInfo>`; configure maximum dimensions; store/copy
  per-slot `rowCount` and `sampleCount`; accept any positive in-bounds shape.
- Plugin bounds: configure in `start()` at 512 channels x 2048 samples,
  capacity four (about 32 MiB slot matrices). Oversized/empty blocks are dropped
  once and counted; no callback resize/reconfigure/retry.
- Worker boundary: pop into a maximum-sized destination, copy the valid region
  into exact-sized worker storage, then apply metadata/settings/reset and model
  configuration. Row transitions and their metadata remain one FIFO item.
- Upstream constraint: `RealTimeMultiSampleArray::info()` briefly locks its own
  mutex once per notification. The plugin cannot remove that without changing
  a global measurement API; its own queue push remains zero-time and does no
  picks/model/settings work.
- Process: add Luna/max RED public-interface coverage, then Sol/ultra queue v2
  implementation and a fresh formal review. The pending original queue review
  may add concurrency findings but no longer blocks freezing this caller seam.

### DISCUSSION PLUGIN-LIFECYCLE-001

- Module: one concrete `AdaptiveDenoising` AbstractAlgorithm adapter owns the
  queue, worker-owned processor/matrix, pending GUI snapshot, atomic drops and
  RTMSA connectors; no framework/base/registry extension.
- Callback: one `info()` snapshot per notification plus exactly one `tryPush`
  per matrix; no output/UI init, FIFF picks, settings/model/reset work, retry or
  wait. Any non-Pushed status increments dropped blocks.
- Worker boundary: consume pending settings/reset, convert FIFF, compare
  metadata/fs/rows/block width/settings, configure/reset/process, initialize
  output metadata and emit the block/diagnostics in FIFO order.
- Exception policy: a configure allocation exception preserves old processor
  ownership, so plugin must catch, deliberately disarm through invalid metadata,
  report fixed exception status and pass the new block through unchanged.
- Modes/UI: disabled/frozen/enabled map to Bypass/ApplyOnly/ApplyAndLearn;
  GUI-worker settings mutex is never used by callback; diagnostics use queued
  signals. Minimal target dependencies exclude unrelated noise-reduction libs.

### PUBLISH PLUGIN-LIFECYCLE-001

- GitHub issue #6 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5200101632`.
- Authenticated `gh` body was immediately PATCHed from a generic current-head
  phrase to exact design commit `3054015b7`; API read-back confirms the final
  callback/worker/exception/mode/dependency contract and restrictions.

### PUBLISH PLUGIN-INGRESS-002

- GitHub issue #6 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5200042268`.
- Authenticated `gh` write/API read-back exactly record native ownership,
  variable-row maxima/extents, 512x2048x4 bounds, upstream accessor constraint,
  TDD/review workflow and unchanged global/plugin/server scope.

### REQUEST W-QUEUE-V2-TEST-001

- From / to: manager / new visible Luna/max queue-v2 test conversation.
- GitHub issues: implementation gate `#5`, plugin caller decision `#6`.
- Exact base: `b4299fdafd9e61779ff2a1482e74ae8a694b7bb4`; separate Codex
  worktree.
- Authorized edit: only
  `src/testframes/test_adaptive_denoising_plugin/test_adaptive_denoising_plugin.cpp`.
  No production/CMake/docs/dependency/build edit.
- Update existing queue test call sites/oracles to the frozen interface:
  `maxChannelCount`, `QSharedPointer<const FiffInfo>`, queued `rowCount` plus
  `sampleCount`. Preserve processor tests and every prior queue behavior.
- Add one public-interface slot configured at max 4x4/capacity three. Push
  2x3, 4x2 and 3x4 matrices with distinct native Qt metadata handles; mutate
  originals; pop into a preallocated 4x4 sentinel destination and prove FIFO,
  exact valid top-left deep copies, exact row/sample extents, untouched tail,
  metadata identity and final Timeout.
- Metadata test must not construct/dereference/link FIFF: use distinct non-null
  fake pointers backed by test tokens plus explicit no-op deleters in
  `QSharedPointer<const FiffInfo>`. Add C++14 nothrow QSharedPointer copy/move
  traits if supported by Qt 5.15.2; do not weaken a failing trait.
- Expected RED: current queue header lacks native handle/max rows/rowCount.
  Manager will build the integrated tracer in the populated workspace before
  Sol implementation. Worker reports isolated dependency gaps honestly.
- Restrictions: read full TDD skill; Luna/max; no nested subagent, manager
  polling, production edit, full scan, full mne_scan, vendor repair or
  rt_server. Proactively send `RESPONSE W-QUEUE-V2-TEST-001` and stop.
- Status: request durable before task creation.

### RESPONSE W-QUEUE-V2-TEST-001-CREATE

- Result: new visible Luna/max worktree setup accepted asynchronously as
  `client-new-thread:94fce46e-d2a5-49d0-a276-f3a395fd0e60`; host `local`.
- Prompt requires exact base `b4299fdaf`, one focused-test file, migration of
  every existing queue oracle plus new mixed-row/native-metadata FIFO tracer,
  explicit expected compile RED and no production edit.
- Coordination: manager will not poll or overlap the focused test source; await
  only proactive structured response after public issue #5 dispatch.

### PUBLISH W-QUEUE-V2-TEST-001-START

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5200050057`.
- Authenticated `gh` write/API read-back exactly record Luna/max task/base/
  single-file scope, migrated/native variable-row oracles, expected compile RED
  and no-production/no-server restrictions.

### MANAGER REVIEW W-QA-PROC-LOCALITY-001

- Provenance/scope: exact `1d60ed4f7` on requested `16c993872`; one authorized
  processor header, 68 insertions, no deletion and clean committed diff.
- Declaration integrity: direct full diff shows only Doxygen additions; it is
  consistent with the worker's identical stripped-token hash and no include,
  enum, default, special-member, signature or member-layout change.
- Contract audit: selection, UI ranges/P cap, Ready fresh-state semantics,
  returned-failure disarm/zero counts, exception preservation/fail-closed
  caller obligation, target-only/error pass-through, hot-path constraints,
  reset and snapshot claims match processor/numerical source and SPEC.
- Finding: none. The concrete adapter becomes more local/deep without a new
  seam or behavior. Accept for cherry-pick; a later complete focused regression
  will compile the unchanged interface, but no comments-only runtime gate is
  required for integration.

### VALIDATION W-QA-PROC-LOCALITY-001

- Integration: worker `1d60ed4f7` cherry-picked as `7f4095a3b`; exact scope is
  the processor public header and declaration tokens remain unchanged.
- Evidence: source/SPEC audit plus clean comment-only diff proves the interface
  now carries its selection, ranges, ownership, disarm/reset, exception and
  hot-path contract locally. Existing compiled ownership traits and the prior
  14/0/0 runtime apply to the unchanged declarations/behavior.
- Decision: close P3 `R-PROC-LOCALITY-001`; publish issue #3 after this record
  is committed/pushed, then verify blob equality and archive the one-shot task.

### PUBLISH W-QA-PROC-LOCALITY-001-GREEN

- GitHub issue #3 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/3#issuecomment-5200025095`.
- Authenticated `gh` write/API read-back exactly record commits, comments-only
  scope, local contract, identical declaration hash and unchanged prior tests.

### RETIREMENT PRECHECK W-QA-PROC-LOCALITY-001

- Worker reported clean detached `1d60ed4f7`; its sole processor-header blob
  `60ee16231` exactly equals integrated HEAD.
- Review and public evidence are durable. The one-shot comments worker may be
  app-archived without deleting or mutating its worktree.

### RESPONSE W-QA-PROC-LOCALITY-001-RETIRE

- App archived completed visible thread
  `019fd512-20ce-75d2-8bcc-892695e67352` after exact content/evidence checks.
- No manual worktree deletion or mutation occurred. Later processor header
  work requires a new minimal Luna/max task.

### RESPONSE W-EXAMPLE-001

- From / to: visible Luna/max example thread
  `019fd2e5-43ca-79c3-822c-20ef795608f1` / manager.
- Exact base/commit: `473d9eeeda55ab280771b24b2a0b2b7b4e803995` /
  `708db5425ad0ae8c653ad3df3f29ff9187ace1ba`; parent is the requested base.
- Scope: examples registration plus new focused example CMake/main only; clean
  detached worker state and diff checks pass.
- Stream: four channel rows, two references, one target and one preserved row;
  1000 Hz, four taps, 128-sample epochs/blocks, 30 s memory, 1e-3 loading,
  therefore R/M/P=2/1/8. Deterministic global-index reference functions feed
  tap-major environmental weights plus a clean 7 Hz sinusoid.
- Stages: two-sample Bypass warmup; five continuous ApplyAndLearn blocks;
  continuing ApplyOnly freeze; reset followed by one-sample Bypass. Each stage
  asserts status, row preservation, finite target, diagnostics/events and the
  expected generation/warmup invariants, exiting nonzero on violation.
- Dependencies: directly compiles the numerical source, includes libraries,
  defines the rtprocessing export macro and links only Qt Core plus Eigen; no
  rtprocessing/FIFF/scan/scShared/scMeas link.
- Worker evidence: isolated configure is blocked before target generation by
  the known ignored Eigen baseline gap, with no dependency/vendor repair or
  runtime claim. Luna/max, no subagent, polling, full scan or rt_server.
- Status: response recorded before manager provenance/diff review or integration.

### MANAGER REVIEW W-EXAMPLE-001

- Provenance: exact `708db5425` on requested `473d9eeed`; clean three-file
  examples-only diff with 407 insertions and no numerical/plugin/test/docs or
  dependency change.
- Seam: target directly compiles the concrete numerical source, exposes only
  the existing public header, links Qt Core/Eigen and uses the repository's
  existing example install/output conventions. No FIFF/scan/client-link claim
  is hidden; that separate final-QA P3 remains explicit.
- Teaching behavior: rows-by-time construction, two causal reference formulas,
  tap-major weights and global sample index make cross-block history visible.
  Assertions sensitively cover exact preserved rows, finite target, statuses,
  R/M/P diagnostics, warmup, future learning generation, frozen zero events
  and reset generation/history.
- Finding: none. Accept for cherry-pick; populated Release build/run and exact
  console output remain mandatory before publishing GREEN or retiring worker.

### VALIDATION W-EXAMPLE-001

- Integration: worker `708db5425` cherry-picked as `26ca3b73e`.
- Configure/build: populated Visual Studio Release configuration enables
  examples, then target `ex_causal_reference_denoising` alone compiles/links;
  only known Eigen C4819 code-page warnings occur.
- Runtime: hidden synchronized process exits zero and ends with
  `example invariants: PASS`. Learning stage generations are 0,1,2,3,4 with
  one accepted update and zero rejects on stages two through five. ApplyOnly
  holds generation four with zero call-local events; reset/bypass reports
  generation zero and warmup two.
- Output evidence: R/M/P=2/1/8; post-model target RMS is about 0.31-0.34 while
  input RMS is about 1.17-1.41, and every built-in exact preservation/finite
  output assertion passes. These values are illustrative, not a new effect gate.
- Restrictions: no full mne_scan, rt_server, FIFF client link or vendor repair.
- Decision: focused example GREEN; publish issue #7 evidence, content-verify
  and archive the one-shot worker after the evidence is durable.

### PUBLISH W-EXAMPLE-001-GREEN

- GitHub issue #7 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/7#issuecomment-5195038508`.
- Connector write and CLI API read-back exactly match worker/integration
  commits, three-file scope, Qt Core/Eigen seam, populated Release success,
  stage/generation/RMS evidence, illustrative-only interpretation and no full
  scan/rt_server/vendor-repair restrictions.
- Status: public GREEN durable; worker content/retirement verification remains.

### PUBLISH W-QUEUE-TEST-003-GREEN

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5195069942`.
- Connector write and API read-back exactly match worker/integration commits,
  8/0/0 Release, invalid-state/null-metadata recovery, lifecycle regression,
  complete focused coverage, restrictions and the next formal review gate.

### RETIREMENT PRECHECK W-QUEUE-TEST-003

- Worker reported clean detached exact `cff55a01e`; manager verifies its sole
  focused-test blob `f13fbec9` exactly equals integrated HEAD.
- All three Luna test slices are durable, GREEN and published. The reusable
  thread has completed its final authorized queue task and may be archived by
  the app without deleting or modifying its worktree.

### RETIREMENT PRECHECK W-EXAMPLE-001

- Worker reported clean detached exact `708db5425`; manager verifies all three
  authorized blobs exactly equal integrated HEAD: registration `3ab9d165`,
  CMake `d9912a44`, main `ccdcdbf4`.
- Review, populated Release PASS and GitHub evidence are durable. The one-shot
  example thread may be app-archived without deleting/modifying its worktree.

### RESPONSE W-QUEUE-TEST-003-RETIRE

- App archived completed visible thread
  `019fd2e0-188f-7a62-a89c-7ceefa15f6cd` successfully.
- No manual worktree deletion or mutation occurred; implementation thread
  `019fd2bd-b720-7b30-8d65-780a0b500924` remains available only for review-
  driven narrow fixes.

### RESPONSE W-EXAMPLE-001-RETIRE

- App archived completed visible thread
  `019fd2e5-43ca-79c3-822c-20ef795608f1` successfully.
- No manual worktree deletion or mutation occurred. Later benchmark work must
  use a new minimal Luna/max task rather than reviving this one-shot example.

### REQUEST R-QUEUE-001

- From / to: manager / new independent visible review conversation.
- GitHub issue: `Edddddddddy/mne-cpp#5`.
- Model/environment: `gpt-5.6-sol`, `ultra`, new saved-project worktree from
  the exact current integration branch; no internal/nested subagents.
- Mode: read-only formal code review. Do not edit, stage, commit, push, mutate
  GitHub, repair dependencies, run full mne_scan or run rt_server.
- Required recovery: read `STATE.md`, `SPEC.md`, complete relevant queue
  REQUEST/RESPONSE sections in `WORKERS.md`, `REVIEW.md`, and `JOURNAL.md`
  tail. Read the complete `codebase-design` skill and relevant deepening guide.
- Exact review surface: queue header/source, focused plugin-test CMake and all
  queue QtTest slots. Inspect numerical/processor/plugin plans only enough to
  judge the seam and intended caller lifecycle.
- Review questions: concrete-module depth/locality; SPSC ownership assumptions;
  free/filled semaphore and ring-index publication; stop races before/during
  push/pop and pending-data discard; stopped reconfigure/destruction quiescence;
  transactional configure; matrix/metadata FIFO/deep-copy lifetime; shared_ptr
  operations; drop-newest; invalid-operation state preservation; timeout edge
  behavior; truthful noexcept; and hot-path no-allocation/no-lock/no-retry claims.
- Evidence: independently inspect exact snapshot/diff ancestry and durable 8/0/0
  report. A fresh build is optional if the known isolated Eigen gap exists; do
  not claim manager runtime as independently reproduced.
- Required response: `RESPONSE R-QUEUE-001` with reviewed SHA and clean status,
  prioritized P0-P3 findings (ID, exact file/line, evidence, impact, correction,
  required test), explicit P0/P1/P2 counts, residual plugin-lifecycle risks and
  PASS/HOLD gate recommendation. Proactively notify manager and stop; no polling.
- Lifecycle: one-shot independent reviewer; archive after response/evidence is
  durable and all blocking findings are resolved or assigned.
- Status: recorded and will be committed/pushed before task creation.

### RESPONSE R-QUEUE-001-CREATE

- Result: new visible Sol/ultra review worktree setup accepted asynchronously.
- Client setup ID:
  `client-new-thread:1030c8cd-96dd-4ad6-a650-927118f3d03f`; host `local`.
- Exact review snapshot: `3d73286836c8a660649c030167ba7d12e10fb572`.
- Prompt integrity: recovery order, full queue/test review, codebase-design
  vocabulary, concurrency/lifecycle/realtime questions, P0-P3 evidence schema,
  PASS/HOLD and read-only/no-subagent/no-polling/no-full-scan/no-rt_server
  restrictions are present.
- Coordination: no final thread ID yet; manager will not poll. Await only the
  proactive `RESPONSE R-QUEUE-001` after public dispatch.

### PUBLISH R-QUEUE-001-START

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5195120967`.
- Connector write and API read-back exactly match Sol/ultra setup ID, snapshot,
  review surface/questions, P0-P3 gate and read-only restrictions.
- Status: public dispatch complete; manager will not poll the reviewer.

### DISCUSSION PLUGIN-INGRESS-001

- GitHub issue #6 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5195227638`.
- Connector write/API read-back exactly record the real QSharedPointer versus
  std::shared_ptr ownership mismatch, first-callback exact-shape bootstrap and
  row-count transition visibility risk.
- Decision: no speculative plugin source until formal queue review and a
  smallest-seam follow-up; no AbstractAlgorithm/global measurement change.

### RESPONSE W-QUEUE-V2-TEST-001

- From / to: visible Luna/max queue-v2 tracer conversation
  `019fd51d-9b99-7e63-b257-8c76b9960134` / manager.
- Exact base/commit:
  `b4299fdafd9e61779ff2a1482e74ae8a694b7bb4` /
  `649bbd1ee10bb884a6f30423a5bd2739e432097f`; worker reports an exact-parent,
  clean detached commit and passing diff check.
- Scope: only
  `src/testframes/test_adaptive_denoising_plugin/test_adaptive_denoising_plugin.cpp`;
  production queue/CMake/processor/core/plugin/docs/vendor remain unchanged.
- Migration: every queue configuration uses `maxChannelCount`; metadata uses
  native `QSharedPointer<const FIFFLIB::FiffInfo>`; popped results assert both
  `rowCount` and `sampleCount`; invalid destinations must preserve extents,
  matrix and metadata sentinels. All prior processor and queue behaviors remain.
- New oracle: configure maxima 4x4/capacity three; push and then mutate 2x3,
  4x2 and 3x4 matrices carrying three distinct non-null fake native metadata
  identities. A single preallocated 4x4 destination must pop them FIFO with
  exact valid rectangles/extents, untouched sentinel tail and matching pointer
  identity, then return Timeout.
- Ownership traits: Qt 5.15.2 same-type `QSharedPointer` copy construction and
  assignment were inspected and namespace-scope C++14 traits require both to
  be nothrow. The fake FiffInfo pointers use explicit no-op deleters and are
  never constructed, dereferenced or linked.
- Worker verification: isolated configure stopped at the known ignored Eigen
  baseline gap before target generation. No vendor repair or build/run claim;
  expected populated RED is the current production's exact-row/std ownership
  interface lacking the v2 names and extents.
- Restrictions: Luna/max, no internal/nested subagent, manager polling, full
  scan, full mne_scan, vendor change or rt_server.
- Status: response recorded before manager provenance/diff review,
  cherry-pick or populated RED.

### MANAGER REVIEW W-QUEUE-V2-TEST-001

- Provenance: exact worker commit `649bbd1ee` has required parent
  `b4299fdaf`, which is an ancestor of integration HEAD. Its committed diff
  passes `git diff --check` and changes exactly the authorized focused test
  source. The parent and current integration test blobs are both
  `dfe51c414`, so no intervening same-file change will be overwritten.
- Migration completeness: all existing queue configuration, destination,
  metadata and pop assertions move to the frozen v2 public names; prior
  processor and queue behavior slots are preserved. Invalid source/destination
  shapes remain invalid relative to configured maxima and now also prove
  extent/metadata sentinel preservation.
- New behavior sensitivity: three different row/sample rectangles fill the
  full capacity before source mutation; FIFO pops into one maximum-sized
  sentinel matrix require exact top-left copies, exact extents, unchanged
  outside cells and native metadata pointer identity. Any exact-row queue,
  shallow/deferred copy, stale extent, whole-slot overwrite or metadata
  mismatch fails observably.
- Test seam: fake non-null FiffInfo addresses stay live for the test duration,
  use explicit no-op deletion, and are only compared, never dereferenced.
  Compile-time nothrow traits make the intended callback ownership assumption
  explicit for the installed Qt 5.15.2 type.
- Finding: none. Accept for cherry-pick and populated public-contract RED; no
  queue-v2 production implementation or GREEN is accepted yet.

### MANAGER VALIDATION W-QUEUE-V2-TEST-001-RED

- Integration: worker `649bbd1ee` cherry-picked as `c646e35d2`; only the
  focused test source changes and all existing slots remain in that source.
- Command: populated Release build of target
  `test_adaptive_denoising_plugin` in `build-causal-reference-denoising`.
- Result: expected compile failure after target generation. MSVC reports that
  `AdaptiveDenoisingBlockQueueConfig` has no `maxChannelCount`,
  `AdaptiveDenoisingQueuedBlock` has no `rowCount`, and native
  `QSharedPointer<const FiffInfo>` cannot bind to the old `std::shared_ptr`
  push/slot interface. The errors occur throughout the migrated prior oracles
  and new mixed-row slot.
- Classification: valid public-interface RED against the frozen queue-v2 seam.
  Known Eigen code-page warnings are nonblocking; no FIFF/full-scan/vendor/
  server failure masks the result.
- Decision: production GREEN may now be dispatched as a separate Sol/ultra
  task. The test remains immutable until queue-v2 is implemented and reviewed.

### PUBLISH W-QUEUE-V2-TEST-001-RED

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5200141019`.
- Authenticated `gh` write/API read-back exactly records worker/integration/
  evidence commits, one-test-file scope, the three public-interface compiler
  failures, valid TDD RED classification and no full-scan/server/vendor work.

### REQUEST W-QUEUE-V2-GREEN-001

- From / to: manager / new visible queue-v2 implementation conversation.
- GitHub issue: `Edddddddddy/mne-cpp#5`.
- Model/environment: `gpt-5.6-sol`, `ultra`, new saved-project worktree from
  the exact integration branch after this request is committed/pushed; no
  internal/nested subagents or manager polling.
- Model decision: this slice owns SPSC publication, stop races, native shared
  ownership and callback hot-path guarantees, so it requires Sol/ultra.
- TDD goal: make the integrated queue-v2 public-contract tracer GREEN without
  editing any test. Preserve all older queue/processor behaviors in the same
  focused executable.
- Interface: replace exact `channelCount` with positive `maxChannelCount`;
  `AdaptiveDenoisingQueuedBlock` reports `rowCount` and `sampleCount`; metadata
  and `tryPush` use `QSharedPointer<const FIFFLIB::FiffInfo>`. Keep one concrete
  final PImpl module with configure/tryPush/waitPop/stop and deleted copy/move;
  add no strategy, registry, base class or FIFF dereference.
- Allocation/state: configure validates positive maxima/capacity within
  QSemaphore's integer range, allocates every slot as
  `maxChannelCount x maxBlockSamples` transactionally and publishes a fresh
  running empty queue only after success. An active configure returns
  AlreadyRunning. Reconfigure/destruction require producer/consumer quiescence.
- Producer: accept only `0 < rows <= maxChannelCount` and
  `0 < cols <= maxBlockSamples`; do exactly one zero-time free-slot acquire;
  Full/invalid/stopped must not write or advance. Copy the exact top-left valid
  rectangle, store both extents and the native metadata handle, then publish
  one filled token. No resize, retry, busy wait, string or FIFF work.
- Consumer: require the destination matrix exactly equals configured maxima;
  do one timed filled-slot acquire. On Popped copy only the valid top-left
  rectangle and both extents/metadata, leaving the destination tail untouched;
  clear the slot handle, advance once and release one free token. Invalid/
  timeout/stopped paths preserve caller destination state.
- Stop/races: atomic idempotent stop prevents later pushes and wakes a blocked
  consumer without reading a slot; pending blocks may be discarded. Audit
  stop before/during acquired push/pop and avoid publishing/releasing a token
  that can corrupt a later fresh configuration. Caller quiescence remains an
  explicit interface constraint before replacing/destroying the PImpl.
- Realtime/noexcept: after configure, tryPush/waitPop/stop must allocate no
  heap memory, use no explicit mutex and create no strings. QSharedPointer
  copies reuse the native control block; verify the installed Qt C++14
  nothrow assumptions and keep declared noexcept truthful.
- Authorized files only:
  `src/applications/mne_scan/plugins/adaptivedenoising/adaptivedenoisingblockqueue.h`
  and `.cpp`. Focused CMake already compiles the source; change it only if a
  demonstrable build requirement exists and report that before doing so.
- Required validation: exact diff/scope/clean checks; focused Release build/run
  if the isolated worktree permits, otherwise report the known ignored-Eigen
  limitation honestly. Do not copy/repair dependencies. Manager owns populated
  GREEN and a fresh independent Sol/ultra queue review.
- Required response: begin `RESPONSE W-QUEUE-V2-GREEN-001`; include exact
  base/commit, complete interface/state/race/ordering/no-allocation argument,
  changed files, build evidence/limitation and clean/no-subagent/no-poll/no-
  full-scan/no-rt_server confirmation. Proactively notify manager and stop.
- Lifecycle: retain only until manager integration/GREEN and review-driven fix
  determination; archive if no same-context correction remains.
- Status: recorded before visible task creation.

### SYSTEM FAILURE W-QUEUE-V2-GREEN-001-CREATE-1

- The first app call was rejected locally as `invalid arguments` because the
  project ID was supplied both at the top level and inside the documented
  target union. No task, worktree, repository, GitHub or dependency mutation
  occurred.
- Decision: retry once using the exact tool schema with project ID only inside
  the worktree target. This is not a duplicate implementation dispatch.

### RESPONSE W-QUEUE-V2-GREEN-001-CREATE

- Result: corrected visible saved-project worktree setup accepted
  asynchronously.
- Client setup ID:
  `client-new-thread:97461215-c1b8-488b-90bd-984aad5d9fd3`; host `local`.
- Model: `gpt-5.6-sol`, `ultra`, satisfying the explicit high-risk concurrency
  assignment.
- Exact requested base:
  `9b526eb1429b3e1292a0c58fae3d73fde34eb97c`.
- Prompt integrity: recovery/skills, immutable public RED, exact two-file
  scope, native ownership/maximum extents/top-left copy, transactional
  allocation, SPSC semaphore/index and stop-race audit, no-allocation/noexcept
  proof and all no-subagent/no-poll/no-full-scan/no-rt_server restrictions are
  present.
- Coordination: setup returned no final thread ID. Manager will not poll and
  awaits only the proactive structured RESPONSE after public dispatch.

### RETIREMENT REQUEST W-QUEUE-V2-TEST-001

- User reconfirmation: clean up completed visible work conversations; preserve
  only conversations that remain useful for the same responsibility.
- Target: one-shot Luna/max tracer thread
  `019fd51d-9b99-7e63-b257-8c76b9960134` after verifying its worktree is clean
  at exact `649bbd1ee`, its sole test blob equals integrated `c646e35d2`, and
  worker/RED/GitHub evidence is durable.
- Safety: archive through the app only; do not manually delete or mutate the
  app-owned worktree. Keep the newly created Sol/ultra queue-v2 implementation
  task until GREEN plus fresh review determines whether a fix follow-up exists.

### PUBLISH W-QUEUE-V2-GREEN-001-START

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5200762566`.
- Authenticated write/API read-back exactly records Sol/ultra model, exact base,
  setup ID, immutable RED, two-file scope, frozen variable-shape/native-owner
  interface, realtime/race gates and proactive/no-polling restrictions.

### RETIREMENT PRECHECK W-QUEUE-V2-TEST-001

- App-owned worker path resolved read-only as
  `C:/Users/lcy/.codex/worktrees/0db3/mne-cpp-main`.
- It is clean/detached at exact `649bbd1ee`; its sole focused-test blob is
  `c74670899`, exactly equal to integration HEAD after `c646e35d2`.
- Worker response, manager review, populated compile RED and both issue #5
  publications are durable. One-shot thread
  `019fd51d-9b99-7e63-b257-8c76b9960134` may be app-archived; no manual
  worktree deletion or mutation is authorized.

### RESPONSE W-QUEUE-V2-TEST-001-RETIRE

- App archived completed Luna/max tracer thread
  `019fd51d-9b99-7e63-b257-8c76b9960134` successfully after exact clean/blob,
  populated RED and GitHub evidence became durable.
- No app-owned worktree was manually deleted or modified. Future queue tests
  require a new minimal Luna/max conversation; the Sol/ultra production task
  remains active for the same-context implementation/review loop.

### REQUEST W-PLUGIN-DATA-001

- From / to: manager / new visible plugin-data lifecycle conversation.
- GitHub issue: `Edddddddddy/mne-cpp#6`.
- Model/environment: `gpt-5.6-sol`, `ultra`, new saved-project worktree from
  the exact integration branch after this request is committed/pushed; no
  internal/nested subagents or manager polling.
- Parallel-scope decision: this task must not edit the active queue header/
  source or focused tests. It adds plugin registration/new plugin files only
  and codes against the queue-v2 interface already frozen in SPEC. Manager
  integrates queue-v2 GREEN before this plugin commit.
- Vertical goal: create the real `adaptivedenoising`/
  `scan_adaptivedenoising` AbstractAlgorithm adapter with the default numerical
  settings and complete input-queue-worker-output lifecycle. No teaching UI,
  pending settings, freeze/reset controls or diagnostics widget in this slice.
- Build/module files: register `add_subdirectory(adaptivedenoising)` in the
  plugin root; add narrow CMake, empty Qt plugin metadata JSON, build-info
  global header/source and `adaptivedenoising.h/.cpp`. Target sources include
  the existing queue and processor implementations; link only Qt Core/Widgets,
  `mne_utils`, `mne_fiff`, `mne_rtprocessing`, Eigen, `scShared`, `scMeas`.
  No icon/resource or dependency copied from noisereduction is required.
- Identity/interface: one concrete `AdaptiveDenoising : AbstractAlgorithm`
  with standard Q_OBJECT/Q_PLUGIN_METADATA/Q_INTERFACES, fresh `clone()`, name
  `Adaptive Denoising`, `_IAlgorithm`, build info and minimal setup widget.
  Do not change AbstractAlgorithm, noisereduction or add a registry/base seam.
- `init()`: create one RTMSA input and RTMSA output connector; DirectConnection
  notify targets `update()`. Set output name. Do not allocate/configure queue in
  callback or initialize GUI/output from callback.
- `start()`: if not already running and no prior producer is in flight,
  configure queue maxima 512x2048/capacity four, preallocate the maximum queued
  destination, reset worker-only processor/output-layout state, start the
  QThread, then publish the atomic accepting-input gate. A failed queue config
  returns false without starting.
- `update()`: double-check an atomic accepting-input/in-flight producer guard;
  dynamically accept RTMSA, call `info()` once per notification, then perform
  exactly one queue `tryPush(matrix, info)` for each matrix. No retry, wait,
  busy loop, FIFF pick, model/settings/reset/output/UI work or plugin mutex.
  Increment atomic `droppedBlocks` on every non-Pushed result.
- `stop()`: clear accepting input first, stop/wake the queue, request worker
  interruption, wait for worker exit, then confirm an already-entered producer
  has left before a later start may reconfigure the PImpl. Use atomic callback
  entry/exit and bounded lifecycle-side synchronization; acquisition never
  waits. Stop is idempotent and clears output after quiescence.
- `run()`: waitPop with a finite interruptible timeout; on each Popped block,
  copy only returned extents into a worker-owned exact `MatrixXd`. Treat null
  metadata or inconsistent `nchan`/`chs.size()`/row count/nonfinite-positive
  sampling rate as invalid and never apply an old model. For valid metadata,
  build the data-only descriptor from `chs[row].kind` and
  `bads.contains(ch_name)` only on the worker.
- Reconfigure: new metadata pointer, sampling frequency, row count, block
  width or initial state configures a fresh processor with default 4 taps,
  interval 128, memory 30 s and regularization 1e-3. Invalid/missing layout
  disarms via processor contract and forwards unchanged. If configure throws,
  catch on worker, deliberately disarm using an invalid descriptor and forward
  unchanged; never use preserved old ownership on new metadata.
- Process/output: v1 default mode is ApplyAndLearn. Only processor-selected
  targets may change. Initialize/reinitialize RTMSA output on new valid
  metadata and emit exact blocks FIFO with multi-array size one. Queue metadata
  stays `QSharedPointer<const FiffInfo>`; the sole permitted const-cast is the
  legacy output `initFromFiffInfo` call, and metadata is never mutated. Null
  metadata cannot safely initialize output and is not emitted as a mislabeled
  block.
- Authorized files only: plugin-root `CMakeLists.txt` plus new/previously
  absent `adaptivedenoising/{CMakeLists.txt,adaptivedenoising.h,
  adaptivedenoising.cpp,adaptivedenoising_global.h,
  adaptivedenoising_global.cpp,adaptivedenoising.json}`. Existing processor/
  queue files are read-only. No UI form/setup class, tests, core/docs/vendor.
- Verification: exact scope/diff/clean checks and static contract audit. Attempt
  only target-local configure/build if possible; current MSVC14.51/Qt5.15.2
  may fail in existing mne_fiff/rtprocessing dependencies. Report that exact
  environment limitation without vendor repair or false GREEN. Never run full
  mne_scan or mne_rt_server.
- Required response: `RESPONSE W-PLUGIN-DATA-001` with exact base/commit,
  file/dependency/interface list, callback and stop-race proof, descriptor/
  configure/disarm/output behavior, evidence/limitation, clean status and
  Sol/ultra/no-subagents/no-manager-poll/no-full-scan/no-rt_server confirmation.
  Proactively notify manager and stop.
- Lifecycle: retain until manager integration and plugin-data review determine
  whether a same-context correction is needed.
- Status: recorded before visible task creation.

### RESPONSE W-PLUGIN-DATA-001-CREATE

- Result: visible saved-project worktree setup accepted asynchronously.
- Client setup ID:
  `client-new-thread:bb93c4e1-6aa9-4ee4-9d52-de30ff1b0489`; host `local`.
- Model: `gpt-5.6-sol`, `ultra`, matching the concurrency/lifecycle risk.
- Exact requested base:
  `8c51ea4ce86bdd6a377f7f87e4ef693de4893491`.
- Prompt integrity: exact seven-file scope, read-only queue/processor/tests,
  narrow target dependencies, complete connector/start-update-stop/run/FIFF/
  exception/output contract, atomic producer quiescence, no UI and all no-
  subagent/no-poll/no-full-scan/no-rt_server restrictions are present.
- Coordination: no final thread ID returned. Manager will not poll and awaits
  only the proactive structured RESPONSE; queue-v2 integrates first.

### PUBLISH W-PLUGIN-DATA-001-START

- GitHub issue #6 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5200889469`.
- Authenticated write/API read-back exactly records Sol/ultra model/base/setup,
  seven-file read-only boundaries, narrow target, callback/producer/worker/
  metadata lifecycle, queue-first integration, UI non-goal and all restrictions.
- Status: dispatch is public; manager will not poll or overlap its files.

### RETIREMENT REQUEST R-QUEUE-001

- Reason: the read-only Sol/ultra review target is the superseded queue-v1
  snapshot `3d73286836c8a660649c030167ba7d12e10fb572`; queue-v2 changes native
  metadata ownership, row extents and variable-row behavior and therefore
  requires a new exact-snapshot review after GREEN.
- Target: visible thread `019fd2fa-a222-7601-9567-980a67f814b5`, app-owned
  worktree `C:/Users/lcy/.codex/worktrees/6cfd/mne-cpp-main`.
- Precheck: worktree is clean/detached at the exact requested v1 snapshot;
  request and public issue #5 dispatch are durable. The task is stale rather
  than reusable for the queue-v2 gate.
- Action: archive through the Codex app only. Do not manually delete or mutate
  its worktree. A new visible Sol/ultra `R-QUEUE-V2-001` will review the exact
  integrated v2 snapshot after manager GREEN.

### RESPONSE R-QUEUE-001-RETIRE

- App result: thread `019fd2fa-a222-7601-9567-980a67f814b5` is archived.
- The stale queue-v1 review conversation is no longer active. Its app-owned
  worktree was not manually deleted or modified.
- A fresh visible Sol/ultra review remains mandatory for the integrated
  queue-v2 snapshot; no v1 review conclusion will be reused as that gate.

### RECOVERY REQUEST W-QUEUE-V2-GREEN-001

- Trigger: one post-compaction app lifecycle snapshot resolves the setup as
  thread `019fd52e-1d77-7a22-a8f3-ab51728560c3` and reports it `idle`, but no
  proactive structured RESPONSE reached the manager conversation.
- Policy: perform exactly one bounded `read_thread` recovery read for this
  idle task. This repairs a known inter-thread delivery gap and is not periodic
  polling. Do not send a duplicate implementation request or edit its scope.
- Expected handling: if the final response exists, persist it before manager
  inspection. If it does not exist, send one explicit RESPONSE request to the
  retained same-responsibility conversation and wait for proactive delivery.

### SYSTEM FAILURE W-QUEUE-V2-GREEN-001-TURN-1

- Single recovery read result: the completed turn contains only the original
  user delegation and no assistant item, final response, error text or tool
  output.
- Read-only worktree proof: `C:/Users/lcy/.codex/worktrees/6e99/mne-cpp-main`
  is clean/detached at exact `9b526eb1429b3e1292a0c58fae3d73fde34eb97c`;
  there is no unstaged/staged diff and no worker commit.
- Classification: execution produced no work. Nothing can be inspected or
  integrated, and no RESPONSE was lost after a commit.

### REQUEST W-QUEUE-V2-GREEN-001-RETRY-1

- Reuse the same visible Sol/ultra conversation
  `019fd52e-1d77-7a22-a8f3-ab51728560c3`; it is the same responsibility and
  its worktree is still the required clean exact base.
- Execute the original immutable queue-v2 request now, editing only the queue
  header/source and returning the required structured proactive RESPONSE.
- Do not create another worker, rebase, poll the manager, use subagents, run a
  full scan or start `mne_rt_server`.

### RESPONSE W-QUEUE-V2-GREEN-001-RETRY-1-DISPATCH

- App accepted the follow-up on existing thread
  `019fd52e-1d77-7a22-a8f3-ab51728560c3`.
- Model override is explicitly `gpt-5.6-sol`, `ultra`; the message repeats the
  exact clean base, two-file scope, proactive manager RESPONSE and no-subagent/
  no-poll/no-full-scan/no-vendor/no-rt_server constraints.
- Coordination: this is one retry in the original conversation, not a new
  task. Manager will not read/wait again while it is running.

### PUBLISH W-QUEUE-V2-GREEN-001-RETRY-1

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5201026440`.
- Authenticated API read-back confirms the no-output first turn, exact clean
  base, same-conversation Sol/ultra retry, unchanged two-file gate and required
  manager GREEN/fresh review.
- The initial read-back command extracted `issuecomment-5201026440` instead of
  the numeric ID and returned HTTP 404; the write had succeeded. Retrying the
  read-only API call with numeric ID `5201026440` succeeded. No duplicate
  comment was created.

### RECOVERY REQUEST W-DOC-001

- Trigger: old visible thread `019fd2fc-0d25-7163-a29d-b7c8840028f9` is
  not-loaded and did not deliver a manager RESPONSE, but its clean app-owned
  worktree contains exact-parent one-file commit `c558acbf8` adding only
  `LEARNING_GUIDE.md`.
- Action: perform one bounded thread read to recover the structured response
  and limitations before manager content review. This is not periodic polling.

### RECOVERY REQUEST W-QA-CORE-CONTRACT-001

- Trigger: old visible thread `019fd336-0d0c-7d02-82d9-a24b511d6aaf` is
  not-loaded without a delivered RESPONSE. Its clean worktree HEAD `7471c6a7a`
  changes only durable dispatch records on parent `7eb04cbfa`; no core header/
  test implementation commit is visible yet.
- Action: perform one bounded thread read. If the turn was empty, reuse this
  same Luna/max responsibility once from its still-clean task context rather
  than creating a duplicate conversation.

### RETIREMENT REQUEST W-QUEUE-GREEN-001

- Target: old queue-v1 implementation thread
  `019fd2bd-b720-7b30-8d65-780a0b500924`, clean/detached app worktree
  `C:/Users/lcy/.codex/worktrees/7228/mne-cpp-main` at exact worker commit
  `039b58d1b1cb2b7d79b8d8db27839a3c9f7ad81b`.
- Reason: its implementation and populated 8/0/0 evidence are durable, but the
  native-owner/variable-row queue-v2 replaces this exact-row seam. It cannot
  receive relevant review fixes now.
- Action: app-archive only; never manually delete or mutate the worktree.

### RESPONSE W-QUEUE-GREEN-001-RETIRE

- App archived old queue-v1 implementation thread
  `019fd2bd-b720-7b30-8d65-780a0b500924` successfully.
- Its clean app-owned worktree was not manually deleted or modified. Durable
  integration/test evidence remains; queue-v2 owns all future fixes/review.

### RESPONSE W-DOC-001

- Recovered once from old visible Luna/max thread
  `019fd2fc-0d25-7163-a29d-b7c8840028f9` after proactive delivery was absent.
- Exact parent: `fa832d51d3b9aa463490cf3afbc2037f71843f41`.
- Commit: `c558acbf80eb425aec9a22641d5a930d4370b2d9`.
- Scope: adds only
  `doc/causal_reference_denoising/LEARNING_GUIDE.md` (376 lines); worker
  worktree is clean.
- Content response: shapes and row ownership; tap-major causal history; EWLS
  pending/committed transaction and forgetting; relative loading/LDLT;
  truthful `noalias()` use; modes/reset/freeze/epochs/latency; allocation and
  fallback contracts; queue seam; complexity/memory; concrete shape walkthrough
  and block timeline; effect versus engineering evidence; research/license and
  v1 non-goals.
- Verification: all eight repository-relative links resolve, fenced blocks and
  heading structure validate, and `git diff --check` passes. Markdown-only, so
  no build was run; no full mne_scan or mne_rt_server.
- Worker confirms Luna/max, no subagent and no manager polling.
- Status: response is durable; manager content/source/link review is next.

### SYSTEM FAILURE W-QA-CORE-CONTRACT-001-TURN-1

- Single recovery read contains only the original user delegation and no
  assistant item, error, tool output or RESPONSE.
- Its app-owned worktree is clean/detached at dispatch-record commit
  `7471c6a7a8f7cf15a64e1d77c08b36e0f20ac0df`; no authorized core header/test
  change exists.
- Classification: no work was performed; nothing is available to integrate.

### REQUEST W-QA-CORE-CONTRACT-001-RETRY-1

- Reuse the same visible Luna/max thread
  `019fd336-0d0c-7d02-82d9-a24b511d6aaf`; do not create a duplicate worker.
- Before editing, detach only that app-owned worktree to the original required
  exact base `7eb04cbfa6aa9cbd09594c6aa2d6fe6aebcc43db`, then execute the unchanged
  two-file Doxygen/special-member/trait request.
- Return the original structured proactive RESPONSE; no subagent, manager
  polling, full scan, vendor repair or mne_rt_server.

### RESPONSE W-PLUGIN-DATA-001

- Exact requested base/parent:
  `8c51ea4ce86bdd6a377f7f87e4ef693de4893491`.
- Worker commit: `5f47187224ede2dac7abede7d63891cad190c879`;
  clean detached worktree and exact-parent proof reported.
- Exact scope: plugin root CMake plus six new `adaptivedenoising` files
  (`CMakeLists.txt`, h/cpp, global h/cpp and zero-byte JSON), seven authorized
  paths and 702 insertions. Queue, processor, tests, core, docs, UI, vendor and
  other plugins are unchanged.
- Target/interface: registers `scan_adaptivedenoising`; concrete final
  `AdaptiveDenoising : AbstractAlgorithm` behind plugin-private PImpl, standard
  Qt plugin identity, RTMSA input/output connectors and default blank setup
  widget. Narrow links are Qt Core/Widgets, mne_utils, mne_fiff,
  mne_rtprocessing, Eigen, scShared and scMeas.
- Producer/lifecycle response: start preallocates queue-v2 512x2048/capacity 4
  and consumer destination, starts worker before opening atomic admission;
  packed admission epoch/in-flight CAS prevents stale callbacks crossing
  restart. stop closes admission, wakes/waits worker and boundedly waits for
  producer quiescence; timeout blocks restart; destructor never terminates the
  QThread and waits safely after closing/stopping again.
- Callback response: one RTMSA `info()` snapshot and one zero-time `tryPush`
  per matrix; every non-Pushed status increments atomic drops. No retry/wait,
  mutex, FIFF pick, processor/config/reset, output or UI work in `update()`.
- Worker response: 50 ms interruptible pop; exact returned rectangle copy;
  validates non-null/nchan/chs/row count/finite-positive sfreq; builds kind/bad
  descriptors on worker; fresh config on metadata/sfreq/shape/width/validity
  transition; catches allocation exceptions, deliberately disarms, then
  forwards unchanged. Default mode is ApplyAndLearn.
- Output response: serial FIFO blocks, multi-array size one, one worker-only Qt
  `constCast` at legacy output initialization, and no metadata mutation.
  Structurally invalid metadata is not emitted under stale labels.
- Evidence: exact seven-path scope, clean diff/status, zero-byte JSON and Qt
  5.15.2 moc success. Target-local CMake stops before target generation at the
  known ignored Eigen gap; no build GREEN, vendor repair, full scan/mne_scan or
  mne_rt_server. Base intentionally has pre-v2 queue; manager integrates queue
  v2 first.
- Worker confirms assigned visible Sol/ultra, no subagents and no manager
  read/poll/wait.
- Status: response durable; manager review is next, integration held behind
  queue-v2 GREEN.

### RESPONSE W-QUEUE-V2-GREEN-001

- Exact required base/parent:
  `9b526eb1429b3e1292a0c58fae3d73fde34eb97c`.
- Implementation commit:
  `90b423e3c93ab744855d71069dec1cae08691f47`; worker worktree clean/detached.
- Exact scope: only `adaptivedenoisingblockqueue.h/.cpp`; immutable focused
  test and all processor/plugin/core/docs/vendor files unchanged.
- Interface: config uses maximum channel/sample bounds and capacity; queued
  block carries maximum matrix, row/sample extents and native
  `QSharedPointer<const FiffInfo>`; final four-method PImpl/deleted copy/move
  surface remains deep and concrete.
- Configure response: validates bounds and QSemaphore int capacity; constructs
  candidate semaphores/vector/all maximum Eigen slots before publishing a fresh
  running PImpl. Allocation exceptions propagate while old stopped ownership
  remains untouched; running reconfigure is rejected.
- Producer response: accepts any positive rectangle within maxima, performs one
  zero-time free acquire, rechecks running at the commit point, scalar-copies
  only the rectangle, stores both extents/native handle, advances once and
  publishes once. Full/stopped/invalid do not mutate the ring.
- Consumer response: exact maximum destination, one timed acquire, running
  commit point, top-left copy with untouched tail, both extents/native metadata,
  one advance/free publication. Invalid/timeout/stopped preserve destination.
- Stop response: acq_rel exchange is idempotent; wakes only an empty waiter;
  operations that pass the running commit point linearize before stop and may
  finish, so documented caller quiescence is required before replacement.
  Fresh PImpl prevents old tokens/indices from leaking across reconfigure.
- Realtime response: native Qt handle operations are guarded by C++14 nothrow
  traits; FIFF remains forward-declared. Hot paths use preallocated scalar
  matrix copies, atomics and preconstructed QSemaphores only, with no resize,
  allocation, retry/busy wait, explicit mutex, string or FIFF access.
- Validation: exact parent/scope/clean and diff checks pass. Isolated focused
  execution remains unavailable solely because ignored Eigen files are absent;
  no dependency repair or false GREEN. Manager owns populated Release run.
- Worker confirms visible Sol/ultra, no subagent, manager poll/read/wait, full
  scan/mne_scan, vendor repair or mne_rt_server.
- Status: response durable; manager exact diff/interface/race review precedes
  cherry-pick and complete populated focused GREEN.

### MANAGER REVIEW W-QUEUE-V2-GREEN-001

- Provenance: exact required parent, exactly the two authorized queue files,
  clean worker status and immutable focused-test blob all pass.
- Interface/depth: one concrete final PImpl still hides slot/semaphore/ring
  state behind four methods; native metadata and row/sample extents are the
  minimum real plugin seam, with no FIFF dereference or hypothetical strategy.
- State/race audit: initial and post-acquire running checks define the push/pop
  commit point. An operation observing running at that point may finish before
  stop; one observing stop returns without publishing caller-visible state.
  Pending data/wake/late old-PImpl tokens are discardable and cannot cross a
  quiesced reconfigure because fresh semaphores/slots/indices are allocated.
- Preservation audit: invalid/full/stopped producer paths never write/advance;
  invalid/timeout/stopped consumer paths preserve the complete destination;
  Popped writes only the valid top-left and returns exact extents/metadata.
- Realtime audit: fixed scalar matrix loops avoid Eigen expression temporaries;
  native handle nothrow traits and preconfigured semaphore/ring state support
  truthful noexcept/no-allocation scope. Caller producer/consumer quiescence is
  explicitly local to the public contract and enforced by the plugin lifecycle.
- Findings: none. Integration and populated focused Release are authorized;
  a fresh independent Sol/ultra formal review remains mandatory afterward.

### BUILD RESPONSE W-QUEUE-V2-GREEN-001

- Integration: worker commit `90b423e3c` cherry-picked without conflict as
  `a21e08e00`; exactly the two queue files changed.
- Command: `cmake --build build-causal-reference-denoising --config Release
  --target test_adaptive_denoising_plugin -- /m:2`.
- Result: compilation fails in `adaptivedenoisingblockqueue.cpp` before link.
  The first errors are C2208/C2059 at member declaration/uses of `slots`; Qt's
  `slots` macro rewrites the identifier, cascading into bogus missing-member
  errors at reserve/emplace/index/copy loops.
- Classification: P1 `R-QUEUE-QT-SLOTS-001`, an implementation build blocker,
  not the known Eigen/Qt dependency failure. No test executable was run.

### REQUEST W-QUEUE-V2-GREEN-001-FIX-1

- From/to: manager / retained Sol/ultra queue implementation conversation
  `019fd52e-1d77-7a22-a8f3-ab51728560c3`.
- Base: worker commit `90b423e3c`; create one delta commit on that parent.
- Scope: `adaptivedenoisingblockqueue.cpp` only. Rename the private Impl member
  `slots` and all its uses to a Qt-safe name such as `queueSlots`. Do not alter
  interface, ordering, atomics, semaphores, copy loops or tests.
- Verification: exact one-file/name-only diff, diff check, clean status; if the
  isolated Eigen gap remains, report it honestly. Manager reruns populated
  Release. No subagent/poll/full scan/vendor/server.

### RESPONSE W-QUEUE-V2-GREEN-001-FIX-1-DISPATCH

- App accepted the narrow follow-up on existing thread
  `019fd52e-1d77-7a22-a8f3-ab51728560c3` with `gpt-5.6-sol`, `ultra`.
- Prompt fixes parent `90b423e3c`, one source file, name-only private rename,
  unchanged public/concurrency/test surface and proactive structured RESPONSE.
- No duplicate worker/worktree was created; manager will not poll it.

### PUBLISH W-QUEUE-V2-GREEN-001-QT-RED

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5201209614`.
- API read-back confirms integrated SHA, focused compile command/error class,
  P1 classification, name-only Sol/ultra fix gate and no server/full scan.

### RESPONSE W-QUEUE-V2-GREEN-001-FIX-1

- Exact required parent: `90b423e3c93ab744855d71069dec1cae08691f47`.
- Delta: `33b3eea1d483507d2667b13e4644930b91a628fd`; clean worker status.
- Exact scope: five insertions/five deletions in
  `adaptivedenoisingblockqueue.cpp` only. Private vector declaration/reserve/
  emplace/producer index/consumer index change from `slots` to `queueSlots`.
- No public interface, state shape/layout, ordering, atomics, semaphores, copy
  loops, tests, CMake, docs, vendor or dependency behavior changed.
- Evidence: installed Qt 5.15.2 maps `slots` to `Q_SLOTS`; token probe changes
  unsafe identifier count 5→0 and `queueSlots` count 0→5. Exact parent/scope/
  name-only/diff/clean checks pass.
- Isolated Eigen baseline remains absent, so worker makes no Release claim and
  performs no dependency repair. Manager owns populated rerun.
- Constraints: Sol/ultra, no subagent/poll/full scan/mne_scan/vendor/rt_server.
- Status: response durable; manager mechanical-delta review precedes cherry-pick.

### MANAGER REVIEW W-QUEUE-V2-GREEN-001-FIX-1

- Exact parent/scope/diff check passes. Word diff contains only five private
  identifier replacements `slots`→`queueSlots` in the one authorized source.
- No semantic, layout, interface, ordering, atomic, semaphore or test change.
  The delta directly removes the Qt macro token; no finding.
- A combined shell call with a one-second timeout ended before cherry-pick and
  left integration HEAD/status unchanged with no CMake/MSBuild process. Retry
  the two actions separately with a normal build timeout.

### BUILD RESPONSE W-QUEUE-V2-GREEN-001-FIX-1

- Integration: worker delta `33b3eea1d` cherry-picked as `c87762e97`; exactly
  one queue source and five identifier replacements.
- Build: focused Release target `test_adaptive_denoising_plugin` compiles and
  links successfully to `out/Release/apps/test_adaptive_denoising_plugin.exe`.
  Only pre-existing Eigen C4819 codepage warnings occur.
- Runtime: the complete executable was invoked three times (default, `-v1`,
  and explicit text-output attempt) and every invocation returned process exit
  zero. This test binary suppresses QtTest console/file reporting in the local
  harness, so totals are derived from the public test structure: prior 14/0/0
  plus the new variable-row/native-metadata slot gives effective 15/0/0; source
  contains no QSKIP/QEXPECT_FAIL/QFAIL path.
- Coverage includes all processor mapping/disarm/reset/boundary cases and all
  queue FIFO/drop-newest/metadata, stop/reconfigure, invalid-state preservation
  and mixed 2/4/3-row top-left/tail/extents/native-owner cases.
- `R-QUEUE-QT-SLOTS-001` closes. Fresh independent Sol/ultra formal review is
  still required before issue #5 closes.

### PUBLISH W-QUEUE-V2-GREEN-001-FINAL

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5201314365`.
- API read-back confirms exact integration/fix SHAs, successful Release build,
  effective 15/0/0 process-zero evidence, P1 closure, fresh-review gate and no
  full mne_scan/mne_rt_server.

### REQUEST R-QUEUE-V2-001

- From/to: manager / new visible read-only queue-v2 reviewer.
- Model: `gpt-5.6-sol`, `ultra`; new app-managed worktree at exact reviewed
  snapshot `e4964aaed44b5030ad4490e0ee7744d796b387cb`.
- Recovery/skills: read STATE, SPEC queue/plugin lifecycle sections, complete
  relevant WORKERS/REVIEW entries and JOURNAL tail; read complete codebase-
  design SKILL/DEEPENING. Use module/interface/seam/depth/leverage/locality.
- Read-only scope: complete queue h/cpp, complete focused plugin test and only
  enough processor/plugin lifecycle source to validate caller quiescence and
  ownership assumptions. Inspect exact worker/fix provenance and manager GREEN.
- Required review: SPSC token/index publication; stop before/during acquired
  push/pop; artificial wake and pending-block discard; fresh-PImpl isolation;
  producer/consumer quiescence contract; native QSharedPointer noexcept/control
  block behavior; configure transaction/allocation; rectangle/extents/tail/
  destination preservation; drop-newest; variable metadata/rows; Qt macro fix;
  no-allocation/no-lock/no-retry/no-string/no-FIFF hot path; public tests'
  sensitivity and any untested race.
- Findings: report P0-P3 with exact file/line, evidence/impact/fix/test. P0/P1/
  P2 must be zero for the gate. Do not edit, commit, push, mutate GitHub,
  dependencies or tasks. No subagents, manager polling, full scan/mne_scan or
  mne_rt_server.
- Required response: proactive `RESPONSE R-QUEUE-V2-001` with exact SHA/clean
  status, inspection/evidence, prioritized findings and gate recommendation.
- Lifecycle: archive after response evidence/fixes/final decision are durable.

### SYSTEM FAILURE R-QUEUE-V2-001-CREATE-1

- First app create call was rejected locally as invalid arguments because the
  saved-project ID was supplied both at the outer argument level and inside
  the documented target union.
- No task, thread, worktree, repository, GitHub or dependency state was created
  or changed. Retry once with project ID only inside the `target` union and the
  same exact snapshot/model/prompt.

### RESPONSE R-QUEUE-V2-001-CREATE

- Corrected app call accepted visible saved-project worktree setup
  `client-new-thread:4b85d632-2892-48c3-8044-846ed7c4ad39`; host `local`.
- Model is `gpt-5.6-sol`, `ultra`; starting ref is exact reviewed GREEN commit
  `e4964aaed44b5030ad4490e0ee7744d796b387cb`.
- Prompt carries full recovery/skill/read-only/concurrency/test/finding gate and
  proactive-response restrictions. No final thread ID exists yet.
- Coordination: publish the dispatch, then do not poll. Reviewer must notify
  the manager proactively.

### PUBLISH R-QUEUE-V2-001-START

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5201388674`.
- API read-back confirms Sol/ultra, exact GREEN snapshot/setup, full concurrency/
  realtime/test gate, P0/P1/P2-zero requirement, read-only/proactive/no-poll
  workflow and no full mne_scan/mne_rt_server.

### RESPONSE W-QA-CORE-CONTRACT-001

- Requested full base recorded in the original task:
  `7eb04cbfa6aa9cbd09594c6aa2d6fe6aebcc43db`; worker proves this object is
  unavailable locally and rejected by origin as not our ref.
- Actual dispatch commit `7471c6a7a` has parent
  `7eb04cbfa766c1cbadcbd870dda80f0557c6f74a`. Worker reset only its worktree to
  that real parent and created exact-parent commit
  `2682034d0e45ecb50b67dea47b929d9d7d3b6957`; it explicitly does not claim the
  impossible requested provenance.
- Exact scope: numerical public header plus focused numerical test only.
- Header response: compact Doxygen covers channel/time shape, selected row
  uniqueness/range/disjointness and P<=256, configure-only transactional
  allocation and throwing preservation, target-only mutation, all three modes,
  whole-block/state-preserving shape/nonfinite errors, reset semantics,
  single-worker ownership, fixed diagnostics and post-configure noexcept/no-
  allocation/no-lock/no-string/no-Qt/FIFF process path.
- Ownership: retains nothrow default/destructor and deletes copy/move ctor/
  assignment; no custom move or runtime behavior.
- Compile contract: C++14 traits for default/copy/move, reset noexcept, MatrixXd
  lvalue to Eigen::Ref convertibility, and process member noexcept type. Worker
  separates the member contract from a direct-call expression whose by-value
  Eigen::Ref wrapper copy may itself be potentially throwing.
- Evidence: exact two-file/diff/clean proof; read-only C++14 syntax probes using
  the populated sibling Eigen plus generated Qt MOC pass for header,
  implementation and focused test. Complete Release/Debug not run in the
  isolated incomplete-Eigen worktree; no vendor repair/runtime claim.
- Constraints: Luna/max, no subagent/manager poll/full scan/mne_scan/vendor/
  rt_server.
- Status: response durable; manager must audit the real-parent delta against
  current blobs and rerun populated Release/Debug before closing P3.

### MANAGER REVIEW W-QA-CORE-CONTRACT-001

- Provenance: actual parent/worker commit is valid, exactly two authorized
  files, and both current integration blobs equal the actual parent blobs, so
  there is no hidden baseline drift.
- Header Doxygen and explicit special members are truthful against SPEC/source;
  no header finding.
- P1 `R-CORE-CXX14-NOEXCEPT-001`: the focused test declares
  `using ProcessMember = ... noexcept`. A minimal MSVC 14.51 probe compiled
  with `/std:c++14` fails at that alias with C2279 (`exception specification
  cannot appear in a typedef declaration`). The same failure occurs before any
  mutation-sensitivity claim can be meaningful.
- Contract nuance: a full call expression from `MatrixXd&` also includes
  construction/copy of by-value `Eigen::Ref`, whose constructors are not
  declared noexcept. It is therefore incorrect to claim the whole caller
  expression is noexcept merely because the member body is declared noexcept.
- Decision: hold commit. Keep header unchanged; revise only the test to remove
  the illegal pointer alias/cast. Retain ownership/default/reset noexcept traits
  and use portable C++14 `decltype`/`is_same` plus Ref convertibility to prove
  MatrixXd-lvalue callability/result. Document that the member declaration is
  locally reviewed, while wrapper construction is outside its noexcept body.

### REQUEST W-QA-CORE-CONTRACT-001-REVISE-1

- From/to: manager / retained Luna/max core-contract conversation
  `019fd336-0d0c-7d02-82d9-a24b511d6aaf`.
- Parent: worker commit `2682034d0`; create one delta commit changing only the
  focused numerical test. Do not alter the accepted header or runtime slots.
- Remove `ProcessMember` and the C++17-only/illegal `noexcept` member-pointer
  alias/cast. Keep default/copy/move/reset traits. Add portable C++14 compile
  checks that `MatrixXd&` converts to `Eigen::Ref<MatrixXd>` and the public call
  expression's `decltype` is `DenoiserProcessResult`; do not falsely assert the
  whole call expression noexcept.
- Verify explicitly with MSVC 14.51 `/std:c++14`, exact one-test-file delta,
  diff/clean checks. Return proactive structured replacement/delta evidence.
  No subagent/poll/full scan/vendor/mne_scan/rt_server.

### RESPONSE W-QA-CORE-CONTRACT-001-REVISE-1-DISPATCH

- App accepted the narrow follow-up on retained thread
  `019fd336-0d0c-7d02-82d9-a24b511d6aaf` with Luna/max.
- Parent is fixed to `2682034d0`; one focused-test-file scope, accepted header/
  runtime immutability, portable C++14 callability/result/reset/ownership traits
  and explicit MSVC 14.51 `/std:c++14` verification are repeated.
- No duplicate task/worktree; manager will not poll.

### PUBLISH W-QA-CORE-CONTRACT-001-REVISE-1

- GitHub final-QA issue #3 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/3#issuecomment-5201485839`.
- API read-back confirms real-parent correction, accepted header, C++14 C2279
  P1, portable test-only revision and manager Release/Debug gate.

### RESPONSE W-QA-CORE-CONTRACT-001-RETRY-1-DISPATCH

- App accepted the follow-up on existing thread
  `019fd336-0d0c-7d02-82d9-a24b511d6aaf`.
- Model is explicitly `gpt-5.6-luna`, `max` as required for the focused
  documentation/compile-contract slice.
- Prompt requires detaching only the worker worktree to exact original base
  `7eb04cbfa6aa9cbd09594c6aa2d6fe6aebcc43db`, then the unchanged two-file
  task and proactive RESPONSE. It repeats all no-subagent/no-poll/no-full-scan/
  no-vendor/no-rt_server constraints.
- No duplicate task/worktree was created; manager will not poll it while active.

### REQUEST W-DOC-001-REVISE-1

- From/to: manager / retained Luna/max documentation conversation
  `019fd2fc-0d25-7163-a29d-b7c8840028f9`.
- Finding: P2 `R-DOC-QUEUE-V2-001` in the only added file. Lines 235-249 still
  say fixed `C x Nmax`, leading-column copy and consumer sample count only,
  omitting frozen queue-v2 maximum channel dimension, per-block row/sample
  extents, top-left rectangle, native `QSharedPointer<const FiffInfo>`, variable
  row transition/tail preservation tests. Lines 164-168 call the adapter/UI
  defaults numerical defaults although the numerical config has no defaults.
- Required replacement: amend `c558acbf8` with the same exact parent and same
  one-file scope. Describe queue-v2 truthfully as `Cmax x Nmax` slots, positive
  variable rectangles, `rowCount`/`sampleCount`, top-left copy, native immutable
  Qt handle, drop-newest, untouched destination tail and worker-only reshape.
  Rename the four values as adapter defaults. Preserve all other guide content.
- Verification: all repository links, Markdown structure, exact one-file scope,
  clean status and diff check. Return replacement SHA and exact line-level
  correction through `RESPONSE W-DOC-001-REVISE-1`; no build/full scan/server.

### RESPONSE W-DOC-001-REVISE-1-DISPATCH

- App accepted the narrow follow-up on retained thread
  `019fd2fc-0d25-7163-a29d-b7c8840028f9` with `gpt-5.6-luna`, `max`.
- It requires an amended replacement with the same exact parent and one-file
  scope, precise adapter-default and queue-v2 corrections, unchanged verified
  content, link/Markdown/diff/clean proof and proactive structured RESPONSE.
- No duplicate conversation/worktree was created; manager will not poll it.

### PUBLISH W-DOC-001-REVISE-1

- GitHub issue #7 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/7#issuecomment-5201120187`.
- Authenticated API read-back confirms the held SHA, P2 evidence, exact narrow
  correction, same-conversation Luna/max amend and no-build/server scope.

### RESPONSE W-QA-CORE-CONTRACT-001-REVISE-1

- From/to: retained visible Luna/max core-contract conversation
  `019fd336-0d0c-7d02-82d9-a24b511d6aaf` / manager.
- Exact parent/delta: `2682034d0e45ecb50b67dea47b929d9d7d3b6957` /
  `bc061b83419bf6037f021791ff8b9ca5435245c8`; worker reports a clean detached
  worktree and exactly one changed file,
  `test_causal_reference_denoiser.cpp`.
- Revision: removes the C++14-illegal `ProcessMember` noexcept member-pointer
  alias/cast; retains nothrow default, deleted copy/move and reset-noexcept
  traits; adds portable `MatrixXd&` to `Eigen::Ref<MatrixXd>` convertibility
  and public `process(...)` call-result type assertions. A concise comment
  distinguishes the explicitly noexcept member body from potentially throwing
  Eigen::Ref argument-wrapper construction.
- Worker verification: MSVC 14.51 `/std:c++14 /EHsc /c` compiles the numerical
  implementation and focused test after temporary Qt MOC generation, with only
  existing Eigen C4819 warnings. Complete Eigen was read from a sibling
  worktree without copy/repair/stage; diff/scope/clean checks pass.
- Restrictions: Luna/max, no subagent, manager polling, full scan/mne_scan,
  vendor repair or mne_rt_server.
- Status: response is durable before manager provenance/delta review,
  integration or populated Release/Debug validation.

### MANAGER REVIEW W-QA-CORE-CONTRACT-001-REVISE-1

- Provenance/scope: delta `bc061b834` has exact parent `2682034d0`, modifies
  only the authorized focused numerical test and passes committed diff check.
  Original `2682034d0` has the verified real parent `7eb04cbfa766...`, only
  the accepted header/test scope, and current integration code blobs still
  equal that real parent.
- C++14 audit: the complete word diff removes the illegal function-type
  noexcept alias/cast. The replacement independently checks MatrixXd-lvalue
  Ref convertibility and the public call expression's exact result type while
  retaining all default/ownership/reset traits. Its comment correctly avoids
  asserting that Eigen::Ref wrapper construction is noexcept.
- Decision: no finding. Integrate `2682034d0` followed by `bc061b834`, then
  build/run the complete populated Release and Debug focused targets before
  closing `R-CORE-CXX14-NOEXCEPT-001` and `R-CORE-LOCALITY-001`.

### RESPONSE R-QUEUE-V2-001

- From/to: fresh visible Sol/ultra queue-v2 reviewer / manager; source thread
  `019fd5d8-380d-7563-8644-264426bda199`.
- Exact review snapshot/state: `e4964aaed44b5030ad4490e0ee7744d796b387cb`,
  clean detached before/after. Read-only review made no edit, commit, push,
  GitHub/dependency/task mutation, subagent, polling or server action.
- Gate: HOLD with P0=0, P1=1, P2=2, P3=0.
- P1 `R-QUEUE-V2-QSEMAPHORE-001`: installed Windows Qt 5.15.2 selects the
  dummy-futex `QSemaphore` fallback, whose acquire/release/available operations
  take `QMutex`; first contention may allocate `QMutexPrivate`. Consequently
  producer `tryPush` can block/allocate and the dependency is not noexcept.
  Required direction is atomic SPSC publication/availability plus a
  preconstructed nonallocating notification seam used only for consumer wake.
- P2 `R-QUEUE-V2-CONCURRENCY-TEST-001`: successful FIFO/publication tests are
  sequential and the empty-waiter marker can precede the actual wait. Add
  sustained overlapping SPSC traffic, producer/consumer stop races, pending-
  block discard/fresh-reconfigure and complete Timeout/Stopped destination
  preservation through the public interface.
- P2 `R-QUEUE-V2-METADATA-LIFETIME-001`: current fake-handle tests keep caller
  owners alive and compare only raw data pointers. Add custom-deleter/liveness
  counters, clear callers after push, prove retention through pop and exactly-
  once destruction; assert nothrow mutable-to-const conversion.
- Accepted static behavior: deep concrete queue seam, transactional configure,
  SPSC index ownership/publication order, rectangle/extents/tail/drop-newest,
  stop/fresh-PImpl isolation under caller quiescence, native Qt ownership and
  plugin admission/in-flight caller ordering are otherwise coherent.
- Evidence distinction: prior populated process-zero/effective 15-case result
  is manager evidence; reviewer performed independent provenance/blob/source/
  Qt implementation audit and no independent build/run.
- Status: response durable before fix-task dispatch or production mutation.

### BUILD RESPONSE W-QA-CORE-CONTRACT-001-RELEASE

- Integration: original worker commit and delta became `47b54efed` and
  `0848634a3` in order, with no conflict.
- Populated Release focused target compiles and links successfully under MSVC
  14.51 C++14; only existing Eigen C4819 code-page warnings occur. The complete
  executable returns process exit zero.
- QtTest output remains suppressed by this local harness even with an explicit
  text reporter, so no newly emitted total is claimed. Runtime slots are
  unchanged and the new namespace-scope traits necessarily compiled. Debug
  execution remains required before closing the findings.

### VALIDATION W-QA-CORE-CONTRACT-001

- Populated Debug target also compiles/links under MSVC 14.51 C++14 with only
  existing Eigen C4819 warnings; its complete executable returns zero.
- Release and Debug both compile the new ownership/reset/callability/result
  traits. Runtime QtTest slots are unchanged from the prior emitted evidence:
  effective Release 44/0/1 and Debug 45/0/0. Debug does not define
  `EIGEN_NO_DEBUG`, so the existing `EIGEN_RUNTIME_NO_MALLOC` slot executes
  rather than skips.
- Decision: close `R-CORE-CXX14-NOEXCEPT-001` and
  `R-CORE-LOCALITY-001`. `R-CORE-LINK-001` remains separately environment-
  deferred under the already published Qt/MSVC vendor incompatibility.

### PUBLISH R-QUEUE-V2-001

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5201624132`.
- Authenticated `gh` write and API read-back exactly record the Sol/ultra HOLD
  gate, P1 Windows QSemaphore dependency, both P2 test gaps, accepted static
  behavior, test-first fix direction and no full mne_scan/mne_rt_server scope.

### PUBLISH W-QA-CORE-CONTRACT-001-FINAL

- GitHub final-QA issue #3 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/3#issuecomment-5201673779`.
- Authenticated `gh` write/API read-back record both integrated SHAs, populated
  Release/Debug process-zero validation, retained suite counts/Debug malloc
  guard, closed C++14/locality findings and unchanged link-smoke deferral.

### RETIREMENT PRECHECK W-QA-CORE-CONTRACT-001

- Worker thread `019fd336-0d0c-7d02-82d9-a24b511d6aaf` is clean/detached at
  final delta `bc061b834`.
- Its public-header and focused-test blobs exactly equal integration HEAD. The
  RESPONSE, manager review, Release/Debug validation and GitHub closure are
  durable. The same-responsibility fix loop is complete, so this conversation
  may be app-archived without deleting its worktree.

### RESPONSE W-QA-CORE-CONTRACT-001-RETIRE

- App archived completed visible thread
  `019fd336-0d0c-7d02-82d9-a24b511d6aaf` after exact clean/blob/validation/
  publication evidence. No manual worktree deletion or mutation occurred.

### REQUEST W-QUEUE-ATOMIC-TEST-001

- From/to: manager / new visible Luna/max queue public-test conversation.
- GitHub issue/findings: #5; `R-QUEUE-V2-CONCURRENCY-TEST-001` and
  `R-QUEUE-V2-METADATA-LIFETIME-001`, with supporting evidence for
  `R-QUEUE-V2-QSEMAPHORE-001`.
- Exact base: `4bb1b2a30`; separate Codex worktree. Authorized edit is only
  `src/testframes/test_adaptive_denoising_plugin/test_adaptive_denoising_plugin.cpp`.
  No production queue/CMake/processor/plugin/docs/dependency/build edit.
- Read complete TDD skill and current public queue contract. Preserve all
  existing processor/queue slots and avoid internal/ring inspection.
- Add sustained overlapping SPSC traffic through `tryPush`/`waitPop` using
  precreated inputs/metadata and preallocated result storage. Exercise variable
  rectangles and deterministic sequence payloads; count `Full` drop-newest
  outcomes; require popped sequence/row/sample/metadata to equal the accepted
  producer subsequence with no tear/reorder/duplicate. Record bounded producer
  call latency with a generous Windows scheduling bound rather than a fragile
  microbenchmark.
- Add bounded stop interaction while producer/consumer are active, accepting
  only outcomes that linearize before stop or return Stopped. Add deterministic
  pending-block stop/discard, quiesced fresh configure/Timeout/new-payload proof.
  For every Timeout/Stopped result, prefill complete matrix/extents/metadata
  sentinels and require them unchanged.
- Add native metadata lifetime proof with fake non-dereferenced FiffInfo pointer
  identities plus observable custom-deleter counters. Clear all caller handles
  immediately after successful push, require no deletion before pop, FIFO
  identity/extents after pop, and exactly-once deletion after popped handles
  clear. Add the Qt 5.15.2-supported C++14 nothrow mutable-to-const
  `QSharedPointer` construction trait.
- Classification: these are public acceptance tracers; honest immediate GREEN
  is allowed where current behavior already conforms. Do not add flaky sleeps,
  source-text tests or production hooks merely to manufacture RED. The Sol
  production task will separately remove QSemaphore and prove the dependency.
- Verification: focused Release build/run if available; otherwise report the
  isolated Eigen limitation. Exact one-file commit, diff/clean proof, no full
  scan/mne_scan/vendor repair/mne_rt_server, no nested subagent or manager poll.
  Proactively send `RESPONSE W-QUEUE-ATOMIC-TEST-001` and stop.
- Model/effort: `gpt-5.6-luna`, `max`. Status: request durable before creating
  the visible task.

### RESPONSE W-QUEUE-ATOMIC-TEST-001-CREATE

- App accepted a new visible saved-project worktree setup as
  `client-new-thread:7119c1cd-0c1c-4632-ade6-edbc357dec0b`; host `local`.
- Model/effort are explicitly `gpt-5.6-luna`/`max`; starting ref is exact
  `4bb1b2a30`. Prompt repeats the one-test-file TDD scope, all public concurrency/
  lifetime oracles, join-safe finite synchronization and proactive response.
- No final thread ID exists yet. Publish the dispatch, then do not poll or
  overlap the focused test file.

### PUBLISH W-QUEUE-ATOMIC-TEST-001-START

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5201732759`.
- Authenticated `gh` write/API read-back exactly record the Luna/max setup/base,
  one-test-file public SPSC/stop/lifetime scope, TDD immediate-GREEN policy,
  later Sol implementation split and no-poll/no-server restrictions.

### MANAGER REVIEW W-PLUGIN-DATA-001

- Provenance/scope: `5f4718722` has exact requested parent `8c51ea4ce`, seven
  authorized plugin registration/adapter files, clean diff and no queue/
  processor/test/core/doc/dependency edit. Empty metadata JSON matches existing
  plugin convention; Qt moc accepted it.
- Deep-module/seam audit: one concrete PImpl adapter owns queue, processor,
  connectors and worker state. `update()` has one metadata snapshot and one
  `tryPush` site per matrix with no lifecycle/settings mutex, model/FIFF picks,
  output, retry or wait. Worker-only validation/config/disarm/const-cast/output
  and bounded stop/restart quiescence are localized and match SPEC.
- P1 `R-PLUGIN-SPSC-PRODUCER-001`: `tryEnterProducer()` rejects only closed or
  count saturation. A second callback that loads state after the first entered
  can CAS count 1 to 2 and then concurrently call the SPSC queue. The packed
  count protects stop/restart lifetime but does not serialize or reject a
  second producer.
- Decision: hold plugin commit. Require a source-only private gate fix that
  admits only an open state with in-flight count zero; a concurrent callback
  returns immediately. Preserve epoch/close/quiescence, callback no-wait/no-
  mutex behavior, public interface and all other plugin files. Queue-v2 atomic
  gate remains a separate earlier integration dependency.

### REQUEST W-PLUGIN-DATA-001-FIX-1

- From/to: manager / retained visible Sol/ultra plugin-data conversation
  `019fd59d-fcc8-7573-964d-ed930c4100a8`.
- Parent: exact worker commit `5f47187224ede2dac7abede7d63891cad190c879`;
  create one delta commit changing only `adaptivedenoising.cpp`.
- In `tryEnterProducer()`, reject whenever the in-flight count is nonzero, in
  addition to closed state. The only admitted transition is the current open
  epoch/count zero to count one. Preserve the one-shot strong CAS, epoch
  confirmation, close/open ordering, `leaveProducer`, stop wait and all caller
  behavior. Do not add lock/wait/retry or change queue/processor/UI/CMake.
- Add compact local comment proving the private gate enforces the queue's SPSC
  producer precondition. Static review must show no path can publish count >1.
  No test-source edit in this fix; the later plugin formal gate will exercise
  concurrent update admission after integration.
- Verification: exact parent/one-source-file semantic diff, moc/syntax if
  available, diff/clean checks. Sol/ultra; no nested subagent, manager polling,
  full scan/mne_scan/vendor repair/mne_rt_server. Proactively send
  `RESPONSE W-PLUGIN-DATA-001-FIX-1` and stop.
- Status: request durable before follow-up dispatch.

### PUBLISH W-PLUGIN-DATA-001-FIX-1

- GitHub issue #6 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5201795105`.
- Authenticated `gh` write/API read-back exactly record accepted plugin seam,
  P1 concurrent-producer evidence/impact, one-source-file zero-or-one admission
  fix, retained Sol/ultra conversation and queue-first/no-server ordering.

### RESPONSE W-PLUGIN-DATA-001-FIX-1-DISPATCH

- App accepted the follow-up on retained visible thread
  `019fd59d-fcc8-7573-964d-ed930c4100a8` with `gpt-5.6-sol`, `ultra`.
- Prompt fixes parent `5f4718722`, exactly one plugin source file and the
  zero-or-one private admission change while preserving epoch/quiescence/
  callback/queue/test/public behavior.
- No duplicate conversation or worktree was created. Manager will not poll or
  overlap the source file; await the proactive structured response.

### RESPONSE W-PLUGIN-DATA-001-FIX-1

- Exact parent/delta: `5f47187224ede2dac7abede7d63891cad190c879` /
  `34a8096f1068e2c60a2ad19418ca8676866f649a`; retained Sol/ultra worktree is
  reported clean/detached and parent match passes.
- Exact scope: only `adaptivedenoising.cpp`, three insertions/one deletion and
  clean diff. No queue/processor/test/UI/CMake/metadata/dependency/vendor edit.
- Correction: the initial predicate now rejects closed state or any nonzero
  in-flight count. Thus the only admitted pre-state is open/count zero and the
  one-shot CAS can publish only count one. A compact comment states that this
  enforces the queue SPSC producer precondition.
- Preserved: one-shot strong CAS, post-entry epoch/closed confirmation,
  close/open/leave/quiescence/stop/restart/drop behavior and no lock/wait/retry/
  public seam.
- Worker proof: deterministic source contract changes from allowing count two
  to nonzero rejection; Qt 5.15.2 moc succeeds with the same 4,269-byte output.
  Isolated plugin build remains blocked before generation by missing ignored
  Eigen files; no repair or false build claim.
- Restrictions: same visible Sol/ultra thread, no subagent/manager poll/full
  scan/mne_scan/vendor/server/GitHub action.
- Status: response durable before manager exact-delta review or integration.

### MANAGER REVIEW W-PLUGIN-DATA-001-FIX-1

- Exact parent and sole source-file scope pass; committed diff check is clean.
  The complete word diff is only one two-line SPSC comment and replacement of
  the saturation equality with nonzero-count rejection.
- State proof: any admitted observation is open with count zero; `entered =
  observed + 1` therefore has count one. CAS failure returns without retry, and
  post-entry closed/epoch mismatch still decrements exactly that one count.
  No successful path can publish count greater than one.
- Decision: no finding. Accept the delta for ordered cherry-pick immediately
  after the atomic queue formal gate; do not integrate the plugin early.

### DISCUSSION QUEUE-ATOMIC-SEAM-001

- Installed MSVC 14.51 audit: ordinary `std::condition_variable` stores state
  in situ and maps notify to noexcept `WakeConditionVariable`, but an unlocked
  producer still leaves a lost-wake window. `condition_variable_any` is rejected
  because it allocates a shared mutex and locks that mutex in `notify_one()`.
- Selected private seam: lock-free monotonic SPSC sequences; Windows auto-reset
  event created at configure and one `SetEvent` from producer/stop; POSIX
  nonblocking pipe created at configure, one write attempt from producer/stop,
  bounded poll/drain on consumer. Sticky state prevents a pre-wait signal loss.
- Producer hot path remains scalar copy, native-handle move, atomic publication
  and one nonthrowing/nonblocking OS signal. No QSemaphore/QMutex, capacity
  wait, allocation, retry loop, public method or strategy seam.
- Required proof: compile-time lock-free atomics, exact release/acquire order,
  stop/quiescence/handle cleanup, public stress/lifetime tests, Windows post-
  configure allocation count and bounded producer latency. Luna tests land
  before the Sol implementation request.

### PUBLISH QUEUE-ATOMIC-SEAM-001

- GitHub issue #5 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5201877301`.
- Authenticated `gh` write/API read-back exactly record the unchanged public
  seam, lock-free sequence ordering, Windows event/POSIX pipe selection,
  rejected condition-variable alternatives, required evidence and test-first
  dispatch order.

### SYSTEM RESPONSE THREAD-LIST-001

- One read-only recent-task list call, intended only to resolve the accepted
  queue test setup's eventual thread ID for later cleanup, returned no result
  across four bounded yields and was terminated before 60 seconds.
- No task/thread/worktree/repository/GitHub/dependency state changed. This is
  not converted into result polling; continue relying on the worker's proactive
  RESPONSE and resolve its final thread ID from that delivery.

### DISCUSSION PLUGIN-UI-IMPLEMENTATION-001

- Existing programmatic QWidget/QFormLayout and queued-signal patterns were
  reviewed. Selected file seam: modify plugin h/cpp/CMake; add one fixed POD
  diagnostics-types header and one setup-widget h/cpp. No `.ui`, qrc, settings
  registry, controller base or runtime strategy.
- Plugin control slots update only a PImpl pending snapshot containing enabled/
  frozen, four numerical settings, settings revision and reset sequence under a
  GUI/worker mutex. Acquisition callback never touches that mutex.
- Worker copies once per dequeued block, applies revision/reset before process,
  chooses one mode for the whole block and emits a fixed POD diagnostics value.
  Widget uses an explicit queued connection and formats all strings on GUI.
- Task remains undispatched until queue atomic and plugin-data gates pass; a
  fresh Sol/ultra block-boundary/diagnostics review follows integration.

### RESPONSE W-QUEUE-ATOMIC-TEST-001

- From/to: visible Luna/max queue-test worker / manager; source thread
  `019fd5fa-f207-7fe3-9189-8d77fc9cfade`.
- Exact parent/commit: `4bb1b2a30d74bc0f403621594f09878255cb9db` /
  `f367fd75a85df1dae17cf41f88bac74169974f8f`; worker is clean/detached and
  exactly one focused test source changes with clean committed diff.
- New public slots: sustained overlapping SPSC accepted-stream preservation;
  active producer/consumer stop race; pending-block stop/fresh reconfigure and
  destination preservation; native metadata lifetime until popped handle clear.
  Existing four queue slots gain complete Timeout/Stopped destination/extents/
  metadata preservation assertions.
- Oracles: precreated variable matrices/metadata and preallocated results;
  Pushed/Full accepted subsequence equals popped sequence/extents/metadata/data;
  sentinel tail preservation; finite atomic/deadline/join-safe synchronization;
  allowed stop linearizations only; fresh queue emptiness/new payload; custom
  deleter/liveness exactly once after caller ownership is cleared.
- Compile contract adds nothrow mutable-to-const QSharedPointer construction
  while retaining same-type traits. All tests stay on the public queue surface.
- Worker verification: C++14 syntax passes using complete Eigen read-only from
  outside the checkout. Isolated vendored Eigen is incomplete. A temporary
  MinGW/Qt build reached an executable but runtime failed `0xC0000139` from an
  ABI-mismatched runtime, so no worker runtime GREEN is claimed and no repo/
  vendor file changed.
- Restrictions: Luna/max, no production/CMake/docs/dependency edit, subagent,
  full scan/mne_scan or mne_rt_server.
- Status: response durable before manager provenance/code/oracle review,
  integration or populated Windows execution.

### ACCEPTANCE W-QUEUE-ATOMIC-TEST-001

- Provenance: commit `f367fd75a` has exact requested parent `4bb1b2a30`; its
  diff contains only the authorized focused test source and passes diff-check.
  The parent source blob equals the current integration blob.
- Manager code review: no assertion or exception-producing test macro runs in
  either worker thread; all joins precede outcome assertions. Finite deadlines,
  stop fallback and bounded waits prevent normal failure paths from leaving a
  joinable thread.
- Behavior review: accepted producer indices are compared in exact FIFO order
  with popped extents, metadata identities, payloads and untouched tails;
  Timeout/Stopped preserve the complete caller destination. Pending data is
  discarded across stop/fresh configure as specified.
- Ownership review: callers clear every native handle immediately after push;
  observable custom deleters remain idle until the popped handle clears and
  then fire exactly once. Tests inspect only the public queue interface.
- Decision: accept with no manager finding. Cherry-pick and populated Windows
  Release execution remain before closing the two queue P2 findings.

### VALIDATION W-QUEUE-ATOMIC-TEST-001

- Integration: worker commit cherry-picked as `16053ffe6`; only the focused
  test source changed and committed diff-check remains clean.
- Command: `cmake --build build-causal-reference-denoising --config Release
  --target test_adaptive_denoising_plugin -- /m:2`, followed by the focused
  executable with `-txt`.
- Result: build/link succeeds; the complete executable exits zero. Three
  immediate direct repeat executions also exit zero, with no hang or race
  failure. The installed reporter emits no textual totals, so no invented
  pass-count is recorded.
- Interpretation: both queue P2 test tracers are runnable and accepted. The
  QSemaphore P1 remains open because this run does not prove producer lock/
  allocation freedom; the atomic production task is still blocking.
- Public evidence/read-back:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5202622017`.

### REQUEST W-QUEUE-ATOMIC-GREEN-001

- From/to: manager / new visible Sol/ultra concurrency implementation task.
- Blocking: yes, issue #5 and every plugin integration task wait for this gate.
- Exact source base: `41a2b76fb3de1db6d0b2542eec792144e826f73d`.
- Authorized files only:
  `src/applications/mne_scan/plugins/adaptivedenoising/adaptivedenoisingblockqueue.h`
  and `.cpp`. The focused test, plugin, processor, CMake, docs and dependencies
  are immutable.
- Preserve the concrete final four-method public seam, config/status/data
  types, variable rectangle/native const metadata ownership, drop-newest,
  destination preservation, transactional configure and caller-quiescence
  requirement. Header comments may be corrected to describe sequences/wake.
- Remove every QSemaphore/QMutex/Qt wait primitive from the queue. Use unsigned
  monotonic SPSC producer/consumer sequences with release/acquire publication;
  built atomic state must have a compile-time always-lock-free proof on the
  supported target. Capacity remains bounded by `INT_MAX`, making unsigned
  distance/wrap handling unambiguous.
- Configure creates all slots and one sticky native wake resource before
  publication: Windows auto-reset event (`CreateEventW`/`SetEvent`/
  `WaitForSingleObject`); POSIX nonblocking pipe plus bounded `poll` and drain.
  Resource creation failure may throw during configure while preserving the
  prior stopped implementation. Destruction closes only after caller
  quiescence.
- Producer path: initial/final running observation, capacity check, scalar copy
  into preallocated slot, native QSharedPointer move, release publication and
  exactly one nonblocking OS signal attempt. It performs no allocation, lock,
  wait, retry/busy loop, Eigen resize/expression temporary, string/FIFF access
  or potentially throwing operation. Full/invalid/stopped never publish or
  advance.
- Consumer may perform bounded OS waits/rechecks to handle a stale sticky wake,
  but must not busy-wait. It returns Popped only after acquire publication;
  Timeout/Stopped/InvalidDestination preserve the complete destination. Stop
  is noexcept/idempotent, release-stores stopped and signals once; pending data
  may be discarded. Fresh configure owns wholly new sequences/slots/wake.
- Required validation: exact-parent/two-file/diff-clean proof; current focused
  public Release suite build/run if available; source audit proving no
  QSemaphore/QMutex and truthful noexcept; no dependency repair, full scan,
  full mne_scan or mne_rt_server. Do not alter the test to manufacture GREEN.
- Response format: `RESPONSE W-QUEUE-ATOMIC-GREEN-001` with commit/parent,
  exact files, sequence/order/stop/wake proof, allocation/lock/noexcept audit,
  build/test result and limitations. Proactively notify the manager once; do
  not poll it or create internal/nested subagents.

### FAILURE W-QUEUE-ATOMIC-GREEN-001-CREATE-1

- The first app creation call was rejected by argument validation because the
  project ID was supplied both at the call top level and inside the project
  target union.
- No task, thread, worktree, repository or external state was created or
  changed. Retry once with the documented target-only project ID; exact base,
  Sol/ultra model and request remain unchanged.

### CREATED W-QUEUE-ATOMIC-GREEN-001

- Corrected app creation accepted a new visible Sol/ultra worktree setup as
  `client-new-thread:3fb21cfc-1d5b-40a1-b6ef-fb57b91db17b`, host `local`, from
  exact requested ref `41a2b76fb3de1db6d0b2542eec792144e826f73d`.
- The prompt contains the full two-file atomic/event/pipe contract and proactive
  RESPONSE requirement. No final thread ID exists yet; do not poll setup.
- Public dispatch/read-back:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5202544531`.
- One bounded post-creation task-list resolution identifies the final visible
  thread as `019fd64b-2553-7102-91d6-1f8d35720823`, active in app worktree
  `8553`, with the exact task title/prompt. This is not repeated polling.

### RESPONSE W-QUEUE-ATOMIC-GREEN-001

- From/to: visible Sol/ultra atomic queue worker
  `019fd64b-2553-7102-91d6-1f8d35720823` / manager.
- Exact parent/commit: `41a2b76fb3de1db6d0b2542eec792144e826f73d` /
  `0a8202e405c41fba809052719b1c1164e1d4591b`; clean detached worktree,
  diff-check, and exactly queue header/source only.
- Public seam is unchanged. QSemaphore/QMutex/Qt waits are removed. Explicit
  atomic state is lock-free `unsigned int`; owner-local indices plus monotonic
  published sequences use release/acquire order and unsigned wrap distance with
  capacity below half the modulus.
- Producer final running observation precedes any write; a successful observed
  operation may finish Pushed after concurrent stop. Consumer checks stop before
  pending data and again before destination mutation. Stop release-exchanges
  once and signals once; fresh configured PImpl owns independent state/resource.
- Windows candidate creates one auto-reset event; producer/stop call one
  `SetEvent`, consumer performs deadline-bounded `WaitForSingleObject` rechecks.
  POSIX candidate creates nonblocking CLOEXEC pipe; producer/stop make one byte
  write attempt, consumer bounded-polls/drains/rechecks. Publication precedes
  signalling and sticky state closes the empty-check/wait lost-wake window.
- Transaction/noexcept audit: all slots/native wake allocate during candidate
  configure only; failures destroy candidate and preserve the stopped old PImpl.
  Producer contains only atomic checks, scalar rectangle copy, fixed extent/
  handle/index work, release publication and one native signal; no allocation,
  Qt semaphore/mutex, wait, retry, string, FIFF or throwing dependency.
- Worker Windows manual Release using current public test: 19/0/0, exit zero;
  overlap pushed/full/popped 300/212/300, max producer call 4100 ns, stop wake
  64 ms versus 3000 ms; five further runs exit zero. WSL GCC/Qt POSIX branch:
  19/0/0, 186/326/186, max 3996 ns, stop wake 50 ms; three further runs zero.
- Isolated CMake configure remains blocked by its ignored incomplete Eigen. No
  vendor/dependency copy/repair occurred. Worker did not add allocation counter
  because tests were immutable; manager still owns canonical populated run and
  separate Luna/max allocation tracer.
- Restrictions: Sol/ultra, no internal subagent, manager polling, full scan/
  mne_scan, mne_rt_server, GitHub or unrelated mutation.
- Status: response durable before manager provenance/code/race review,
  integration, canonical CMake execution or formal finding closure.

### ACCEPTANCE W-QUEUE-ATOMIC-GREEN-001

- Exact requested parent, two-file scope, diff-check and worker clean state all
  pass. Current integration queue blobs equal the worker parent blobs, so the
  production delta is directly applicable without hidden overlap.
- Sequence review: release producer publication/acquire consumer read and
  release consumer progress/acquire producer reuse form both required happens-
  before edges. Capacity `<=INT_MAX` is strictly below half the unsigned range;
  modulo difference is unambiguous and owner-local indices preserve arbitrary
  capacities across counter wrap.
- Stop review: the final acquire running observation is the operation/stop
  ordering point. False paths preserve state/destination; true paths may finish
  after a racing stop under the caller-quiescence contract. Pending data is
  deliberately discardable and fresh PImpl owns unrelated resources/state.
- Wake review: publication precedes one native signal. Sticky event/pipe state
  covers notification-before-wait; stale/coalesced wakes trigger bounded atomic
  rechecks. POSIX drain is consumer-only finite progress, not producer retry.
- Resource/hot-path review: candidate RAII preserves stopped ownership on every
  construction failure. `tryPush` has fixed scalar loops only and no Qt wait/
  mutex, allocation, expression temporary, retry/busy loop, string/FIFF or
  throwing dependency. Public seam remains a deep concrete module.
- Decision: accept with no manager P0-P3 finding. Cherry-pick, populated CMake
  Release plus repeats, separate allocation tracer and formal reviewer remain
  before closing `R-QUEUE-V2-QSEMAPHORE-001`.

### VALIDATION W-QUEUE-ATOMIC-GREEN-001-ATTEMPT-1

- Integration: atomic production is cherry-picked as `3ff146836`; exact
  two-file diff-check passes.
- Canonical populated Release target compiles/links. The main invocation and
  five immediate complete repeat invocations all return zero.
- Reporter retrieval limitation: Qt did not create the requested absolute temp
  `-o ...,txt` file, so `Get-Content` produced a nonterminating file-not-found
  error and no manager-emitted total is claimed from this attempt. Retry the
  reporter once from the temp working directory without rebuilding; worker's
  independent Windows/WSL 19/0/0 remains durable.

### VALIDATION W-QUEUE-ATOMIC-GREEN-001-ATTEMPT-2

- A single no-rebuild retry invokes the absolute canonical executable from the
  temp working directory with a relative Qt text reporter target. Process exits
  zero, but the app/tool path again emits neither console nor result file.
- Decision: stop reporter retries. Canonical claim is build/link plus main/five
  repeat zero exits; exact 19/0/0 totals come from worker's independent Windows
  and POSIX public-suite files.
- Public integration/read-back:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5202971691`.

### REQUEST W-QUEUE-ALLOC-TEST-001

- From/to: manager / new visible Luna/max test conversation.
- Blocking: yes, queue P1 formal closure waits for this evidence.
- Exact base: `9b7b1c779` (atomic production integrated; canonical tests zero).
- Authorized file only:
  `src/testframes/test_adaptive_denoising_plugin/test_adaptive_denoising_plugin.cpp`.
  Queue/plugin/processor/core/CMake/docs/vendor/dependencies are immutable.
- Install complete C++14 global scalar/array throwing/nothrow and sized where
  required `new`/`delete` replacements in the test translation unit using
  `malloc/free`. A thread-local boolean controls counting; a global atomic
  counter increments only when the producer thread flag is true.
- Extend the existing sustained overlapping SPSC public test rather than
  duplicate its traffic: enable the flag immediately before each `tryPush` and
  disable immediately after. All configuration, workload/metadata/result/thread
  construction, consumer work, latency storage, logging and assertions remain
  outside counted allocation scope. Require exactly zero allocations.
- Preserve all current accepted-subsequence FIFO/extents/metadata/tail/Full and
  one-second latency oracles and every other public slot. Do not inspect source
  or ring internals; do not change production to manufacture GREEN.
- Use TDD skill/public behavior. Build/run populated Release if available;
  otherwise provide syntax/scope evidence and manager owns canonical run. Exact
  one-file commit/parent/diff/clean proof; no full scan/mne_scan/mne_rt_server,
  vendor repair or internal subagent. Proactively send
  `RESPONSE W-QUEUE-ALLOC-TEST-001`; do not poll manager.

### FAILURE W-QUEUE-ALLOC-TEST-001-CREATE-1

- The first local task-creation script failed to parse before any app call
  because unescaped backticks around the word `new` terminated its JavaScript
  template literal.
- No task, worktree, repository or external state changed. Retry once with the
  same exact base/model/scope using plain prompt text.

### CREATED W-QUEUE-ALLOC-TEST-001

- Corrected app call accepted visible Luna/max worktree setup
  `client-new-thread:c4ec3fcf-866e-41d9-8c7d-9fd4680dc7f2`, host `local`, from
  exact ref `9b7b1c779`.
- Prompt contains the one-test-file global-allocation/thread-local counted-region
  contract and proactive response rule. No final thread ID exists yet; do not
  poll setup.
- Public dispatch/read-back:
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5202951958`.

### RESOLVED W-QUEUE-ALLOC-TEST-001

- One read-only task-list lookup resolved setup
  `client-new-thread:c4ec3fcf-866e-41d9-8c7d-9fd4680dc7f2` to visible Luna/max
  thread `019fd670-c791-7000-8a5f-4538396a54d6`, app worktree `4954`.
- The exact base, one-test-file scope and proactive RESPONSE contract are
  unchanged. Use at most one bounded wait; do not continuously poll or read
  the worker worktree.

### RESPONSE W-QUEUE-ALLOC-TEST-001

- From/to: visible Luna/max test thread
  `019fd670-c791-7000-8a5f-4538396a54d6` / manager.
- Exact parent: `9b7b1c779fbe6ce52001a18dc3dfbe0a11a9188d`.
- Worker commit: `ea81256b1283982062b0e4df6007e1226e89675f`.
- Exact scope: only
  `src/testframes/test_adaptive_denoising_plugin/test_adaptive_denoising_plugin.cpp`;
  detached worktree reports clean.
- Adds malloc/free-backed C++14 global scalar/array throwing, nothrow and sized
  allocation/deallocation replacements. Zero-size allocation is normalized;
  throwing forms throw `bad_alloc`; nothrow forms return null.
- A thread-local flag is enabled only for the complete producer
  `tryPush(block, metadata)` expression. The global atomic counter increments
  only after a successful allocation while that producer flag is true.
- All setup, queue configuration, precreated Eigen/metadata/result storage,
  thread construction, consumer work, joins, logging and assertions remain
  outside the counted boundary. Existing FIFO/drop/extents/metadata/deep-copy/
  tail/preservation/latency oracles remain unchanged; the new exact oracle is
  zero counted producer allocations.
- Worker diff-check passes. Runtime is honestly unclaimed because the isolated
  worktree lacks a populated Release build and complete ignored Eigen baseline;
  no dependency repair, full mne_scan or mne_rt_server occurred. Manager owns
  canonical review/build/run.

### REVIEW W-QUEUE-ALLOC-TEST-001

- Exact commit object, parent `9b7b1c779`, one-file scope and diff-check pass.
  The current integration test blob is identical to the worker parent blob, so
  the delta applies without hidden test divergence.
- Manager reviewed all added overloads and the complete sustained-SPSC change.
  C++14 scalar/array throwing, nothrow, unsized and sized forms are present at
  global scope; zero-size, failure and free pairing are correct.
- The atomic and thread-local controls are zero-initialized before ordinary
  allocations. The flag covers the complete public call expression, including
  the by-value native metadata copy, and cleanup is the immediately following
  statement under the queue's `noexcept` contract. Setup, consumer and oracle
  work remain outside the boundary.
- Decision: ACCEPT with no manager P0-P3. Cherry-pick and canonical populated
  Release build/run are required before using this as formal queue evidence.

### VALIDATION W-QUEUE-ALLOC-TEST-001

- Worker delta is integrated as `34aff76b6` and pushed. Canonical populated
  MSVC Release target `test_adaptive_denoising_plugin` compiles and links; only
  existing Eigen C4819 code-page warnings appear.
- The complete executable main run exits zero. Five immediate complete repeats
  also return `0,0,0,0,0` with no timeout or deadlock.
- The zero-allocation check is a hard `QCOMPARE` inside the sustained public
  SPSC slot, so a zero process result proves it passed together with the
  existing FIFO/drop/extents/metadata/deep-copy/tail/latency assertions.
- Qt text reporter invocation still emits no file or stream; do not infer or
  publish a manager test-total count. Exact worker cross-platform totals remain
  separate evidence. This one-shot Luna test conversation is retirement-ready
  after formal-review dispatch no longer needs it.

### REQUEST R-QUEUE-ATOMIC-002

- From/to: manager / retained independent Sol/ultra queue reviewer thread
  `019fd5d8-380d-7563-8644-264426bda199`.
- Blocking: yes; issue #5 and plugin integration wait for this formal gate.
- Exact review snapshot: `9535bf8c7`, clean pushed integration state after
  atomic queue implementation, public concurrency/lifetime tests, producer
  allocation tracer and canonical GREEN evidence.
- Read-only scope: queue header/source, focused plugin test, their CMake seam,
  current SPEC/STATE/WORKERS/REVIEW/JOURNAL evidence and issue #5. No edit,
  stage, commit, push, GitHub mutation, dependency repair or server run.
- Re-evaluate original findings explicitly:
  `R-QUEUE-V2-QSEMAPHORE-001`, `R-QUEUE-V2-CONCURRENCY-TEST-001` and
  `R-QUEUE-V2-METADATA-LIFETIME-001`. Inspect atomic ordering/wrap/full/reuse,
  Windows event and POSIX pipe lost-wake/stop behavior, transactional resources,
  producer no-lock/no-wait/no-allocation/noexcept truth, full destination
  preservation and QSharedPointer lifetime evidence.
- Report P0-P3 with exact file/line, evidence, impact, fix and required test;
  distinguish independently reproduced evidence from manager/worker evidence.
  P0/P1 must be zero; P2 must be fixed or explicitly deferred before PASS.
- Use codebase-design deep-module vocabulary and proactively return
  `RESPONSE R-QUEUE-ATOMIC-002`; do not poll manager or use subagents.

### DISPATCHED R-QUEUE-ATOMIC-002

- The retained visible thread accepted the exact request with explicit
  `gpt-5.6-sol` / `ultra` overrides. It owns read-only review of `9535bf8c7` and
  must proactively return; manager will not continuously poll it.

### RESPONSE R-QUEUE-ATOMIC-002

- Reviewer: retained visible Sol/ultra thread
  `019fd5d8-380d-7563-8644-264426bda199`; exact clean detached snapshot
  `9535bf8c77ea02cb650d56b97816c9ef88198b6a`, read-only throughout.
- Gate: HOLD; P0=0, P1=0, P2=2, P3=1.
- CLOSED original `R-QUEUE-V2-QSEMAPHORE-001`: production contains no Qt
  semaphore/mutex/wait and producer work is lock-free atomics, scalar copy,
  nothrow native owner move and one nonblocking native signal.
- CLOSED original `R-QUEUE-V2-METADATA-LIFETIME-001`: public test clears caller
  owners after push, proves liveness through pop and exactly-once deletion after
  destination clear; installed Qt native refcount operations are noexcept.
- OPEN P2 `R-QUEUE-ATOMIC-POSIX-EINTR-001` at queue cpp `150-155,339-340,
  399-423,428-433`: one POSIX nonblocking pipe write discards EINTR. If it is
  interrupted before transfer after the consumer's last atomic recheck, push or
  stop remains discoverable only after the full wait timeout, contradicting the
  prompt sticky-wake contract. Required: private non-lossy/bounded fallback with
  no producer allocation/lock/wait/unbounded retry plus deterministic POSIX
  fault-injection test for push and stop.
- OPEN P2 `R-QUEUE-V2-CONCURRENCY-TEST-001` at focused test `1451-1511,
  1513-1638,1660-1672,1706-1807,1828-1838` and old waiter `994-1033`:
  sustained traffic may accept only capacity before consumer drain; stop may
  occur before public push/wait calls. Current assertions can therefore pass
  without concurrent slot reuse or an actually blocked waiter. Required public
  deterministic phases: pushed > capacity, pop before producer completion,
  successful prelude, positive blocked-wait elapsed bound, live producer stop,
  full rectangle/extents/metadata/tail/sentinel checks.
- OPEN P3 `R-QUEUE-ATOMIC-SPEC-LOCALITY-001` at SPEC `538-550`: stale zero-time
  free-slot semaphore and unspecified consumer-tail wording contradict current
  atomic/native wake and untouched-tail interface. Doc-only reconciliation and
  targeted historical/rejection-only semaphore search required.
- Static atomic wrap/publication/reuse, Windows event, transactional resource,
  rectangle/extents/drop/metadata behavior are otherwise accepted. Reviewer
  independently claims source/provenance/Qt-header audit, not build/runtime;
  manager/worker executable evidence is clearly separated.

### REQUEST W-QUEUE-CONCURRENCY-TEST-002

- From/to: manager / new visible Luna/max TDD conversation.
- Blocking: yes; closes regression sensitivity half of queue formal P2.
- Exact base: `2530c78642c4742ffcb06bed99e4d7048192de64`.
- Authorized file only:
  `src/testframes/test_adaptive_denoising_plugin/test_adaptive_denoising_plugin.cpp`.
  Production queue/plugin/processor/core/CMake/docs/vendor/dependencies immutable.
- Add deterministic bounded public-interface phases, not source/private hooks:
  force a capacity-N queue to publish >N accepted blocks by holding the producer
  after initial fill, requiring a consumer pop before producer completion, then
  reusing the freed wrapped slot with distinct rectangle/metadata. Assert exact
  accepted/pop order, payload, extents, native owner and untouched tail.
- Strengthen stop evidence with a successful push/pop prelude, an empty public
  wait whose measured call duration has a positive lower bound before stop, a
  live producer loop/gate at stop, bounded join and exact Popped/Timeout/Stopped
  destination/extents/metadata/tail preservation. Remove/replace marker-only
  assertions that can pass before public calls are entered.
- Keep all existing tests/zero-allocation oracle and finite deadlines. Use TDD;
  show the old checked-in assertions' false-green sensitivity and the new oracle
  RED/GREEN classification honestly. Exact one-file commit/parent/diff/clean
  proof; populated Release if available, otherwise manager run. No server/full
  scan/subagent/manager polling; proactively return structured RESPONSE.

### REQUEST W-QUEUE-SPEC-001

- From/to: manager / new visible Luna/max documentation conversation.
- Blocking: P3, parallel-safe with the test task.
- Exact base: `2530c78642c4742ffcb06bed99e4d7048192de64`.
- Authorized file only: `doc/causal_reference_denoising/SPEC.md`; no code/test/
  CMake/other-doc/vendor/dependency changes.
- Reconcile lines near 538-550 with the authoritative atomic queue: remove the
  prescriptive free-slot semaphore/zero-time acquire, describe release/acquire
  SPSC sequences plus configure-created native sticky wake, and require the
  maximum-sized consumer destination outside-tail to remain untouched on pop.
  Preserve historical/review text that explicitly names rejected QSemaphore as
  history, not a live requirement.
- Verify targeted searches show no prescriptive semaphore contradiction and
  all current atomic/native/tail terms agree with header/source/learning guide.
  Exact one-file comment-only commit/parent/diff/clean proof; Luna/max, no build,
  server, subagent or manager polling; proactively return structured RESPONSE.

### PREPARED W-QUEUE-POSIX-EINTR-TEST-001

- Dispatch waits for `W-QUEUE-CONCURRENCY-TEST-002` integration because both own
  the focused test source; no overlap is allowed.
- Read-only environment probe confirms WSL Ubuntu 24.04, GCC 13.3, CMake 3.28.3,
  Qt5Core/Qt5Test 5.15.13 and repository access at the `/mnt/c` path.
- Planned narrow RED seam: Linux-only GNU linker `--wrap=write` in focused CMake
  plus test-translation-unit thread-local `__wrap_write`/`__real_write`. Force
  exactly the queue caller thread's next native signal to return pre-transfer
  EINTR for a blocked long-timeout push and stop; require prompt Popped/Stopped,
  exact payload or complete destination preservation. No public queue hook.

### UPDATE W-QUEUE-CONCURRENCY-TEST-002

- One bounded 60-second task wait timed out with the visible Luna/max thread
  still active. Its latest proactive progress says the deterministic sustained
  and stop-race gates are implemented and it is checking C++14/QtTest syntax.
- No response/commit is accepted yet. Manager will not poll again; integration
  waits for the worker's proactive structured `RESPONSE`.

### RESPONSE W-QUEUE-CONCURRENCY-TEST-002

- From visible Luna/max thread
  `019fd69f-9196-7a82-9c3f-941076fb84fa`.
- Exact parent `2530c78642c4742ffcb06bed99e4d7048192de64`; commit
  `7aa5360a2a17f9f550679eb55f63d6837cf4a490`; one changed path only:
  `test_adaptive_denoising_plugin.cpp`; worker worktree clean.
- Sustained public SPSC now gates an initial capacity fill, requires a consumer
  pop before producer completion, forces a later distinct wrapped-slot push and
  asserts `pushed > capacity`, exact accepted FIFO/extents/native metadata/
  payload/tail plus zero producer allocations.
- Stop coverage reports a successful push/pop prelude, measured positive empty
  wait, live producer at stop, finite deadlines/joins, and complete Popped/
  Timeout/Stopped payload/extents/metadata/tail or preservation oracles.
- Worker MSVC 14.51 C++14 Release-style manual build reports 19/0/0 plus three
  repeat zero exits. Isolated CMake remains blocked only by its incomplete
  ignored Eigen checkout; no vendor repair or server/full application run.
- Status: response durable; manager provenance/diff/oracle review pending.

### REQUEST W-QUEUE-CONCURRENCY-TEST-002-REVISE-1

- From: manager. To: retained Luna/max processor/queue-test thread
  `019fd69f-9196-7a82-9c3f-941076fb84fa`. Blocking: yes.
- Base: exact worker commit `7aa5360a2`; authorized file remains only the
  focused test source. Do not amend/rewrite the accepted parent; return one
  small delta commit.
- Problem: the active-stop marker is written before the public long wait, so a
  pre-call scheduling pause can still make an immediate post-stop call appear
  to prove a blocked wake.
- Required response: time the complete consumer stop-wait call, publish its
  elapsed duration with the returned `Stopped` status, and after join require a
  positive lower bound (at least 5 ms) and the existing prompt upper bound.
  Preserve forced wrap/reuse, every payload/extents/metadata/tail/preservation
  oracle, allocation count, finite deadlines and post-join Qt assertions.
- Evidence: exact delta/parent/scope/clean proof and full focused MSVC Release-
  style result plus repeats; no production/CMake/vendor/server/full app change.
- Dispatch: accepted by the retained visible thread with explicit Luna/max;
  it must proactively return `RESPONSE W-QUEUE-CONCURRENCY-TEST-002-REVISE-1`
  and must not poll the manager.
- Delivery fault: a cursor-based wait reports the follow-up turn completed, but
  thread status is `notLoaded` and neither wait nor one read returns an
  assistant response. Do not infer a commit. Send one recovery request asking
  the same conversation to resend only its structured response/status.
- Recovery dispatch: the same Luna/max thread accepted a response-only request;
  it is forbidden to edit/amend/reset/rebuild/redo and must report either its
  exact existing delta commit or explicitly that no commit exists.

### RESPONSE W-QUEUE-CONCURRENCY-TEST-002-REVISE-1

- Recovery response from retained Luna/max thread: no revision delta commit
  exists. The clean detached worktree remains at original commit `7aa5360a2`
  with exact parent `2530c7864` and one focused-test path only.
- The requested stopped-wait elapsed measurement/assertion is not present and
  no revision GREEN is claimed. Existing `7aa5360a2` manual evidence remains
  19/0/0 plus three repeats with forced reuse and zero counted allocations.
- Manager action: integrate the accepted original concurrency improvement with
  `R-QUEUE-STOP-WAIT-ELAPSED-001` explicitly open, validate it canonically,
  archive this used conversation, then create a fresh Luna/max one-file task
  for the missing elapsed-wait delta.

### INTEGRATED W-QUEUE-CONCURRENCY-TEST-002

- Worker commit `7aa5360a2` is integrated as `1a478f6c3` after exact parent,
  one-file scope, diff-check and public-oracle review. The separate elapsed-wait
  P2 remains explicitly open; this integration does not claim queue gate PASS.
- Populated MSVC Release target compiles/links with only existing Eigen C4819
  warnings. The complete focused executable and three immediate repeats return
  `0,0,0,0`, exercising the hard forced-wrap/reuse/FIFO/allocation assertions.
- The used retained test conversation may now be archived. A new Luna/max task
  will own only the missing stopped-wait elapsed delta from the new integration
  base, in accordance with the no-reuse rule for spent conversations.
- Cleanup: visible thread `019fd69f-9196-7a82-9c3f-941076fb84fa` is app-
  archived after its exact clean commit and canonical integration evidence; its
  app worktree is not manually deleted.

### REQUEST W-QUEUE-STOP-WAIT-TEST-001

- From: manager. To: new visible Luna/max test conversation. Blocking: yes.
- Exact base: `8539c8e8564c0bee5da00023817f77e50d2212c1`.
- Authorized path only:
  `src/testframes/test_adaptive_denoising_plugin/test_adaptive_denoising_plugin.cpp`.
- Add the missing regression-sensitive measurement around the complete
  consumer-thread stop-phase public `waitPop` call in
  `queueStopRacesActiveProducerAndConsumer`. Publish the elapsed duration with
  the `Stopped` result and assert after join `>=5 ms` and `<1500 ms`.
- Preserve all forced-wrap/reuse, public payload/extents/native-owner/tail,
  Timeout/Stopped preservation, allocation, deadline/fallback and post-join Qt
  assertions. No production/CMake/docs/vendor/full application/server change.
- Required response: exact parent/new delta/scope/clean proof and full focused
  MSVC Release plus at least three repeat results; proactively send structured
  `RESPONSE W-QUEUE-STOP-WAIT-TEST-001` without manager polling.
- First creation attempt is rejected during local argument validation because
  project ID was duplicated at top level and inside `target`; no thread or
  worktree is created. Retry once with project ID only inside `target`.
- Corrected setup accepted from exact requested commit as
  `client-new-thread:ae1d9eae-8a70-4a18-bc5f-4c83f61ac9ba` on local host with
  explicit Luna/max. Await proactive response; do not poll or inspect worktree.
- One task-list resolution maps the setup to visible thread
  `019fd71e-6cf2-7e52-a410-29c374cb0e09`, app worktree `f36c`; it is active on
  the requested exact scope. No further listing/polling is needed.
- Bounded progress snapshot: worker is still active; isolated CMake reaches
  MSVC/Qt then stops at the known incomplete ignored Eigen checkout. It is
  checking only for a usable read-only populated baseline and otherwise will
  return the limitation explicitly. No response/commit is accepted yet.

### FAILURE W-QUEUE-STOP-WAIT-TEST-001

- Visible Luna/max thread `019fd71e-6cf2-7e52-a410-29c374cb0e09` completes with
  app status `systemError` after a file-change marker but without a final
  RESPONSE or commit identity. No worker output is accepted and manager does
  not inspect/copy its worktree.
- Archive this failed one-shot conversation and create a new visible Luna/max
  retry from the same exact committed source base and one-file scope.
- Cleanup complete: failed thread is app-archived; its worktree is not manually
  deleted and no file/commit from it is consumed.

### REQUEST W-QUEUE-STOP-WAIT-TEST-001-RETRY-1

- Same requirement as `W-QUEUE-STOP-WAIT-TEST-001`: exact base
  `8539c8e8564c0bee5da00023817f77e50d2212c1`, focused test source only, measure
  the complete consumer stopped-wait call and assert `>=5 ms` and `<1500 ms`
  after join while preserving all existing public concurrency oracles.
- Use a fresh visible Luna/max worktree/conversation. Commit before any optional
  runtime workaround; if isolated Eigen blocks build, return the clean exact
  commit and let manager run the populated canonical target.
- Corrected setup accepted as
  `client-new-thread:c101f7af-8999-4032-8fd3-387441e0a909` with explicit
  Luna/max. Await proactive response without polling or worktree inspection.

### RESPONSE W-QUEUE-STOP-WAIT-TEST-001-RETRY-1

- From visible Luna/max thread `019fd76b-9843-7d11-a81a-1dd993fbabb2`.
- Reported commit `cee7ed3257a904294f24452d25afa06cd87fef16`, one focused-
  test file only and clean detached worktree. The response's `parent` field is
  invalid: it contains manager thread ID
  `019fcdc3-4a1e-76d1-8140-1bd521219297`, not a Git SHA. Manager must query the
  commit object and accept only if its real parent is exact `8539c8e85`.
- Reported delta times the complete stop-phase consumer `waitPop`, publishes
  atomic elapsed milliseconds, logs it and asserts `Stopped`, `>=5 ms` and
  `<1500 ms` after join while retaining all previous oracles.
- Isolated CMake reached MSVC/Qt but stopped at the known incomplete Eigen
  checkout; no dependency repair/search. Manager owns populated Release run.
- Status: response durable; exact Git provenance/diff/oracle review pending.

### INTEGRATED W-QUEUE-STOP-WAIT-TEST-001-RETRY-1

- Direct Git proof corrects the malformed response field: `cee7ed325` parent is
  exact `8539c8e85`; scope is one focused test file and diff-check passes.
- Integrated as `103d5a5a8`. Populated MSVC Release compiles/links with only
  existing Eigen C4819 warnings; the complete focused executable plus three
  repeats return `0,0,0,0`.
- `R-QUEUE-STOP-WAIT-ELAPSED-001` closes. The one-shot retry conversation can be
  app-archived after this evidence is pushed; no worktree deletion.
- Cleanup complete: visible retry thread
  `019fd76b-9843-7d11-a81a-1dd993fbabb2` is app-archived after integration and
  canonical GREEN; its worktree is not manually deleted.

### REQUEST W-QUEUE-POSIX-EINTR-TEST-001

- From: manager. To: new visible Luna/max test worker. Blocking: yes.
- Exact source base: `d5fd0e4ee` on the pushed integration branch.
- Authorized paths only: focused test source and its focused `CMakeLists.txt`.
- Add Linux-only GNU/Clang link wrapping of `write` (`-Wl,--wrap=write`) and a
  test-translation-unit thread-local one-shot wrapper. When armed on the queue
  caller thread, exactly the next native signal write returns `-1/EINTR` before
  transfer; all other writes delegate to `__real_write`.
- Add public push and stop cases with a blocked long-timeout consumer. Forced
  post-publication EINTR must still yield prompt exact `Popped`; forced stop
  EINTR must yield prompt `Stopped` with full destination preservation. Use a
  generous `<500 ms` prompt bound versus a 3000 ms timeout and finite joins.
- This is expected RED on current POSIX production and must not add a public
  hook or modify queue/plugin/processor/core/docs/vendor/server files. Windows
  compile/tests remain unaffected by conditional code.
- Required response: exact commit/parent/two-file scope, deterministic wrapper
  proof, WSL RED if service is available, otherwise explicit environment block,
  Windows syntax/build non-regression if available, diff-check and clean status.
- Visible Luna/max worktree setup accepted as
  `client-new-thread:c4c3d466-a99d-4429-8804-0e68220c357a` from exact requested
  ref. Await proactive response; do not poll or inspect its worktree.

### RESPONSE W-QUEUE-POSIX-EINTR-TEST-001

- From visible Luna/max thread `019fd779-c771-7f40-8ce7-b23cdc7fa175`.
- Exact parent `d5fd0e4ee22d266bf38e18b3eaee777308779170`; commit
  `c4b13a220f163511e5d84ed6378fc7545edd4965`; exactly the focused CMake and
  test source; clean detached worktree and diff-check pass.
- Linux non-Apple GNU/Clang CMake adds `--wrap=write`; guarded test wrapper uses
  a thread-local one-shot arm and delegates every unarmed write. Public push and
  stop cases use 3000 ms waits, `<500 ms` prompt gates, exact payload/extents/
  native metadata/tail or full preservation, finite fallback and wrapper count
  one assertions.
- WSL `uname` timed out, so no POSIX runtime RED is claimed or fabricated. The
  worker did not reset/shutdown WSL. Isolated Windows configure remains blocked
  before target generation by the known incomplete ignored Eigen checkout.
- Status: response durable; manager provenance/source/CMake/oracle review and
  populated Windows non-regression remain before integration.

### Conversation cleanup rule reaffirmed

- User again requires timely cleanup. Archive every one-shot/failed/spent
  visible conversation immediately after response, exact clean/scope review,
  integration and durable test/GitHub evidence. Retain only conversations with
  a concrete same-responsibility follow-up: atomic POSIX fix, final queue review
  and SPEC wording until their gates close. Never manually delete app worktrees.
- Cleanup action: superseded pre-atomic queue-v2 implementation conversation
  `019fd52e-1d77-7a22-a8f3-ab51728560c3` is app-archived. It has no remaining
  responsibility; no worktree is manually deleted.

### PREPARED W-QUEUE-POSIX-EINTR-GREEN-001

- Dispatch only after the public WSL `--wrap=write` tracer is integrated RED.
  Reuse retained Sol/ultra atomic implementation conversation
  `019fd64b-2553-7102-91d6-1f8d35720823`.
- Private fix remains inside `NativeWake`: on POSIX cap each consumer `poll`
  slice (planned 25 ms) while preserving the caller's original steady-clock
  deadline and outer atomic sequence/running rechecks. A producer/stop still
  makes exactly one nonblocking `write`; no retry, allocation, lock or wait is
  added to the producer path. Windows auto-reset-event behavior is unchanged.
- Authorized production scope will be queue header/source only; a later narrow
  SPEC wording follow-up may state the exact bounded recheck slice.

### Plugin-data integration preflight

- Read-only merge-tree/diff preflight for accepted plugin commits
  `5f4718722` plus `34a8096f1` against current integration finds no overlapping
  conflict marker. Pending scope remains exactly the plugins CMake registration
  plus six new `adaptivedenoising` files and the one-source admission fix.
- This is not integration or build acceptance. The commits remain held behind
  final queue PASS and will be cherry-picked in original-then-fix order.

### FAILURE QUEUE-CORRECTION-PROJECT-LIST-1

- First read-only app project-list call produced no result within 60 seconds and
  was terminated to preserve manager commentary cadence. No task/worktree,
  repository or external state changed.
- Retry once with a short bounded read; if still unavailable, use a previously
  verified saved-project identifier rather than polling.

### FAILURE QUEUE-CORRECTION-CREATE-1

- Parallel app create calls duplicated `projectId` at the request root and the
  project target. Schema validation rejected both calls before task/worktree
  creation; repository and external state remain unchanged.
- Retry once with project ID only inside the project target. Exact bases,
  Luna/max models, prompts and disjoint file scopes remain unchanged.

### CREATED W-QUEUE-CONCURRENCY-TEST-002

- Corrected app call accepts visible Luna/max worktree setup
  `client-new-thread:d045832c-271e-4ea6-8e0c-d6e4e0c0980e` from exact
  `2530c78642c4742ffcb06bed99e4d7048192de64`.
- Scope remains the focused plugin test source only with deterministic public
  concurrent-reuse/blocked-stop evidence. No final thread ID exists yet; await
  proactive RESPONSE without polling.

### CREATED W-QUEUE-SPEC-001

- Corrected app call accepts separate visible Luna/max worktree setup
  `client-new-thread:69772d64-aeb0-47af-bacc-a1b154974991` from the same exact
  base.
- Scope remains SPEC only and is parallel-safe with the test editor. No final
  thread ID exists yet; await proactive RESPONSE without polling.

### DISPATCHED QUEUE-HOLD-CORRECTIONS-001

- Issue #5 comment/read-back succeeds at
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5203461409`.
- It records exact base, both Luna/max setup IDs/disjoint scopes, forced reuse/
  blocked-stop acceptance, SPEC correction and the later POSIX-RED/Sol-fix
  sequence. Both tasks remain proactive-response only.

### RESOLVED QUEUE-HOLD-CORRECTION-THREADS-001

- A single read-only task-list lookup resolves the concurrency setup to visible
  thread `019fd69f-9196-7a82-9c3f-941076fb84fa`, app worktree `a656`.
- It resolves the SPEC setup to visible thread
  `019fd69f-9196-7a82-9c3f-94295c3cbeb5`, app worktree `17e6`.
- Both are active Luna/max tasks on the exact requested base and scopes. Do not
  poll or inspect their worktrees; consume only proactive structured responses.

### RESPONSE W-QUEUE-SPEC-001

- Luna/max thread `019fd69f-9196-7a82-9c3f-94295c3cbeb5` returns commit
  `96055bf1350e9b753f265156c9a8b819cd87006c` on exact parent
  `2530c78642c4742ffcb06bed99e4d7048192de64`.
- Exact scope: only `doc/causal_reference_denoising/SPEC.md`, worktree clean,
  diff-check pass; no build/full scan/server/vendor/subagent work.
- Reported correction: live queue contract now uses always-lock-free unsigned
  SPSC sequences, acquire/release publication/reuse, immediate Full/drop-newest,
  configure-created Windows auto-reset event/POSIX nonblocking CLOEXEC pipe,
  bounded consumer rechecks, untouched Popped tail, complete Timeout/Stopped/
  InvalidDestination preservation and caller quiescence.
- Remaining semaphore mentions are reported explicitly historical or rejected,
  not prescriptive. Manager exact semantic/search review precedes integration.

### REVIEW W-QUEUE-SPEC-001

- Exact object/parent/one-SPEC-file scope and diff-check pass. Direct review of
  the complete changed paragraph matches the queue header/source/learning guide:
  lock-free sequence ordering, drop-newest, native event/pipe, bounded rechecks,
  untouched Popped tail, complete non-success preservation and quiescence.
- Targeted search finds six semaphore/QSemaphore contexts: the old global
  CircularBuffer baseline, callback no-semaphore statements, the formal Windows
  rejection and no-QSemaphore evidence. None prescribes it for the live queue.
- Decision: ACCEPT with no manager P0-P3. Integrate; keep the same Luna/max
  conversation reusable through the final queue review in case the later POSIX
  bounded-recheck correction needs one wording-only follow-up.

### VALIDATION W-QUEUE-SPEC-001

- Accepted commit is cherry-picked/pushed as integration `c56f19269`. Exact
  one-SPEC-file committed diff-check passes and the manager's targeted legacy/
  rejection-only semaphore search remains unchanged.
- `R-QUEUE-ATOMIC-SPEC-LOCALITY-001` is manager-closed; retain the worker only
  through final exact-snapshot reviewer confirmation or a narrow POSIX wording
  follow-up.

### ARCHIVED W-QUEUE-ALLOC-TEST-001

- App archive succeeds for one-shot Luna/max thread
  `019fd670-c791-7000-8a5f-4538396a54d6` after exact commit, clean response,
  manager review, integration, canonical Release and public evidence became
  durable. Its app-owned worktree was not manually altered or deleted.

### RETIREMENT-READY W-QUEUE-ATOMIC-TEST-001

- Thread `019fd5fa-f207-7fe3-9189-8d77fc9cfade` is one-shot. Exact commit,
  clean status, one-file blob, manager review, integration, populated Release,
  repeat runs and public production follow-up are durable.
- It has no remaining same-responsibility work. Archive through the app without
  manually modifying or deleting its app-owned worktree.

### ARCHIVED W-QUEUE-ATOMIC-TEST-001

- App archive succeeded for visible thread
  `019fd5fa-f207-7fe3-9189-8d77fc9cfade`. Its app-owned worktree was not
  manually changed or deleted.

### DISCUSSION PLUGIN-UI-PATTERN-002

- Read-only repository audit confirms the central widget destroys each setup
  widget and established plugins return a fresh instance from `setupWidget()`.
- Decision: the future adaptive widget is also fresh per call and the plugin
  stores no widget pointer. QObject auto-disconnect plus an explicit queued
  diagnostics connection protects destruction; persistent settings remain only
  in the plugin PImpl pending snapshot.
- The diagnostics POD reuses existing configure/process enum and scalar fields
  plus dropped blocks. It must not become a second state model.

### PREPARED W-QUEUE-ALLOC-TEST-001

- Not dispatched; wait for accepted atomic production so test and production
  never overlap.
- Future Luna/max scope is the focused test source only. Add a Windows producer-
  thread allocation-counting slot using complete C++14 global new/delete
  replacements gated by a thread-local flag only around `tryPush()`.
- All configuration/workload/metadata/thread/result/logging work occurs outside
  the counted region. Require zero counted allocations, preserve public FIFO/
  extents/metadata outcomes, report latency distribution and retain the current
  nonflaky one-second hard upper bound.
- Source/platform/no-QSemaphore proof remains a Sol/ultra review responsibility;
  the public test must not inspect private state or parse implementation text.

### DISCUSSION ISSUE-5-ATOMIC-CONTRACT-001

- Read-only issue audit finds #5 body still requires a zero-timeout QSemaphore
  acquire, contradicting the formal Windows P1 and frozen atomic/event/pipe
  correction already dispatched.
- Decision: replace only issue #5 body with the current public seam, atomic SPSC
  wake design, completed public-test evidence, active Sol task and remaining
  allocation/re-review gates. Preserve title, issue number and open state.
- Result: `gh issue edit` succeeds; authenticated `gh issue view` read-back
  confirms the replacement body and unchanged title/number/open state.

### PREPARED W-DOC-001-REVISE-2

- Not dispatched while retained Luna/max doc conversation is actively handling
  `REVISE-1`; do not overlap the same learning-guide file.
- New P2 `R-DOC-BENCHMARK-EVIDENCE-002`: held guide contains no durable actual
  benchmark configuration/result despite issue #7 acceptance requiring it.
- After the proactive current response, send one combined narrow follow-up:
  replace any QSemaphore wording with the accepted atomic sequence/native wake
  seam and add exact `--benchmark` workload plus measured
  p50/p95/max `2.725/4.317/8.557 ms`, explicitly engineering evidence.
- Public finding/read-back:
  `https://github.com/Edddddddddy/mne-cpp/issues/7#issuecomment-5202684965`.

### RESPONSE W-DOC-001-REVISE-1

- Retained Luna/max documentation thread
  `019fd2fc-0d25-7163-a29d-b7c8840028f9` returned replacement commit
  `fb748082f4578e1a59b84c3241bf17e490563f41` on exact parent
  `fa832d51d3b9aa463490cf3afbc2037f71843f41`.
- Exact scope is one `LEARNING_GUIDE.md`. Adapter defaults are now correctly
  labelled 4 taps/128 samples/30 seconds/1e-3. Queue text now uses preallocated
  maximum slots, variable top-left rectangles, row/sample extents, native
  `QSharedPointer<const FiffInfo>`, drop-newest/no-advance and untouched tails.
- Worker reports eight repository links resolve, thirteen fences balance,
  diff-check passes and worktree is clean. No build/server/vendor/subagent work.
- This response is intentionally held: it predates the accepted atomic native
  wake and lacks the measured benchmark command/workload/results. Manager will
  inspect the one-file replacement, then send `W-DOC-001-REVISE-2` to the same
  reusable Luna/max conversation without overlapping another editor.

### REVIEW W-DOC-001-REVISE-1

- Exact commit/parent/one-new-file scope/diff-check pass. The guide is a deep
  learning artifact with correct Eigen shapes, tap-major order, noalias/LDLT,
  causal history, update boundaries and effect-versus-engineering distinction.
- `R-DOC-QUEUE-V2-001` is only partially fixed: variable rectangles, extents,
  native ownership and tails are correct, but section 6 still prescribes
  `QSemaphore::tryAcquire(1,0)`, which is superseded and formally rejected on
  Windows. `R-DOC-BENCHMARK-EVIDENCE-002` also remains open because no benchmark
  command, workload or measured p50/p95/max exists.
- Decision: HOLD replacement `fb748082f`; send one same-file Luna/max revision.

### REQUEST W-DOC-001-REVISE-2

- From/to: manager / retained reusable Luna/max documentation thread
  `019fd2fc-0d25-7163-a29d-b7c8840028f9`.
- Exact base: `fb748082f4578e1a59b84c3241bf17e490563f41`.
- Authorized file only:
  `doc/causal_reference_denoising/LEARNING_GUIDE.md`; no source, tests, CMake,
  vendor, dependency or other document changes.
- Replace the remaining QSemaphore text with the implemented queue: always-
  lock-free unsigned SPSC producer/consumer sequences, acquire/release slot
  publication/reuse, preallocated maximum rectangles, one nonblocking native
  signal attempt, Windows configure-created auto-reset event and POSIX configure-
  created nonblocking CLOEXEC pipe, bounded consumer wait/rechecks, idempotent
  stop wake and caller quiescence before reconfigure/destruction. State that
  producer `tryPush` has no Qt semaphore/mutex, wait, retry or post-configure C++
  allocation and that the focused producer allocation assertion is zero.
- Add exact engineering benchmark reproduction:
  build target `ex_causal_reference_denoising`, run with `--benchmark`; 1000 Hz,
  270 rows, 16 references, 250 targets, 4 preserved rows, 128 samples/block,
  4 taps/P=64, 100 warmup plus 1000 timed ApplyAndLearn blocks. Nearest-rank
  Release p50/p95/max are `2.725/4.317/8.557 ms`; p95 passes `<128 ms`.
  Label it engineering timing evidence, not denoising-quality/clinical proof.
- Preserve all correct content and links; recheck link targets, fences and
  diff-check. Commit exact one-file delta, clean status, proactively return
  `RESPONSE W-DOC-001-REVISE-2`; no manager polling/subagent/server/full scan.

### DISPATCHED W-DOC-001-REVISE-2

- Retained thread `019fd2fc-0d25-7163-a29d-b7c8840028f9` accepted the exact
  guide-only request with explicit `gpt-5.6-luna` / `max` overrides. Await its
  proactive response; do not poll or overlap the learning-guide file.

### RESPONSE W-DOC-001-REVISE-2

- Retained Luna/max thread proactively returned replacement commit
  `6333b995ded9dd6445c0a440f157211b95db4271` with original exact parent
  `fa832d51d3b9aa463490cf3afbc2037f71843f41`. It amends the complete guide
  rather than adding a delta parent; exact scope remains one new
  `LEARNING_GUIDE.md`.
- Queue section now reports always-lock-free SPSC sequences, acquire/release
  publication/reuse, Windows event/POSIX pipe wake, bounded rechecks,
  idempotent stop/quiescence, producer constraints and sustained/zero-allocation
  public coverage. Worker reports no `QSemaphore` remains.
- Benchmark section adds focused Release build/run commands, 1000 Hz/270 rows/
  128 samples/16 references/250 targets/4 preserved/4 taps/P64, 100 warmup plus
  1000 timed blocks and recorded nearest-rank p50/p95/max
  `2.725/4.317/8.557 ms`; strict p95 `<128 ms` is labelled engineering evidence.
- Worker reports focused Release build and benchmark PASS, eight links resolve,
  fourteen fences balance, diff-check and clean one-file worktree pass. No full
  mne_scan, mne_rt_server, vendor/dependency/source/test/subagent work occurred.
- Response is durable before manager exact replacement/content review.

### REVIEW W-DOC-001-REVISE-2

- Exact object/parent/one-new-file scope and diff-check pass. Direct comparison
  with held `fb748082f` is confined to the requested queue rewrite and benchmark
  subsection; all accepted numerical/Eigen/license/non-goal content remains.
- Manager verifies all eight repository-relative links exist, 28 fence markers
  balance, and case-insensitive QSemaphore occurrence count is zero.
- Atomic sequence/native event/pipe, bounded consumer, quiescence, producer
  constraints and public zero-allocation/concurrency/lifetime evidence match the
  integrated implementation without overclaiming OS or end-to-end guarantees.
- Release commands name the real focused target/executable. Workload and recorded
  nearest-rank `2.725/4.317/8.557 ms` evidence match durable benchmark results and
  are explicitly scoped to development-machine engineering timing.
- Decision: ACCEPT with no manager P0-P3. Integrate replacement and replay the
  documented focused Release build/default/benchmark commands before closing
  the two documentation P2 findings.

### VALIDATION W-DOC-001-REVISE-2

- Full replacement is integrated/pushed as `b59221e78`; exact one-guide diff-
  check passes. Canonical Release build of `ex_causal_reference_denoising`
  succeeds.
- Default run exits zero with `example invariants: PASS`, reaches generation 4,
  freeze reports zero updates and reset restores generation zero/warmup.
- `--benchmark` exits zero with dimensions 270/16 references/250 targets/4
  preserved/128 samples/4 taps/P64, 100 warmup and 1000 timed blocks. Current
  nearest-rank p50/p95/max are `2.174/2.748/4.130 ms`; p95 `<128 ms` PASS and
  final generation/accepted/rejected are `1099/1099/0`.
- This replay is faster than the durable recorded `2.725/4.317/8.557 ms`; the
  guide explicitly states load variation, and both executions pass the same
  engineering gate. `R-DOC-QUEUE-V2-001` and
  `R-DOC-BENCHMARK-EVIDENCE-002` close. Thread is retirement-ready after issue
  #7 publication/closure.

### ARCHIVED W-DOC-001

- Issue #7 final evidence is published/read back at
  `https://github.com/Edddddddddy/mne-cpp/issues/7#issuecomment-5203274698` and
  the issue is verified CLOSED with reason COMPLETED.
- App archive succeeds for retained Luna/max documentation thread
  `019fd2fc-0d25-7163-a29d-b7c8840028f9` after all replacements, manager review,
  integration and replay evidence became durable. Its app worktree was not
  manually modified or deleted.

### FAILURE EPIC-2-DOC-CHECK-1

- First `gh issue edit` attempt passed the multi-line body as a normal PowerShell
  argument. Argument splitting made `gh` reject locally with an unknown flag
  before any GitHub request/mutation.
- Repository and issue #2 remain unchanged. Retry once by piping the exact
  replaced body to `gh issue edit --body-file -`, then read back the checkbox.

### FAILURE EPIC-2-DOC-CHECK-2

- In this PowerShell/native-command host, the standard-input pipeline is still
  surfaced to `gh issue edit` as multiple positional arguments; `gh` rejects
  locally with `received 11` before any mutation.
- Issue #2 remains unchanged. The next bounded retry uses an explicit PowerShell
  argument array/splat so the complete body is one `--body` argument, followed
  by authenticated read-back.

### FAILURE EPIC-2-DOC-CHECK-3

- Explicit PowerShell argument-array invocation still passes the multi-line body
  through the legacy native parser as split arguments; `gh` rejects locally
  before any request. Issue #2 remains unchanged after three CLI forms.
- Stop CLI retries. Use the installed GitHub connector's structured
  `update_issue` call with one replacement body, then authenticate the result by
  independent `gh issue view` read-back.

### RESOLVED EPIC-2-DOC-CHECK

- Before any connector write, an independent authenticated `gh issue view`
  shows the epic body already contains checked `#7`. At least one prior command
  therefore mutated the remote body despite returning a local argument error or
  ambiguous nonzero status.
- Treat remote read-back as authoritative. Cancel the prepared connector write;
  do not risk a duplicate/stale body replacement. Epic remains open with #4/#7
  checked and #5/#6/#3 unchecked.

### DISCUSSION SPEC-QUEUE-ATOMIC-CONSISTENCY-001

- The later atomic-correction section was authoritative, but the earlier
  adapter-seam paragraph still named QSemaphore. It is corrected in place to
  the same release/acquire sequence plus one native wake contract.
- Metadata failure is clarified: valid FiffInfo with invalid layout/settings is
  exact pass-through; structurally unusable/null metadata disarms and is not
  emitted under stale labels because no safe RTMSA output metadata exists.
- GitHub issue #6 body is updated and read back to match this distinction, the
  accepted SPSC admission fix, held integration order and fresh-widget/pending-
  snapshot UI seam. Title/number/open state are unchanged.

### REVIEW W-QUEUE-POSIX-EINTR-TEST-001

- Exact object `c4b13a220` has parent `d5fd0e4ee`, changes only the focused
  test CMake/source, and passes `git diff --check`.
- The Linux GNU/Clang guard and `-Wl,--wrap=write` condition match. The wrapper
  is caller-thread-local, injects exactly one pre-transfer `EINTR`, delegates
  all other writes, and each public test requires the wrapper count to be one.
- Producer test retains exact variable rectangle/extents/native metadata/tail;
  stop test retains complete destination/extents/metadata. Both use finite
  completion and fallback cleanup. Windows/Apple compilation is unaffected.
- P2: both consumer markers precede the public `waitPop` and elapsed assertions
  accept zero. A scheduling pause between marker and call can let push/stop
  occur first, so the later immediate Popped/Stopped result may false-green the
  lost-wake implementation. Do not integrate until a positive lower bound is
  compiled into both prompt tests.

### REQUEST W-QUEUE-POSIX-EINTR-TEST-001-REVISE-1

- from / manager thread `019fcdc3-4a1e-76d1-8140-1bd521219297`
- to / retained Luna/max thread `019fd779-c771-7f40-8ce7-b23cdc7fa175`
- blocking / yes, before EINTR tracer integration and atomic production fix
- exact parent / `c4b13a220f163511e5d84ed6378fc7545edd4965`
- scope / `test_adaptive_denoising_plugin.cpp` only; CMake and production are
  immutable
- request / add a named 5 ms prompt-wait lower bound and require both complete
  public wait durations to be `>=5 ms` while retaining `<500 ms`, all wrapper,
  payload/preservation, finite cleanup and conditional-compilation oracles.
- response / new delta commit, exact parent/scope/diff-check, focused evidence
  if available, truthful WSL limitation, proactive structured response only.
- dispatch / retained visible thread accepted the follow-up with explicit
  `gpt-5.6-luna` / `max`; await proactive response without polling.

### RESPONSE W-QUEUE-POSIX-EINTR-TEST-001-REVISE-1

- exact parent / `c4b13a220f163511e5d84ed6378fc7545edd4965`
- commit / `89acb78bd2b1b48f8d359ad82bf5a866d66d97ff`
- correction / both EINTR prompt tests add named 5 ms lower bounds and replace
  elapsed `>=0` with `>=5 ms`, retaining the existing `<500 ms` upper bound.
- scope / focused test source only; CMake, production, guards, wrapper count,
  payload/extents/native metadata/tail, stopped-destination and finite cleanup
  are reported unchanged.
- evidence / exact parent and one-file commit, diff-check and clean detached
  status reported. WSL startup remains unavailable; no POSIX runtime claim,
  reset, shutdown, dependency repair, manager polling or server run.
- manager status / response durable; direct Git object/diff and populated
  Windows conditional non-regression remain before integration acceptance.

### FAILURE VALIDATION-QUEUE-EINTR-WINDOWS-LOOKUP-001

- The canonical MSVC Release target recompiles and links successfully, but the
  first validation script searches only below `build-causal-reference-denoising`
  and then throws `focused executable not found`.
- CMake's authoritative output line places the executable at
  `out/Release/apps/test_adaptive_denoising_plugin.exe`; this is a manager
  harness path error after a successful build, not a source/build/test failure.
- Corrective action: run the exact reported executable path, retain the full
  reporter output and three immediate repeat exit codes. No source/dependency
  edit and no server run.

### FAILURE VALIDATION-QUEUE-EINTR-WINDOWS-REPORTER-002

- The corrected-path script launches the focused executable and repeats, but
  the final `Get-Content` fails because the Qt text reporter file is still held
  by a process. The script exits before publishing a reliable combined result.
- No test PASS/FAIL total is inferred. Inspect only the focused process state
  and reporter timestamp/content after it is released; then use explicit
  `Start-Process -Wait -PassThru` executions or direct console output so each
  exit is durably observed before the next run.
- This is a validation-harness/process-lifetime issue; tracked source and
  dependencies remain untouched.

### VALIDATION W-QUEUE-POSIX-EINTR-TEST-001-REVISE-1

- Direct Git proof accepts exact parent `c4b13a220`, one focused-test-file
  delta and the two named `>=5 ms`, `<500 ms` assertions. Manager integrates
  the tracer series as `0d1d1f949` plus `b6cf3a13b`.
- Populated MSVC Release target recompiles and links successfully. After the
  two recorded harness mistakes, the released reporter file is read directly:
  QtTest reports 19 passed, 0 failed/skipped/blacklisted; SPSC pushed/full/
  popped are 455/57/455 with zero counted producer allocations, and stopped
  wait is 30 ms.
- Three subsequent executions use explicit `Start-Process -Wait -PassThru` and
  exit `0,0,0`. Linux-only wrapper slots are correctly excluded on Windows;
  no POSIX RED/GREEN is claimed while WSL startup remains unavailable.
- Decision / accept and integrate test tracer; close only the blocked-wait test
  sensitivity P2. Production POSIX EINTR finding remains for Sol/ultra.

### ARCHIVED W-QUEUE-POSIX-EINTR-TEST-001

- Issue #5 integration evidence is published/read back at
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5206564743`.
- App archive succeeds for visible Luna/max thread
  `019fd779-c771-7f40-8ce7-b23cdc7fa175` after exact integration, 19/0/0 and
  three repeat zero exits. Its worktree is not manually changed or deleted.

### REQUEST W-QUEUE-POSIX-EINTR-GREEN-001

- from / manager thread `019fcdc3-4a1e-76d1-8140-1bd521219297`
- to / retained atomic Sol/ultra thread
  `019fd64b-2553-7102-91d6-1f8d35720823`
- blocking / yes, final production correction before fresh queue review
- exact base / `f1151d1346cddfb42359ac015545d874fb42d36c`
- scope / `adaptivedenoisingblockqueue.cpp` only; public header, tests, CMake,
  plugin, processor, docs and dependencies immutable
- request / keep the existing outer deadline loop; on POSIX cap each native
  consumer `poll` duration to 25 ms so a lost EINTR signal is observed by the
  next atomic sequence/running recheck. Windows event wait remains full caller
  remainder. Producer/stop keep one nonblocking write with no retry/wait/lock/
  allocation; public seam and SPSC ordering remain unchanged.
- verification / exact-parent one-file commit and diff-check; Windows MSVC
  Release focused suite; POSIX wrapped suite only if WSL starts without global
  reset/shutdown; source proof that producer/stop paths are unchanged.
- response / proactive `RESPONSE W-QUEUE-POSIX-EINTR-GREEN-001`; no manager
  polling and no internal/nested subagent or mne_rt_server.
- dispatch / retained visible thread accepts the exact-base follow-up with
  explicit `gpt-5.6-sol` / `ultra`; await proactive response without polling.
- public dispatch / issue #5 comment read back at
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5206658788`.
- wait status / one 120-second bounded app wait returned no completion or
  actionable progress payload; no result accepted and no further polling now.

### RESPONSE W-QUEUE-POSIX-EINTR-GREEN-001

- exact parent / `f1151d1346cddfb42359ac015545d874fb42d36c`
- commit / `236db13903cc4d64268aa292f162e88eefbbb54e`
- scope / queue implementation source only; 12 insertions/2 deletions, clean
  detached worktree and diff-check reported.
- correction / POSIX `NativeWake::wait` passes `poll` at most 25 ms, including
  zero for nonpositive timeouts. Existing outer deadline/recheck loop is
  unchanged, so lost producer/stop bytes are observed through the next atomic
  sequence/running check. Comment distinguishes EAGAIN coalescing from EINTR
  byte loss.
- invariants / worker reports byte-identical Windows wake, `tryPush` and `stop`;
  POSIX signal executable code still performs one nonblocking write with no
  retry, lock, wait, allocation or public seam change.
- Windows evidence / manual populated MSVC Release 19/0/0, overlap
  pushed/full/popped 371/141/371, max producer 33.4 us, zero counted producer
  allocations, stop wake 62 ms.
- POSIX evidence / current-source GNU/Qt ordinary wrapped suite compiles and
  reports 19/0/0. First MOC omitted GNU predefines, so forced slots were not
  registered; corrected MOC contains both exact slot names, but WSL stopped
  completing during rebuild. Forced-EINTR execution is explicitly unclaimed;
  no WSL reset/shutdown, dependency repair or fabricated result.
- manager status / response durable; exact object/diff/source proof, integration
  canonical Windows run and independent Sol/ultra review remain.

### VALIDATION W-QUEUE-POSIX-EINTR-GREEN-001 - Windows

- Direct Git proof accepts exact parent `f1151d134`, one queue-source file and
  diff-check. Diff is the POSIX signal comment plus a C++14 25 ms `poll` cap;
  manager extraction proves `tryPush` and `stop` byte-identical and finds no
  forbidden Qt/condition-variable synchronization dependency.
- Integrated as `f7c22717e`. Canonical populated MSVC Release recompiles and
  links the focused target with only known Eigen C4819 warnings.
- Full waited reporter execution returns zero and QtTest reports 19/0/0. The
  current SPSC sample is pushed/full/popped 453/59/453, maximum producer call
  1.5 us and zero counted allocations; stopped wait is 30 ms. Three explicit
  waited repeats exit `0,0,0`.
- Windows/non-POSIX behavior is GREEN. One bounded canonical WSL wrapped attempt
  remains before formal review; inability to start/complete must be recorded as
  environment evidence rather than a failure of this source.

### FAILURE VALIDATION-QUEUE-POSIX-WRAP-001

- WSL now starts, but the first manager multi-line Bash invocation is split by
  the Windows native argument boundary. A `pkg-config` include flag such as
  `-I/usr/include/x86_64-linux-gnu/qt5` is executed as a command and exits 127.
- Failure occurs before MOC, compilation or test execution. It is a manager
  harness quoting error, not queue source or WSL toolchain evidence.
- Corrective action: transmit the exact script as Base64 to one bounded Bash
  process, retain `/tmp` cleanup and the explicit MOC slot-name checks, then
  run the wrapped executable once. No repository/vendor/WSL-state mutation.

### VALIDATION W-QUEUE-POSIX-EINTR-GREEN-001 - POSIX forced EINTR

- Base64 transport fixes only the host quoting harness; the same bounded `/tmp`
  compile/run uses repository source read-only and cleans all temporary output.
- MOC is explicitly given Linux/GNU predefines; manager verifies both exact
  forced slot names in generated output before compilation. GCC 13.2/Qt 5.15.13
  compiles with C++14/O2/pthreads and links GNU `--wrap=write`.
- Complete WSL Ubuntu run exits zero with 21 passed, zero failed/skipped/
  blacklisted. Both `queuePopsPromptlyAfterInterruptedProducerSignal` and
  `queueStopsPromptlyAfterInterruptedStopSignal` PASS.
- SPSC sample is pushed/full/popped 373/139/373, max producer call 16.466 us,
  zero counted producer allocations; stop wake 50 ms and forced stop wait are
  within their prompt bounds.
- Combined manager evidence is Windows 19/0/0 plus three repeat exits and POSIX
  21/0/0 including both forced EINTR cases. Dispatch exact-snapshot formal review.

### REQUEST R-QUEUE-ATOMIC-003

- from / manager thread `019fcdc3-4a1e-76d1-8140-1bd521219297`
- to / retained independent review thread
  `019fd5d8-380d-7563-8644-264426bda199`
- model / `gpt-5.6-sol` / `ultra`
- exact snapshot / `5aa43160b64bf0a96d2918331ca8c28d4dc078ab`
- mode / read-only formal review; no edits, commits, pushes or GitHub mutation
- scope / complete atomic queue header/source, focused tests/CMake, live SPEC
  queue contract, plugin caller compatibility only as necessary, prior findings
  and durable Windows/POSIX evidence.
- required decisions / P0-P3 findings with exact files/lines/evidence/fix/tests;
  explicitly close or retain `R-QUEUE-V2-QSEMAPHORE-001`, concurrency/lifetime,
  POSIX EINTR, SPEC locality and test-sensitivity findings; verify deep module,
  producer realtime/no-allocation/no-lock, SPSC ordering/wrap, stop races,
  transactional resources, lost-wake progress and public preservation.
- gate / PASS only if P0/P1 are zero and every P2 is fixed or explicitly
  justified for deferral; distinguish manager/worker runtime claims from
  independently reproduced evidence.
- response / proactive `RESPONSE R-QUEUE-ATOMIC-003`; exact SHA/clean state,
  prioritized findings and PASS/HOLD. No manager polling or subagent/server.
- public evidence / issue #5 read-back at
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5207390978`.
- dispatch / retained visible review thread accepts the exact-snapshot follow-up
  with explicit `gpt-5.6-sol` / `ultra`; await proactive response without
  polling or inspecting its worktree.

### RESPONSE R-QUEUE-ATOMIC-003 - HOLD / request SHA mismatch

- requested SHA / invalid full object
  `5aa43160b64bf0a96d2918331ca8c28d4dc078ab`
- actual pushed commit / `5aa43160bbae27560c4f4fe013728a6476013db0`
- Reviewer fetches the pushed branch read-only, refuses to substitute a same-
  prefix object, and returns HOLD without reviewing code or issuing P0-P3.
- Final reviewer state remains clean/detached at prior `9535bf8c7`; only local
  Git metadata changed. No source/build/test/GitHub/server action occurred.
- Manager disposition / request identity error. No queue finding or regression
  is inferred; issue a corrected exact-SHA follow-up to the same Sol/ultra
  review conversation.

### REQUEST R-QUEUE-ATOMIC-003-RETRY-1

- exact snapshot / `5aa43160bbae27560c4f4fe013728a6476013db0`
- scope/gate / identical to `R-QUEUE-ATOMIC-003`; read-only complete queue,
  tests/CMake, live SPEC, caller compatibility and all prior finding disposition.
- response / proactive exact-SHA P0-P3 PASS/HOLD; no source or external mutation.
- dispatch / retained review thread accepts corrected full SHA with explicit
  Sol/ultra; its prior fetch already materialized the object. Await proactive
  response without polling.

### FAILURE MANAGER-GIT-SANDBOX-001

- After the corrected review dispatch, the execution identity changes to
  `CodexSandboxOffline`. Per-command `safe.directory` restores read access, but
  staging/commit fails because `.git/index.lock` is not writable; push also
  fails immediately because shell network access is disabled.
- The corrected request and failure remain written in durable project markdown
  but are not yet committed/pushed. No source, index or remote mutation occurs.
- Do not change global Git configuration or permissions. Continue read-only
  review coordination and retry ordinary commit/push only after the environment
  restores repository metadata/network access.

### PROGRESS R-QUEUE-ATOMIC-003-RETRY-1

- One bounded wait confirms the reviewer is exact/detached/clean at the corrected
  snapshot and is auditing durable contracts/findings before implementation and
  tests. No final response or gate decision yet; no further polling now.

### PREFLIGHT W-PLUGIN-DATA-001 - current queue snapshot

- Read-only merge-tree from current committed HEAD `8d04813fb` to held plugin
  `5f4718722` finds no conflict marker; merge base remains `8c51ea4ce`.
- Held scope is plugin registration plus six new plugin files, followed by the
  one-source SPSC admission fix `34a8096f1`; both diffs pass whitespace checks.
- Integration remains blocked only by queue formal PASS. No code is consumed
  while the review is active.

### PROGRESS R-QUEUE-ATOMIC-003-RETRY-1 - sensitivity audit

- Reviewer remains active and reports checking exact production/test support
  for 25 ms POSIX fallback and whole-call elapsed-time closure claims. This is
  not a gate decision; plugin lifecycle remains held.
- Management timebox / finalize the requested P0-P3 PASS/HOLD now from existing
  evidence; do not add builds or broaden review. Residual nonblocking uncertainty
  may be reported as P3. One message only, without interrupting the active turn.

### RESPONSE R-QUEUE-ATOMIC-003-RETRY-1

- reviewed SHA / `5aa43160bbae27560c4f4fe013728a6476013db0`
- state / exact clean detached before and after; read-only, no build/mutation
- gate / PASS; P0=0, P1=0, P2=0, P3=0; no new finding
- deep module / four-operation plugin-private interface retains platform wake,
  ring allocation/ownership and policy behind PImpl with high depth/leverage/
  locality and no public strategy/registry seam.
- closures / QSemaphore P1, concurrency sensitivity, metadata lifetime, POSIX
  EINTR, SPEC locality, stopped-wait elapsed and EINTR blocked-wait findings all
  close with exact source/test/contract evidence.
- additional audit / sequence/full/wrap, release/acquire, stop commit points,
  transactional native resources, pending discard/fresh configure, caller
  quiescence, exact rectangle/tail/metadata and noexcept all accepted.
- evidence boundary / reviewer independently audits exact source/test/SPEC and
  clean provenance; manager Windows 19/0/0 plus repeats and WSL 21/0/0 are not
  relabeled as independent execution.
- recommendation / PASS queue gate with no P2 deferral required. Held plugin
  SPSC admission/lifecycle is compatibility evidence only and remains the next
  integration/review milestone.

### INTEGRATION W-PLUGIN-DATA-001

- Worker plugin commit `5f4718722` cherry-picks as `85b40cb9e`; exact-parent
  admission correction `34a8096f1` cherry-picks as `3099a4292`.
- Integration is conflict-free and diff-check passes. Scope is plugin registry,
  six new plugin files and the one-source admission predicate correction.
- Existing queue, processor, numerical core, focused tests, AbstractAlgorithm,
  noisereduction, AdaptiveTSSS and dependencies are untouched by this series.
- Validation/review status / code is integrated but not yet accepted. Run
  target-local MOC/CMake/static checks and independent Sol/ultra lifecycle/
  realtime review before dispatching Luna/max UI work.

### FAILURE VALIDATION-PLUGIN-MOC-HARNESS-001

- The first manager MOC/static-check command is rejected by execution policy
  before launch because its temporary-directory cleanup contains a recursive
  removal. No MOC, source scan or filesystem mutation occurs.
- This is a harness-policy failure, not plugin evidence. Retry with one temporary
  output file and nonrecursive cleanup; do not weaken the validation scope.
- Revision / the nonrecursive single-file cleanup form is also rejected before
  execution. Policy therefore disallows the explicit removal operation itself.
  Retry by writing MOC output under the existing ignored focused build tree and
  leave it as a normal build artifact; no cleanup command.
- Revision 2 / the no-cleanup command reaches PowerShell parsing but an inline
  nested count expression has an unmatched subexpression delimiter. Parser
  exits before MOC. Retry with each count assigned to a simple variable.
- Revision 3 / simplified command executes MOC, which reports `Undefined
  interface` because the standalone invocation lacks the project include root
  that defines `SCSHAREDLIB::AbstractAlgorithm` for `Q_INTERFACES`.
- This is an incomplete standalone MOC environment, not a header verdict. Locate
  the repository interface include root and rerun with the plugin JSON plus all
  required include paths.

### FAILURE PUSH-PLUGIN-MOC-RECORD-001

- Documentation commit `3b5afea3b` succeeds locally, but its ordinary push
  fails during GitHub TLS handshake with `SSL_ERROR_SYSCALL`.
- Remote state is unchanged; branch is clean/ahead. Record locally and retry one
  non-force push, with no history rewrite.

### VALIDATION W-PLUGIN-DATA-001 - MOC and ingress source contract

- Standalone Qt 5.15.2 MOC with repository roots
  `src/applications/mne_scan/libs` and `src/libraries` accepts the integrated
  plugin header plus empty metadata JSON and emits 4,269 bytes.
- Exact `AdaptiveDenoising::update` extraction contains one `info()` line and
  one `tryPush()` line, with zero mutex/wait/sleep lines and zero processor/
  configure/reset/output-init/setValue lines. The loop still performs one push
  for each input matrix as the frozen contract requires.
- This validates plugin identity and acquisition-path locality. Next attempt
  target-local CMake build with every other application/server disabled.

### FAILURE VALIDATION-PLUGIN-TARGET-TIMEOUT-001

- Dedicated configure/build explicitly enables only mne_scan and disables
  mne_rt_server/all other applications/examples/tests, then requests only
  `scan_adaptivedenoising`.
- The combined command emits no final captured output before the 124-second tool
  limit and is terminated. No build PASS/FAIL or dependency diagnosis is
  inferred from the timeout.
- Inspect cache/target artifacts and any residual compiler process read-only;
  then resume only the generated target with a bounded build, or record the
  exact dependency failure if already available. Never start the server.

### VALIDATION W-PLUGIN-DATA-001 - target environment result

- Read-only state after timeout proves configure success, Visual Studio 18
  generator, `BUILD_MNE_SCAN=ON`, `BUILD_MNE_RT_SERVER=OFF`, generated
  `scan_adaptivedenoising.vcxproj`, and zero residual build processes.
- Resumed target build fails in pre-existing `mne_fiff` dependency before any
  plugin translation unit: Qt 5.15.2 `qlist.h:915` cannot resolve
  `stdext::make_checked_array_iterator` under MSVC 14.51. The same error repeats
  across FIFF sources/MOC and matches the durable core-link environment block.
- Plugin target build/link is environment-deferred; vendor Qt is unchanged.
  Plugin-local MOC and acquisition-path contract remain GREEN. Dispatch exact-
  snapshot Sol/ultra lifecycle review with this evidence boundary explicit.

### REQUEST R-PLUGIN-DATA-001

- from / manager thread `019fcdc3-4a1e-76d1-8140-1bd521219297`
- model / new visible `gpt-5.6-sol` / `ultra` review conversation
- exact snapshot / `fd33dc9cd1a2ce90a30fdedd1ffde71c8514ec56`
- mode / read-only; no edits, commits, pushes, GitHub/dependency mutation
- scope / plugin registration/CMake/header/source/global metadata, queue and
  processor caller contracts, existing focused tests and durable SPEC/review
  records only as needed. UI is intentionally absent and not a finding here.
- review / deep concrete adapter seam; SPSC zero-to-one admission/epoch and
  producer quiescence; start/stop/destructor/restart; DirectConnection callback
  locality/drop counting; per-block FIFF validation/mapping/reconfigure/disarm;
  exception fail-closed; FIFO output/metadata ownership/sole constCast; worker
  interruption and output clearing; CMake dependency/identity locality.
- evidence boundary / MOC and update contract pass; real target is blocked in
  pre-existing `mne_fiff` Qt5/MSVC stdext failure before plugin compilation;
  focused processor/queue cross-platform evidence is durable but not independent.
- gate / P0/P1 zero; P2 fixed or explicitly deferred. Report exact file/line,
  evidence, impact, fix and test. Return proactive `RESPONSE R-PLUGIN-DATA-001`
  with exact SHA/clean state and PASS/HOLD; no subagent/full app/server/manager poll.

### SETUP R-PLUGIN-DATA-001

- accepted / `2026-08-07T02:21:39+08:00`
- setup / `client-new-thread:3696dfeb-1800-4b9a-a8cf-a0d70d7eb2f0`
- host / `local`
- model / `gpt-5.6-sol`
- reasoning / `ultra`
- project / `local-b0db0afa904757a4167f8694f3f76d67`
- environment / new visible worktree from exact existing ref
  `fd33dc9cd1a2ce90a30fdedd1ffde71c8514ec56`
- state / asynchronous worktree setup accepted. The reviewer must proactively
  return its response; manager will not continuously poll or inspect its files.
- public dispatch / issue #6 read-back confirms the exact snapshot, scope,
  evidence boundary and gate at
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5208166450`.
- resolution / a later single thread-list lookup resolves the active visible
  reviewer as `019fd84f-03c6-79f1-977f-b278d55777ba`, clean worktree path
  `C:/Users/lcy/.codex/worktrees/3f96/mne-cpp-main`. Do not poll its progress.

### FOLLOWUP R-PLUGIN-DATA-001-FINALIZE

- from / manager
- to / retained reviewer `019fd84f-03c6-79f1-977f-b278d55777ba`
- blocking / yes for lifecycle gate
- request / finish the current read-only MSVC `/Zs` syntax check, then finalize
  the required exact-SHA P0-P3 PASS/HOLD from evidence already collected. Do
  not add validation branches or expand into the intentionally later UI scope.
- evidence rule / retain the target-build Qt/MSVC dependency boundary and label
  residual uncertainty as a finding rather than delaying the formal response.
- response / proactive `RESPONSE R-PLUGIN-DATA-001`; no manager polling.
- dispatch / app accepts the follow-up on the same visible reviewer thread;
  model/reasoning remain Sol/ultra and the active turn is not interrupted.

### RESPONSE R-PLUGIN-DATA-001

- reviewed / exact clean/detached
  `fd33dc9cd1a2ce90a30fdedd1ffde71c8514ec56`; read-only before/after
- gate / HOLD; P0=0, P1=0, P2=4, P3=1
- prior P1 / `R-PLUGIN-SPSC-PRODUCER-001` closed: zero-to-one CAS admission,
  epoch confirmation and producer quiescence expose no two-producer path.
- P2 `R-PLUGIN-DESTRUCTOR-BOUND-001` / bounded public stop falls back in the
  destructor to unbounded worker wait and 1 ms producer polling. Fix a safe
  ownership protocol or explicitly defer/document the terminal safety policy;
  test timeout, restart refusal, later retry and destruction.
- P2 `R-PLUGIN-ADMISSION-DROP-001` / a Busy concurrent notification returns
  before matrix inspection and drop accounting, underreporting data loss.
  Distinguish Busy from Closed, count every Busy matrix without queue entry,
  wait, lock or retry, and test concurrent multi-matrix accounting/FIFO.
- P2 `R-PLUGIN-LIFECYCLE-TEST-001` / the focused target does not compile the
  real plugin and exercises none of admission/epoch/stop/restart/output/FIFF
  lifecycle. Add a supported public lifecycle harness or explicitly defer with
  the Qt/MSVC target boundary and exact residual risk.
- P2 `R-PLUGIN-STATIC-REGISTRATION-001` / static builds define QT_STATICPLUGIN
  but neither link `scan_adaptivedenoising` nor import `AdaptiveDenoising` in
  mne_scan. Add link/import wiring and a static discovery smoke or defer scope.
- P3 `R-PLUGIN-ATOMIC-LOCKFREE-001` / callback uint32/uint64 atomics have no
  C++14 always-lock-free compile guard. Add architecture-sensitive macro/type
  assertions in the real plugin target.
- accepted source behavior / deep concrete adapter; worker-only FIFF mapping,
  fail-closed disarm, FIFO output, invalid-metadata suppression, sole legacy
  constCast and admitted-callback locality are coherent.
- independent evidence / Qt MOC succeeds; MSVC 14.29 C++14 `/Zs` accepts exact
  plugin source. MSVC 14.51 reproduces the known pre-plugin Qt qlist failure.
  Manager build/MOC/queue/processor evidence remains separately attributed.

### DECISION R-PLUGIN-DATA-001-ACTION

- fix now / admission Busy-vs-Closed accounting, callback atomic lock-free
  compile constraints, and static link/import wiring.
- defer explicitly / terminal destructor boundedness plus the full real-plugin
  lifecycle harness. Memory-safe teardown currently requires waiting for live
  thread/callback ownership; returning on a deadline would create use-after-free.
  The supported-runtime harness is also blocked by the existing Qt/MSVC FIFF
  dependency. Create a dedicated follow-up issue linked from #3/#6.
- review / after fixes and durable deferral, send a fresh exact snapshot to a
  Sol/ultra reviewer. UI stays blocked until PASS.
- follow-up / GitHub issue #8 now owns the destructor/harness deferral:
  `https://github.com/Edddddddddy/mne-cpp/issues/8`. Final-QA comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/3#issuecomment-5208313558`;
  plugin comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5208313573`.

### REQUEST W-PLUGIN-ADMISSION-FIX-001

- from / manager
- to / retained visible plugin lifecycle implementation conversation
  `019fd59d-fcc8-7573-964d-ed930c4100a8`
- model / `gpt-5.6-sol` / `ultra`
- blocking / yes for lifecycle gate; independent of the static-registration task
- exact base / use the pushed integration request snapshot supplied at dispatch;
  prove detached clean HEAD before editing
- scope / edit only `adaptivedenoising.h` and `adaptivedenoising.cpp`. No CMake,
  mne_scan main, queue, processor, core, tests, UI, vendor or dependency changes.
- admission / replace the boolean guard result with private fixed Entered/Busy/
  Closed semantics. Only Entered owns the producer count. A failed zero-to-one
  CAS on an otherwise open run is Busy; a closed/epoch-invalidated admission is
  Closed. No retry loop, lock, wait, string, FIFF selection or queue call is added.
- update / Closed returns silently. Busy may inspect only the dynamic RTMSA and
  its existing matrix list, performs no `info()` snapshot and no `tryPush`, and
  atomically counts every rejected matrix exactly. Entered retains one `info()`
  snapshot and exactly one `tryPush` per matrix; every non-Pushed result counts
  one dropped block. Preserve prompt callback return and SPSC safety.
- lock-free / add portable C++14 compile-time guards proving the actual uint32
  admission and uint64 drop-counter atomic widths map to an always-lock-free
  standard atomic macro on the supported build. Do not use C++17 traits.
- locality / add compact header contract beside the destructor: bounded public
  stop may fail; host must retry/quiesce before destruction, and the destructor
  deliberately waits as the memory-safe terminal fallback tracked by issue #8.
- verification / exact diff/source checks; standalone Qt MOC; MSVC 14.29 C++14
  `/Zs` if available; a deterministic source/compile proof that Closed is silent,
  Busy counts matrix cardinality without `info`/`tryPush`, Entered retains the
  frozen callback path and both atomics are guarded. No fake lifecycle GREEN.
- response / exact parent/commit/files, state transition table, callback audit,
  commands/results/limits and clean state; proactive response, no subagent,
  manager polling, full app or `mne_rt_server`.
- dispatch / retained thread `019fd59d-fcc8-7573-964d-ed930c4100a8` accepts the
  exact-base follow-up with explicit Sol/ultra. It must proactively respond and
  manager will not poll or inspect its worktree.

### RESPONSE W-PLUGIN-ADMISSION-FIX-001 - SETUP BLOCKED

- thread / `019fd59d-fcc8-7573-964d-ed930c4100a8`
- evidence / configured app worktree
  `C:/Users/lcy/.codex/worktrees/48c9/mne-cpp-main` and its parent no longer
  exist; initial command fails with Windows error 267 before Git/recovery reads.
- result / worker correctly refuses to adopt another directory because exact
  clean base `48672b02eb35de300a046079028e96ec6d1b25b6` cannot be proven.
- mutation / none: no file/Git/build/dependency/vendor/GitHub/app/server change,
  no subagent and no manager polling.
- disposition / this conversation is no longer reusable. Archive it after this
  evidence is pushed, then create a fresh visible Sol/ultra worktree task with
  the same request and exact base.

### SETUP W-PLUGIN-ADMISSION-FIX-001-RETRY-1

- prior lifecycle / app archives spent thread
  `019fd59d-fcc8-7573-964d-ed930c4100a8`; no manual worktree deletion.
- setup / accepted as
  `client-new-thread:58ed8491-eb34-43fc-8382-7d6a691c81ce` on local host.
- model / new visible `gpt-5.6-sol` / `ultra`
- environment / app-managed worktree from exact existing ref
  `48672b02eb35de300a046079028e96ec6d1b25b6`
- request / unchanged two-source admission/drop/atomic/destructor-contract scope;
  explicitly ignore the archived missing worktree and prove this fresh base.
- state / asynchronous setup accepted; await proactive response without polling.
- resolution / one app listing resolves active thread
  `019fd86c-b508-7491-876e-1972be35875a` at its fresh worktree; identity lookup
  only, no progress polling.
- public dispatch / issue #6 read-back confirms both disjoint immediate fixes,
  archived spent thread and UI hold at
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5208361509`.

### FOLLOWUP W-PLUGIN-ADMISSION-FIX-001-FINALIZE

- to / `019fd86c-b508-7491-876e-1972be35875a`
- request / stop validation expansion after the completed MOC/available-compiler
  checks; commit the exact two-file implementation, prove clean state and return
  the required transition/callback/evidence response.
- boundary / do not add tests, CMake, harnesses or UI and do not relabel syntax/
  source proof as lifecycle runtime evidence. One noninterrupting message only.
- dispatch / app accepts the message on the same Sol/ultra thread; worker stops
  expansion after current MOC/compiler/source checks and proactively responds.

### RESPONSE W-PLUGIN-ADMISSION-FIX-001-RETRY-1

- parent / exact `48672b02eb35de300a046079028e96ec6d1b25b6`
- commit / `2b571453da200af02ee437bbcac0a545021786cb`
- scope / exactly `adaptivedenoising.h/.cpp`, 61 insertions/17 deletions;
  detached clean worker state and diff-check pass.
- transitions / Closed bit or close/epoch-invalidated CAS is Closed with no count
  ownership and silent return; open nonzero count or same-epoch failed CAS is
  Busy with no ownership; confirmed zero-to-one CAS is Entered and guard-owned;
  invalidated post-CAS releases its provisional owned count before Closed.
- callback / Closed returns before RTMSA type inspection. Busy dynamically
  identifies RTMSA, obtains only its existing matrix list and atomically counts
  exact cardinality; no `info()`/`tryPush`. Entered retains one `info()` snapshot,
  one `tryPush` per matrix and one increment per non-Pushed result.
- realtime/locality / no admission retry, lock, wait, string, FIFF selection or
  queue call; translation-unit-private state machine adds no public strategy.
- atomic/destructor / portable C++14 type/macro assertions require always-lock-
  free actual uint32/uint64 atomics. Header documents bounded-stop failure,
  required host retry/quiescence and issue #8 memory-safe terminal wait.
- verification / Qt 5.15.2 MOC exit zero (4,310 bytes); MSVC 14.29 C++14 `/Zs`
  exit zero with only existing Eigen C4819 warnings; deterministic source oracle
  passes. These are not lifecycle runtime evidence.
- next / manager direct provenance/transition/source review, integration and
  fresh exact-snapshot Sol/ultra lifecycle review.

### INTEGRATION W-PLUGIN-ADMISSION-FIX-001-RETRY-1

- manager proof / exact parent, two authorized files, diff-check and transition
  audit pass. Close/epoch/count races retain single ownership; provisional
  successful CAS releases its count before returning Closed.
- callback proof / Busy adds exact list cardinality without `info()` or queue;
  Entered preserves one metadata snapshot/one push site; Closed returns before
  dynamic type inspection. No new lock/wait/retry/public strategy is present.
- integration / conflict-free cherry-pick creates `6ab7326ac`; tracked state is
  clean and preserved user paths remain untouched.
- next / populated MOC/C++14/source validation, issue publication and fresh
  exact-snapshot formal lifecycle review.

### VALIDATION W-PLUGIN-ADMISSION-FIX-001-RETRY-1

- Qt MOC / exit zero, generated 4,269 bytes with the correct repository include
  roots and plugin metadata.
- callback oracle / one `info()`, one `tryPush` site, zero mutex/wait/sleep;
  Closed precedes dynamicCast; Busy counts `matrices.size()` with zero info/push;
  two always-lock-free guard messages and one admission CAS.
- compiler / after correcting the recorded manager `vcvars64` quoting harness,
  Visual Studio MSVC 14.29.30133 C++14 `/Zs` accepts the integrated translation
  unit; only existing Eigen C4819 code-page warnings occur.
- evidence boundary / syntax/source/MOC only, not lifecycle runtime. Real target
  remains blocked under MSVC 14.51 before plugin compilation by existing Qt.
- publication / issue #6 read-back:
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5208508501`.
- lifecycle / completed Sol/ultra worker
  `019fd86c-b508-7491-876e-1972be35875a` is app-archived after integration,
  validation and publication; no manual worktree deletion.

### REQUEST R-PLUGIN-DATA-002

- from / manager
- to / retained independent Sol/ultra plugin lifecycle reviewer
  `019fd84f-03c6-79f1-977f-b278d55777ba`
- exact snapshot / assign the pushed request HEAD at dispatch and require clean
  detach before review
- mode / read-only; no edits/commit/push/GitHub/dependency mutation
- review / re-audit only prior findings on the integrated fixes: typed admission
  and exact Busy drop count; uint32/uint64 lock-free guards; static link/import
  and generated project reference; local destructor contract plus durable issue
  #8 deferrals for terminal teardown and real lifecycle harness.
- regression / ensure zero-to-one/epoch/quiescence and admitted callback locality
  remain correct; verify no UI was added and its absence remains out of scope.
- evidence boundary / manager MOC/callback oracle/MSVC14.29 `/Zs` and static
  configure are attributed to manager; no real target runtime is claimed because
  MSVC14.51 still fails in existing Qt/mne_fiff before plugin translation units.
- gate / P0/P1 zero and every P2 fixed or explicitly durable-deferred. Return
  exact-SHA clean proof, disposition of all five prior findings, any new P0-P3
  and PASS/HOLD. No full app/server/subagent/manager polling.
- dispatch / retained reviewer thread `019fd84f-03c6-79f1-977f-b278d55777ba`
  accepts exact snapshot `3dca8628eb43029ea35980e869da1a44d21b053a`
  with explicit Sol/ultra and unchanged read-only constraints.
- state / await proactive response without polling; do not start UI edits.

### REQUEST W-PLUGIN-STATIC-001

- from / manager
- to / new visible Luna/max worker
- blocking / yes for lifecycle gate; file-disjoint from admission correction
- exact base / use the pushed integration request snapshot supplied at dispatch;
  prove detached clean HEAD before editing
- scope / edit only `src/applications/mne_scan/mne_scan/CMakeLists.txt` and
  `src/applications/mne_scan/mne_scan/main.cpp`.
- change / inside existing `NOT BUILD_SHARED_LIBS` link list add
  `scan_adaptivedenoising`; inside existing `STATICBUILD` import list add
  `Q_IMPORT_PLUGIN(AdaptiveDenoising)`. Add no qrc initialization because the
  plugin has no qrc. Preserve shared-build behavior and all existing guards/order.
- verification / `git diff --check`; exact two-file source/CMake oracle showing
  the link/import are both static-only and absent from shared behavior; configure
  static mne_scan with `BUILD_MNE_RT_SERVER=OFF` if feasible and record the known
  Qt/MSVC dependency boundary without vendor repair. No full app/server run.
- response / exact parent/commit/two-file diff, guards, configure result and clean
  state; Luna/max, no subagent or manager polling.
- setup / accepted as
  `client-new-thread:8153b471-f290-4f15-8d7a-ac87cfeee593` on local host with
  explicit Luna/max and an app-managed worktree from exact base
  `48672b02eb35de300a046079028e96ec6d1b25b6`.
- state / asynchronous setup accepted; await proactive response without polling.
- resolution / one app listing resolves active thread
  `019fd869-e6d4-77c0-8edf-833cc23656c8`; identity lookup only.
- public dispatch / shared issue #6 comment:
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5208361509`.

### RESPONSE W-PLUGIN-STATIC-001

- parent / exact `48672b02eb35de300a046079028e96ec6d1b25b6`
- commit / `945b345a78d8788464fa13be0545ea2447448988`
- scope / exactly mne_scan `CMakeLists.txt` and `main.cpp`; two additions,
  zero removals; clean detached worker state and diff-check pass.
- implementation / add static-only `scan_adaptivedenoising` link and static-only
  `Q_IMPORT_PLUGIN(AdaptiveDenoising)`; shared link body unchanged and no qrc
  initializer added.
- verification / normalized parent oracle confirms all non-addition lines are
  identical. Static configure sets mne_scan ON and mne_rt_server OFF but stops
  before target generation because the isolated ignored Eigen baseline is
  incomplete; no vendor repair or runtime claim.
- next / manager provenance/diff review, then conflict-free integration and
  static source/configure evidence in the populated workspace.

### INTEGRATION W-PLUGIN-STATIC-001

- manager proof / worker object exists, exact parent matches, two authorized
  files only and committed diff-check passes; direct diff is exactly two lines.
- integration / conflict-free cherry-pick creates `78d443144`; current tracked
  state remains clean and preserved user paths remain untouched.
- result / static mne_scan link/import omission is source-corrected. Populated
  static configure/source evidence and fresh lifecycle review remain required
  before formally closing `R-PLUGIN-STATIC-REGISTRATION-001`.

### VALIDATION W-PLUGIN-STATIC-001

- harness note / the first read-only `rg` option scan used a PowerShell wildcard
  that Windows rejects after returning the needed top-level option lines. It
  changed nothing; direct application CMake inspection supplied the exact flags.
- configure / populated workspace Visual Studio 18/MSVC 19.51 static configure
  succeeds with `BUILD_SHARED_LIBS=OFF`, `BUILD_MNE_SCAN=ON`,
  `BUILD_MNE_RT_SERVER=OFF`, other applications/examples/tests disabled.
- generated evidence / both `mne_scan.vcxproj` and
  `scan_adaptivedenoising.vcxproj` exist; the former has a project reference and
  Release/Debug link dependency on `scan_adaptivedenoising`.
- runtime boundary / no executable is built or run. The known Qt/MSVC `mne_fiff`
  compile block remains the real target limitation; no vendor repair/server.
- publication / issue #6 read-back:
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5208434849`.
- lifecycle / completed Luna/max thread
  `019fd869-e6d4-77c0-8edf-833cc23656c8` is app-archived after integration,
  populated configure evidence and publication; no manual worktree deletion.

### PREPARED REQUEST W-PLUGIN-UI-TEST-001

- state / prepared only; do not dispatch until `R-PLUGIN-DATA-001` passes or
  its lifecycle findings are corrected. The exact base will be the resulting
  pushed integration HEAD.
- model / new visible `gpt-5.6-luna` / `max` test conversation
- skill / complete `tdd` skill; public behavior and UI-observable signals only
- scope / add only `src/testframes/CMakeLists.txt` plus a new focused
  `test_adaptive_denoising_ui` CMake/test source. Production plugin/widget,
  numerical, queue, processor, vendor and full application files are immutable.
- target seam / directly compile the future plugin-private setup-widget source
  and include its fixed diagnostics header, linking only Qt Core/Widgets/Test
  and Eigen. Do not link `scan_adaptivedenoising`, `mne_fiff`, `scShared`,
  `scMeas` or `mne_rtprocessing`, so the existing Qt/MSVC FIFF block is absent.
- RED behavior / the frozen widget/header/source do not yet exist. Register a
  focused public QtTest target whose initial failure is the missing production
  UI seam; do not manufacture a different failure or change dependencies.
- control oracle / query stable object names and verify enabled default true;
  taps 1..32/default 4; update interval 16..2048/default 128; memory 1..300/
  default 30; regularization 1e-8..1/default 1e-3; frozen default false; reset
  button present. Exact control names are `enabledCheckBox`,
  `tapCountSpinBox`, `adaptationIntervalSpinBox`, `memoryTimeSecondsSpinBox`,
  `regularizationSpinBox`, `frozenCheckBox` and `resetButton`. Verify each
  edit/reset emits the corresponding frozen typed widget signal.
- diagnostics oracle / deliver one fixed data-only diagnostics value through
  the widget public slot and verify GUI-thread labels expose plugin/configure/
  process state, R/M/P, warmup, generation, accepted/rejected updates, three
  RMS values and dropped blocks. No QString/container/owner is stored in the
  diagnostics type; add C++14 copy/metatype checks where portable.
- result / return exact parent/commit, three-file scope, RED command/output,
  object names and exact oracles. No subagent, manager polling, full app or
  `mne_rt_server`.

### PREPARED REQUEST W-PLUGIN-UI-GREEN-001

- state / dispatch only after the UI tracer is integrated and produces the
  intended RED. Exact base is that pushed tracer commit.
- model / new visible `gpt-5.6-luna` / `max` implementation conversation
- skills / complete `codebase-design` and `tdd`; implement only the frozen
  private view/pending-snapshot seam needed to make the tracer GREEN.
- scope / edit only `adaptivedenoising.h/.cpp`, its plugin CMake, and add a
  private diagnostics-types header plus setup-widget header/source. Queue,
  processor, numerical code, focused tests, other plugins and vendor files are
  immutable.
- diagnostics / fixed data-only value with plugin/configure/process statuses,
  R/M/P, warmup, generation, accepted/rejected events, three RMS values and
  dropped blocks; no QString/container/owner. Register it before worker emits.
- widget / programmatic QWidget/QFormLayout only. It emits typed enabled,
  frozen, four-setting and reset signals; consumes diagnostics in one GUI-thread
  slot; uses the tracer's stable object names/ranges/defaults. No `.ui`, qrc,
  controller base, settings registry or stored widget pointer.
- plugin controls / narrow Qt slots update one mutex-protected pending snapshot
  containing enabled/frozen/settings plus monotonic settings revision and reset
  sequence. `setupWidget()` returns a fresh widget, connects control signals to
  those slots and connects diagnostics explicitly with `Qt::QueuedConnection`.
- worker boundary / after each successful dequeue, copy the pending snapshot
  exactly once before configuration/process. A new settings revision forces a
  fresh processor configure before that whole block; a new reset sequence calls
  reset before that block; disabled/frozen/enabled select BypassTrackHistory,
  ApplyOnly and ApplyAndLearn respectively. Acquisition never touches this mutex
  or snapshot and retains exactly one queue push attempt per matrix.
- failure/output / metadata/layout/settings exceptions fail closed and emit a
  fixed diagnostic while valid blocks remain FIFO/pass-through as appropriate.
  Human-readable formatting occurs only in the widget slot. Restart preserves
  pending UI values but forces worker configuration from the fresh stream state.
- verification / make `test_adaptive_denoising_ui` GREEN; run standalone MOC
  for plugin/widget with correct include roots; rerun update-path extraction.
  Attempt no full app/server and do not patch the known Qt/MSVC FIFF dependency.
- response / exact parent/commit/files, signal/slot and snapshot ordering,
  MOC/test commands/results, limitations and clean state; no subagent/polling.

### PREPARED REQUEST R-PLUGIN-UI-001

- state / dispatch as a new visible read-only review only after UI GREEN is
  integrated and manager verification is durable.
- model / `gpt-5.6-sol` / `ultra`
- review / deep view seam; fixed diagnostics/metatype; explicit queued GUI
  conversion; fresh widget lifetime; pending snapshot locality; settings/reset
  monotonicity; exact one-copy block-boundary application; reconfigure/reset/mode
  ordering; acquisition callback isolation; restart/fail-closed/output behavior;
  CMake/MOC locality and test sensitivity.
- gate / P0/P1 zero and every P2 fixed or explicitly durable-deferred. Reviewer
  is read-only, reports exact lines/evidence/fix/tests, and runs no full app or
  `mne_rt_server`.

### PUBLICATION R-PLUGIN-DATA-002

- issue / `Edddddddddy/mne-cpp#6`
- comment / `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5208534174`
- read-back / authenticated comment ID `5208534174` matches the retained
  Sol/ultra reviewer, exact snapshot `3dca8628eb43029ea35980e869da1a44d21b053a`,
  five-finding scope, manager-evidence boundary and PASS rule.
- state / await one proactive PASS/HOLD response; teaching UI remains held and
  no duplicate reviewer is created.

### WAIT R-PLUGIN-DATA-002-001

- mechanism / one 60-second cursor-aware app wait after GitHub publication.
- result / timeout with the retained reviewer still active; no approval/input
  request and no final gate.
- latest progress / recovery records agree on three immediate correction
  dispositions and two explicit issue-#8 deferrals; reviewer is auditing exact
  current-line behavior and unchanged queue/processor contracts.
- next / no repeated polling. Continue only read-only UI/task preflight and
  await the proactive structured PASS/HOLD response.

### PREFLIGHT W-PLUGIN-UI-TEST-001

- source audit / current plugin `setupWidget()` is only `new QWidget`; no
  teaching widget, diagnostics metatype, UI control signal or plugin snapshot
  exists yet, so the focused missing-header/source tracer remains a truthful
  RED rather than a manufactured failure.
- build seam / `src/testframes/CMakeLists.txt` has one direct registration point;
  the plugin CMake currently lists only the existing plugin/queue/processor/global
  files. A new focused Widgets/Test target can compile the future widget source
  directly without `mne_fiff`, `scShared`, `scMeas` or the plugin target.
- frozen names / exact control, diagnostic-label and typed signal names are now
  local in SPEC. The tracer must observe Qt public behavior only and must not
  parse source or inspect plugin PImpl state.
- GitHub audit / issues #2/#3/#5/#6/#8 remain OPEN; epic #2 has only #4/#7
  checked. No closure or checkbox mutation occurs before lifecycle/UI gates.

### RESPONSE R-PLUGIN-DATA-002

- exact snapshot / clean detached
  `3dca8628eb43029ea35980e869da1a44d21b053a`; initial/final status, staged and
  unstaged diffs all empty.
- mode / independent Sol/ultra, read-only; no source/dependency/GitHub/build/
  test mutation and no full app/server run.
- gate / PASS. New findings P0=0, P1=0, P2=0, P3=0; gate-relevant open counts
  P0=0, P1=0, P2=0.
- closed / `R-PLUGIN-SPSC-PRODUCER-001`: only a one-shot zero-to-one CAS admits
  Entered, provisional invalidation releases its count, and stop/restart require
  closed/quiescent admission before queue replacement.
- closed / `R-PLUGIN-ADMISSION-DROP-001`: Closed returns before type inspection;
  Busy reads only the RTMSA matrix list and counts exact cardinality with no
  info/push/config/reset/output/lock/wait; Entered retains one info snapshot and
  one push per matrix.
- closed / `R-PLUGIN-ATOMIC-LOCKFREE-001`: actual uint32/uint64 aliases map to
  C++14 `ATOMIC_*_LOCK_FREE == 2`; guarded members are exactly producer state and
  dropped count.
- closed / `R-PLUGIN-STATIC-REGISTRATION-001`: static-only target link/import,
  no qrc initializer and unchanged shared behavior match the generated project
  reference evidence.
- deferred / `R-PLUGIN-DESTRUCTOR-BOUND-001`: bounded public stop plus documented
  host retry/quiescence remains; unbounded destructor fallback is the memory-safe
  policy until issue #8 supplies a supported lifecycle/teardown harness.
- deferred / `R-PLUGIN-LIFECYCLE-TEST-001`: focused tests still do not compile
  the real plugin; issue #8 explicitly owns admission/epoch/stop/restart/output/
  metadata/destructor runtime coverage on a supported toolchain.
- regression / worker-only FIFF mapping, disarm/fail-closed behavior, FIFO output,
  restart ordering, queue/processor blobs and narrow dependency seam remain
  coherent. Teaching UI is intentionally absent and is not a finding.
- evidence boundary / reviewer source/provenance only; manager MOC/callback
  oracle/MSVC14.29 syntax/static configure remain manager evidence. MSVC14.51
  real target is still blocked before plugin translation units by existing Qt/
  mne_fiff code; vendor Qt is unchanged.
- next / publish PASS, close issue #5 with issue #8 residual link, update epic
  #2, then dispatch `W-PLUGIN-UI-TEST-001` from the new exact pushed base.

### FAILURE FINAL-FOCUSED-REPLAY-HARNESS-001

- build / current Release focused core, plugin test and example targets all
  rebuild successfully from the canonical build directory.
- run harness / a four-task `Start-Process` table passes an empty ArgumentList
  to the no-argument example, causing a PowerShell parameter validation error.
  The loop then retains a stale process variable, so its displayed example exit
  value is invalid and must not be used. Core/plugin processes returned zero but
  their redirected files are empty, so no totals are claimed from this attempt.
- valid partial evidence / the benchmark invocation independently exits zero at
  p50/p95/max `2.152/2.528/3.617 ms`, generation/accepted/rejected
  `1099/1099/0`, and p95 `<128 ms` PASS.
- correction / after lifecycle review persistence, run each test/example with
  an explicit argument path/reporter and capture independent exit/totals.

### VALIDATION FINAL-FOCUSED-REPLAY-001

- correction / use explicit per-executable launches; give Qt tests their own
  `-o <path>,txt` reporter and omit `ArgumentList` entirely for the no-argument
  example. Every exit and report is independent; no stale process variable.
- build / canonical Release targets `test_causal_reference_denoiser`,
  `test_adaptive_denoising_plugin` and `ex_causal_reference_denoising` all
  rebuild successfully.
- numerical / exit zero, QtTest `45 passed, 0 failed/skipped/blacklisted`;
  stream/probe chunk relative differences are zero, noise reduction is
  `58.6541 dB`, clean amplitude error `0.000113195`.
- processor/queue / exit zero, QtTest `19/0/0`; current overlap pushed/full/
  popped `163/349/163`, maximum producer call `5.8 us`, counted producer
  allocations zero, stop wait `30 ms`.
- example / exit zero and `example invariants: PASS`; learn reaches generation
  four, ApplyOnly accepts/rejects zero and reset restores generation zero.
- benchmark / exit zero and PASS; 270 rows, 16 references, 250 targets, four
  preserved rows, 128 samples, four taps/P64, 100 warmup + 1000 timed blocks;
  nearest-rank p50/p95/max `2.227/2.643/3.787 ms`, final generation/accepted/
  rejected `1099/1099/0`, p95 `<128 ms`.
- boundary / focused Release only; no real plugin target, full mne_scan or
  `mne_rt_server` execution and no vendor/dependency changes.

### VALIDATION FINAL-CORE-DEBUG-001

- build / `cmake --build build-causal-reference-denoising --config Debug
  --target test_causal_reference_denoiser` succeeds. This repository CMake
  intentionally maps both configuration outputs to `out/Release/apps`.
- harness failure / the first launch assumes `out/Debug/apps` and fails before
  execution because that path does not exist. No test result is inferred.
- correction / launch the exact executable path printed by MSBuild with a
  dedicated reporter. Exit zero; QtTest totals `45 passed, 0 failed/skipped/
  blacklisted`, chunk stream/probe differences zero, noise reduction 58.6541 dB
  and clean amplitude error 0.000113195.
- boundary / focused Debug numerical test only; no plugin/full app/server or
  source/dependency modification.

### ARCHIVED COMPLETED QUEUE/PLUGIN GATES

- archived / retained atomic implementation thread
  `019fd64b-2553-7102-91d6-1f8d35720823` after final queue PASS and issue #5
  closure.
- archived / retained queue reviewer thread
  `019fd5d8-380d-7563-8644-264426bda199` after all P0-P3 dispositions became
  durable.
- archived / retained plugin lifecycle reviewer thread
  `019fd84f-03c6-79f1-977f-b278d55777ba` after R-PLUGIN-DATA-002 PASS,
  publication and closure.
- cleanup / app archival only; no app worktree is manually removed. The active
  visible code task set is now only `W-PLUGIN-UI-TEST-001`.

### WAIT W-PLUGIN-UI-TEST-001-001

- mechanism / one 60-second cursor-aware app wait after issue publication and
  final focused replay.
- result / timeout with the Luna/max worker active; no input/approval request,
  RED evidence, commit or final response.
- progress / worker confirms lifecycle PASS released the gate and is reading
  exact widget names, signals, diagnostics, fresh-widget contract and the
  Widgets/Test/Eigen-only seam before the review ledger/journal tail.
- next / no repeated polling. Await proactive response; do not inspect its
  worktree or dispatch production before the RED result is integrated.

### PREPARED GITHUB ISSUE-5-BODY-CLOSEOUT

- target / closed issue `Edddddddddy/mne-cpp#5` only; title/state/comments are
  unchanged.
- reason / the body still names R-PLUGIN-DATA-001 as active and lists remaining
  acceptance despite the later PASS/closure comment.
- update / replace only that stale status tail with exact R-PLUGIN-DATA-002
  PASS, issue-#8 residual deferrals and completed worker-lifecycle acceptance.
- verification / connector body update followed by authenticated `gh issue
  view` read-back. No source/branch/worker or epic mutation.

### RESOLVED GITHUB ISSUE-5-BODY-CLOSEOUT

- update / issue #5 body now records atomic/worker acceptance, exact
  R-PLUGIN-DATA-002 PASS and explicit issue-#8 residual deferrals; the stale
  active-review and remaining-acceptance text is removed.
- formatting / the first structured replacement omitted one blank line before
  `Parent epic`; an immediate body-only correction restores Markdown spacing.
- read-back / issue remains CLOSED/COMPLETED; PASS/follow-up text is present,
  stale text is absent and spacing oracle passes. Title/comments/epic/source/
  branch/worker state are unchanged.

### PUBLICATION R-PLUGIN-DATA-002 / CLOSURE ISSUE-5

- review comment / issue #5
  `https://github.com/Edddddddddy/mne-cpp/issues/5#issuecomment-5208587267`.
- plugin comment / issue #6
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5208587498`.
- state / authenticated read-back confirms issue #5 CLOSED with reason
  COMPLETED, issue #6 OPEN, issue #2 OPEN and epic child `#5` checked.
- residual / issue #8 remains OPEN and continues to own both explicitly
  deferred lifecycle/teardown findings.

### REQUEST W-PLUGIN-UI-TEST-001

- from / manager thread `019fcdc3-4a1e-76d1-8140-1bd521219297`
- to / new visible app-managed worktree conversation
- model / `gpt-5.6-luna` / `max`
- blocking / yes, first vertical UI slice after lifecycle PASS
- exact base / `4cec85cbe6ed53bb07e5ebdf8f6de96675e7c204`
- skill / read complete `tdd` skill; use public Qt behavior only and one tracer
  test, not implementation-detail/source-text tests.
- scope / edit only `src/testframes/CMakeLists.txt` plus new
  `src/testframes/test_adaptive_denoising_ui/CMakeLists.txt` and
  `test_adaptive_denoising_ui.cpp`. Production plugin/widget, queue, processor,
  numerical, other tests, vendor and dependency files are immutable.
- target / register `test_adaptive_denoising_ui`; compile the future
  plugin-private setup-widget source directly and include its fixed diagnostics
  header; link only Qt Core/Widgets/Test and Eigen. Do not link
  `scan_adaptivedenoising`, `mne_fiff`, `mne_rtprocessing`, `scShared` or
  `scMeas`.
- RED / current widget/header/source do not exist. Configuration/build must
  reach the missing production UI seam; do not manufacture another failure or
  copy/repair the isolated ignored Eigen baseline.
- controls / test the exact SPEC object names, ranges/defaults and the seven
  typed signals: enabled/frozen, taps, update interval, memory, regularization
  and reset.
- diagnostics / deliver one fixed POD diagnostics value through the public
  widget slot and assert the fourteen exact value-label names expose plugin/
  configure/process status, R/M/P, warmup, generation, accepted/rejected, three
  RMS values and drops. Add portable C++14 copy/metatype checks; no private
  state, QString/container/owner in the data type.
- response / proactive `RESPONSE W-PLUGIN-UI-TEST-001` with exact parent/
  commit/three paths, test slot/oracles, intended RED command/output, dependency
  proof and clean status. No subagent, manager polling, full app or
  `mne_rt_server`.

### SETUP W-PLUGIN-UI-TEST-001

- setup / accepted as
  `client-new-thread:978a2856-8c82-4b58-b5f3-8d11562ac0cc` on local host.
- model / explicit `gpt-5.6-luna` / `max`.
- environment / app-managed project worktree from local ref
  `codex/worker-ui-test-base`, verified by manager to resolve exactly to
  `4cec85cbe6ed53bb07e5ebdf8f6de96675e7c204`.
- create-harness failures / the first two app calls used the project target
  union at the wrong object level and were rejected as invalid arguments before
  thread creation. They caused no app worktree, source, Git or GitHub mutation.
- recovery / manager corrected only the app argument shape; the worker prompt,
  exact base, Luna/max model and three-path scope are unchanged.
- local base ref / retain `codex/worker-ui-test-base` until app setup and worker
  integration are complete, then delete the spent local-only ref after proof.
- state / asynchronous setup accepted; await proactive response without polling.
- resolved thread / one identity-only app listing maps the setup to visible
  thread `019fd892-ff55-7903-ab3e-25890ccb10b5`, active in app worktree `d206`.
- public dispatch / issue #6 comment/read-back:
  `https://github.com/Edddddddddy/mne-cpp/issues/6#issuecomment-5208612705`.
