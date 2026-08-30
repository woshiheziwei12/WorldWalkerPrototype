#!/usr/bin/env python3
"""Recompute the complete W11 combat-plan gate from sealed project evidence."""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import re
import struct
import sys
import tempfile
from pathlib import Path

sys.dont_write_bytecode = True

from verify_w11_steam_acceptance_kit import verify as verify_kit_manifest
from verify_w11_steam_package_manifest import verify as verify_package_manifest


class CompletionAuditError(RuntimeError):
    pass


REQUIRED_AUTOMATION_TESTS = {
    "WorldWalker.W11.Combat.AbilityRejection",
    "WorldWalker.W11.Combat.BasicAttackDefinition",
    "WorldWalker.W11.Combat.ChapterTransitionOwnership",
    "WorldWalker.W11.Combat.CombatCueDeduplication",
    "WorldWalker.W11.Combat.ContentContracts",
    "WorldWalker.W11.Combat.CooldownScaling",
    "WorldWalker.W11.Combat.DamageBreakdown",
    "WorldWalker.W11.Combat.DeathOnce",
    "WorldWalker.W11.Combat.DodgeImmunity",
    "WorldWalker.W11.Combat.EncounterBudgetRefill",
    "WorldWalker.W11.Combat.EnemyTelegraphLifecycle",
    "WorldWalker.W11.Combat.EnemyTelegraphRecovery",
    "WorldWalker.W11.Combat.HudRuntimeNotificationContracts",
    "WorldWalker.W11.Combat.ImpactPresentationTiers",
    "WorldWalker.W11.Combat.ManualEvidenceProgress",
    "WorldWalker.W11.Combat.ManualEvidenceSessionContract",
    "WorldWalker.W11.Combat.MemoryRunLifecycle",
    "WorldWalker.W11.Combat.NodeParticipantSets",
    "WorldWalker.W11.Combat.NonDamageEffects",
    "WorldWalker.W11.Combat.ProjectileSingleHit",
    "WorldWalker.W11.Combat.RecoveryPunishTelemetry",
    "WorldWalker.W11.Combat.ServerRequestValidation",
    "WorldWalker.W11.Combat.StableMultiTargetOrder",
    "WorldWalker.W11.Combat.StageCleanup",
    "WorldWalker.W11.Combat.StaleEncounterCallbacks",
    "WorldWalker.W11.Combat.TerminalPrecedence",
    "WorldWalker.W11.Combat.VictoryOnce",
    "WorldWalker.W11.Online.SteamEvidenceContracts",
    "WorldWalker.W11.Presentation.CharacterAnimationFallbackContracts",
    "WorldWalker.W11.Presentation.HudIncomingDirection",
    "WorldWalker.W11.Unit.DefinitionIdentity",
    "WorldWalker.W11.Unit.DeterministicNameHash",
    "WorldWalker.W11.Unit.StatRules",
}

SCENARIO_GATES = {
    "BasicJoin": "BASIC_JOIN_PASSED",
    "FriendInvite": "FRIEND_INVITE_JOIN_PASSED",
    "HostExit": "HOST_EXIT_OBSERVED",
    "ClientDisconnect": "CLIENT_DISCONNECT_OBSERVED",
    "PlayerSync": "PLAYER_SYNC_PASSED",
}

VISUAL_RESOLUTIONS = {
    "3840x2160": (3840, 2160),
    "2560x1440": (2560, 1440),
    "1920x1080": (1920, 1080),
    "1280x720": (1280, 720),
}

SUBJECTIVE_CHECK_IDS = {
    "enemyReadability",
    "projectileEvasion",
    "dodgeResponse",
    "recoveryWindow",
    "feedbackHierarchy",
    "impactComfort",
    "headphoneMix",
    "hudResolutionCoverage",
    "combatPacing",
}

