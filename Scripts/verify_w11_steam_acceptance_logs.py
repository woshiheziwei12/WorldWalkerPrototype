import argparse
import datetime as dt
import hashlib
import json
import re
import sys
import tempfile
from pathlib import Path
from typing import Optional

sys.dont_write_bytecode = True

from verify_w11_steam_package_manifest import (
    PackageManifestError,
    verify as verify_package_manifest,
)
from verify_w11_steam_acceptance_kit import (
    KitManifestError,
    verify as verify_kit_manifest,
)


KEY_VALUE = re.compile(r"([A-Za-z][A-Za-z0-9]*)=([^\s]+)")
BLOCKING_PATTERNS = (
    "Fatal error:",
    "Assertion failed:",
    "Ensure condition failed",
)
SCENARIOS = (
    "BasicJoin",
    "FriendInvite",
    "HostExit",
    "ClientDisconnect",
    "PlayerSync",
)


class SteamEvidenceError(RuntimeError):
    pass


def validate_app_id(app_id):
    if app_id == "480" or not app_id.isdigit() or int(app_id) <= 0:
        raise SteamEvidenceError("App ID must be a non-placeholder numeric project Steam App ID")


def validate_scenario(scenario):
    if scenario not in SCENARIOS:
        raise SteamEvidenceError(f"scenario must be one of {','.join(SCENARIOS)}")


def validate_token(token):
    if not re.fullmatch(r"[A-Za-z0-9_-]{8,64}", token):
        raise SteamEvidenceError("token must match [A-Za-z0-9_-]{8,64}")


def validate_package_hash(package_hash, allow_editor_integration=False):
    if allow_editor_integration and package_hash == "EDITOR":
        return
    if not re.fullmatch(r"[0-9a-fA-F]{64}", package_hash or ""):
        raise SteamEvidenceError("package hash must be a SHA-256, or EDITOR for integration")


def validate_steam_account_id(account_id):
    if not re.fullmatch(r"[0-9]{15,20}", account_id or "") or set(account_id) == {"0"}:
        raise SteamEvidenceError("formal cooked evidence requires a valid Steam64 LocalUserId")


def require_formal_package_manifest(package_manifest, allow_editor_integration=False):
    """Require live package verification for every formal cooked evidence gate."""
    if not allow_editor_integration and package_manifest is None:
        raise SteamEvidenceError(
            "formal cooked evidence requires --package-manifest so the actual package is "
            "reverified at the evidence gate; --package-hash alone is not sufficient"
        )


def is_within(path: Path, root: Path) -> bool:
    try:
        path.resolve().relative_to(root.resolve())
    except ValueError:
        return False
    return True


def validate_output_location(
    output: Optional[Path], package_manifest: Optional[Path]
) -> None:
    """Keep generated evidence outside sealed package/kit inventory."""
    if output is None or package_manifest is None:
        return
    package_root = package_manifest.resolve().parent
    if is_within(output, package_root):
        raise SteamEvidenceError("evidence output must stay outside the sealed package root")
    kit_root = package_root.parent
    kit_manifest = kit_root / "W11SteamAcceptanceKitManifest.json"
    if package_root.name.lower() == "package" and kit_manifest.is_file():
        mutable_root = kit_root / "Saved"
        if is_within(output, kit_root) and not is_within(output, mutable_root):
            raise SteamEvidenceError(
                "portable-kit evidence output must be inside the declared mutable Saved directory"
            )


def load_events(path):
    try:
        text = path.read_text(encoding="utf-8", errors="replace")
    except OSError as exc:
        raise SteamEvidenceError(f"cannot read log {path}: {exc}") from exc
    for pattern in BLOCKING_PATTERNS:
        if pattern in text:
            raise SteamEvidenceError(f"{path.name} contains blocking engine error: {pattern}")
    events = [
        dict(KEY_VALUE.findall(line))
        for line in text.splitlines()
        if "W11_STEAM_EVIDENCE " in line
    ]
    if not events:
        raise SteamEvidenceError(f"{path.name} contains no W11_STEAM_EVIDENCE events")
    return text, events


def require_context(
    events, role, scenario, app_id, token, package_hash, label,
    allow_editor_integration=False,
):
    for index, event in enumerate(events):
        expected = {
            "Role": role,
            "Scenario": scenario,
            "Token": token,
            "PackageHash": package_hash,
            "OSS": "STEAM",
            "AppId": app_id,
        }
        for key, value in expected.items():
            if event.get(key) != value:
                raise SteamEvidenceError(
                    f"{label} event {index} has {key}={event.get(key)!r}, expected {value!r}"
                )
    environments = [event for event in events if event.get("Event") == "Environment"]
    if len(environments) != 1:
        raise SteamEvidenceError(f"{label} must contain exactly one Environment event")
    environment = environments[0]
    if environment.get("Success") != "1" or environment.get("ReleaseReady") != "1":
        raise SteamEvidenceError(f"{label} did not run in a release-eligible Steam environment")
    runtime = environment.get("Runtime")
    if runtime not in ("CookedGame", "EditorStandalone"):
        raise SteamEvidenceError(f"{label} has an unknown Runtime={runtime!r}")
    if any(event.get("Runtime") != runtime for event in events):
        raise SteamEvidenceError(f"{label} contains inconsistent Runtime facts")
    if runtime != "CookedGame" and not allow_editor_integration:
        raise SteamEvidenceError(
            f"{label} used Runtime={runtime}; formal Steam evidence requires a cooked packaged game"
        )
    if runtime == "CookedGame" and package_hash == "EDITOR":
        raise SteamEvidenceError(f"{label} used the editor package identity in a cooked game")
    if runtime == "EditorStandalone" and package_hash != "EDITOR":
        raise SteamEvidenceError(f"{label} editor integration must use PackageHash=EDITOR")
    local_user_id = environment.get("LocalUserId", "NONE")
    if runtime == "CookedGame":
        validate_steam_account_id(local_user_id)
    if any(event.get("LocalUserId", "NONE") != local_user_id for event in events):
        raise SteamEvidenceError(f"{label} contains inconsistent LocalUserId facts")
    return runtime, local_user_id


def successful_event(events, event_name, label):
    matches = [
        event for event in events
        if event.get("Event") == event_name and event.get("Success") == "1"
    ]
    if not matches:
        raise SteamEvidenceError(f"{label} has no successful {event_name} event")
    return matches[-1]


