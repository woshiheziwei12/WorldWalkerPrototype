import argparse
import datetime as dt
import hashlib
import json
import re
import struct
import sys
import tempfile
from pathlib import Path

from w11_manual_acceptance_schema import (
    FINAL_RESULTS,
    SIGNOFF_SCHEMA_VERSION,
    SUBJECTIVE_CHECKS,
    VISUAL_RESOLUTIONS,
    build_signoff_template,
)
from verify_w11_steam_package_manifest import (
    PackageManifestError,
    verify as verify_package_manifest,
)


class SignoffError(RuntimeError):
    pass


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_json(path, label):
    try:
        return json.loads(path.read_text(encoding="utf-8-sig"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise SignoffError(f"cannot read {label} {path}: {exc}") from exc


def resolve_session_member(session_root, value, label):
    candidate = Path(value)
    if not candidate.is_absolute():
        candidate = session_root / candidate
    candidate = candidate.resolve()
    try:
        candidate.relative_to(session_root)
    except ValueError as exc:
        raise SignoffError(f"{label} must stay inside the session directory: {candidate}") from exc
    return candidate


def require_text(value, label):
    if not isinstance(value, str) or not value.strip():
        raise SignoffError(f"{label} must be filled")
    return value.strip()


def read_png_dimensions(path):
    data = path.read_bytes()[:24]
    if len(data) < 24 or data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        raise SignoffError(f"visual evidence is not a PNG with an IHDR header: {path}")
    return struct.unpack(">II", data[16:24])


def validate_result(value, label):
    if value not in FINAL_RESULTS:
        raise SignoffError(f"{label} must be one of {', '.join(FINAL_RESULTS)}, found {value!r}")
    return value


def finalize_session(session_path, output_path=None, force=False):
    session_path = session_path.resolve()
    session_root = session_path.parent
    manifest = load_json(session_path, "session manifest")
    expected_roles = ("Observe", "Evasion", "Dodge", "Recovery", "Feedback")
    if manifest.get("schema") != 3:
        raise SignoffError(f"session schema must be 3, found {manifest.get('schema')!r}")
    if manifest.get("seed") != 424242:
        raise SignoffError("session seed must be 424242")
    if tuple(manifest.get("roles", ())) != expected_roles:
        raise SignoffError("session roles do not match the five required roles in collection order")
    role_status = manifest.get("roleStatus")
    if not isinstance(role_status, dict) or set(role_status) != set(expected_roles):
        raise SignoffError("session roleStatus does not contain the five required roles")
    if any(role_status[role] != "COMPLETED" for role in expected_roles):
        raise SignoffError("all five session roles must be COMPLETED before subjective sign-off")
    if set(manifest.get("visualEvidenceResolutions", ())) != set(VISUAL_RESOLUTIONS):
        raise SignoffError("session manifest does not declare the four required visual resolutions")
    if manifest.get("objectiveGate") != "PASSED":
        raise SignoffError("session objectiveGate must be PASSED before subjective sign-off")

    runtime_mode = manifest.get("runtimeMode")
    runtime_kind = manifest.get("runtime")
    package_hash = manifest.get("packageHash")
    package_manifest_text = manifest.get("packageManifest")
    if runtime_mode == "Editor":
        if runtime_kind != "EditorStandalone" or package_hash != "EDITOR":
            raise SignoffError("Editor session must use EditorStandalone and PackageHash=EDITOR")
        if manifest.get("processTarget") != "Editor":
            raise SignoffError("Editor session must declare ProcessTarget=Editor")
        if package_manifest_text:
            raise SignoffError("Editor session must not claim a cooked package manifest")
        sealed_runtime = {
            "mode": runtime_mode,
            "runtime": runtime_kind,
            "processTarget": "Editor",
            "packageHash": package_hash,
        }
    elif runtime_mode == "Cooked":
        if runtime_kind != "CookedGame" or not re.fullmatch(r"[0-9a-f]{64}", package_hash or ""):
            raise SignoffError("Cooked session must use CookedGame and a lowercase SHA-256 package hash")
        if manifest.get("processTarget") != "RuntimeExecutable":
            raise SignoffError("Cooked session must declare ProcessTarget=RuntimeExecutable")
        package_manifest = Path(require_text(package_manifest_text, "session packageManifest")).resolve()
        if not package_manifest.is_file():
            raise SignoffError(f"session package manifest does not exist: {package_manifest}")
        if sha256(package_manifest) != package_hash:
            raise SignoffError("session package manifest SHA-256 no longer matches packageHash")
        try:
            verify_package_manifest(
                package_manifest,
                require_full_inventory=True,
                require_process_target=True,
            )
        except PackageManifestError as exc:
            raise SignoffError(
                f"session package full-inventory validation failed: {exc}"
            ) from exc
        sealed_runtime = {
            "mode": runtime_mode,
            "runtime": runtime_kind,
            "processTarget": "RuntimeExecutable",
            "runtimeExecutable": load_json(
                package_manifest, "cooked package manifest"
            )["runtimeExecutable"],
            "packageHash": package_hash,
            "packageManifest": str(package_manifest),
        }
    else:
        raise SignoffError(f"session runtimeMode must be Editor or Cooked, found {runtime_mode!r}")

    packet_path = resolve_session_member(session_root, manifest.get("packet", ""), "objective packet")
    signoff_path = resolve_session_member(session_root, manifest.get("signoff", ""), "sign-off file")
    if not packet_path.is_file():
        raise SignoffError(f"objective packet does not exist: {packet_path}")
    if not signoff_path.is_file():
        raise SignoffError(f"sign-off file does not exist: {signoff_path}")

    signoff = load_json(signoff_path, "sign-off file")
    if signoff.get("schema") != SIGNOFF_SCHEMA_VERSION:
        raise SignoffError(
            f"sign-off schema must be {SIGNOFF_SCHEMA_VERSION}, found {signoff.get('schema')!r}"
        )
    if signoff.get("objectivePacket") != packet_path.name:
        raise SignoffError("sign-off objectivePacket does not match the session packet")

    operator = require_text(signoff.get("operator"), "operator")
    manifest_operator = require_text(manifest.get("operator"), "session operator")
    if operator != manifest_operator:
        raise SignoffError("sign-off operator does not match the session operator")
    date_text = require_text(signoff.get("date"), "date")
    try:
        dt.date.fromisoformat(date_text)
    except ValueError as exc:
        raise SignoffError("date must use ISO YYYY-MM-DD format") from exc

    environment = signoff.get("environment")
    if not isinstance(environment, dict):
        raise SignoffError("environment must be an object")
    expected_environment = ("inputDevice", "display", "audioDevice", "performanceNotes")
    if set(environment) != set(expected_environment):
        raise SignoffError("environment fields do not match the required schema")
    for field in expected_environment:
        require_text(environment.get(field), f"environment.{field}")

    checks = signoff.get("checks")
    expected_checks = tuple(check_id for check_id, _ in SUBJECTIVE_CHECKS)
    if not isinstance(checks, dict) or set(checks) != set(expected_checks):
        raise SignoffError("subjective checks do not match the nine required checks")
    adjustment_required = False
    sealed_checks = {}
    for check_id, prompt in SUBJECTIVE_CHECKS:
        check = checks.get(check_id)
        if not isinstance(check, dict):
            raise SignoffError(f"checks.{check_id} must be an object")
        if check.get("prompt") != prompt:
            raise SignoffError(f"checks.{check_id}.prompt does not match the controlled checklist")
        result = validate_result(check.get("result"), f"checks.{check_id}.result")
        notes = require_text(check.get("notes"), f"checks.{check_id}.notes")
        adjustment_required |= result == "NEEDS_ADJUSTMENT"
        sealed_checks[check_id] = {"result": result, "notes": notes}

    visual = signoff.get("visualEvidence")
    if not isinstance(visual, dict) or set(visual) != set(VISUAL_RESOLUTIONS):
        raise SignoffError("visualEvidence must contain exactly the four required resolutions")
    sealed_visual = {}
    visual_adjustment_required = False
    for resolution in VISUAL_RESOLUTIONS:
        entry = visual.get(resolution)
        if not isinstance(entry, dict):
            raise SignoffError(f"visualEvidence.{resolution} must be an object")
        result = validate_result(entry.get("result"), f"visualEvidence.{resolution}.result")
        notes = require_text(entry.get("notes"), f"visualEvidence.{resolution}.notes")
        artifact_text = require_text(entry.get("artifact"), f"visualEvidence.{resolution}.artifact")
        artifact = resolve_session_member(session_root, artifact_text, f"visualEvidence.{resolution}.artifact")
        if artifact.suffix.lower() != ".png" or not artifact.is_file():
            raise SignoffError(f"visual evidence must be an existing PNG: {artifact}")
        width, height = read_png_dimensions(artifact)
        expected_width, expected_height = (int(value) for value in resolution.split("x"))
        if (width, height) != (expected_width, expected_height):
            raise SignoffError(
                f"visual evidence {artifact.name} is {width}x{height}, expected {resolution}"
            )
        visual_adjustment_required |= result == "NEEDS_ADJUSTMENT"
        sealed_visual[resolution] = {
            "result": result,
            "notes": notes,
            "artifact": artifact.relative_to(session_root).as_posix(),
            "width": width,
            "height": height,
            "sha256": sha256(artifact),
        }
    adjustment_required |= visual_adjustment_required

    expected_overall = "NEEDS_ADJUSTMENT" if adjustment_required else "PASS"
    overall = validate_result(signoff.get("overallResult"), "overallResult")
    if overall != expected_overall:
        raise SignoffError(
            f"overallResult must be {expected_overall} based on the individual results, found {overall}"
        )
    signature = require_text(signoff.get("signature"), "signature")

    packet_text = packet_path.read_text(encoding="utf-8-sig")
    if "客观日志门禁：**PASSED**" not in packet_text:
        raise SignoffError("objective packet does not declare the objective log gate as PASSED")
    logs = manifest.get("logs")
    if not isinstance(logs, dict) or set(logs) != set(expected_roles):
        raise SignoffError("session manifest does not contain exactly the five objective logs")
    sealed_logs = {}
    for role, value in logs.items():
        log_path = resolve_session_member(session_root, value, f"logs.{role}")
        if not log_path.is_file():
            raise SignoffError(f"objective log does not exist: {log_path}")
        digest = sha256(log_path)
        if digest not in packet_text:
            raise SignoffError(f"objective packet does not contain the current SHA-256 for {log_path.name}")
        sealed_logs[role] = {
            "path": log_path.relative_to(session_root).as_posix(),
            "sha256": digest,
        }

    evidence = {
        "schema": 2,
        "finalizedAt": dt.datetime.now(dt.timezone.utc).isoformat(),
        "objectiveGate": "PASSED",
        "subjectiveStatus": overall,
        "visualEvidenceStatus": "NEEDS_ADJUSTMENT" if visual_adjustment_required else "PASS",
        "operator": operator,
        "date": date_text,
        "signature": signature,
        "runtime": sealed_runtime,
        "environment": environment,
        "checks": sealed_checks,
        "visualEvidence": sealed_visual,
        "packet": {
            "path": packet_path.relative_to(session_root).as_posix(),
            "sha256": sha256(packet_path),
        },
        "signoff": {
            "path": signoff_path.relative_to(session_root).as_posix(),
            "sha256": sha256(signoff_path),
        },
        "logs": sealed_logs,
    }

    if output_path is None:
        output_path = session_root / "W11人工单人验收签字证据.json"
    else:
        output_path = resolve_session_member(session_root, output_path, "sealed evidence output")
    if output_path.exists() and not force:
        raise SignoffError(f"sealed evidence already exists: {output_path}; use --force to replace it")
    output_path.write_text(json.dumps(evidence, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    manifest["subjectiveStatus"] = overall
    manifest["visualEvidenceStatus"] = evidence["visualEvidenceStatus"]
    manifest["signoffEvidence"] = str(output_path)
    manifest["finalizedAt"] = evidence["finalizedAt"]
    manifest["updatedAt"] = evidence["finalizedAt"]
    session_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return evidence, output_path


def write_test_png(path, width, height):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + struct.pack(">I", 13)
        + b"IHDR"
        + struct.pack(">II", width, height)
        + b"\x08\x06\x00\x00\x00"
        + b"\x00\x00\x00\x00"
    )


def self_test():
    with tempfile.TemporaryDirectory(prefix="w11_signoff_") as temp:
        root = Path(temp)
        logs = {}
        packet_lines = ["# Objective packet", "客观日志门禁：**PASSED**"]
        for role in ("Observe", "Evasion", "Dodge", "Recovery", "Feedback"):
            log = root / f"W11Manual{role}.log"
            log.write_text(f"synthetic {role}\n", encoding="utf-8")
            logs[role] = str(log)
            packet_lines.append(sha256(log))
        packet = root / "W11人工单人验收记录.md"
        packet.write_text("\n".join(packet_lines), encoding="utf-8")
        signoff_path = root / "W11人工单人验收签字.json"
        signoff = build_signoff_template("SelfTest", packet.name)
        signoff["date"] = "2026-08-30"
        signoff["environment"] = {
            "inputDevice": "Keyboard",
            "display": "Synthetic display",
            "audioDevice": "Synthetic headphones",
            "performanceNotes": "No visible stalls",
        }
        for entry in signoff["checks"].values():
            entry["result"] = "PASS"
            entry["notes"] = "Observed in the synthetic finalizer contract test."
        for resolution, entry in signoff["visualEvidence"].items():
            width, height = (int(value) for value in resolution.split("x"))
            write_test_png(root / entry["artifact"], width, height)
            entry["result"] = "PASS"
            entry["notes"] = "No clipping in the synthetic resolution contract test."
        signoff["overallResult"] = "PASS"
        signoff["signature"] = "SelfTest"
        signoff_path.write_text(json.dumps(signoff, ensure_ascii=False, indent=2), encoding="utf-8")
        package_root = root / "SyntheticCookedPackage"
        package_relative_paths = (
            "Windows/WorldWalkerPrototype.exe",
            "Windows/WorldWalkerPrototype/Binaries/Win64/WorldWalkerPrototype.exe",
            "Windows/WorldWalkerPrototype/Content/Paks/test.pak",
            "Windows/WorldWalkerPrototype/Content/Paks/test.utoc",
            "Windows/WorldWalkerPrototype/Content/Paks/test.ucas",
        )
        package_artifacts = []
        for index, relative in enumerate(package_relative_paths):
            artifact_path = package_root / Path(relative)
            artifact_path.parent.mkdir(parents=True, exist_ok=True)
            artifact_path.write_bytes(f"W11-signoff-package-{index}".encode("ascii"))
            package_artifacts.append(
                {
                    "path": relative,
                    "bytes": artifact_path.stat().st_size,
                    "sha256": sha256(artifact_path),
                }
            )
        package_manifest = package_root / "W11SteamPackageManifest.json"
        package_manifest.write_text(
            json.dumps(
                {
                    "schema": 4,
                    "runtime": "CookedGame",
                    "processTarget": "RuntimeExecutable",
                    "platform": "Win64",
                    "acceptanceSteamAppIdEmbedded": False,
                    "packagedDefaultPlatformService": "Null",
                    "packagedDefaultSteamDevAppId": "480",
                    "contentContainerCount": 3,
                    "fullInventorySealed": True,
                    "processTargetSealed": True,
                    "bootstrapExecutable": "Windows/WorldWalkerPrototype.exe",
                    "runtimeExecutable": (
                        "Windows/WorldWalkerPrototype/Binaries/Win64/WorldWalkerPrototype.exe"
                    ),
                    "artifactCount": len(package_artifacts),
                    "artifacts": package_artifacts,
                },
                indent=2,
            )
            + "\n",
            encoding="utf-8",
        )
        package_hash = sha256(package_manifest)
        session = root / "session.json"
        session.write_text(
            json.dumps(
                {
                    "schema": 3,
                    "seed": 424242,
                    "operator": "SelfTest",
                    "runtimeMode": "Cooked",
                    "runtime": "CookedGame",
                    "processTarget": "RuntimeExecutable",
                    "packageManifest": str(package_manifest),
                    "packageHash": package_hash,
                    "roles": ["Observe", "Evasion", "Dodge", "Recovery", "Feedback"],
                    "roleStatus": {
                        "Observe": "COMPLETED",
                        "Evasion": "COMPLETED",
                        "Dodge": "COMPLETED",
                        "Recovery": "COMPLETED",
                        "Feedback": "COMPLETED",
                    },
                    "logs": logs,
                    "objectiveGate": "PASSED",
                    "subjectiveStatus": "PENDING",
                    "packet": str(packet),
                    "signoff": str(signoff_path),
                    "visualEvidenceResolutions": list(VISUAL_RESOLUTIONS),
                },
                ensure_ascii=False,
                indent=2,
            ),
            encoding="utf-8",
        )
        evidence, output = finalize_session(session)
        if (evidence["subjectiveStatus"] != "PASS"
                or evidence["runtime"]["packageHash"] != package_hash
                or evidence["runtime"]["processTarget"] != "RuntimeExecutable"
                or not output.is_file()):
            raise SignoffError("self-test did not produce a sealed PASS result")
        updated = load_json(session, "self-test session")
        if updated.get("subjectiveStatus") != "PASS":
            raise SignoffError("self-test did not update the session subjective status")

        original_package_manifest = package_manifest.read_bytes()
        package_manifest.write_bytes(b"tampered package manifest")
        try:
            finalize_session(session, root / "package-negative.json", force=True)
        except SignoffError as exc:
            if "package manifest SHA-256" not in str(exc):
                raise
        else:
            raise SignoffError("self-test accepted a changed cooked package manifest")
        package_manifest.write_bytes(original_package_manifest)

        unexpected_package_file = package_root / "Windows" / "runtime-generated.ini"
        unexpected_package_file.write_text("mutable", encoding="utf-8")
        try:
            finalize_session(session, root / "inventory-negative.json", force=True)
        except SignoffError as exc:
            if "full inventory" not in str(exc):
                raise
        else:
            raise SignoffError("self-test accepted an unsealed extra cooked package file")
        unexpected_package_file.unlink()

        wrong = root / signoff["visualEvidence"]["1280x720"]["artifact"]
        write_test_png(wrong, 1279, 720)
        try:
            finalize_session(session, root / "negative.json", force=True)
        except SignoffError as exc:
            if "expected 1280x720" not in str(exc):
                raise
        else:
            raise SignoffError("self-test accepted a visual artifact with the wrong dimensions")


def main():
    parser = argparse.ArgumentParser(
        description="Validate and seal a completed W11 human sign-off with four exact-resolution PNG artifacts."
    )
    parser.add_argument("--session", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    try:
        if args.self_test:
            self_test()
            print(
                "W11_MANUAL_ACCEPTANCE_SIGNOFF_SELF_TEST_PASSED "
                "Checks=9 Resolutions=4 PngDimensions=Verified TamperHashes=Verified "
                "PackageProvenance=Verified FullInventory=Verified ProcessTarget=Verified"
            )
            return
        if not args.session:
            raise SignoffError("--session is required")
        evidence, output = finalize_session(args.session, args.output, args.force)
        print(
            "W11_MANUAL_ACCEPTANCE_SIGNOFF_FINALIZED "
            f"Output={output.resolve()} ObjectiveGate=PASSED "
            f"Subjective={evidence['subjectiveStatus']} "
            f"VisualEvidence={evidence['visualEvidenceStatus']}"
        )
    except SignoffError as exc:
        print(f"W11_MANUAL_ACCEPTANCE_SIGNOFF_FAILED: {exc}", file=sys.stderr)
        raise SystemExit(1) from exc


if __name__ == "__main__":
    main()