NETWORK_GATES = ("natTraversal", "reconnect", "hostExitRecovery")
SHA256_PATTERN = re.compile(r"[0-9a-f]{64}")
APP_ID_PATTERN = re.compile(r"[1-9][0-9]+")
STEAM_ID_PATTERN = re.compile(r"[1-9][0-9]{14,19}")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_json(path: Path, label: str) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8-sig"))
    except (OSError, json.JSONDecodeError) as exc:
        raise CompletionAuditError(f"cannot read {label} {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise CompletionAuditError(f"{label} must contain a JSON object: {path}")
    return value


def require_sha(value, label: str) -> str:
    text = str(value or "").lower()
    if not SHA256_PATTERN.fullmatch(text):
        raise CompletionAuditError(f"{label} must be a lowercase SHA-256")
    return text


def resolve_artifact(base: Path, value, label: str) -> Path:
    text = str(value or "").strip()
    if not text:
        raise CompletionAuditError(f"{label} path is empty")
    path = Path(text)
    if not path.is_absolute():
        path = base / path
    path = path.resolve()
    if not path.is_file():
        raise CompletionAuditError(f"{label} does not exist: {path}")
    return path


def path_within(path: Path, root: Path) -> bool:
    try:
        path.resolve().relative_to(root.resolve())
        return True
    except ValueError:
        return False


def validate_artifact_record(base: Path, record, label: str) -> Path:
    if not isinstance(record, dict):
        raise CompletionAuditError(f"{label} must be an artifact object")
    path = resolve_artifact(base, record.get("path"), label)
    expected = require_sha(record.get("sha256"), f"{label}.sha256")
    actual = sha256(path)
    if actual != expected:
        raise CompletionAuditError(
            f"{label} SHA-256 mismatch: expected {expected}, found {actual}"
        )
    return path


def png_dimensions(path: Path) -> tuple[int, int]:
    data = path.read_bytes()[:24]
    if len(data) < 24 or data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        raise CompletionAuditError(f"visual evidence is not a PNG with IHDR: {path}")
    return struct.unpack(">II", data[16:24])


def gate(status: str, summary: str, evidence=None, blockers=None) -> dict:
    return {
        "status": status,
        "summary": summary,
        "evidence": evidence or {},
        "blockers": blockers or [],
    }


def audit_engineering(automation_index: Path, package_manifest: Path, kit_manifest: Path) -> dict:
    automation = load_json(automation_index, "automation index")
    tests = automation.get("tests")
    if not isinstance(tests, list):
        raise CompletionAuditError("automation index has no test list")
    paths = [item.get("fullTestPath") for item in tests if isinstance(item, dict)]
    path_set = set(paths)
    if len(paths) != len(path_set):
        raise CompletionAuditError("automation index contains duplicate test paths")
    missing = sorted(REQUIRED_AUTOMATION_TESTS - path_set)
    unexpected = sorted(path_set - REQUIRED_AUTOMATION_TESTS)
    if missing or unexpected:
        raise CompletionAuditError(
            f"automation test inventory mismatch: missing={missing} unexpected={unexpected}"
        )
    if (
        automation.get("succeeded") != len(REQUIRED_AUTOMATION_TESTS)
        or automation.get("succeededWithWarnings") != 0
        or automation.get("failed") != 0
        or automation.get("notRun") != 0
        or automation.get("inProcess") != 0
        or any(item.get("state") != "Success" for item in tests)
        or any(item.get("warnings") != 0 or item.get("errors") != 0 for item in tests)
    ):
        raise CompletionAuditError("automation index is not an exact 33/33 clean pass")

    package_manifest = package_manifest.resolve()
    kit_manifest = kit_manifest.resolve()
    artifacts, containers = verify_package_manifest(
        package_manifest, require_full_inventory=True, require_process_target=True
    )
    tools, nested_containers = verify_kit_manifest(kit_manifest)
    kit_data = load_json(kit_manifest, "portable kit manifest")
    nested_package = resolve_artifact(
        kit_manifest.parent, kit_data.get("packageManifest"), "nested package manifest"
    )
    package_hash = sha256(package_manifest)
    nested_package_hash = sha256(nested_package)
    if package_hash != nested_package_hash:
        raise CompletionAuditError("source and portable package manifests are not identical")
    if require_sha(kit_data.get("packageManifestSha256"), "kit package identity") != package_hash:
        raise CompletionAuditError("portable kit does not seal the selected package manifest")
    return gate(
        "PASSED",
        "33/33 automation and the current schema 4 package/schema 3 kit are intact",
        {
            "automationIndex": str(automation_index.resolve()),
            "automationTests": len(paths),
            "packageManifest": str(package_manifest),
            "packageManifestSha256": package_hash,
            "packageArtifacts": artifacts,
            "contentContainers": containers,
            "kitManifest": str(kit_manifest),
            "kitManifestSha256": sha256(kit_manifest),
            "kitArtifacts": tools,
            "nestedContentContainers": nested_containers,
        },
    )


def audit_manual(signoff_evidence: Path | None, package_hash: str) -> dict:
    if signoff_evidence is None:
        return gate(
            "MISSING",
            "five-role human completion and subjective sign-off are absent",
            blockers=[
                "Run the five interactive Cooked sessions",
                "Capture all four real-resolution PNGs",
                "Finalize a PASS sign-off evidence JSON",
            ],
        )
    evidence_path = signoff_evidence.resolve()
    evidence = load_json(evidence_path, "manual sign-off evidence")
    if evidence.get("schema") != 2:
        raise CompletionAuditError("manual sign-off evidence must use schema 2")
    for key in ("objectiveGate", "subjectiveStatus", "visualEvidenceStatus"):
        expected = "PASSED" if key == "objectiveGate" else "PASS"
        if evidence.get(key) != expected:
            raise CompletionAuditError(f"manual sign-off {key} is not passed")
    runtime = evidence.get("runtime")
    if not isinstance(runtime, dict):
        raise CompletionAuditError("manual sign-off runtime provenance is missing")
    expected_runtime = {
        "mode": "Cooked",
        "runtime": "CookedGame",
        "processTarget": "RuntimeExecutable",
        "packageHash": package_hash,
    }
    for key, value in expected_runtime.items():
        if runtime.get(key) != value:
            raise CompletionAuditError(
                f"manual sign-off runtime {key}={runtime.get(key)!r}, expected {value!r}"
            )
    package_manifest = resolve_artifact(
        evidence_path.parent, runtime.get("packageManifest"), "manual package manifest"
    )
    if sha256(package_manifest) != package_hash:
        raise CompletionAuditError("manual sign-off package identity no longer matches")
    verify_package_manifest(
        package_manifest, require_full_inventory=True, require_process_target=True
    )
    packet_path = validate_artifact_record(
        evidence_path.parent, evidence.get("packet"), "manual packet"
    )
    signoff_path = validate_artifact_record(
        evidence_path.parent, evidence.get("signoff"), "manual signoff"
    )
    if "客观日志门禁：**PASSED**" not in packet_path.read_text(encoding="utf-8-sig"):
        raise CompletionAuditError("manual objective packet does not declare a passed log gate")
    signoff_source = load_json(signoff_path, "manual signoff source")
    for field in ("operator", "date", "signature"):
        if not str(evidence.get(field) or "").strip() or evidence.get(field) != signoff_source.get(field):
            raise CompletionAuditError(f"manual sign-off {field} is empty or differs from its source")
    environment = evidence.get("environment")
    source_environment = signoff_source.get("environment")
    required_environment = {"inputDevice", "display", "audioDevice", "performanceNotes"}
    if (
        not isinstance(environment, dict)
        or set(environment) != required_environment
        or environment != source_environment
        or any(not str(environment.get(field) or "").strip() for field in required_environment)
    ):
        raise CompletionAuditError("manual sign-off environment is incomplete or differs from its source")
    logs = evidence.get("logs")
    expected_roles = {"Observe", "Evasion", "Dodge", "Recovery", "Feedback"}
    if not isinstance(logs, dict) or set(logs) != expected_roles:
        raise CompletionAuditError("manual sign-off must seal exactly the five evidence roles")
    log_hashes = []
    for role in sorted(expected_roles):
        path = validate_artifact_record(evidence_path.parent, logs[role], f"manual log {role}")
        log_hashes.append(sha256(path))
    if len(set(log_hashes)) != len(log_hashes):
        raise CompletionAuditError("manual sign-off reuses objective log content")
    visuals = evidence.get("visualEvidence")
    if not isinstance(visuals, dict) or set(visuals) != set(VISUAL_RESOLUTIONS):
        raise CompletionAuditError("manual sign-off must seal all four visual resolutions")
    visual_hashes = []
    source_visuals = signoff_source.get("visualEvidence")
    if not isinstance(source_visuals, dict) or set(source_visuals) != set(VISUAL_RESOLUTIONS):
        raise CompletionAuditError("manual signoff source has an invalid visual checklist")
    for resolution, dimensions in VISUAL_RESOLUTIONS.items():
        record = visuals[resolution]
        source_record = source_visuals[resolution]
        if not isinstance(record, dict) or not isinstance(source_record, dict):
            raise CompletionAuditError(f"visual evidence {resolution} is not an object")
        path = validate_artifact_record(
            evidence_path.parent,
            {"path": record.get("artifact"), "sha256": record.get("sha256")},
            f"visual evidence {resolution}",
        )
        if record.get("result") != "PASS":
            raise CompletionAuditError(f"visual evidence {resolution} is not PASS")
        if (
            source_record.get("result") != "PASS"
            or record.get("result") != source_record.get("result")
            or record.get("notes") != source_record.get("notes")
            or record.get("artifact") != source_record.get("artifact")
            or not str(record.get("notes") or "").strip()
        ):
            raise CompletionAuditError(f"visual evidence {resolution} differs from its signoff source")
        if png_dimensions(path) != dimensions:
            raise CompletionAuditError(f"visual evidence {resolution} has wrong dimensions")
        if record.get("width") != dimensions[0] or record.get("height") != dimensions[1]:
            raise CompletionAuditError(f"visual evidence {resolution} has inconsistent sealed dimensions")
        visual_hashes.append(sha256(path))
    if len(set(visual_hashes)) != len(visual_hashes):
        raise CompletionAuditError("manual sign-off reuses identical visual evidence files")
    checks = evidence.get("checks")
    source_checks = signoff_source.get("checks")
    if (
        not isinstance(checks, dict)
        or set(checks) != SUBJECTIVE_CHECK_IDS
        or not isinstance(source_checks, dict)
        or set(source_checks) != SUBJECTIVE_CHECK_IDS
    ):
        raise CompletionAuditError("manual sign-off must seal nine subjective checks")
    for check_id in SUBJECTIVE_CHECK_IDS:
        item = checks[check_id]
        source_item = source_checks[check_id]
        if (
            not isinstance(item, dict)
            or not isinstance(source_item, dict)
            or item.get("result") != "PASS"
            or source_item.get("result") != "PASS"
            or item.get("result") != source_item.get("result")
            or item.get("notes") != source_item.get("notes")
            or not str(item.get("notes") or "").strip()
        ):
            raise CompletionAuditError(f"manual subjective check {check_id} is invalid")
    return gate(
        "PASSED",
        "five objective roles, nine human checks, and four visual resolutions are sealed",
        {"signoffEvidence": str(evidence_path), "operator": evidence.get("operator")},
    )


def validate_role_receipt(
    receipt_path: Path,
    receipt_hash: str,
    log_record: dict,
    role: str,
    aggregate: dict,
    package_hash: str,
    kit_hash: str,
) -> None:
    if sha256(receipt_path) != receipt_hash:
        raise CompletionAuditError(f"{role} receipt hash no longer matches: {receipt_path}")
    receipt = load_json(receipt_path, f"{role} receipt")
    expected = {
        "schema": 2,
        "kind": "W11SteamRoleReceipt",
        "gate": "ROLE_LOG_AND_POST_EXIT_PACKAGE_PASSED",
        "role": role,
        "scenario": aggregate["scenario"],
        "expectedPlayers": aggregate["expectedPlayers"],
        "appId": aggregate["appId"],
        "token": aggregate["token"],
        "sessionId": aggregate["sessionId"],
        "runtime": "CookedGame",
        "localUserId": log_record["localUserId"],
        "packageManifestSha256": package_hash,
        "packageVerification": "FullInventory",
        "processTarget": "RuntimeExecutable",
        "postExitPackageVerified": True,
        "logSha256": log_record["sha256"],
        "kitManifestSha256": kit_hash,
        "kitVerification": "FullInventory",
        "launchContract": "RunnerOnlyUserDirOutsidePackage",
        "verificationContract": "PreLaunchAndPostExitFullInventory",
        "preLaunchKitVerified": True,
        "postExitKitVerified": True,
    }
    for key, value in expected.items():
        if receipt.get(key) != value:
            raise CompletionAuditError(
                f"{receipt_path.name} has {key}={receipt.get(key)!r}, expected {value!r}"
            )


def audit_steam_scenarios(
    evidence_paths: list[Path], package_hash: str, kit_hash: str
) -> tuple[dict, str | None]:
    if not evidence_paths:
        return gate(
            "MISSING",
            "real cooked Steam evidence for all five scenarios is absent",
            blockers=["Collect schema 5 aggregates for BasicJoin, FriendInvite, HostExit, ClientDisconnect, and PlayerSync"],
        ), None
    if len(evidence_paths) != len(SCENARIO_GATES):
        raise CompletionAuditError(
            f"exactly five Steam scenario aggregates are required, found {len(evidence_paths)}"
        )
    aggregates = {}
    app_ids = set()
    tokens = set()
    for path in evidence_paths:
        path = path.resolve()
        value = load_json(path, "Steam scenario aggregate")
        scenario = value.get("scenario")
        if scenario not in SCENARIO_GATES or scenario in aggregates:
            raise CompletionAuditError(f"invalid or duplicate Steam scenario: {scenario!r}")
        if value.get("schema") != 5 or value.get("gate") != SCENARIO_GATES[scenario]:
            raise CompletionAuditError(f"{scenario} is not a schema 5 passed aggregate")
        if value.get("runtime") != "CookedGame":
            raise CompletionAuditError(f"{scenario} did not use CookedGame")
        expected_players = value.get("expectedPlayers")
        if scenario == "PlayerSync":
            if expected_players not in (2, 3, 4):
                raise CompletionAuditError("PlayerSync must prove 2-4 participants")
        elif expected_players != 2:
            raise CompletionAuditError(f"{scenario} must prove exactly two participants")
        app_id = str(value.get("appId") or "")
        if not APP_ID_PATTERN.fullmatch(app_id) or app_id == "480":
            raise CompletionAuditError(f"{scenario} has an invalid project App ID")
        token = str(value.get("token") or "")
        if not re.fullmatch(r"[A-Za-z0-9_-]{8,64}", token):
            raise CompletionAuditError(f"{scenario} has an invalid evidence token")
        if value.get("packageManifestSha256") != package_hash:
            raise CompletionAuditError(f"{scenario} package identity mismatch")
        if value.get("kitManifestSha256") != kit_hash:
            raise CompletionAuditError(f"{scenario} portable kit identity mismatch")
        for key, expected in {
            "processTarget": "RuntimeExecutable",
            "kitVerification": "FullInventory",
            "launchContract": "RunnerOnlyUserDirOutsidePackage",
            "verificationContract": "PreLaunchAndPostExitFullInventory",
            "preLaunchKitVerified": True,
            "postExitKitVerified": True,
        }.items():
            if value.get(key) != expected:
                raise CompletionAuditError(f"{scenario} missing sealed contract {key}")
        host = value.get("host")
        clients = value.get("clients")
        receipts = value.get("roleReceipts")
        if not isinstance(host, dict) or not isinstance(clients, list) or not isinstance(receipts, dict):
            raise CompletionAuditError(f"{scenario} participant records are incomplete")
        if len(clients) != expected_players - 1:
            raise CompletionAuditError(f"{scenario} client count does not match expectedPlayers")
        participant_records = [host, *clients]
        participant_ids = [str(item.get("localUserId") or "") for item in participant_records]
        if any(not STEAM_ID_PATTERN.fullmatch(item) for item in participant_ids):
            raise CompletionAuditError(f"{scenario} has an invalid Steam64 participant identity")
        if len(set(participant_ids)) != len(participant_ids):
            raise CompletionAuditError(f"{scenario} reuses a Steam account")
        if value.get("participantLocalUserIds") != participant_ids:
            raise CompletionAuditError(f"{scenario} participant identity order is inconsistent")
        log_paths = []
        log_hashes = []
        for index, record in enumerate(participant_records):
            log_path = validate_artifact_record(path.parent, record, f"{scenario} participant log {index}")
            log_paths.append(log_path)
            log_hashes.append(sha256(log_path))
        if len(set(log_hashes)) != len(log_hashes):
            raise CompletionAuditError(f"{scenario} reuses participant log content")
        host_receipt_record = receipts.get("host")
        client_receipt_records = receipts.get("clients")
        if not isinstance(host_receipt_record, dict) or not isinstance(client_receipt_records, list):
            raise CompletionAuditError(f"{scenario} role receipt records are incomplete")
        if len(client_receipt_records) != len(clients):
            raise CompletionAuditError(f"{scenario} client receipt count mismatch")
        receipt_pairs = [
            (host_receipt_record, host, "Host"),
            *[(record, client, "Client") for record, client in zip(client_receipt_records, clients)],
        ]
        receipt_paths = []
        for record, log_record, role in receipt_pairs:
            receipt_path = resolve_artifact(path.parent, record.get("path"), f"{scenario} {role} receipt")
            receipt_hash = require_sha(record.get("sha256"), f"{scenario} {role} receipt hash")
            validate_role_receipt(
                receipt_path, receipt_hash, log_record, role, value, package_hash, kit_hash
            )
            receipt_paths.append(receipt_path)
        if len(set(receipt_paths)) != len(receipt_paths):
            raise CompletionAuditError(f"{scenario} reuses a role receipt file")
        aggregates[scenario] = str(path)
        app_ids.add(app_id)
        tokens.add(token)
    if set(aggregates) != set(SCENARIO_GATES):
        raise CompletionAuditError("Steam scenario aggregate set is incomplete")
    if len(app_ids) != 1:
        raise CompletionAuditError("Steam scenarios do not use one project App ID")
    if len(tokens) != len(SCENARIO_GATES):
        raise CompletionAuditError("Steam scenarios must use distinct evidence tokens")
    app_id = next(iter(app_ids))
    return gate(
        "PASSED",
        "all five cooked Steam scenarios have sealed schema 5 evidence",
        {"appId": app_id, "scenarios": aggregates},
    ), app_id


def audit_network(
    evidence_path: Path | None, app_id: str | None, package_hash: str, kit_hash: str
) -> dict:
    if evidence_path is None:
        return gate(
            "MISSING",
            "cross-NAT, reconnect, and host-exit recovery evidence is absent",
            blockers=["Freeze the host-exit recovery policy and collect direct cross-network evidence"],
        )
    if app_id is None:
        raise CompletionAuditError("network resilience evidence requires passed Steam scenarios")
    evidence_path = evidence_path.resolve()
    value = load_json(evidence_path, "Steam network evidence")
    expected = {
        "schema": 1,
        "kind": "W11SteamNetworkEvidence",
        "appId": app_id,
        "packageManifestSha256": package_hash,
        "kitManifestSha256": kit_hash,
        "policyFrozen": True,
    }
    for key, required in expected.items():
        if value.get(key) != required:
            raise CompletionAuditError(
                f"Steam network evidence has {key}={value.get(key)!r}, expected {required!r}"
            )
    if not str(value.get("hostExitRecoveryStrategy") or "").strip():
        raise CompletionAuditError("Steam network evidence does not name the frozen host-exit strategy")
    gates = value.get("gates")
    if not isinstance(gates, dict) or set(gates) != set(NETWORK_GATES):
        raise CompletionAuditError("Steam network evidence gate inventory is incomplete")
    for name in NETWORK_GATES:
        item = gates[name]
        if not isinstance(item, dict) or item.get("status") != "PASSED":
            raise CompletionAuditError(f"Steam network gate {name} is not PASSED")
        artifacts = item.get("artifacts")
        if not isinstance(artifacts, list) or not artifacts:
            raise CompletionAuditError(f"Steam network gate {name} has no direct artifacts")
        for index, record in enumerate(artifacts):
            validate_artifact_record(evidence_path.parent, record, f"network {name} artifact {index}")
    return gate(
        "PASSED",
        "cross-NAT, reconnect, and frozen host-exit recovery evidence are sealed",
        {"networkEvidence": str(evidence_path)},
    )


def audit_chapters(evidence_path: Path | None, project_root: Path) -> dict:
    if evidence_path is None:
        return gate(
            "MISSING",
            "five-chapter content freeze and formal route integration are absent",
            blockers=["Freeze route counts/content, build five chapter maps, bosses, market nodes, and safe-node resume"],
        )
    evidence_path = evidence_path.resolve()
    value = load_json(evidence_path, "chapter routing evidence")
    evidence_kind = value.get("kind")
    content_frozen = value.get("contentFrozen")
    if evidence_kind not in {"W11ChapterRoutingEvidence", "W11ChapterRoutingCandidateEvidence"}:
        raise CompletionAuditError(f"chapter evidence has unsupported kind={evidence_kind!r}")
    if evidence_kind == "W11ChapterRoutingEvidence" and content_frozen is not True:
        raise CompletionAuditError("formal chapter evidence must set contentFrozen=true")
    if evidence_kind == "W11ChapterRoutingCandidateEvidence" and content_frozen is not False:
        raise CompletionAuditError("candidate chapter evidence must set contentFrozen=false")
    expected = {
        "schema": 1,
        "chapterCount": 5,
        "transitionOwnershipConsumed": True,
        "immortalMarketRouted": True,
        "safeNodeResume": True,
    }
    for key, required in expected.items():
        if value.get(key) != required:
            raise CompletionAuditError(
                f"chapter evidence has {key}={value.get(key)!r}, expected {required!r}"
            )
    chapter_ids = value.get("chapterIds")
    bosses = value.get("smallBossIds")
    maps = value.get("chapterMaps")
    if not isinstance(chapter_ids, list) or len(chapter_ids) != 5 or len(set(chapter_ids)) != 5:
        raise CompletionAuditError("chapter evidence must name five unique chapter IDs")
    if not isinstance(bosses, list) or len(bosses) != 5 or len(set(bosses)) != 5:
        raise CompletionAuditError("chapter evidence must name five unique small bosses")
    if str(value.get("finalBossId") or "").strip().lower() not in {
        "guchangyuan", "gu_changyuan", "顾长渊"
    }:
        raise CompletionAuditError("chapter evidence must identify 顾长渊 as the final boss")
    if not isinstance(maps, list) or len(maps) != 5:
        raise CompletionAuditError("chapter evidence must seal five chapter maps")
    map_chapters = []
    map_paths = []
    root = project_root.resolve()
    for index, record in enumerate(maps):
        path = validate_artifact_record(root, record, f"chapter map {index}")
        try:
            path.relative_to(root / "Content" / "WorldWalker" / "Worlds" / "W11_RogueSurvival")
        except ValueError as exc:
            raise CompletionAuditError(f"chapter map is outside the W11 content root: {path}") from exc
        if path.suffix.lower() != ".umap":
            raise CompletionAuditError(f"chapter map is not a .umap: {path}")
        map_chapters.append(record.get("chapterId"))
        map_paths.append(path)
    if map_chapters != chapter_ids or len(set(map_paths)) != 5:
        raise CompletionAuditError("chapter map order/identity does not match chapterIds")
    if not content_frozen:
        if value.get("engineeringStatus") != "PASSED":
            raise CompletionAuditError("candidate chapter evidence must have engineeringStatus=PASSED")
        runtime = value.get("runtime") or {}
        required_runtime = {
            "enteredNodes": 26,
            "combatNodes": 11,
            "safeNodeWrites": 15,
            "bossPhaseEntries": 18,
            "snapshotDeletedAfterVictory": True,
        }
        for key, required in required_runtime.items():
            if runtime.get(key) != required:
                raise CompletionAuditError(
                    f"candidate chapter evidence runtime has {key}={runtime.get(key)!r}, expected {required!r}"
                )
        return gate(
            "MISSING",
            "five-chapter engineering candidate is verified; human content freeze/sign-off is absent",
            {
                "chapterCandidateEvidence": str(evidence_path),
                "chapterMaps": [str(item) for item in map_paths],
                "enteredNodes": runtime["enteredNodes"],
                "combatNodes": runtime["combatNodes"],
                "bossPhaseEntries": runtime["bossPhaseEntries"],
            },
            blockers=["Freeze final route counts/content and provide a non-empty human verifier name"],
        )
    if not str(value.get("verifiedBy") or "").strip():
        raise CompletionAuditError("chapter evidence requires a human verifier name")
    return gate(
        "PASSED",
        "five frozen chapters, maps, small bosses, final boss, market routing, and resume are sealed",
        {"chapterEvidence": str(evidence_path), "chapterMaps": [str(item) for item in map_paths]},
    )


def classify(gates: dict) -> tuple[str, int]:
    statuses = {item["status"] for item in gates.values()}
    if "FAILED" in statuses:
        return "FAILED", 1
    if statuses == {"PASSED"}:
        return "PASSED", 0
    return "INCOMPLETE", 2


def run_audit(args) -> tuple[dict, int]:
    project_root = args.project_root.resolve()
    gates = {}
    engineering = audit_engineering(
        args.automation_index, args.package_manifest, args.kit_manifest
    )
    gates["engineering"] = engineering
    package_hash = engineering["evidence"]["packageManifestSha256"]
    kit_hash = engineering["evidence"]["kitManifestSha256"]
    gates["manualAcceptance"] = audit_manual(args.manual_signoff, package_hash)
    steam_gate, app_id = audit_steam_scenarios(args.steam_evidence, package_hash, kit_hash)
    gates["steamScenarios"] = steam_gate
    gates["steamNetwork"] = audit_network(
        args.network_evidence, app_id, package_hash, kit_hash
    )
    gates["chapterIntegration"] = audit_chapters(args.chapter_evidence, project_root)
    status, exit_code = classify(gates)
    report = {
        "schema": 1,
        "kind": "W11PlanCompletionAudit",
        "auditedAt": dt.datetime.now(dt.timezone.utc).isoformat(),
        "status": status,
        "gateSummary": {
            "passed": sum(item["status"] == "PASSED" for item in gates.values()),
            "missing": sum(item["status"] == "MISSING" for item in gates.values()),
            "failed": sum(item["status"] == "FAILED" for item in gates.values()),
            "total": len(gates),
        },
        "gates": gates,
        "completionRule": "All five gates must be PASSED; missing external evidence never downgrades to success.",
    }
    return report, exit_code


def write_json(path: Path, value: dict, force: bool) -> None:
    path = path.resolve()
    if path.exists() and not force:
        raise CompletionAuditError(f"output already exists: {path}; use --force to replace it")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def validate_output_path(path: Path, package_manifest: Path, kit_manifest: Path) -> None:
    output = path.resolve()
    package_root = package_manifest.resolve().parent
    kit_root = kit_manifest.resolve().parent
    if path_within(output, package_root):
        raise CompletionAuditError("completion audit output must stay outside the sealed package")
    if path_within(output, kit_root) and not path_within(output, kit_root / "Saved"):
        raise CompletionAuditError(
            "completion audit output inside the portable kit must use its mutable Saved directory"
        )


def write_test_png(path: Path, width: int, height: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + struct.pack(">I", 13)
        + b"IHDR"
        + struct.pack(">II", width, height)
        + b"\x08\x06\x00\x00\x00"
        + b"\x00\x00\x00\x00"
    )


def self_test() -> None:
    # Reuse the package/kit verifiers' own tamper fixtures before testing this layer.
    from verify_w11_steam_acceptance_kit import self_test as kit_self_test
    from verify_w11_steam_package_manifest import self_test as package_self_test

    package_self_test()
    kit_self_test()
    with tempfile.TemporaryDirectory(prefix="w11_completion_") as temporary:
        root = Path(temporary)
        automation = root / "index.json"
        tests = [
            {"fullTestPath": name, "state": "Success", "warnings": 0, "errors": 0}
            for name in sorted(REQUIRED_AUTOMATION_TESTS)
        ]
        automation.write_text(
            json.dumps({
                "succeeded": 33,
                "succeededWithWarnings": 0,
                "failed": 0,
                "notRun": 0,
                "inProcess": 0,
                "tests": tests,
            }),
            encoding="utf-8",
        )
        loaded = load_json(automation, "self-test automation")
        if len(loaded["tests"]) != 33 or set(item["fullTestPath"] for item in loaded["tests"]) != REQUIRED_AUTOMATION_TESTS:
            raise CompletionAuditError("automation inventory self-test failed")

        session = root / "Manual"
        session.mkdir()
        package_manifest = root / "package.json"
        package_manifest.write_text("package", encoding="utf-8")
        package_hash = sha256(package_manifest)
        artifacts = {}
        for name in ("packet", "signoff"):
            path = session / f"{name}.txt"
            path.write_text(name, encoding="utf-8")
            artifacts[name] = {"path": path.name, "sha256": sha256(path)}
        logs = {}
        for role in ("Observe", "Evasion", "Dodge", "Recovery", "Feedback"):
            path = session / f"{role}.log"
            path.write_text(role, encoding="utf-8")
            logs[role] = {"path": path.name, "sha256": sha256(path)}
        visuals = {}
        for resolution, dimensions in VISUAL_RESOLUTIONS.items():
            path = session / f"{resolution}.png"
            write_test_png(path, *dimensions)
            visuals[resolution] = {
                "artifact": path.name,
                "sha256": sha256(path),
                "result": "PASS",
                "notes": resolution,
                "width": dimensions[0],
                "height": dimensions[1],
            }
        check_ids = sorted(SUBJECTIVE_CHECK_IDS)
        source_signoff = {
            "operator": "Human",
            "date": "2026-08-30",
            "signature": "Human",
            "environment": {
                "inputDevice": "Keyboard",
                "display": "4K",
                "audioDevice": "Headphones",
                "performanceNotes": "Stable",
            },
            "checks": {
                check_id: {"result": "PASS", "notes": check_id}
                for check_id in check_ids
            },
            "visualEvidence": {
                resolution: {
                    "result": "PASS",
                    "notes": resolution,
                    "artifact": record["artifact"],
                }
                for resolution, record in visuals.items()
            },
        }
        signoff_source_path = session / "signoff.json"
        signoff_source_path.write_text(json.dumps(source_signoff), encoding="utf-8")
        artifacts["signoff"] = {
            "path": signoff_source_path.name,
            "sha256": sha256(signoff_source_path),
        }
        manual = {
            "schema": 2,
            "objectiveGate": "PASSED",
            "subjectiveStatus": "PASS",
            "visualEvidenceStatus": "PASS",
            "operator": "Human",
            "date": "2026-08-30",
            "signature": "Human",
            "environment": source_signoff["environment"],
            "runtime": {
                "mode": "Cooked",
                "runtime": "CookedGame",
                "processTarget": "RuntimeExecutable",
                "packageHash": package_hash,
                "packageManifest": str(package_manifest),
            },
            "packet": artifacts["packet"],
            "signoff": artifacts["signoff"],
            "logs": logs,
            "visualEvidence": visuals,
            "checks": {
                check_id: {"result": "PASS", "notes": check_id}
                for check_id in check_ids
            },
        }
        manual_path = session / "evidence.json"
        manual_path.write_text(json.dumps(manual), encoding="utf-8")
        # The final package verifier is intentionally not invoked for this synthetic manual fixture.
        # Exercise its tamper-sensitive components directly.
        for record in [*logs.values(), *artifacts.values()]:
            validate_artifact_record(session, record, "self-test artifact")
        for record in visuals.values():
            validate_artifact_record(
                session,
                {"path": record["artifact"], "sha256": record["sha256"]},
                "self-test visual artifact",
            )
        if any(png_dimensions(session / f"{res}.png") != dims for res, dims in VISUAL_RESOLUTIONS.items()):
            raise CompletionAuditError("visual dimension self-test failed")

        chapter_root = root / "Content" / "WorldWalker" / "Worlds" / "W11_RogueSurvival" / "Maps"
        chapter_maps = []
        chapter_ids = [f"Chapter{index}" for index in range(1, 6)]
        for chapter_id in chapter_ids:
            path = chapter_root / f"L_W11_{chapter_id}.umap"
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(chapter_id, encoding="utf-8")
            chapter_maps.append({
                "chapterId": chapter_id,
                "path": path.relative_to(root).as_posix(),
                "sha256": sha256(path),
            })
        chapter_evidence = root / "chapters.json"
        chapter_evidence.write_text(json.dumps({
            "schema": 1,
            "kind": "W11ChapterRoutingEvidence",
            "contentFrozen": True,
            "chapterCount": 5,
            "transitionOwnershipConsumed": True,
            "immortalMarketRouted": True,
            "safeNodeResume": True,
            "chapterIds": chapter_ids,
            "smallBossIds": [f"Boss{index}" for index in range(1, 6)],
            "finalBossId": "GuChangyuan",
            "chapterMaps": chapter_maps,
            "verifiedBy": "Human",
        }), encoding="utf-8")
        if audit_chapters(chapter_evidence, root)["status"] != "PASSED":
            raise CompletionAuditError("chapter evidence self-test failed")
        tampered = chapter_root / "L_W11_Chapter1.umap"
        tampered.write_text("tampered", encoding="utf-8")
        try:
            audit_chapters(chapter_evidence, root)
        except CompletionAuditError as exc:
            if "SHA-256 mismatch" not in str(exc):
                raise
        else:
            raise CompletionAuditError("chapter evidence self-test accepted a tampered map")

        missing_gates = {
            "engineering": gate("PASSED", "fixture"),
            "manualAcceptance": gate("MISSING", "fixture"),
        }
        if classify(missing_gates) != ("INCOMPLETE", 2):
            raise CompletionAuditError("incomplete classification self-test failed")
        if classify({"all": gate("PASSED", "fixture")}) != ("PASSED", 0):
            raise CompletionAuditError("passed classification self-test failed")
        package_root = root / "Package"
        kit_root = root / "Kit"
        package_root.mkdir()
        kit_root.mkdir()
        try:
            validate_output_path(
                package_root / "audit.json",
                package_root / "W11SteamPackageManifest.json",
                kit_root / "W11SteamAcceptanceKitManifest.json",
            )
        except CompletionAuditError as exc:
            if "sealed package" not in str(exc):
                raise
        else:
            raise CompletionAuditError("output boundary self-test accepted a package mutation")


def build_parser() -> argparse.ArgumentParser:
    script_root = Path(__file__).resolve().parent
    project_root = script_root.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project-root", type=Path, default=project_root)
    parser.add_argument(
        "--automation-index",
        type=Path,
        default=project_root / "Saved" / "Automation" / "W11CombatPlanFinal43" / "index.json",
    )
    parser.add_argument(
        "--package-manifest",
        type=Path,
        default=project_root / "Saved" / "SteamAcceptance" / "Packages" /
        "W11Steam_Final43_Development" / "W11SteamPackageManifest.json",
    )
    parser.add_argument(
        "--kit-manifest",
        type=Path,
        default=project_root / "Saved" / "SteamAcceptance" / "Kits" /
        "W11Steam_Final43_Portable_v8" / "W11SteamAcceptanceKitManifest.json",
    )
    parser.add_argument("--manual-signoff", type=Path)
    parser.add_argument("--steam-evidence", type=Path, action="append", default=[])
    parser.add_argument("--network-evidence", type=Path)
    parser.add_argument("--chapter-evidence", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    return parser


def main() -> int:
    args = build_parser().parse_args()
    try:
        if args.self_test:
            self_test()
            print(
                "W11_PLAN_COMPLETION_SELF_TEST_PASSED "
                "AutomationInventory=33 PackageKitTamper=1 ManualArtifacts=1 "
                "VisualDimensions=4 ChapterTamper=1 OutputBoundary=1 "
                "IncompleteNeverPasses=1"
            )
            return 0
        report, exit_code = run_audit(args)
        if args.output:
            validate_output_path(args.output, args.package_manifest, args.kit_manifest)
            write_json(args.output, report, args.force)
        summary = report["gateSummary"]
        print(
            f"W11_PLAN_COMPLETION_{report['status']} "
            f"Passed={summary['passed']} Missing={summary['missing']} "
            f"Failed={summary['failed']} Total={summary['total']} "
            f"Output={str(args.output.resolve()) if args.output else 'NONE'}"
        )
        return exit_code
    except Exception as exc:
        report = {
            "schema": 1,
            "kind": "W11PlanCompletionAudit",
            "auditedAt": dt.datetime.now(dt.timezone.utc).isoformat(),
            "status": "FAILED",
            "error": str(exc),
        }
        if getattr(args, "output", None):
            try:
                validate_output_path(args.output, args.package_manifest, args.kit_manifest)
                write_json(args.output, report, args.force)
            except Exception:
                pass
        print(f"W11_PLAN_COMPLETION_FAILED: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