def validate_role_log(
    path, role, app_id, token, package_hash, allow_editor_integration=False,
    scenario="BasicJoin", expected_players=2,
):
    validate_scenario(scenario)
    if not 2 <= expected_players <= 4:
        raise SteamEvidenceError("expected player count must be between 2 and 4")
    if scenario != "PlayerSync" and expected_players != 2:
        raise SteamEvidenceError("only PlayerSync may use an expected player count other than 2")
    if allow_editor_integration and scenario != "BasicJoin":
        raise SteamEvidenceError("non-BasicJoin scenarios require cooked release evidence")
    _, events = load_events(path)
    runtime, local_user_id = require_context(
        events, role, scenario, app_id, token, package_hash, path.name,
        allow_editor_integration,
    )
    if role == "Host":
        successful_event(events, "HostRequest", path.name)
        terminal = successful_event(events, "HostComplete", path.name)
        session_id = terminal.get("SessionId", "NONE")
        if scenario == "ClientDisconnect":
            left = successful_event(events, "ParticipantLeft", path.name)
            try:
                remaining = int(left.get("Players", "-1"))
            except ValueError as exc:
                raise SteamEvidenceError(f"{path.name} has an invalid ParticipantLeft count") from exc
            if remaining != expected_players - 1:
                raise SteamEvidenceError(
                    f"{path.name} ParticipantLeft has Players={remaining}, expected {expected_players - 1}"
                )
    else:
        find = None
        if scenario == "FriendInvite":
            invite = successful_event(events, "InviteAccepted", path.name)
            if invite.get("MetadataMatch") != "1":
                raise SteamEvidenceError(f"{path.name} invite metadata was not verified")
            join_request = successful_event(events, "JoinRequest", path.name)
            if join_request.get("Source") != "Invite":
                raise SteamEvidenceError(f"{path.name} did not join from the accepted invite")
        else:
            successful_event(events, "FindRequest", path.name)
            find = successful_event(events, "FindComplete", path.name)
            try:
                result_count = int(find.get("Results", "0"))
            except ValueError as exc:
                raise SteamEvidenceError(f"{path.name} has an invalid FindComplete result count") from exc
            if result_count < 1:
                raise SteamEvidenceError(f"{path.name} successful FindComplete has no results")
            successful_event(events, "JoinRequest", path.name)
        terminal = successful_event(events, "JoinComplete", path.name)
        if scenario == "FriendInvite" and terminal.get("Source") != "Invite":
            raise SteamEvidenceError(f"{path.name} JoinComplete did not preserve Source=Invite")
        session_id = terminal.get("SessionId", "NONE")
        if scenario == "FriendInvite" and invite.get("SessionId") != session_id:
            raise SteamEvidenceError(f"{path.name} invite and joined session identities differ")
        if scenario == "HostExit":
            failure = successful_event(events, "NetworkFailure", path.name)
            if failure.get("FailureType") not in ("ConnectionLost", "ConnectionTimeout", "FailureReceived"):
                raise SteamEvidenceError(
                    f"{path.name} did not observe a host-loss network failure"
                )
    if not session_id or session_id == "NONE":
        raise SteamEvidenceError(f"{path.name} does not expose a resolved Steam SessionId")
    return events, session_id, runtime, local_user_id


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_portable_kit_identity(kit_manifest, package_manifest):
    if kit_manifest is None:
        if package_manifest is not None:
            package_manifest = package_manifest.resolve()
            candidate = (
                package_manifest.parent.parent / "W11SteamAcceptanceKitManifest.json"
            )
            if package_manifest.parent.name.lower() == "package" and candidate.is_file():
                raise SteamEvidenceError(
                    "portable package evidence requires --kit-manifest; kit identity cannot be omitted"
                )
        return None
    if package_manifest is None:
        raise SteamEvidenceError("--kit-manifest requires --package-manifest")
    kit_manifest = kit_manifest.resolve()
    package_manifest = package_manifest.resolve()
    try:
        verify_kit_manifest(kit_manifest)
        manifest = json.loads(kit_manifest.read_text(encoding="utf-8-sig"))
    except (KitManifestError, OSError, json.JSONDecodeError) as exc:
        raise SteamEvidenceError(f"portable kit manifest failed validation: {exc}") from exc
    expected_package_manifest = (
        kit_manifest.parent / Path(*manifest["packageManifest"].split("/"))
    ).resolve()
    if expected_package_manifest != package_manifest:
        raise SteamEvidenceError(
            "portable kit and --package-manifest do not identify the same nested package"
        )
    package_hash = sha256(package_manifest)
    if str(manifest.get("packageManifestSha256", "")).lower() != package_hash:
        raise SteamEvidenceError("portable kit package manifest identity mismatch")
    return {
        "manifest": str(kit_manifest),
        "manifestSha256": sha256(kit_manifest),
        "launchContract": "RunnerOnlyUserDirOutsidePackage",
        "verificationContract": "PreLaunchAndPostExitFullInventory",
        "preLaunchKitVerified": True,
        "postExitKitVerified": True,
    }


def write_json_output(path, evidence, force=False):
    if path.exists() and not force:
        raise SteamEvidenceError(f"output already exists: {path}; use --force to replace it")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(evidence, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )


def build_role_receipt(
    log_path, role, scenario, expected_players, app_id, token, package_hash,
    session_id, runtime, local_user_id, kit_identity=None,
):
    if runtime != "CookedGame":
        raise SteamEvidenceError("durable role receipts are only valid for cooked release evidence")
    receipt = {
        "schema": 2 if kit_identity else 1,
        "kind": "W11SteamRoleReceipt",
        "verifiedAt": dt.datetime.now(dt.timezone.utc).isoformat(),
        "gate": "ROLE_LOG_AND_POST_EXIT_PACKAGE_PASSED",
        "role": role,
        "scenario": scenario,
        "expectedPlayers": expected_players,
        "appId": app_id,
        "token": token,
        "sessionId": session_id,
        "runtime": runtime,
        "localUserId": local_user_id,
        "packageManifestSha256": package_hash,
        "packageVerification": "FullInventory",
        "processTarget": "RuntimeExecutable",
        "postExitPackageVerified": True,
        "logFileName": log_path.name,
        "logSha256": sha256(log_path),
    }
    if kit_identity:
        receipt.update({
            "kitManifestSha256": kit_identity["manifestSha256"],
            "kitVerification": "FullInventory",
            "launchContract": kit_identity["launchContract"],
            "verificationContract": kit_identity["verificationContract"],
            "preLaunchKitVerified": kit_identity["preLaunchKitVerified"],
            "postExitKitVerified": kit_identity["postExitKitVerified"],
        })
    return receipt


