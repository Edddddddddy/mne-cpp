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
