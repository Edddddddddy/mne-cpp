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

### E-017 - First valid TDD RED reproduced

- Actor: manager
- Evidence: isolated target configure succeeded; the target build failed with
  `C1083` because `rtprocessing/causalreferencedenoiser.h` does not yet exist.
- Result: one-test tracer seam is valid and free of unrelated MNE dependencies.
- Next: publish the RED milestone and dispatch `W-CORE` to make it green before
  adding the next behavior.

### E-018 - RED milestone published

- Actor: manager.
- Evidence: pushed integration HEAD `bc60ad25d` and posted GitHub issue #2
  comment `issuecomment-5183095071` with the focused command and expected
  missing-header failure.
- Result: the public development record now matches the local TDD state.
- Next: dispatch the logged minimum GREEN request `W-CORE-001`.

### E-019 - First core GREEN request prepared

- Actor: manager.
- Request: `W-CORE-001`.
- Scope: only the numerical production seam and CMake wiring required by the
  existing tracer; no EWLS implementation and no second behavioral test.
- Model/environment: Luna/max, new isolated worktree.
- Next: commit this request record, dispatch it, then record the returned thread
  and worktree identifiers.

### E-020 - First core GREEN worker dispatched

- Actor: manager.
- Request: `W-CORE-001`.
- Client setup ID: `client-new-thread:719101fb-b9e7-4b6f-99ae-7f77dca31fe6`.
- Result: Codex accepted a Luna/max worktree task based on the integration
  branch; the final thread/worktree identifiers are pending setup.
- Next: resolve setup, persist the identifiers, then wait for the structured
  response.

### E-021 - First core GREEN worktree ready

- Actor: manager.
- Thread: `019fce0e-59ad-7ce1-91a3-db5f4fd233e8`.
- Worktree: `C:/Users/lcy/.codex/worktrees/ef9b/mne-cpp-main`.
- Result: `W-CORE-001` is active with Luna/max and the recorded minimum scope.
- Next: wait for `RESPONSE W-CORE-001`, then inspect its diff before any
  cherry-pick.

### E-022 - First core GREEN response reviewed

- Actor: `W-CORE` and manager.
- Response: `RESPONSE W-CORE-001`, commit `c3958cde9`.
- Evidence: four-file, 132-line addition; whitespace check passed; public names
  match the tracer and SPEC; the focused target remains outside the full MNE
  dependency graph.
- Limitation: the worker correctly stopped at the known clean-worktree Eigen
  baseline gap and did not claim a passing build.
- Decision: accept the commit for cherry-pick and reproduce GREEN in the
  populated integration workspace before expanding the TDD suite.
- Next: cherry-pick and run only `test_causal_reference_denoiser`.

### E-023 - First core tracer is GREEN

- Actor: manager.
- Integrated commit: `fc86ff7a6`.
- Evidence: the focused Release target compiled and linked the tracer plus the
  new numerical source; the test executable returned exit code 0. No
  `mne_rt_server` or full application target was run.
- Warnings: existing Eigen headers emitted MSVC C4819 code-page warnings only.
- Result: the first public configure-plus-bypass slice is accepted as GREEN.
- Next: publish this evidence, then request one transactional configure RED.

### E-024 - First GREEN milestone published

- Actor: manager.
- Evidence: pushed through `629921a99` and posted issue #2 comment
  `issuecomment-5183209939` with the focused build and exit-code evidence.
- Result: GitHub and durable local records agree on the first GREEN.
- Next: dispatch `W-TEST-CORE-002`.

### E-025 - Transactional configure RED prepared

- Actor: manager.
- Request: `W-TEST-CORE-002`.
- Scope: one new test slot that observes rejection and preservation of the
  previously committed row layout through shape validation; test-only.
- Model/environment: Luna/max, new isolated worktree.
- Next: commit this request record and dispatch it.

### E-026 - Transactional configure test worker active

- Actor: manager.
- Request: `W-TEST-CORE-002`.
- Thread: `019fce18-2d75-7cf3-ab76-3a8694b4edca`.
- Worktree: `C:/Users/lcy/.codex/worktrees/8a0d/mne-cpp-main`.
- Result: Luna/max worker is active with test-only scope.
- Next: wait for its structured response and inspect the single-file diff.

### E-027 - Transactional configure test response reviewed

- Actor: `W-TEST-CORE` and manager.
- Response: `RESPONSE W-TEST-CORE-002`, commit `6b6ccf5c4`.
- Evidence: one test slot and one changed file; the rejected candidate differs
  in channel count and has an invalid zero block bound, while subsequent
  process shape statuses expose whether the old layout was preserved.
- Decision: accept for integration and reproduce the missing-enum/status RED
  in the populated main workspace.
- Next: cherry-pick and build only `test_causal_reference_denoiser`.

### E-028 - Transactional configure RED reproduced

- Actor: manager.
- Integrated test commit: `e467f4f54`.
- Evidence: the focused Release build reached the test source and failed only
  on missing `InvalidConfiguration` and `InvalidShape` enum members at the
  intended assertions; compiler codes C2838/C2065.
- Result: accepted as the second valid RED.
- Next: dispatch `W-CORE-002` for the minimum configuration/shape GREEN.

### E-029 - Transactional configure GREEN prepared

- Actor: manager.
- Request: `W-CORE-002`.
- Scope: two status values, transactional commit of channel/block bounds, and
  allocation-free bypass shape validation only.
- Model/environment: Luna/max, new isolated worktree.
- Next: commit this record, publish RED evidence, and dispatch the worker.

### E-030 - Transactional configure worker worktree created

- Actor: manager.
- Client setup ID: `client-new-thread:8edf70e8-a904-4eb0-aa66-52a9f3b7cb65`.
- Worktree: `C:/Users/lcy/.codex/worktrees/a425/mne-cpp-main` at `3c338c8f2`.
- Incident: two read-only app thread-list queries hung and were terminated;
  no duplicate worker was created and no repository state was changed by the
  queries.
- Next: retry a bounded thread lookup later, record the ID, and continue with
  the existing worktree only.

### E-031 - Transactional configure worker thread resolved

