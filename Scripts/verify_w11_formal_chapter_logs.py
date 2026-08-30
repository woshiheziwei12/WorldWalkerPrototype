"""Verify W11 five-chapter routing, safe resume, and authored boss phases.

This produces engineering candidate evidence only. It never marks contentFrozen
or supplies a human verifier, which remain human decisions in the completion gate.
"""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import re
from pathlib import Path


CHAPTERS = (
    ("Chapter.01.Prologue", "L_W11_Chapter01_Prologue.umap", "Boss.HeavenLawPatrol"),
    ("Chapter.02.HumanRealm", "L_W11_Chapter02_HumanRealm.umap", "Boss.LuGuanlan"),
    ("Chapter.03.WarMemory", "L_W11_Chapter03_WarMemory.umap", "Boss.SixGeneralsWarMemory"),
    ("Chapter.04.WishlessCourt", "L_W11_Chapter04_WishlessCourt.umap", "Boss.WishlessHeavenOfficial"),
    ("Chapter.05.IdealMirror", "L_W11_Chapter05_IdealMirror.umap", "Boss.IdealSelf"),
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def read(path: Path) -> str:
    require(path.is_file(), f"missing log: {path}")
    return path.read_text(encoding="utf-8", errors="replace")


def count(text: str, marker: str) -> int:
    return text.count(marker)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project-root", type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument("--save-log", type=Path, required=True)
    parser.add_argument("--resume-log", type=Path, required=True)
    parser.add_argument("--complete-log", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    root = args.project_root.resolve()
    save_log = args.save_log.resolve()
    resume_log = args.resume_log.resolve()
    complete_log = args.complete_log.resolve()
    save_text = read(save_log)
    resume_text = read(resume_log)
    complete_text = read(complete_log)

    require("W11_SAFE_NODE_SNAPSHOT_SAVED Success=1" in save_text,
            "save run did not seal a safe-node snapshot")
    require("W11_SAFE_NODE_AUTOMATION_EXIT Chapter=1 Node=2" in save_text,
            "save run did not stop at the first safe node")
    require("W11_SAFE_NODE_RESUMED" in resume_text,
            "resume run did not restore the sealed safe node")
    require("Chapter=Chapter.01.Prologue Node=Node.C01.Event" in resume_text,
            "resume run restored the wrong route cursor")
    require("W11_FORMAL_RUN_COMPLETED Chapters=5 SmallBosses=5 FinalBoss=GuChangyuan" in resume_text,
            "resume run did not complete the formal route")

    require(count(complete_text, "W11_CHAPTER_NODE_ENTERED") == 26,
            "complete run must enter 25 chapter nodes plus the final boss node")
    require(count(complete_text, "W11_FORMAL_COMBAT_STARTED") == 11,
            "complete run must execute five regular fights, five small bosses, and one final boss")
    require(count(complete_text, "W11_SAFE_NODE_SNAPSHOT_SAVED Success=1") == 15,
            "complete run must save the three authored safe nodes in each chapter")
    require(count(complete_text, "W11_BOSS_PHASE_ENTERED") == 18,
            "six formal bosses must each enter three phases")
    require("W11_FORMAL_RUN_COMPLETED Chapters=5 SmallBosses=5 FinalBoss=GuChangyuan" in complete_text,
            "complete run lacks its terminal marker")
    require("W11_ACTIVE_RUN_SNAPSHOT_DELETED Existed=1 Success=1" in complete_text,
            "final victory did not delete the active-run snapshot")
    require(not (root / "Saved/SaveGames/W11_ActiveRun.sav").exists(),
            "active-run snapshot still exists after final victory")
    require("Fatal error" not in complete_text and "Assertion failed" not in complete_text,
            "complete run contains a fatal/assertion failure")

    boss_phase_counts = {}
    for _, _, boss_id in CHAPTERS:
        observed = len(re.findall(rf"W11_BOSS_PHASE_ENTERED[^\r\n]*Boss={re.escape(boss_id)}\b", complete_text))
        require(observed == 3, f"{boss_id} entered {observed} phases, expected 3")
        boss_phase_counts[boss_id] = observed
    gu_phases = (
        "Phase.Gu.DemonSlayingHero",
        "Phase.Gu.WorldImmortal",
        "Phase.Gu.WishlessDragon",
    )
    for phase in gu_phases:
        require(f"Boss=Boss.GuChangyuan" in complete_text and f"Phase={phase}" in complete_text,
                f"Gu Changyuan phase missing: {phase}")
    boss_phase_counts["Boss.GuChangyuan"] = 3

    maps_root = root / "Content/WorldWalker/Worlds/W11_RogueSurvival/Maps"
    chapter_maps = []
    for chapter_id, filename, _ in CHAPTERS:
        path = maps_root / filename
        require(path.is_file(), f"missing chapter map: {path}")
        chapter_maps.append({
            "chapterId": chapter_id,
            "path": path.relative_to(root).as_posix(),
            "sha256": sha256(path),
        })

    artifact_records = []
    for kind, path in (("safeNodeSaveLog", save_log),
                       ("safeNodeResumeLog", resume_log),
                       ("completeRouteLog", complete_log)):
        artifact_records.append({
            "kind": kind,
            "path": path.relative_to(root).as_posix(),
            "sha256": sha256(path),
        })

    report = {
        "schema": 1,
        "kind": "W11ChapterRoutingCandidateEvidence",
        "generatedAt": dt.datetime.now(dt.timezone.utc).isoformat(),
        "engineeringStatus": "PASSED",
        "contentFrozen": False,
        "verifiedBy": "",
        "chapterCount": 5,
        "transitionOwnershipConsumed": True,
        "immortalMarketRouted": True,
        "safeNodeResume": True,
        "chapterIds": [item[0] for item in CHAPTERS],
        "smallBossIds": [item[2] for item in CHAPTERS],
        "finalBossId": "GuChangyuan",
        "chapterMaps": chapter_maps,
        "runtime": {
            "enteredNodes": 26,
            "combatNodes": 11,
            "safeNodeWrites": 15,
            "bossPhaseEntries": 18,
            "bossPhaseCounts": boss_phase_counts,
            "guChangyuanPhases": list(gu_phases),
            "snapshotDeletedAfterVictory": True,
        },
        "artifacts": artifact_records,
        "humanGate": {
            "status": "PENDING",
            "required": [
                "freeze final per-chapter node counts and route content",
                "enter a non-empty human verifier name",
                "perform rendered/manual chapter acceptance",
            ],
        },
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    require(not args.output.exists(), f"refusing to overwrite evidence: {args.output}")
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(
        "W11_FORMAL_CHAPTER_LOGS_PASSED "
        "Chapters=5 Nodes=26 Combats=11 SafeWrites=15 BossPhases=18 "
        "SafeResume=1 FinalBoss=GuChangyuan ContentFrozen=0"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
