# Unit tests for QuestSync helper logic

from typing import Dict, List

# Minimal Python mirror of ApplyQuestStageByHash

def apply_stage_by_hash(name_map: Dict[int, str], stage_map: Dict[int, int], hash_val: int, stage: int, log: List[str]) -> None:
    if hash_val not in name_map:
        log.append(f"warn unknown quest hash {hash_val}")
        return
    stage_map[hash_val] = stage

def test_invalid_quest_hash():
    logs: List[str] = []
    names: Dict[int, str] = {}
    stages: Dict[int, int] = {}
    apply_stage_by_hash(names, stages, 0xDEADBEEF, 3, logs)
    assert logs and "unknown quest hash" in logs[0]
    assert 0xDEADBEEF not in stages