- Actor: manager.
- Thread: `019fce1d-7aad-7f12-9781-8497c9f66123`.
- Evidence: the local session index associated the newest session with
  worktree `a425`; a direct zero-timeout thread snapshot confirmed active
  status.
- Result: normal bounded waiting can resume without duplicate dispatch.
- Next: wait for `RESPONSE W-CORE-002`.

### E-032 - Transactional configure worker system error

- Actor: Codex app and manager.
- Thread: `019fce1d-7aad-7f12-9781-8497c9f66123` ended `systemError` without a
  response.
- Evidence: worktree `a425` remains clean at `3c338c8f2`, with no new commit or
  diff.
- Decision: record the failed attempt and create one Luna/max replacement with
  the unchanged minimum scope; do not infer or reconstruct missing work.
- Next: dispatch `W-CORE-002-R`.

### E-033 - Transactional configure replacement active

- Actor: manager.
- Request: `W-CORE-002-R`.
- Thread: `019fce3e-f589-7a73-b2f4-bb8bad58f90b`.
- Worktree: `C:/Users/lcy/.codex/worktrees/06cd/mne-cpp-main`.
- Result: direct snapshot confirms Luna/max is active with the unchanged
  numerical header/source-only scope.
- Next: wait for the structured replacement response.

### E-034 - Transactional configure replacement reviewed

- Actor: `W-CORE` replacement and manager.
- Response: `RESPONSE W-CORE-002-R`, commit `8d9a65010`.
- Evidence: two numerical files only; invalid bounds return before state writes,
  valid bounds commit, and process performs only fixed scalar shape checks.
- Decision: accept for cherry-pick; runtime GREEN still requires the populated
  main workspace focused build and test run.
- Next: integrate and run only `test_causal_reference_denoiser`.

### E-035 - Transactional configure slice is GREEN

- Actor: manager.
- Integrated commit: `cc84fd082`.
- Evidence: focused Release target built and linked; the two-slot Qt test
  executable returned exit code 0. No full application or rt_server was run.
- Result: rejected configuration preserves committed shape and invalid blocks
  pass through unchanged with explicit status.
- Next: publish evidence and create one configuration-invariant RED slot.

### E-036 - Transactional GREEN published

- Actor: manager.
- Evidence: pushed through `7dcb8e016` and posted issue #2 comment
  `issuecomment-5183704507`.
- Result: GitHub issue and durable records match the tested state.
- Next: dispatch `W-TEST-CORE-003`.

### E-037 - Configuration-invariant RED prepared

- Actor: manager.
- Request: `W-TEST-CORE-003`.
- Scope: one data-driven test behavior for all remaining configure invariants;
  test source only.
- Model/environment: Luna/max, new isolated worktree.
- Next: commit the request and dispatch it.

### E-038 - Configuration-invariant test worker active

- Actor: manager.
- Request: `W-TEST-CORE-003`.
- Thread: `019fce45-615b-7612-9f23-3694f6c581eb`.
- Worktree: `C:/Users/lcy/.codex/worktrees/02c2/mne-cpp-main`.
- Result: Luna/max is active with one-file, test-only scope.
- Next: wait for `RESPONSE W-TEST-CORE-003`.

### E-039 - Configuration record path clarified

- Actor: worker and manager.
- Evidence: the worker first tried root-level record names; manager verified
  the required files exist under `doc/causal_reference_denoising/` in worktree
  `02c2` at `652492ff6`.
- Response: `REQUEST W-TEST-CORE-003-PATH` provides the exact paths and keeps
  the task non-blocking and unchanged.
- Next: forward the recorded path clarification and continue waiting.

### E-040 - Configuration-invariant test reviewed

- Actor: `W-TEST-CORE` and manager.
- Response: `RESPONSE W-TEST-CORE-003`, commit `eb7573efe`.
- Evidence: one test file, one data-driven behavior, 26 deterministic rows;
  no production or CMake changes.
- Decision: accept the test design. Main workspace must demonstrate runtime
  RED because the clean worker could not generate the focused target.
- Next: cherry-pick, build, and capture the Qt text result under the ignored
  focused build directory.

### E-041 - Configuration-invariant runtime RED reproduced

- Actor: manager.
- Integrated test commit: `19a23b9a6`.
- Evidence: synchronized Qt run produced a text report with 4 passes, 26
  intended failures, and exit code 26; first failure was
  `samplingFrequencyHz_zero`.
- Correction: a prior direct PowerShell invocation did not wait for the GUI
  executable and produced no report, so it was explicitly excluded as evidence.
- Result: accepted runtime RED.
- Next: dispatch `W-CORE-003` for validation-only GREEN.

### E-042 - Configuration validation worker active

- Actor: manager.
- Request: `W-CORE-003`.
- Thread: `019fce4c-69f2-7193-b5c7-47bad21bf7c0`.
- Worktree: `C:/Users/lcy/.codex/worktrees/5822/mne-cpp-main`.
- Result: Luna/max is active with numerical validation-only scope.
- Next: wait for `RESPONSE W-CORE-003`.

### E-043 - Configuration validation response reviewed

- Actor: `W-CORE` and manager.
- Response: `RESPONSE W-CORE-003`, commit `f91bab110`.
- Evidence: one `.cpp` change; scalar checks and row loops are allocation-free;
  overflow-safe feature comparison is protected by earlier non-empty/positive
  short-circuit checks.
- Decision: accept for integration; runtime GREEN remains unclaimed until the
  synchronized focused Qt run passes.
- Next: cherry-pick and run the text-report test.

### E-044 - Configuration validation is GREEN

- Actor: manager.
- Integrated commit: `e80e02098`.
- Evidence: focused Release build succeeded; synchronized Qt report contains
  30 passes, zero failures, and exit code 0.
- Result: all current configure and shape behaviors are accepted.
- Next: publish evidence and dispatch the first causal EWLS streaming tracer.

### E-045 - Worker lifecycle rule added by user

- Actor: user and manager.
- Decision: retire non-reusable subagent conversations and create fresh ones
  when needed; retain related conversations that remain reusable. The manager
  selects Luna/max or Sol according to task risk and role.