def require_release_receipt_inputs(
    host_receipt, client_receipts, client_count, allow_editor_integration=False
):
    supplied_clients = client_receipts or []
    if allow_editor_integration:
        if host_receipt is not None or supplied_clients:
            raise SteamEvidenceError("editor integration does not accept cooked release receipts")
        return
    if host_receipt is None or len(supplied_clients) != client_count:
        raise SteamEvidenceError(
            "formal cooked aggregation requires one --host-receipt and one matching "
            "--client-receipt for every client log"
        )


def validate_role_receipt(
    receipt_path, log_path, role, scenario, expected_players, app_id, token,
    package_hash, session_id, local_user_id, kit_identity=None,
):
    try:
        receipt = json.loads(receipt_path.read_text(encoding="utf-8-sig"))
    except (OSError, json.JSONDecodeError) as exc:
        raise SteamEvidenceError(f"cannot read role receipt {receipt_path}: {exc}") from exc
    expected = {
        "schema": 2 if kit_identity else 1,
        "kind": "W11SteamRoleReceipt",
        "gate": "ROLE_LOG_AND_POST_EXIT_PACKAGE_PASSED",
        "role": role,
        "scenario": scenario,
        "expectedPlayers": expected_players,
        "appId": app_id,
        "token": token,
        "sessionId": session_id,
        "runtime": "CookedGame",
        "localUserId": local_user_id,
        "packageManifestSha256": package_hash,
        "packageVerification": "FullInventory",
        "processTarget": "RuntimeExecutable",
        "postExitPackageVerified": True,
        "logFileName": log_path.name,
        "logSha256": sha256(log_path),
    }
    if kit_identity:
        expected.update({
            "kitManifestSha256": kit_identity["manifestSha256"],
            "kitVerification": "FullInventory",
            "launchContract": kit_identity["launchContract"],
            "verificationContract": kit_identity["verificationContract"],
            "preLaunchKitVerified": True,
            "postExitKitVerified": True,
        })
    for key, value in expected.items():
        if receipt.get(key) != value:
            raise SteamEvidenceError(
                f"{receipt_path.name} has {key}={receipt.get(key)!r}, expected {value!r}"
            )
    record = {
        "path": str(receipt_path.resolve()),
        "sha256": sha256(receipt_path),
        "postExitPackageVerified": True,
    }
    if kit_identity:
        record.update({
            "kitManifestSha256": kit_identity["manifestSha256"],
            "preLaunchKitVerified": True,
            "postExitKitVerified": True,
        })
    return record


def validate_release_receipts(
    host_receipt, client_receipts, host_log, client_logs, scenario,
    expected_players, app_id, token, package_hash, session_id,
    host_local_user_id, client_local_user_ids, kit_identity=None,
):
    receipt_paths = [host_receipt.resolve(), *(path.resolve() for path in client_receipts)]
    if len(set(receipt_paths)) != len(receipt_paths):
        raise SteamEvidenceError("host and client role receipts must be distinct files")
    host_record = validate_role_receipt(
        host_receipt, host_log, "Host", scenario, expected_players, app_id,
        token, package_hash, session_id, host_local_user_id, kit_identity,
    )
    client_records = [
        validate_role_receipt(
            receipt, log, "Client", scenario, expected_players, app_id,
            token, package_hash, session_id, local_user_id, kit_identity,
        )
        for receipt, log, local_user_id in zip(
            client_receipts, client_logs, client_local_user_ids
        )
    ]
    return {"host": host_record, "clients": client_records}


def validate_group(
    host_path, client_paths, app_id, token, package_hash,
    allow_editor_integration=False, scenario="BasicJoin", expected_players=2,
):
    validate_scenario(scenario)
    if len(client_paths) != expected_players - 1:
        raise SteamEvidenceError(
            f"expected {expected_players - 1} client logs for {expected_players} players, found {len(client_paths)}"
        )
    resolved_paths = [host_path.resolve(), *(path.resolve() for path in client_paths)]
    if len(set(resolved_paths)) != len(resolved_paths):
        raise SteamEvidenceError("host and client logs must be distinct files")
    host_events, host_session, host_runtime, host_local_user_id = validate_role_log(
        host_path, "Host", app_id, token, package_hash, allow_editor_integration,
        scenario, expected_players,
    )
    participant_matches = [
        event for event in host_events
        if event.get("Event") == "ParticipantJoined"
        and event.get("Success") == "1"
        and event.get("Players") == str(expected_players)
        and event.get("SessionId") == host_session
    ]
    if not participant_matches:
        raise SteamEvidenceError(
            f"{host_path.name} never reached Players={expected_players} in SessionId={host_session}"
        )
    client_records = []
    client_local_user_ids = []
    for client_path in client_paths:
        client_events, client_session, client_runtime, client_local_user_id = validate_role_log(
            client_path, "Client", app_id, token, package_hash,
            allow_editor_integration, scenario, expected_players,
        )
        if host_runtime != client_runtime:
            raise SteamEvidenceError(
                f"host/client runtime mismatch: host={host_runtime} client={client_runtime}"
            )
        if scenario != "FriendInvite":
            find = successful_event(client_events, "FindComplete", client_path.name)
            found_session = find.get("FirstSessionId", "NONE")
            if host_session != found_session:
                raise SteamEvidenceError(
                    f"Steam find identity mismatch: host={host_session} find={found_session}"
                )
        if host_session != client_session:
            raise SteamEvidenceError(
                f"Steam join identity mismatch: host={host_session} join={client_session}"
            )
        client_records.append({
            "path": str(client_path.resolve()),
            "sha256": sha256(client_path),
            "localUserId": client_local_user_id,
            "failedEvidenceEvents": sum(
                event.get("Success") == "0" for event in client_events
            ),
        })
        client_local_user_ids.append(client_local_user_id)
    log_hashes = [sha256(host_path), *(record["sha256"] for record in client_records)]
    if len(set(log_hashes)) != len(log_hashes):
        raise SteamEvidenceError("host and client logs must have distinct SHA-256 content")
    if host_runtime == "CookedGame":
        local_user_ids = [host_local_user_id, *client_local_user_ids]
        if len(set(local_user_ids)) != len(local_user_ids):
            raise SteamEvidenceError(
                "formal cooked aggregation requires a distinct Steam LocalUserId for every participant"
            )
    gate_by_scenario = {
        "BasicJoin": "BASIC_JOIN_PASSED",
        "FriendInvite": "FRIEND_INVITE_JOIN_PASSED",
        "HostExit": "HOST_EXIT_OBSERVED",
        "ClientDisconnect": "CLIENT_DISCONNECT_OBSERVED",
        "PlayerSync": "PLAYER_SYNC_PASSED",
    }
    gate = (
        gate_by_scenario[scenario]
        if host_runtime == "CookedGame" else "EDITOR_INTEGRATION_PASSED"
    )
    not_proven = ["NATTraversal", "Reconnect", "HostMigration"]
    if scenario != "FriendInvite":
        not_proven.append("FriendInvite")
    if scenario != "HostExit":
        not_proven.append("HostExitBehavior")
    not_proven.append("HostExitRecovery")
    if scenario != "ClientDisconnect":
        not_proven.append("ClientDisconnectBehavior")
    return {
        "schema": 4,
        "verifiedAt": dt.datetime.now(dt.timezone.utc).isoformat(),
        "gate": gate,
        "scenario": scenario,
        "expectedPlayers": expected_players,
        "appId": app_id,
        "token": token,
        "sessionId": host_session,
        "runtime": host_runtime,
        "participantLocalUserIds": [host_local_user_id, *client_local_user_ids],
        "packageManifestSha256": package_hash,
        "host": {
            "path": str(host_path.resolve()),
            "sha256": sha256(host_path),
            "localUserId": host_local_user_id,
            "failedEvidenceEvents": sum(event.get("Success") == "0" for event in host_events),
        },
        "clients": client_records,
        "notProven": not_proven,
    }


