# Phase 13 - CleanUp

```yaml
ticket_id: "CLEANUP-ALL"
summary: "Eliminate remaining stubs, TODOs, and FIXME placeholders"
context_files: []
spec: >
  Perform a repository-wide sweep and promote every placeholder into a concrete implementation:

  1. **Discovery phase (auto):**
     • Recursively scan `/src`, `/runtime`, `/gui`, `/voice`, `/server` for lines matching:
         - `TODO(`, `// TODO`, `/* TODO`
         - `FIXME(`, `FIXME:`, `// FIXME`
         - Comments containing `"stub"`, `"placeholder"`, `"not implemented"`.
     • Build a map: `{file, lineNo, snippet}` for each hit.

  2. **Implementation phase (scope):**
     • For each hit, either:
        a. Replace the stub with actual functional code that follows project conventions,  
           *or*  
        b. If the task belongs to a future ticket already queued, convert the comment to  
           `// FIXME(<ticket_id>)` with a brief note (max 60 chars).
     • Prioritise runtime-critical areas first:
        - Network flows (packets, snapshots, interest logic)
        - Simulation loops (prediction, physics, AI)
        - UI handlers that still log instead of drawing
        - Audio / VOIP ring buffers
     • Ensure no compiler-visible placeholders remain (`return 0; /* TODO */` etc.).

  3. **Static validation (self-check):**
     • After edits, run internal syntax check for each modified file (balance braces, no unused vars).
     • Confirm `git grep -n "TODO\\|FIXME\\|placeholder\\|stub"` returns **zero** matches inside `/src`, `/runtime`, `/gui`, `/voice`, `/server` paths (allow them only in `/tests/`).

  4. **Patch output:**
     • Produce a single multi-file unified diff covering all replacements.
     • Summarise count of resolved stubs in `log:` → `resolved_count`.
     • If any placeholders remain by necessity, list them under `log:` → `deferred:` with their ticket IDs.

hints:
  - Keep individual functions ≤80 LOC; break helpers into static/private methods.
  - Preserve existing coding style (snake_case for C++, camelCase for REDscript).
  - Update header comments to remove “stub” wording once implemented.
```