- Platform mapping: the app exposes archive rather than hard thread deletion,
  so archive is the safe retirement operation; app-owned worktrees are not
  manually removed.
- Next: persist the rule in `SPEC.md` and `WORKERS.md`, then archive the logged
  one-shot/failed/superseded threads while retaining selected `W-DESIGN-C`.

### E-046 - First worker retirement set prepared

- Actor: manager.
- Request: `CLEANUP-001`.
- Scope: archive rejected design alternatives and completed/failed one-shot
  core workers through `W-CORE-003`; keep the selected reusable design-C
  conversation and the manager task.
- Next: commit this record, perform archive calls, and record exact results.

### E-047 - First worker retirement set completed

- Actor: manager and Codex app.
- Response: `CLEANUP-001` archived all 11 requested non-reusable threads.
- Retained: `W-DESIGN-C` and the manager task remain available because their
  context is still related/reusable.
- Result: lifecycle policy is active; no worktree was manually removed.
- Next: dispatch the already logged `W-TEST-CORE-004` request on Luna/max.

### E-048 - Causal EWLS tracer worker active

- Actor: manager.
- Request: `W-TEST-CORE-004`.
- Thread: `019fd00e-e7ff-7220-96b2-1310acbebf25`.
- Worktree: `C:/Users/lcy/.codex/worktrees/725c/mne-cpp-main`.
- Result: Luna/max is active with one-file causal-tracer test scope.
- Lifecycle note: the app cleaned archived worker worktrees; the manager did
  not manually remove them.
- Next: wait for `RESPONSE W-TEST-CORE-004`.

### E-049 - Causal EWLS tracer response reviewed

- Actor: `W-TEST-CORE` and manager.
- Response: `RESPONSE W-TEST-CORE-004`, commit `37edd7ef5`.
- Evidence: one test slot; independent two-tap feature samples solve the known
  `[2,3]` model and the post-boundary sample tests future-only application.
- Decision: accept for integration and require a clean missing-`Processed` RED
  before production streaming state begins.
- Next: cherry-pick and build only the focused target.

### E-050 - Causal EWLS tracer RED reproduced

- Actor: manager.
- Integrated test commit: `fb0a42385`.
- Evidence: focused build reached the new test and failed only because
  `DenoiserProcessStatus::Processed` is absent, with C2838/C2065 at the two
  intended assertions.
- Result: valid compile RED.
- Next: archive the one-shot test worker and dispatch high-risk `W-CORE-004`.

### E-051 - Sol selected for first streaming implementation

- Actor: manager under the user model-selection rule.
- Decision: use Sol/ultra rather than Luna/max for `W-CORE-004` because the
  slice combines numerical conditioning, causal state ordering, Eigen storage,
  LDLT, and allocation-free hot-path requirements.
- Scope: only the existing causal tracer GREEN; later safety/edge behaviors
  remain separate RED slices.
- Next: commit the request, archive `W-TEST-CORE-004`, then dispatch Sol/ultra.

### E-052 - Causal tracer test worker retired

- Actor: manager and Codex app.
- Request: `CLEANUP-002`.
- Result: one-shot `W-TEST-CORE-004` returned `archived: true`; its test commit,
  response, and RED evidence were already durable.
- Next: create the new Sol/ultra `W-CORE-004` worktree.

### E-053 - Causal EWLS implementation worker active

- Actor: manager.
- Request: `W-CORE-004`.
- Thread: `019fd018-8939-7e72-9fc8-17085c79806e`.
- Worktree: `C:/Users/lcy/.codex/worktrees/9bd8/mne-cpp-main`.
- Result: Sol/ultra is active and explicitly using codebase-design plus TDD
  with numerical header/source-only scope.
- Next: wait for `RESPONSE W-CORE-004`.

### E-054 - Unauthorized nested reviewer delegation corrected

- Actor: `W-CORE-004` and manager.
- Evidence: worker commentary announced two nested Sol/ultra read-only
  reviewers without a prior manager-routed durable request.
- Decision: their findings are not accepted; `REQUEST W-CORE-004-DELEGATION`
  requires the worker to stop/disregard them, report identifiers/status, spawn
  no more, and finish alone. Formal review stays with the manager.
- Next: commit and forward the correction, then continue bounded waiting.

### E-055 - Causal EWLS implementation response reviewed

- Actor: `W-CORE-004` and manager.
- Response: `RESPONSE W-CORE-004`, commit `f3a909941`.
- Evidence: two numerical files, transactional PImpl allocation, tap-major
  history/features, preallocated EWLS/LDLT state, and future-only model commit.
- Delegation result: both unauthorized nested reviewers were interrupted and
  disregarded, as required.
- Decision: accept for cherry-pick and main-workspace compile/runtime checks;
  do not claim hot-path allocation proof until the dedicated guard test.
- Next: build and run only `test_causal_reference_denoiser`.

### E-056 - First causal EWLS implementation is GREEN

- Actor: manager.
- Integrated commit: `10e33a1b8`.
- Evidence: focused Release build succeeded and synchronized Qt report contains
  31 passes, zero failures, and exit code 0.
- Result: causal warmup, tap ordering, future-only boundary commit, ApplyOnly,
  and target-only write are accepted for the current tracer.
- Next: publish evidence, retire the non-reusable implementation threads, and
  dispatch atomic nonfinite RED on Luna/max.

### E-057 - Causal implementation threads retired

- Actor: manager and Codex app.
- Request: `CLEANUP-003`.
- Result: `W-CORE-004` and both interrupted unauthorized nested reviewer
  threads returned `archived: true`.
- Evidence remains durable in commits, test reports, and `WORKERS.md`.
- Next: create fresh Luna/max `W-TEST-CORE-005`.

### E-058 - Atomic nonfinite test worker active

- Actor: manager.
- Request: `W-TEST-CORE-005`.
- Thread: `019fd040-bb35-7e83-9dc8-ee2abfbb21d1`.
- Worktree: `C:/Users/lcy/.codex/worktrees/e95c/mne-cpp-main`.
- Result: Luna/max is active with one-file test-only scope and explicit no-
  subagent instruction.
- Next: wait for `RESPONSE W-TEST-CORE-005`.

