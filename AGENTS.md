# AGENTS.md  
*(Designed for **ChatGPT o3 Codex** — nine parallel, role-focused instances of the **same model**)*

This swarm will assemble a full **4-player co-op + death-match** mod for *Cyberpunk 2077* on top of a clean RED4ext SDK.  
Tickets in `PHASE_PLAN.md` are sized so **one agent can finish each in ? 20 minutes** (wall-clock).  
All code is delivered through static reasoning; no live compilation or runtime tests are performed.

---

## 1?Agent Roster

| ID | Agent Name    | Back-End Engine | Domain / Responsibilities |
|----|---------------|-----------------|---------------------------|
| **01** | **Conductor**     | ChatGPT o3 | Pulls next ticket, picks a specialist, forwards work, merges or escalates. |
| **02** | **PromptSmith**   | ChatGPT o3 | Rewrites raw tickets into explicit, self-contained specs with paths, type hints, and edge-case notes. |
| **03** | **NetCore**       | ChatGPT o3 | Networking: ENet wrapper, handshake, tick-lock, RNG sync, delta-snapshot protocol. |
| **04** | **Gameplay**      | ChatGPT o3 | RED4ext hooks: quest/trigger sync, inventory ownership, damage validation, simple DM rules. |
| **05** | **UI/UX**         | ChatGPT o3 | Menus, server browser, chat overlay, health/armor bars, settings panel. |
| **06** | **PhysicsSync**   | ChatGPT o3 | Prediction & reconciliation, lag compensation, rollback, deterministic vehicle physics. |
| **07** | **Painter**       | ChatGPT o3 | Generates placeholder UI/texture assets (SVG/ASCII stubs) from text prompts. |
| **08** | **ServerDaemon**  | ChatGPT o3 | Dedicated-server scripts, master-server heartbeat JSON, session pruning. |
| **09** | **QA-Gremlin**    | ChatGPT o3 | Static lint, diff validation, file-touch hygiene, schema checks; blocks bad patches. |

*All nine share identical weights; the “persona” is enforced via unique system prompts.*

---

## 2?Time Budget

* **Hard cap:** 20 minutes per ticket.  
* **Suggested split:**  
  * 3 min?— digest ticket & outline plan  
  * 12 min — code / edit  
  * 3 min?— static self-review  
  * 2 min?— assemble reply

Agents must reply before 20 min; target 18–19 min to avoid cutoff.

---

## 3?Message Schema

### 3.1?Incoming ticket object

```yaml
ticket_id: "P4-1"
summary: "Quest stage broadcast"
context_files:
  - path: src/runtime/QuestSync.reds
    excerpt: |
      // ...
spec: >
  Hook QuestSystem::AdvanceStage; on server send QuestStageMsg, on client apply.
hints:
  - Use QuestSystem.GetInstance(this.GetGame()).```

### 3.2?Required reply

```yaml
ticket_id: "P4-1"
response_status: "DONE"          # or "FAILED"
patch: |
  --- a/src/runtime/QuestSync.reds
  +++ b/src/runtime/QuestSync.reds
  @@
   func OnAdvanceStage(questName: CName) -> Void {
  +    if Net_IsAuthoritative() {
  +        Net_SendQuestStageMsg(questName);
  +    }
        ...
  log:
    elapsed_seconds: 957
    static_checks: "lint-ok"
    notes: "Null guard for invalid quest names."```

---

## 4?Static Self-Checks (QA-Gremlin)

1. Valid diff format — proper ---/+++ headers & @@ hunks.

2. Syntax sanity — braces/quotes balanced, obvious Redscript/C++ typos flagged.

3. File hygiene — may only edit files listed in context_files (plus brand-new files under allowed dirs).

4. No unchecked TODOs — convert to // FIXME(next ticket) if work is deferred.

5. LF endings only.

If any fails, QA-Gremlin sets response_status: "FAILED" and static_checks: "static-fail:<reason>".

---

## 5?Execution Micro-Checklist (every agent)

1. Restate the goal internally.

2. Locate edit anchors via context_files excerpts.

3. Draft coherent code — avoid piecemeal scatter.

4. Mental lint pass (syntax, style).

5. Update docstrings / header comments.

6. Produce one unified diff block per file changed.

7. Fill reply YAML (including elapsed_seconds).
 
---

## 6 Failure & Retry Logic

| Event               | Automatic Handling                                                              |
| ------------------- | ------------------------------------------------------------------------------- |
| Static check fails  | QA-Gremlin returns `"FAILED"`; Conductor re-queues ticket with failure notes.   |
| Agent cannot finish | Agent replies `"FAILED"` early; Conductor retries or escalates.                 |
| Timeout imminent    | Agent sends partial patch with `// FIXME(timeout)` markers rather than nothing. |

---

## 7?Output Quality Tips

* Favour incremental edits over wholesale rewrites.

* Use concise, precise comments — one-liners beat essay blocks.

* Predict downstream needs — add stubs/hooks where the next ticket clearly depends on them.

* Keep each patch ? 400 lines to stay human-reviewable.

---

## 8 Glossary

| Term          | Definition                                                             |
| ------------- | ---------------------------------------------------------------------- |
| **Snapshot**  | Delta-compressed world diff, sequenced by tick.                        |
| **Dirty-bit** | Field flag set when a value changes since the last snapshot.           |
| **Rollback**  | Client rewinds *N* ms and replays inputs for authoritative validation. |

---

End of AGENTS.md
 