def validate_pair(
    host_path, client_path, app_id, token, package_hash,
    allow_editor_integration=False, scenario="BasicJoin", expected_players=2,
):
    return validate_group(
        host_path, [client_path], app_id, token, package_hash,
        allow_editor_integration, scenario, expected_players,
    )


def event_line(
    event, role, token, app_id, package_hash, success=1, runtime="CookedGame",
    scenario="BasicJoin", local_user_id=None, **facts
):
    if local_user_id is None:
        local_user_id = (
            "76561198000000001" if role == "Host" else "76561198000000002"
        )
    values = {
        "Event": event,
        "Role": role,
        "Scenario": scenario,
        "Token": token,
        "PackageHash": package_hash,
        "OSS": "STEAM",
        "AppId": app_id,
        "LocalUserId": local_user_id,
        "Runtime": runtime,
        "Success": str(success),
        **{key: str(value) for key, value in facts.items()},
    }
    return "LogWorldWalkerW11: Display: W11_STEAM_EVIDENCE " + " ".join(
        f"{key}={value}" for key, value in values.items()
    )


def self_test():
    app_id = "1234567"
    token = "W11_20260830_ABCD"
    package_hash = "0123456789abcdef" * 4
    session_id = "109775241012345678"
    host_user_id = "76561198000000001"
    client_user_id = "76561198000000002"
    with tempfile.TemporaryDirectory(prefix="w11_steam_evidence_") as temp:
        root = Path(temp)
        host = root / "host.log"
        client = root / "client.log"
        host.write_text(
            "\n".join(
                (
                    event_line("Environment", "Host", token, app_id, package_hash, ReleaseReady=1),
                    event_line("HostRequest", "Host", token, app_id, package_hash, Reason="Accepted"),
                    event_line("HostComplete", "Host", token, app_id, package_hash, SessionId=session_id),
                    event_line(
                        "ParticipantJoined", "Host", token, app_id, package_hash,
                        PlayerId=257, Players=2, SessionId=session_id,
                    ),
                )
            ),
            encoding="utf-8",
        )
        client.write_text(
            "\n".join(
                (
                    event_line("Environment", "Client", token, app_id, package_hash, ReleaseReady=1),
                    event_line("FindRequest", "Client", token, app_id, package_hash, Reason="Accepted"),
                    event_line(
                        "FindComplete", "Client", token, app_id, package_hash,
                        Results=1, FirstSessionId=session_id,
                    ),
                    event_line("JoinRequest", "Client", token, app_id, package_hash, Reason="Accepted"),
                    event_line("JoinComplete", "Client", token, app_id, package_hash, Result=0, SessionId=session_id),
                )
            ),
            encoding="utf-8",
        )
        evidence = validate_pair(host, client, app_id, token, package_hash)
        if evidence["gate"] != "BASIC_JOIN_PASSED" or evidence["sessionId"] != session_id:
            raise SteamEvidenceError("self-test did not produce the expected basic join evidence")
        host_receipt = root / "host.receipt.json"
        client_receipt = root / "client.receipt.json"
        write_json_output(
            host_receipt,
            build_role_receipt(
                host, "Host", "BasicJoin", 2, app_id, token, package_hash,
                session_id, "CookedGame", host_user_id,
            ),
        )
        write_json_output(
            client_receipt,
            build_role_receipt(
                client, "Client", "BasicJoin", 2, app_id, token, package_hash,
                session_id, "CookedGame", client_user_id,
            ),
        )
        require_release_receipt_inputs(host_receipt, [client_receipt], 1)
        receipt_records = validate_release_receipts(
            host_receipt, [client_receipt], host, [client], "BasicJoin", 2,
            app_id, token, package_hash, session_id, host_user_id, [client_user_id],
        )
        if not receipt_records["host"]["postExitPackageVerified"]:
            raise SteamEvidenceError("self-test did not preserve post-exit package verification")
        kit_identity = {
            "manifest": str((root / "W11SteamAcceptanceKitManifest.json").resolve()),
            "manifestSha256": "a" * 64,
            "launchContract": "RunnerOnlyUserDirOutsidePackage",
            "verificationContract": "PreLaunchAndPostExitFullInventory",
            "preLaunchKitVerified": True,
            "postExitKitVerified": True,
        }
        portable_host_receipt = root / "host.portable.receipt.json"
        portable_client_receipt = root / "client.portable.receipt.json"
        write_json_output(
            portable_host_receipt,
            build_role_receipt(
                host, "Host", "BasicJoin", 2, app_id, token, package_hash,
                session_id, "CookedGame", host_user_id, kit_identity,
            ),
        )
        write_json_output(
            portable_client_receipt,
            build_role_receipt(
                client, "Client", "BasicJoin", 2, app_id, token, package_hash,
                session_id, "CookedGame", client_user_id, kit_identity,
            ),
        )
        portable_receipts = validate_release_receipts(
            portable_host_receipt, [portable_client_receipt], host, [client],
            "BasicJoin", 2, app_id, token, package_hash, session_id,
            host_user_id, [client_user_id], kit_identity,
        )
        if not portable_receipts["host"]["postExitKitVerified"]:
            raise SteamEvidenceError("self-test did not preserve portable kit verification")
        kit_tampered_receipt = root / "client.portable.tampered.receipt.json"
        kit_tampered_data = json.loads(
            portable_client_receipt.read_text(encoding="utf-8")
        )
        kit_tampered_data["kitManifestSha256"] = "b" * 64
        write_json_output(kit_tampered_receipt, kit_tampered_data)
        kit_receipt_tamper_rejected = False
        try:
            validate_release_receipts(
                portable_host_receipt, [kit_tampered_receipt], host, [client],
                "BasicJoin", 2, app_id, token, package_hash, session_id,
                host_user_id, [client_user_id], kit_identity,
            )
        except SteamEvidenceError as exc:
            kit_receipt_tamper_rejected = "kitManifestSha256" in str(exc)
        if not kit_receipt_tamper_rejected:
            raise SteamEvidenceError("self-test accepted a mismatched portable kit receipt")
        kit_manifest_required = False
        try:
            validate_release_receipts(
                portable_host_receipt, [portable_client_receipt], host, [client],
                "BasicJoin", 2, app_id, token, package_hash, session_id,
                host_user_id, [client_user_id],
            )
        except SteamEvidenceError as exc:
            kit_manifest_required = "schema=2" in str(exc)
        if not kit_manifest_required:
            raise SteamEvidenceError("self-test accepted portable receipts without kit identity")
        missing_receipt_rejected = False
        try:
            require_release_receipt_inputs(None, [], 1)
        except SteamEvidenceError as exc:
            missing_receipt_rejected = "requires one --host-receipt" in str(exc)
        if not missing_receipt_rejected:
            raise SteamEvidenceError("self-test accepted cooked aggregation without role receipts")
        tampered_receipt = root / "client.tampered.receipt.json"
        tampered_data = json.loads(client_receipt.read_text(encoding="utf-8"))
        tampered_data["logSha256"] = "f" * 64
        write_json_output(tampered_receipt, tampered_data)
        receipt_tamper_rejected = False
        try:
            validate_release_receipts(
                host_receipt, [tampered_receipt], host, [client], "BasicJoin", 2,
                app_id, token, package_hash, session_id,
                host_user_id, [client_user_id],
            )
        except SteamEvidenceError as exc:
            receipt_tamper_rejected = "logSha256" in str(exc)
        if not receipt_tamper_rejected:
            raise SteamEvidenceError("self-test accepted a tampered role receipt")
        host_editor = root / "host_editor.log"
        client_editor = root / "client_editor.log"
        host_editor.write_text(
            host.read_text(encoding="utf-8")
            .replace("Runtime=CookedGame", "Runtime=EditorStandalone")
            .replace(f"PackageHash={package_hash}", "PackageHash=EDITOR"),
            encoding="utf-8",
        )
        client_editor.write_text(
            client.read_text(encoding="utf-8")
            .replace("Runtime=CookedGame", "Runtime=EditorStandalone")
            .replace(f"PackageHash={package_hash}", "PackageHash=EDITOR"),
            encoding="utf-8",
        )
        try:
            validate_pair(host_editor, client_editor, app_id, token, "EDITOR")
        except SteamEvidenceError as exc:
            if "requires a cooked packaged game" not in str(exc):
                raise
        else:
            raise SteamEvidenceError("self-test accepted editor integration logs as formal release evidence")
        editor_evidence = validate_pair(
            host_editor, client_editor, app_id, token, "EDITOR", allow_editor_integration=True
        )
        if editor_evidence["gate"] != "EDITOR_INTEGRATION_PASSED":
            raise SteamEvidenceError("self-test did not preserve the editor-integration evidence boundary")
        wrong_package = root / "client_wrong_package.log"
        wrong_package.write_text(
            client.read_text(encoding="utf-8").replace(package_hash, "f" * 64),
            encoding="utf-8",
        )
        try:
            validate_pair(host, wrong_package, app_id, token, package_hash)
        except SteamEvidenceError as exc:
            if "PackageHash" not in str(exc):
                raise
        else:
            raise SteamEvidenceError("self-test accepted mismatched package identities")
        tampered = client.read_text(encoding="utf-8").replace(session_id, "different", 1)
        client.write_text(tampered, encoding="utf-8")
        try:
            validate_pair(host, client, app_id, token, package_hash)
        except SteamEvidenceError as exc:
            if "identity mismatch" not in str(exc):
                raise
        else:
            raise SteamEvidenceError("self-test accepted mismatched Steam session identities")
        try:
            validate_app_id("480")
        except SteamEvidenceError:
            pass
        else:
            raise SteamEvidenceError("self-test accepted placeholder App ID 480")
        try:
            validate_token("bad token")
        except SteamEvidenceError:
            pass
        else:
            raise SteamEvidenceError("self-test accepted an invalid evidence token")

        def write_scenario_pair(name, scenario, host_tail=(), client_prefix=(), client_tail=()):
            scenario_host = root / f"host_{name}.log"
            scenario_client = root / f"client_{name}.log"
            scenario_host.write_text(
                "\n".join((
                    event_line(
                        "Environment", "Host", token, app_id, package_hash,
                        scenario=scenario, ReleaseReady=1,
                    ),
                    event_line(
                        "HostRequest", "Host", token, app_id, package_hash,
                        scenario=scenario, Reason="Accepted",
                    ),
                    event_line(
                        "HostComplete", "Host", token, app_id, package_hash,
                        scenario=scenario, SessionId=session_id,
                    ),
                    event_line(
                        "ParticipantJoined", "Host", token, app_id, package_hash,
                        scenario=scenario, PlayerId=257, Players=2,
                        SessionId=session_id,
                    ),
                    *host_tail,
                )),
                encoding="utf-8",
            )
            browser_join = (
                event_line(
                    "FindRequest", "Client", token, app_id, package_hash,
                    scenario=scenario, Reason="Accepted",
                ),
                event_line(
                    "FindComplete", "Client", token, app_id, package_hash,
                    scenario=scenario, Results=1, FirstSessionId=session_id,
                ),
                event_line(
                    "JoinRequest", "Client", token, app_id, package_hash,
                    scenario=scenario, Reason="Accepted", Source="Browser",
                ),
            )
            scenario_client.write_text(
                "\n".join((
                    event_line(
                        "Environment", "Client", token, app_id, package_hash,
                        scenario=scenario, ReleaseReady=1,
                    ),
                    *client_prefix,
                    *(browser_join if scenario != "FriendInvite" else ()),
                    event_line(
                        "JoinComplete", "Client", token, app_id, package_hash,
                        scenario=scenario, Result=0, SessionId=session_id,
                        Source="Invite" if scenario == "FriendInvite" else "Browser",
                    ),
                    *client_tail,
                )),
                encoding="utf-8",
            )
            return scenario_host, scenario_client

        invite_prefix = (
            event_line(
                "InviteAccepted", "Client", token, app_id, package_hash,
                scenario="FriendInvite", Controller=0, SessionId=session_id,
                MetadataMatch=1,
            ),
            event_line(
                "JoinRequest", "Client", token, app_id, package_hash,
                scenario="FriendInvite", Reason="Accepted", Source="Invite",
                SessionId=session_id,
            ),
        )
        invite_host, invite_client = write_scenario_pair(
            "invite", "FriendInvite", client_prefix=invite_prefix,
        )
        invite_evidence = validate_pair(
            invite_host, invite_client, app_id, token, package_hash,
            scenario="FriendInvite",
        )
        if invite_evidence["gate"] != "FRIEND_INVITE_JOIN_PASSED":
            raise SteamEvidenceError("self-test did not prove the invite join path")
        invite_wrong_source = root / "client_invite_wrong_source.log"
        invite_wrong_source.write_text(
            invite_client.read_text(encoding="utf-8").replace("Source=Invite", "Source=Browser"),
            encoding="utf-8",
        )
        try:
            validate_pair(
                invite_host, invite_wrong_source, app_id, token, package_hash,
                scenario="FriendInvite",
            )
        except SteamEvidenceError as exc:
            if "did not join from the accepted invite" not in str(exc):
                raise
        else:
            raise SteamEvidenceError("self-test accepted an invite log with a browser join source")

        host_exit_tail = (
            event_line(
                "NetworkFailure", "Client", token, app_id, package_hash,
                scenario="HostExit", FailureType="ConnectionLost", SessionId="NONE",
            ),
        )
        host_exit_host, host_exit_client = write_scenario_pair(
            "host_exit", "HostExit", client_tail=host_exit_tail,
        )
        if validate_pair(
            host_exit_host, host_exit_client, app_id, token, package_hash,
            scenario="HostExit",
        )["gate"] != "HOST_EXIT_OBSERVED":
            raise SteamEvidenceError("self-test did not prove host-exit observation")
        host_exit_missing_failure = root / "client_host_exit_missing_failure.log"
        host_exit_missing_failure.write_text(
            "\n".join(
                line for line in host_exit_client.read_text(encoding="utf-8").splitlines()
                if "Event=NetworkFailure" not in line
            ),
            encoding="utf-8",
        )
        try:
            validate_pair(
                host_exit_host, host_exit_missing_failure, app_id, token, package_hash,
                scenario="HostExit",
            )
        except SteamEvidenceError as exc:
            if "no successful NetworkFailure" not in str(exc):
                raise
        else:
            raise SteamEvidenceError("self-test accepted HostExit without a network failure")

        disconnect_tail = (
            event_line(
                "ParticipantLeft", "Host", token, app_id, package_hash,
                scenario="ClientDisconnect", PlayerId=257, Players=1,
                SessionId=session_id,
            ),
        )
        disconnect_host, disconnect_client = write_scenario_pair(
            "client_disconnect", "ClientDisconnect", host_tail=disconnect_tail,
        )
        if validate_pair(
            disconnect_host, disconnect_client, app_id, token, package_hash,
            scenario="ClientDisconnect",
        )["gate"] != "CLIENT_DISCONNECT_OBSERVED":
            raise SteamEvidenceError("self-test did not prove client-disconnect observation")
        disconnect_wrong_count = root / "host_client_disconnect_wrong_count.log"
        disconnect_wrong_count.write_text(
            disconnect_host.read_text(encoding="utf-8").replace(
                "PlayerId=257 Players=1", "PlayerId=257 Players=2"
            ),
            encoding="utf-8",
        )
        try:
            validate_pair(
                disconnect_wrong_count, disconnect_client, app_id, token, package_hash,
                scenario="ClientDisconnect",
            )
        except SteamEvidenceError as exc:
            if "ParticipantLeft has Players=2" not in str(exc):
                raise
        else:
            raise SteamEvidenceError("self-test accepted an incorrect disconnect population")

        sync_host = root / "host_sync.log"
        sync_clients = []
        sync_host.write_text(
            "\n".join((
                event_line(
                    "Environment", "Host", token, app_id, package_hash,
                    scenario="PlayerSync", ReleaseReady=1,
                ),
                event_line(
                    "HostRequest", "Host", token, app_id, package_hash,
                    scenario="PlayerSync", Reason="Accepted",
                ),
                event_line(
                    "HostComplete", "Host", token, app_id, package_hash,
                    scenario="PlayerSync", SessionId=session_id,
                ),
                *(
                    event_line(
                        "ParticipantJoined", "Host", token, app_id, package_hash,
                        scenario="PlayerSync", PlayerId=255 + players,
                        Players=players, SessionId=session_id,
                    )
                    for players in range(1, 5)
                ),
            )),
            encoding="utf-8",
        )
        for index in range(3):
            sync_client = root / f"client_sync_{index + 1}.log"
            sync_client_user_id = f"7656119800000000{index + 2}"
            sync_client.write_text(
                "\n".join((
                    event_line(
                        "Environment", "Client", token, app_id, package_hash,
                        scenario="PlayerSync", local_user_id=sync_client_user_id,
                        ReleaseReady=1,
                    ),
                    event_line(
                        "FindRequest", "Client", token, app_id, package_hash,
                        scenario="PlayerSync", local_user_id=sync_client_user_id,
                        Reason="Accepted",
                    ),
                    event_line(
                        "FindComplete", "Client", token, app_id, package_hash,
                        scenario="PlayerSync", local_user_id=sync_client_user_id,
                        Results=1, FirstSessionId=session_id,
                    ),
                    event_line(
                        "JoinRequest", "Client", token, app_id, package_hash,
                        scenario="PlayerSync", local_user_id=sync_client_user_id,
                        Reason="Accepted", Source="Browser",
                    ),
                    event_line(
                        "JoinComplete", "Client", token, app_id, package_hash,
                        scenario="PlayerSync", local_user_id=sync_client_user_id,
                        Result=0, SessionId=session_id,
                        Source="Browser",
                    ),
                )),
                encoding="utf-8",
            )
            sync_clients.append(sync_client)
        sync_evidence = validate_group(
            sync_host, sync_clients, app_id, token, package_hash,
            scenario="PlayerSync", expected_players=4,
        )
        if sync_evidence["gate"] != "PLAYER_SYNC_PASSED" or len(sync_evidence["clients"]) != 3:
            raise SteamEvidenceError("self-test did not prove four-player identity")

        duplicate_log = root / "client_sync_duplicate.log"
        duplicate_log.write_bytes(sync_clients[0].read_bytes())
        duplicate_log_rejected = False
        try:
            validate_group(
                sync_host, [sync_clients[0], sync_clients[1], duplicate_log],
                app_id, token, package_hash, scenario="PlayerSync", expected_players=4,
            )
        except SteamEvidenceError as exc:
            duplicate_log_rejected = "distinct SHA-256 content" in str(exc)
        if not duplicate_log_rejected:
            raise SteamEvidenceError("self-test accepted duplicated client log content")

        duplicate_identity = root / "client_sync_duplicate_identity.log"
        duplicate_identity.write_text(
            sync_clients[2].read_text(encoding="utf-8").replace(
                "LocalUserId=76561198000000004",
                "LocalUserId=76561198000000002",
            ) + "\nLogWorldWalkerW11: Display: distinct-file-marker\n",
            encoding="utf-8",
        )
        duplicate_identity_rejected = False
        try:
            validate_group(
                sync_host, [sync_clients[0], sync_clients[1], duplicate_identity],
                app_id, token, package_hash, scenario="PlayerSync", expected_players=4,
            )
        except SteamEvidenceError as exc:
            duplicate_identity_rejected = "distinct Steam LocalUserId" in str(exc)
        if not duplicate_identity_rejected:
            raise SteamEvidenceError("self-test accepted duplicate Steam participant identity")

        missing_sync_client_rejected = False
        try:
            validate_group(
                sync_host, sync_clients[:2], app_id, token, package_hash,
                scenario="PlayerSync", expected_players=4,
            )
        except SteamEvidenceError as exc:
            missing_sync_client_rejected = "expected 3 client logs" in str(exc)
        if not missing_sync_client_rejected:
            raise SteamEvidenceError("self-test accepted incomplete four-player evidence")

        formal_manifest_required = False
        try:
            require_formal_package_manifest(None, False)
        except SteamEvidenceError as exc:
            formal_manifest_required = "requires --package-manifest" in str(exc)
        if not formal_manifest_required:
            raise SteamEvidenceError("self-test accepted hash-only formal cooked evidence")
        require_formal_package_manifest(None, True)

        portable_root = root / "portable"
        portable_package = portable_root / "Package"
        portable_package.mkdir(parents=True)
        portable_manifest = portable_package / "W11SteamPackageManifest.json"
        portable_manifest.write_text("{}", encoding="utf-8")
        package_output_rejected = False
        try:
            validate_output_location(portable_package / "evidence.json", portable_manifest)
        except SteamEvidenceError as exc:
            package_output_rejected = "outside the sealed package root" in str(exc)
        if not package_output_rejected:
            raise SteamEvidenceError("self-test accepted evidence output inside a sealed package")
        (portable_root / "W11SteamAcceptanceKitManifest.json").write_text(
            "{}", encoding="utf-8"
        )
        portable_layout_kit_required = False
        try:
            verify_portable_kit_identity(None, portable_manifest)
        except SteamEvidenceError as exc:
            portable_layout_kit_required = "requires --kit-manifest" in str(exc)
        if not portable_layout_kit_required:
            raise SteamEvidenceError(
                "self-test allowed portable package evidence to omit kit identity"
            )
        immutable_kit_output_rejected = False
        try:
            validate_output_location(portable_root / "evidence.json", portable_manifest)
        except SteamEvidenceError as exc:
            immutable_kit_output_rejected = "mutable Saved directory" in str(exc)
        if not immutable_kit_output_rejected:
            raise SteamEvidenceError("self-test accepted output in immutable portable-kit space")
        validate_output_location(portable_root / "Saved" / "evidence.json", portable_manifest)
        validate_output_location(root / "external" / "evidence.json", portable_manifest)


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Verify W11 real-Steam join, invite, disconnect, host-exit, and 2-4 player "
            "logs with shared scenario/token/package/session identity."
        )
    )
    parser.add_argument("--host", type=Path)
    parser.add_argument("--client", type=Path, action="append")
    parser.add_argument("--host-receipt", type=Path)
    parser.add_argument("--client-receipt", type=Path, action="append")
    parser.add_argument("--role", choices=("Host", "Client"))
    parser.add_argument("--log", type=Path)
    parser.add_argument("--scenario", choices=SCENARIOS, default="BasicJoin")
    parser.add_argument("--expected-players", type=int, default=2)
    parser.add_argument("--app-id")
    parser.add_argument("--token")
    parser.add_argument("--package-hash")
    parser.add_argument("--package-manifest", type=Path)
    parser.add_argument("--kit-manifest", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--allow-editor-integration", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    try:
        if args.self_test:
            self_test()
            print(
                "W11_STEAM_ACCEPTANCE_LOG_SELF_TEST_PASSED "
                "SharedToken=1 SharedSessionId=1 SharedPackageHash=1 PlaceholderRejected=1 "
                "CookedRequired=1 EditorIntegrationSeparated=1 FriendInvite=1 HostExit=1 "
                "ClientDisconnect=1 PlayerSync4=1 MissingClientRejected=1 "
                "EventTamperRejected=3 FormalManifestRequired=1 PostExitPackageGate=1 "
                "PackageOutputRejected=1 KitMutableOutput=1 RoleReceipts=1 "
                "ExternalOutputAllowed=1 ReceiptTamperRejected=1 MissingReceiptRejected=1 "
                "PortableKitReceipts=1 KitReceiptTamperRejected=1 KitManifestRequired=1 "
                "PortableLayoutKitManifestRequired=1 "
                "DistinctAccounts=4 DuplicateLogRejected=1 DuplicateAccountRejected=1"
            )
            return
        if not args.app_id or not args.token:
            raise SteamEvidenceError("--app-id and --token are required")
        validate_app_id(args.app_id)
        validate_token(args.token)
        if args.package_hash and args.package_manifest:
            raise SteamEvidenceError("use only one of --package-hash or --package-manifest")
        require_formal_package_manifest(args.package_manifest, args.allow_editor_integration)
        if args.package_manifest:
            try:
                verify_package_manifest(
                    args.package_manifest.resolve(),
                    require_full_inventory=True,
                    require_process_target=True,
                )
            except PackageManifestError as exc:
                raise SteamEvidenceError(f"package manifest failed validation: {exc}") from exc
            package_hash = sha256(args.package_manifest.resolve())
        elif args.package_hash:
            package_hash = args.package_hash.lower()
        elif args.allow_editor_integration:
            package_hash = "EDITOR"
        else:
            raise SteamEvidenceError("formal Steam evidence requires --package-manifest or --package-hash")
        validate_package_hash(package_hash, args.allow_editor_integration)
        if args.allow_editor_integration and args.kit_manifest:
            raise SteamEvidenceError("editor integration does not accept --kit-manifest")
        kit_identity = verify_portable_kit_identity(
            args.kit_manifest, args.package_manifest
        )
        validate_output_location(args.output, args.package_manifest)

        if args.role or args.log:
            if (
                not args.role or not args.log or args.host or args.client
                or args.host_receipt or args.client_receipt
            ):
                raise SteamEvidenceError(
                    "single-role mode requires --role/--log and accepts only its own --output receipt"
                )
            if not args.allow_editor_integration and args.output is None:
                raise SteamEvidenceError(
                    "formal cooked single-role evidence requires --output for its durable receipt"
                )
            _, session_id, runtime, local_user_id = validate_role_log(
                args.log, args.role, args.app_id, args.token, package_hash,
                args.allow_editor_integration, args.scenario, args.expected_players,
            )
            if args.output:
                receipt = build_role_receipt(
                    args.log, args.role, args.scenario, args.expected_players,
                    args.app_id, args.token, package_hash, session_id, runtime,
                    local_user_id, kit_identity,
                )
                write_json_output(args.output, receipt, args.force)
            print(
                "W11_STEAM_ACCEPTANCE_ROLE_GATE_PASSED "
                f"Role={args.role} Scenario={args.scenario} "
                f"ExpectedPlayers={args.expected_players} AppId={args.app_id} Token={args.token} "
                f"PackageHash={package_hash} SessionId={session_id} Runtime={runtime} "
                f"LocalUserId={local_user_id} "
                f"KitIdentity={'SEALED' if kit_identity else 'NOT_PORTABLE'} "
                f"Receipt={str(args.output.resolve()) if args.output else 'EDITOR'}"
            )
            return

        if not args.host or not args.client:
            raise SteamEvidenceError("pair mode requires --host and --client")
        require_release_receipt_inputs(
            args.host_receipt, args.client_receipt, len(args.client),
            args.allow_editor_integration,
        )
        evidence = validate_group(
            args.host, args.client, args.app_id, args.token, package_hash,
            args.allow_editor_integration, args.scenario, args.expected_players,
        )
        if args.package_manifest:
            evidence["schema"] = 5 if kit_identity else evidence["schema"]
            evidence["packageManifest"] = str(args.package_manifest.resolve())
            evidence["processTarget"] = "RuntimeExecutable"
            if kit_identity:
                evidence.update({
                    "kitManifest": kit_identity["manifest"],
                    "kitManifestSha256": kit_identity["manifestSha256"],
                    "kitVerification": "FullInventory",
                    "launchContract": kit_identity["launchContract"],
                    "verificationContract": kit_identity["verificationContract"],
                    "preLaunchKitVerified": True,
                    "postExitKitVerified": True,
                })
            evidence["roleReceipts"] = validate_release_receipts(
                args.host_receipt, args.client_receipt, args.host, args.client,
                args.scenario, args.expected_players, args.app_id, args.token,
                package_hash, evidence["sessionId"],
                evidence["host"]["localUserId"],
                [client["localUserId"] for client in evidence["clients"]],
                kit_identity,
            )
        if args.output:
            write_json_output(args.output, evidence, args.force)
        result_labels = {
            "BASIC_JOIN_PASSED": "W11_STEAM_ACCEPTANCE_BASIC_JOIN_PASSED",
            "FRIEND_INVITE_JOIN_PASSED": "W11_STEAM_ACCEPTANCE_FRIEND_INVITE_JOIN_PASSED",
            "HOST_EXIT_OBSERVED": "W11_STEAM_ACCEPTANCE_HOST_EXIT_OBSERVED",
            "CLIENT_DISCONNECT_OBSERVED": "W11_STEAM_ACCEPTANCE_CLIENT_DISCONNECT_OBSERVED",
            "PLAYER_SYNC_PASSED": "W11_STEAM_ACCEPTANCE_PLAYER_SYNC_PASSED",
            "EDITOR_INTEGRATION_PASSED": "W11_STEAM_ACCEPTANCE_EDITOR_INTEGRATION_PASSED",
        }
        result_label = result_labels[evidence["gate"]]
        print(
            f"{result_label} "
            f"Scenario={evidence['scenario']} ExpectedPlayers={evidence['expectedPlayers']} "
            f"AppId={evidence['appId']} Token={evidence['token']} "
            f"PackageHash={evidence['packageManifestSha256']} "
            f"SessionId={evidence['sessionId']} Runtime={evidence['runtime']} "
            f"RoleReceipts={0 if args.allow_editor_integration else 1 + len(args.client)} "
            f"KitIdentity={'SEALED' if kit_identity else 'NOT_PORTABLE'} "
            f"DistinctParticipants={len(evidence['participantLocalUserIds'])} "
            f"NotProven={','.join(evidence['notProven'])}"
        )
    except (SteamEvidenceError, PackageManifestError, KitManifestError, ValueError) as exc:
        print(f"W11_STEAM_ACCEPTANCE_LOG_FAILED: {exc}", file=sys.stderr)
        raise SystemExit(1) from exc


if __name__ == "__main__":
    main()