### E-059 - Atomic nonfinite test response reviewed

- Actor: `W-TEST-CORE` and manager.
- Response: `RESPONSE W-TEST-CORE-005`, commit `9ef2ffe08`.
- Evidence: one file, two data rows, control/subject continuation and probe
  comparison proves all relevant streaming state is unchanged by rejection.
- Decision: accept for integration and require missing-`NonFiniteInput` RED.
- Next: cherry-pick and build only the focused target.

### E-060 - Atomic nonfinite RED reproduced

- Actor: manager.
- Integrated test commit: `1eaa5ba41`.
- Evidence: focused compile failed only on missing `NonFiniteInput` at the
  intended assertion, C2838/C2065.
- Result: valid RED; bounded preflight implementation may begin.
- Next: archive the test worker and dispatch fresh Luna/max `W-CORE-005`.

### E-061 - Atomic nonfinite test worker retired

- Actor: manager and Codex app.
- Request: `CLEANUP-004`.
- Result: `W-TEST-CORE-005` returned `archived: true` after its response, commit,
  and RED evidence were durable.
- Next: create fresh Luna/max `W-CORE-005`.

### E-062 - Atomic nonfinite implementation worker active

- Actor: manager.
- Request: `W-CORE-005`.
- Thread: `019fd049-160c-7902-b82a-a587ce0a995e`.
- Worktree: `C:/Users/lcy/.codex/worktrees/bb5c/mne-cpp-main`.
- Result: Luna/max is active with numerical header/source-only scope and no
  subagent authority.
- Next: wait for `RESPONSE W-CORE-005`.

### E-063 - Atomic nonfinite response delivery reconciled

- Actor: user, manager, and `W-CORE-005`.
- User report: the worker UI said its result was sent, but the manager notice
  appeared missing.
- Resolution: the bounded wait API returned the completed structured response,
  commit `3040ef33e`, and no-subagent confirmation. Direct git inspection
  verified the two-file diff; no duplicate worker was started.
- Model rule reaffirmed: Luna/max for bounded test/implementation tasks;
  Sol/ultra for manager-selected high-risk math/concurrency/review work.
- Next: integrate and run the complete focused test report.

### E-064 - Atomic nonfinite slice is GREEN

- Actor: manager.
- Integrated commit: `b8d865917`.
- Evidence: focused Release build and synchronized Qt report with 33 passes,
  zero failures, and exit code 0.
- Result: selected reference/target nonfinite blocks are atomic pass-through and
  leave streaming state unchanged.
- Next: publish/retire, then create chunk-equivalence acceptance test.

### E-065 - Atomic nonfinite implementation worker retired

- Actor: manager and Codex app.
- Request: `CLEANUP-005`.
- Result: `W-CORE-005` returned `archived: true`; response delivery, commit, and
  GREEN report are durable.
- Next: create fresh Luna/max `W-TEST-CORE-006`.

### E-066 - Chunk-equivalence test worker active

- Actor: manager.
- Request: `W-TEST-CORE-006`.
- Thread: `019fd0cc-9f37-7dd0-9051-4086072efabe`.
- Worktree: `C:/Users/lcy/.codex/worktrees/639d/mne-cpp-main`.
- Result: Luna/max is active with one public-interface test slot and no
  subagents.
- Next: wait for `RESPONSE W-TEST-CORE-006`.

### E-067 - Chunk-equivalence test response received

- Actor: `W-TEST-CORE-006` and manager.
- Response: test-only commit `8aa84d032` with a 21-sample deterministic stream,
  chunks `{1,3,2,5,4,6}`, four adaptation boundaries, and a common ApplyOnly
  probe.
- Evidence: the worker reports `git diff --check` success, no subagents, and no
  production/CMake changes. Runtime classification remains infrastructure-
  limited because the clean worktree lacks the ignored Eigen baseline.
- Decision: record the response before acting; manager must now inspect the
  exact diff and run it in the populated integration workspace.

### E-068 - Chunk-equivalence test diff accepted

- Actor: manager using codebase-design and TDD review criteria.
- Scope evidence: one public-interface Qt slot and one test file only; no
  production, CMake, plugin, dependency, or unrelated edits.
- Contract evidence: the test spans causal warmup, four fixed update epochs,
  irregular chunk boundaries, a common future ApplyOnly probe, and exact
  preservation of reference/unselected rows.
- Decision: accept commit `8aa84d032`; commit the durable response/review record
  before cherry-pick, then build and run the focused target.

### E-069 - Chunk-equivalence contract is GREEN

- Actor: manager.
- Integrated test commit: `b6f5b9204`.
- Build evidence: the focused Release target compiled and linked successfully;
  only the known Eigen/MSVC C4819 code-page warnings appeared.
- Runtime evidence: synchronized Qt report contains 34 passes, zero failures,
  and process exit code 0. Stream and future ApplyOnly probe relative errors
  are both exactly zero; reference and unselected rows are exact.
- Result: finite-stream output and learned state are independent of block
  partitioning for this acceptance stream at the required `1e-10` tolerance.
- Next: publish the milestone, retire the one-shot worker, and start reset/mode
  state-semantics TDD on a fresh Luna/max worker.

### E-070 - Chunk-equivalence milestone published and worker retired

- Actor: manager, GitHub, and Codex app.
- Publish result: branch pushed through `57d5e2535`; issue #2 comment
  `5188904172` records the zero relative errors and 34/0 focused report.
- Retirement result: `W-TEST-CORE-006` thread
  `019fd0cc-9f37-7dd0-9051-4086072efabe` returned `archived: true`.
- Next: dispatch the already specified isolated reset-contract worker.

### E-071 - Reset contract worker request prepared

- Actor: manager under the user model-selection and TDD rules.
- Request: `W-TEST-CORE-007`, exactly one test-only public-interface behavior.
- Model decision: Sol/ultra because observable reset equivalence must detect
  retained lag history, epoch statistics, committed weights, or counters.
- Isolation: detached worktree at
  `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-007`; no nested agents.
- Next: commit the request, create the worktree from current HEAD, and dispatch.

### E-072 - Reset contract worker active

- Actor: manager and collaboration agent `/root/w_test_core_007`.
- Worktree: `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-007`, detached
  from integration commit `71aefcd26`.
- Model: `gpt-5.6-sol`, `ultra`; no nested subagents are authorized.
- Result: the exact test-only reset request is active.
- Next: inspect later core gaps read-only while waiting for the structured
  response; do not edit the overlapping test source in the manager workspace.

### E-073 - Reset worker status requested

- Actor: manager.
- Evidence: repeated bounded waits show `/root/w_test_core_007` still running;
  the detached worktree remains clean at `71aefcd26`.
- Action: sent non-blocking `REQUEST W-TEST-CORE-007-STATUS` for a concise
  progress/blocker note without changing scope.
- Next: continue bounded waiting; do not duplicate the task.

### E-074 - Reset worker reports no blocker

- Actor: `/root/w_test_core_007` and manager.
- Response: recovery/skill reading and seam/test inspection are complete; the
  worktree remains clean at `71aefcd26`, with no `AGENTS.md` and no subagents.
- Progress: the worker is designing one public sequence that independently
  exposes retained history, statistics, pending count, and model weights.
- Decision: continue the same Sol/ultra worker without duplicate dispatch.

### E-075 - Reset test implementation narrowed

- Actor: manager.
- Evidence: later bounded waits still show a clean worktree, so no partial edit
  exists to protect or review.
- Action: `REQUEST W-TEST-CORE-007-NARROW` asks the existing worker to stop
  design expansion and implement the sufficient prime/reset/new-boundary/probe
  black-box sequence now.
- Scope/model remain unchanged; no duplicate worker is created.

### E-076 - Reset worker confirms bounded implementation

- Actor: `/root/w_test_core_007` and manager.
- Response: one Qt slot uses tap three/interval three, primes a committed model
  plus partial epoch, resets, then compares five new training samples and a
  two-sample ApplyOnly probe with a fresh control.
- Evidence: this crosses post-reset warmup and a new boundary; retained model,
  history, statistics, or epoch count can affect observable outputs/state.
- Status: validation and commit remain in progress; no subagents.

### E-077 - Reset worker asked to finish without unavailable build

- Actor: manager.
- Evidence: the detached worktree now contains exactly one modified test file,
  110 inserted lines, and passes `git diff --check`; it still lacks the ignored
  populated-main Eigen baseline.
- Action: `REQUEST W-TEST-CORE-007-FINISH` directs the worker to stop spending
  time on unavailable build infrastructure, commit, and respond.
- Next: receive and review the exact commit; manager owns runtime validation.

### E-078 - Non-responsive reset worker scheduled for interruption

- Actor: manager under the user worker lifecycle policy.
- Evidence: implementation exists as one diff-checked test file, yet repeated
  waits after explicit finish guidance yielded neither commit nor response.
- Decision: record then interrupt `/root/w_test_core_007`; retain its isolated
  working-tree modification for manager review instead of opening a duplicate
  worker or losing work.
- Next: inspect the exact recovered diff and accept/reject it against the
  original public reset contract.

### E-079 - Reset test commit recovered and reviewed

- Actor: manager after interrupting `/root/w_test_core_007`.
- Discovery: the worker had just committed `7640f3750`; the detached worktree
  is clean, so no modification was lost.
- Review: one test slot correctly primes model/history/partial-epoch state,
  resets, crosses a fresh boundary against a new control, and probes the new
  model at `1e-12`; non-target rows are exact.
- Decision: accept the diff. Trigger one response-only turn to complete the
  REQUEST/RESPONSE ledger, with no further edits allowed, before cherry-pick.

### E-080 - Reset worker response protocol recovered

- Actor: `/root/w_test_core_007` and manager.
- Response: existing commit `7640f3750`, exact prime/reset/new-boundary/probe
  sequence, one-file scope, clean diff/worktree evidence, no subagents.
- Limitation: worker runtime remains correctly unclaimed because its detached
  worktree lacks ignored Eigen baseline files.
- Decision: accept the response and commit; integrate and run only the populated
  main-workspace focused target.

### E-081 - Reset-to-fresh-state contract is GREEN

- Actor: manager.
- Integrated test: `679fc427b`.
- Evidence: focused Release build succeeded; synchronized Qt report has 35
  passes, zero failures, exit code 0, and both reset training/probe relative
  differences equal zero.
- Result: no production edit is needed for reset semantics.
- Next: publish evidence, clean the non-reusable detached worktree, then test
  ApplyOnly/Bypass non-learning state against analytically known weights.

### E-082 - Reset milestone published and isolated worker retired

- Actor: manager, GitHub, Git worktree management, and collaboration runtime.
- Result: issue comment `5189127958` records 35/0 and zero reset errors; detached
  worktree `mne-cpp-worker-w-test-core-007` was verified clean/content-matched
  and removed; the agent is completed.
- Next: start the independently isolated non-learning-mode contract.

### E-083 - Non-learning mode worker request prepared

- Actor: manager under TDD and user model-selection rules.
- Request: `W-TEST-CORE-008`, one analytic data-driven slot for ApplyOnly and
  BypassTrackHistory.
- Oracle: known two-tap weights and outputs, not cross-comparison of two
  implementations; adversarial interval detects learning and later probe
  detects both model change and missing history advancement.
- Model: Sol/high, proportional to bounded test-only state reasoning; no
  subagents.
- Next: commit/push the request, create its detached worktree, and dispatch.

### E-084 - Non-learning mode worker active

- Actor: manager and `/root/w_test_core_008`.
- Worktree: `C:/Users/lcy/Desktop/meg/mne-cpp-worker-w-test-core-008`, detached
  at integration commit `b27b328d9`.
- Model: Sol/high; exact analytic one-slot scope and no-subagent rule accepted.
- Next: wait for the structured response while avoiding overlapping test edits.

### E-085 - Non-learning mode test response received

- Actor: `/root/w_test_core_008` and manager.
- Response: test-only commit `ec69672b6` with ApplyOnly/Bypass data rows, known
  model `2r(t)+3r(t-1)`, adversarial full interval, and analytic future probe.
- Evidence: one file, clean diff/worktree, no subagents; worker runtime remains
  infrastructure-limited by the known ignored Eigen baseline.
- Decision: record before acting; manager now inspects the exact commit and
  validates the oracle arithmetic before integration.

### E-086 - Non-learning response SHA reconciled

- Actor: manager.
- Issue: the response's full SHA does not resolve although short SHA
  `ec69672b6` matches the worker HEAD.
- Evidence: the clean detached worktree reports HEAD
  `ec69672b6ebda8871de7a8787dabb3f58047e51a` via `git rev-parse`.
- Decision: treat the mismatch as a response typo, record it explicitly, and
  review/integrate the resolved object without duplicate implementation.

### E-087 - Non-learning analytic test accepted

- Actor: manager using codebase-design/TDD review criteria.
- Arithmetic: known model `[2,3]`, adversarial residuals `[480,-422]`, probe
  features `[7,-4]`/`[-6,7]`, and targets `[2,9]` are consistent with tap-major
  current/lag ordering.
- Robustness: tolerance `1e-5` covers only the configured `1e-8` loading and
  forgetting perturbation; unintended learning or missing history is much
  larger.
- Decision: accept resolved commit `ec69672b6`; integrate and run focused tests.

### E-088 - All three mode semantics are GREEN

- Actor: manager.
- Integrated test: `58c4646ae`.
- Evidence: focused Release build succeeds; synchronized report has 37 passes,
  zero failures, exit code 0. ApplyOnly and Bypass analytic rows both pass.
- Result: committed model preservation, causal history tracking, ApplyOnly
  subtraction, and exact Bypass are covered without a production change.
- Next: publish/retire, then review the diagnostics and solver-failure seam.

### E-089 - Generic code-review skill fallback recorded

- Actor: manager.
- Catalog evidence: this session exposes `codebase-design`, TDD, and GitHub
  workflow skills but no generic skill named `code-review`.
- Decision: use the documented fallback: independent Sol/ultra read-only
  reviewer, codebase-design vocabulary, and structured P0-P3 findings in
  `REVIEW.md`. No plugin installation is inferred.

### E-090 - Mode milestone published and worker cleaned

- Actor: manager, GitHub, and Git worktree management.
- Result: issue comment `5189222318` records 37/0 mode evidence; the clean
  `W-TEST-CORE-008` worktree content matched integration and was removed.
- Next: start read-only diagnostics seam review.

### E-091 - Diagnostics seam review request prepared

- Actor: manager using codebase-design and review fallback.
- Request: `R-DIAGNOSTICS-001`, Sol/ultra, read-only, no subagents.
- Focus: minimal fixed-size result fields, multi-boundary update/rejection
  reporting, error/mode/reset/RMS semantics, recoverable finite-overflow/LDLT
  behavior, and focused test ordering.
- Next: commit/push request and dispatch reviewer on the current snapshot.

### E-092 - Diagnostics seam reviewer active

- Actor: manager and `/root/r_diagnostics_001`.
- Model/scope: Sol/ultra, read-only current integration snapshot, no subagents.
- Result: exact fixed-size interface/failure-semantics review is active.
- Next: wait for the structured response; do not edit numerical interface while
  review is running.

### E-093 - Diagnostics reviewer status requested

- Actor: manager.
- Evidence: multiple bounded waits show the Sol/ultra read-only review still
  active; the repository remains unmodified by the reviewer.
- Action: non-blocking `REQUEST R-DIAGNOSTICS-001-STATUS`, with no scope change.
- Next: keep waiting without duplicate review dispatch.

### E-094 - Diagnostics reviewer reports no blocker

- Actor: `/root/r_diagnostics_001` and manager.
- Response: record/skill/source/test inspection is complete at snapshot
  `2f6b6c8d6`; fixed fields and transactional rejection policy are being
  resolved.
- Scope: read-only, no subagents, no rt_server.
- Decision: continue the same review; do not preempt its interface conclusion.

### E-095 - Diagnostics reviewer asked to conclude

- Actor: manager.
- Evidence: review inputs are complete, no blocker exists, and several later
  bounded waits produced no new dependency or question.
- Action: `REQUEST R-DIAGNOSTICS-001-FINISH` asks for the structured conclusion
  now, without plugin expansion or code edits.
- Next: receive one final interface/finding set and retire the reviewer.

### E-096 - Diagnostics seam review completed

- Actor: `/root/r_diagnostics_001` and manager.
- Result: selected one fixed-size result snapshot with configured counts,
  post-call warmup/generation, per-call accept/reject counts and stable RMS.
- Semantics: solve rejection is an update event on a valid processed block, not
  a primary atomic error; committed/pending EWLS stats become transactional.
- Findings: two P1, three P2, one P3, no P0; all are recorded in `REVIEW.md`.
- Scope: reviewer was read-only and spawned no subagents.
- Next: implement the first compile-RED diagnostics lifecycle test.

### E-097 - Diagnostics lifecycle test request prepared

- Actor: manager under TDD and selected seam.
- Request: `W-TEST-CORE-009`, one test source/slot, Sol/high, no subagents.
- Sequence: NotConfigured, Bypass warmup, ApplyOnly warmup completion, one
  accepted ApplyAndLearn boundary, reset snapshot; analytic RMS at every call.
- Next: commit/push records, create detached worktree, dispatch.

### E-098 - Diagnostics lifecycle test worker active

- Actor: manager and `/root/w_test_core_009`.
- Worktree/model: detached `mne-cpp-worker-w-test-core-009` at `1d7729ab3`,
  Sol/high, no subagents.
- Result: the exact one-slot NotConfigured/warmup/RMS/generation lifecycle test
  is active.
- Next: wait for structured response; do not edit the overlapping test source.

### E-099 - Diagnostics lifecycle test response received

- Actor: `/root/w_test_core_009` and manager.
- Response: one-slot test commit `bdfca52d3` covering NotConfigured, warmup via
  Bypass/ApplyOnly, one accepted boundary, analytic RMS, and reset snapshot.
- Evidence: one file, clean diff/worktree, no subagents; detached runtime is
  limited only by known ignored Eigen files.
- Decision: record before acting; manager now inspects exact assertions and
  reproduces the intended compile RED after acceptance.

### E-100 - Diagnostics lifecycle test accepted

- Actor: manager using selected diagnostics seam and TDD criteria.
- Review: one-slot scope, quiet-NaN unconfigured result, warmup sequence,
  accepted generation, reset snapshot, and `sqrt(84.5)` oracle are consistent.
- Decision: accept `bdfca52d3`; integrate and build only the focused target to
  reproduce missing-interface RED.

### E-101 - Diagnostics lifecycle compile RED reproduced

- Actor: manager.
- Integrated test: `a0fcf0e68`.
- Evidence: focused compile fails on unknown diagnostics type, missing
  NotConfigured, and absent result diagnostics member at the intended slot.
- Result: valid RED; no unrelated production failure or rt_server execution.
- Next: retire the test worker and dispatch bounded hot-path implementation.

### E-102 - Diagnostics lifecycle implementation request prepared

- Actor: manager under TDD and selected diagnostics seam.
- Request: `W-CORE-006`, Sol/high, numerical header/source only, no subagents.
- Scope: fixed result, post-call snapshot, stable scaled RMS, generation and
  boundary event aggregation. Transactional pending statistics remain a later
  independent RED/GREEN slice.
- Next: commit/push, clean test worktree, create implementation worktree, dispatch.

### E-103 - Diagnostics lifecycle implementation active

- Actor: manager and `/root/w_core_006`.
- Cleanup: completed `W-TEST-CORE-009` worktree was clean, content-matched to
  integration, and removed.
- Worktree/model: detached `mne-cpp-worker-w-core-006` at `8637bb118`, Sol/high,
  no subagents.
- Next: wait for the numerical header/source response; no overlapping edits.

### E-104 - Diagnostics implementation worker transport failure

- Actor: `/root/w_core_006` runtime and manager.
- Result: the worker response stream disconnected before completion; no
  structured response or commit is accepted.
- Classification: external transport failure, not product RED/GREEN.
- Decision: record first, then inspect isolated worktree. Retire/recreate the
  one-shot worker if it lacks a complete clean commit; do not reuse blindly.

### E-105 - Failed diagnostics worker has no recoverable edit

- Actor: manager.
- Evidence: `mne-cpp-worker-w-core-006` is clean at `8637bb118`, with no diff
  and no implementation commit.
- Decision: retire the failed agent/worktree and dispatch fresh Sol/high
  `W-CORE-006-R` under the unchanged, already durable implementation scope.
- Next: commit/push, remove the clean worktree, create replacement, dispatch.

### E-106 - Replacement diagnostics worker active

- Actor: manager and `/root/w_core_006_r`.
- Cleanup: failed clean worktree removed; no code lost.
- Worktree/model: detached `mne-cpp-worker-w-core-006-r` at `810e11d65`,
  Sol/high, no subagents.
- Next: wait for complete header/source commit; no overlapping edits.

### E-107 - Replacement diagnostics worker status requested

- Actor: manager.
- Evidence: repeated bounded waits show `/root/w_core_006_r` running; detached
  worktree remains clean at `810e11d65`.
- Action: non-blocking status request with no scope change or duplicate worker.
- Next: await progress/blocker response.

### E-108 - Replacement diagnostics implementation complete in worktree

- Actor: `/root/w_core_006_r` and manager.
- Response: no blocker; header/source implementation is complete, diff-check
  clean, and undergoing final static/compile-oriented review.
- Limitation: runtime is manager-owned due the documented ignored Eigen baseline.
- Scope: no subagents; original bounded slice unchanged.
- Next: wait for commit/structured response without duplicate work.

### E-109 - Replacement response transport failed after implementation

- Actor: `/root/w_core_006_r` runtime and manager.
- Result: final response stream disconnected, but two scoped numerical files
  remain modified and diff-check clean in the isolated worktree.
- Decision: preserve and review the complete diff; do not reopen implementation.
  Manager may create a recovery commit only after verifying every branch of the
  selected lifecycle contract.

### E-110 - Diagnostics implementation recovery diff accepted

- Actor: manager using codebase-design/TDD and the selected diagnostics seam.
- Review: exact two-file scope; all early-return snapshots, post-call warmup,
  event/generation aggregation and scaled RMS paths match the RED contract.
- Real-time check: no explicit allocation/lock/string path added; later
  transactional/app-overflow findings remain intentionally open.
- Decision: commit the existing isolated diff as manager recovery without
  editing it, then integrate and validate.

### E-111 - Diagnostics implementation recovered as a commit

- Actor: manager in the detached replacement worktree.
- Commit: `eb342faf5`, exactly the accepted numerical header/source diff.
- Evidence: worktree is clean after commit; no production edit was added during
  recovery and no worker runtime claim is inferred.
- Note: a document commit attempted from the detached worktree had no staged
  document change, as expected; the main-workspace durable record remains
  intact and is committed before cherry-pick.
- Next: integrate `eb342faf5` and run the focused target/report.

### E-112 - Fixed diagnostics lifecycle is GREEN

- Actor: manager.
- Integrated production: `b5f29fff1`.
- Evidence: focused Release build succeeds; synchronized Qt report has 38
  passes, zero failures and exit code 0.
- Review effect: close P3 `R-STATUS-001`; P1 `R-DIAG-001` remains in progress
  until mixed boundary counts are tested.
- Next: publish/retire and dispatch extreme finite transactional RED.

### E-113 - Transactional rejection test request prepared

- Actor: manager under reviewer P1 and TDD.
- Request: `W-TEST-CORE-010`, Sol/ultra, one test slot/source, no subagents.
- Stream: zero-model accept, two finite-overflow rejections, normal recovery
  accept in one block, then future probe; expected events `2/2`, generation2.
- Next: commit/push, clean recovery worktree, create test worktree, dispatch.

### E-114 - Transactional rejection test worker active

- Actor: manager and `/root/w_test_core_010`.
- Publish/cleanup: diagnostics issue comment `5189807403`; clean recovery
  worktree removed after content match.
- Worktree/model: detached `mne-cpp-worker-w-test-core-010` at `ff0fc018d`,
  Sol/ultra, no subagents.
- Next: wait for one-slot test response; no overlapping test edits.

### E-115 - Transactional rejection test response received

- Actor: `/root/w_test_core_010` and manager.
- Response: one-slot commit `3d38866c5` with accepted/rejected generation oracle
  `2/2`, two extreme finite poisoned epochs, recovery epoch and future probe.
- Evidence: one file/68 insertions, clean diff/worktree, no subagents; detached
  runtime limitation is the known ignored Eigen baseline.
- Decision: record before acting; inspect exact assertions, then reproduce
  current poisoned-statistics runtime RED in the main workspace.

### E-116 - Transactional rejection test accepted

- Actor: manager using the selected transactional diagnostics seam and TDD.
- Review: exact one-slot/test-only scope; four-boundary event oracle, finite
  extreme-input rejection, unchanged first-block output, stable RMS and future
  recovered-model probe are internally consistent.
- Expected RED: current in-place statistics remain poisoned after the first
  extreme epoch, producing events `1/3`, generation one and probe residual six.
- Decision: accept `3d38866c5`; commit the durable review, then integrate and
  reproduce runtime RED before dispatching production work.

### E-117 - Transactional recovery runtime RED reproduced

- Actor: manager under TDD.
- Integrated test: `d1ffa0904`.
- Evidence: focused Release build succeeded; synchronized Qt report returned
  38 passes, one intended failure at `modelUpdatesAccepted == 2`, and exit code
  one. Every pre-existing numerical test remains GREEN.
- Diagnosis: in-place `G/H` overflow persists across epoch boundaries, so the
  later normal epoch cannot recover a model.
- Next: commit/push the RED, clean the one-shot test worktree after exact
  content verification, then dispatch a fresh Sol/ultra implementation worker.

### E-118 - Transactional test worker retired

- Actor: manager under the user worker lifecycle policy.
- Evidence: detached worktree was clean at `3d38866c5`; its focused test file
  exactly matched integrated `d1ffa0904`; Git worktree removal succeeded.
- Lifecycle: `/root/w_test_core_010` is one-shot and will not be reused. The
  collaboration runtime has no hard-delete operation, so only immutable task
  history remains.
- Next: record the fresh implementation request before dispatch.

### E-119 - Transactional implementation request prepared

- Actor: manager under TDD, codebase-design and P1 `R-SOLVE-001`.
- Request: `W-CORE-007`, Sol/ultra, no subagents, numerical header/source only.
- Scope: preallocated pending/candidate EWLS statistics; boundary accept commits
  `G/H/W`, rejection ages committed `G/H`, discards the epoch and permits a
  later same-block recovery. No other P2 or plugin work.
- Next: commit/push this request, create a detached worktree from current HEAD,
  and dispatch the fresh worker.

### E-120 - Transactional implementation worker active

- Actor: manager and `/root/w_core_007`.
- Worktree/model: detached `mne-cpp-worker-w-core-007` at `5eff9abb9`,
  Sol/ultra, no subagents.
- Scope: numerical header/source only; pending/candidate epoch transaction and
  same-block recovery, with later P2 findings explicitly excluded.
- Next: await the structured commit response; manager will not edit overlapping
  numerical files while the worker is active.

### E-121 - Transactional RED issue update prepared

- Actor: manager under the GitHub milestone workflow.
- Planned issue #2 evidence: integrated test `d1ffa0904`, focused Release build
  success, synchronized 38-pass/one-intended-failure report, and active bounded
  Sol/ultra fix scope under `R-SOLVE-001`.
- Next: commit/push this record, publish the concise issue comment, then record
  its returned URL without changing worker scope.

### E-122 - Transactional RED published to issue #2

- Actor: manager through the authenticated GitHub workflow.
- Result: issue comment
  `https://github.com/Edddddddddy/mne-cpp/issues/2#issuecomment-5189966843`
  records test commit, 38/1 evidence, failure semantics and the bounded P1 fix.
- Next: continue waiting for `W-CORE-007`; no duplicate worker or scope change.

### E-123 - Transactional worker status requested

- Actor: manager.
- Evidence: bounded waits show `/root/w_core_007` running; detached worktree is
  still clean at `5eff9abb9` with no overlapping or out-of-scope edit.
- Action: prepare a non-blocking progress/blocker request without changing the
  implementation contract or dispatching a duplicate.
- Next: commit/push, forward the status request, then continue bounded waiting.

### E-124 - Transactional worker reports source patch complete

- Actor: `/root/w_core_007` and manager.
- Result: no blocker; committed/pending/candidate `G/H` storage and elapsed
  epoch decay implement the selected composition and rejection transaction.
- Scope: source only, no public-interface change, no subagents; final static
  review and available validation remain active.
- Decision: record before inspecting the isolated diff; continue the same
  worker and do not duplicate implementation.

### E-125 - Transactional worker asked to conclude

- Actor: manager.
- Read-only evidence: exact one-source-file patch, clean diff check and selected
  three-layer statistics transaction; no public/test/build scope expansion.
- Action: prepare a finish request because detached build infrastructure is the
  already documented non-actionable limitation.
- Next: commit/push, forward, and await the clean implementation commit.

### E-126 - Transactional implementation response received

- Actor: `/root/w_core_007` and manager.
- Response: clean one-source-file commit `0e5519417` implementing the selected
  committed/pending/candidate transaction with no public-interface change.
- Evidence: clean diff/HEAD, scalar recovered-model oracle, known detached
  Eigen runtime limitation, and no subagents.
- Decision: record before acting; next perform exact commit review, integrate
  only if accepted, and require all 39 focused checks GREEN.

### E-127 - Transactional implementation commit accepted

- Actor: manager using codebase-design/TDD and P1 review criteria.
- Review: exact one-source-file scope; mathematically correct pending recurrence
  and candidate composition; success commits `G/H/W` only after checks;
  rejection ages committed stats and cannot leak poisoned candidate state.
- Real-time/state check: new matrices allocate only during configure; reset and
  paused-mode behavior include the new pending state; no explicit hot-path
  allocation/lock/string is introduced.
- Decision: accept `0e5519417` for cherry-pick and focused build/runtime; later
  P2 strict-pivot/application-overflow findings remain open.
