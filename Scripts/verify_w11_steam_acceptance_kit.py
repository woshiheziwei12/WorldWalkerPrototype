#!/usr/bin/env python3
"""Verify a portable W11 Steam acceptance kit and its nested cooked package."""

import argparse
import hashlib
import json
import sys
import tempfile
from pathlib import Path, PurePosixPath

sys.dont_write_bytecode = True

from verify_w11_steam_package_manifest import (
    PackageManifestError,
    verify as verify_package_manifest,
)


class KitManifestError(RuntimeError):
    pass


REQUIRED_TOOLS = {
    "Tools/RunW11SteamAcceptance.ps1",
    "Tools/TestW11SteamAcceptanceEnvironment.ps1",
    "Tools/verify_w11_steam_acceptance_logs.py",
    "Tools/verify_w11_steam_package_manifest.py",
    "Tools/verify_w11_steam_acceptance_kit.py",
    "README_W11_STEAM_ACCEPTANCE.md",
}
SUPPORTED_SCENARIOS = [
    "BasicJoin",
    "FriendInvite",
    "HostExit",
    "ClientDisconnect",
    "PlayerSync",
]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def safe_path(root: Path, relative: str) -> Path:
    logical = PurePosixPath(relative)
    if logical.is_absolute() or ".." in logical.parts or not logical.parts:
        raise KitManifestError(f"unsafe kit path: {relative!r}")
    resolved = (root / Path(*logical.parts)).resolve()
    try:
        resolved.relative_to(root.resolve())
    except ValueError as error:
        raise KitManifestError(f"kit path escapes root: {relative!r}") from error
    return resolved


def verify(manifest_path: Path) -> tuple[int, int]:
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    except (OSError, json.JSONDecodeError) as error:
        raise KitManifestError(f"cannot read kit manifest: {error}") from error
    schema = manifest.get("schema")
    if schema not in (1, 2, 3) or manifest.get("kind") != "W11SteamAcceptanceKit":
        raise KitManifestError("unsupported W11 Steam acceptance kit manifest")
    if manifest.get("runtime") != "CookedGame" or manifest.get("platform") != "Win64":
        raise KitManifestError("kit is not a Win64 CookedGame acceptance kit")
    if schema >= 2 and manifest.get("supportedScenarios") != SUPPORTED_SCENARIOS:
        raise KitManifestError("kit scenario contract is incomplete or out of order")
    if schema == 3:
        if manifest.get("fullInventorySealed") is not True:
            raise KitManifestError("schema 3 kit must seal its full immutable inventory")
        if manifest.get("mutablePaths") != ["Saved/"]:
            raise KitManifestError("schema 3 kit mutable paths must be exactly Saved/")
        if manifest.get("launchContract") != "RunnerOnlyUserDirOutsidePackage":
            raise KitManifestError(
                "schema 3 kit must require the runner-only external UserDir launch contract"
            )
        if manifest.get("verificationContract") != "PreLaunchAndPostExitFullInventory":
            raise KitManifestError(
                "schema 3 kit must require pre-launch and post-exit full inventory verification"
            )

    root = manifest_path.parent
    package_relative = manifest.get("packageManifest")
    if package_relative != "Package/W11SteamPackageManifest.json":
        raise KitManifestError("kit package manifest must use the portable Package location")
    package_manifest = safe_path(root, package_relative)
    if not package_manifest.is_file():
        raise KitManifestError("nested package manifest is missing")
    if sha256(package_manifest) != str(manifest.get("packageManifestSha256", "")).lower():
        raise KitManifestError("nested package manifest SHA-256 mismatch")
    try:
        _, containers = verify_package_manifest(
            package_manifest,
            require_full_inventory=True,
            require_process_target=True,
        )
    except PackageManifestError as error:
        raise KitManifestError(f"nested package failed validation: {error}") from error

    artifacts = manifest.get("artifacts")
    if not isinstance(artifacts, list) or not artifacts:
        raise KitManifestError("kit tool artifact list is empty")
    paths = set()
    for artifact in artifacts:
        relative = artifact.get("path")
        if not isinstance(relative, str) or relative in paths:
            raise KitManifestError(f"invalid or duplicate kit artifact: {relative!r}")
        paths.add(relative)
        path = safe_path(root, relative)
        if not path.is_file():
            raise KitManifestError(f"missing kit artifact: {relative}")
        if path.stat().st_size != artifact.get("bytes"):
            raise KitManifestError(f"kit artifact size mismatch: {relative}")
        if sha256(path) != str(artifact.get("sha256", "")).lower():
            raise KitManifestError(f"kit artifact SHA-256 mismatch: {relative}")
    missing = REQUIRED_TOOLS - paths
    if missing:
        raise KitManifestError(f"required portable tools are not sealed: {sorted(missing)}")
    if schema == 3:
        if manifest.get("artifactCount") != len(artifacts):
            raise KitManifestError("kit artifact count does not match the sealed inventory")
        actual_paths = set()
        manifest_resolved = manifest_path.resolve()
        for path in root.rglob("*"):
            if not path.is_file() or path.resolve() == manifest_resolved:
                continue
            relative = path.relative_to(root).as_posix()
            if relative.startswith("Package/") or relative.startswith("Saved/"):
                continue
            actual_paths.add(relative)
        if actual_paths != paths:
            missing_paths = sorted(paths - actual_paths)
            unexpected_paths = sorted(actual_paths - paths)
            raise KitManifestError(
                "kit full inventory mismatch: "
                f"missing={missing_paths}, unexpected={unexpected_paths}"
            )
    return len(artifacts), containers


def write_artifact(path: Path, data: bytes) -> dict:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    return {
        "path": path.as_posix(),
        "bytes": path.stat().st_size,
        "sha256": sha256(path),
    }


def self_test() -> None:
    with tempfile.TemporaryDirectory(prefix="w11_steam_kit_") as temporary:
        root = Path(temporary)
        package_paths = (
            "Windows/WorldWalkerPrototype.exe",
            "Windows/WorldWalkerPrototype/Binaries/Win64/WorldWalkerPrototype.exe",
            "Windows/WorldWalkerPrototype/Content/Paks/test.pak",
            "Windows/WorldWalkerPrototype/Content/Paks/test.utoc",
            "Windows/WorldWalkerPrototype/Content/Paks/test.ucas",
        )
        package_artifacts = []
        package_root = root / "Package"
        for index, relative in enumerate(package_paths):
            path = package_root / Path(*PurePosixPath(relative).parts)
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(f"package-{index}".encode("ascii"))
            package_artifacts.append(
                {"path": relative, "bytes": path.stat().st_size, "sha256": sha256(path)}
            )
        package_manifest = package_root / "W11SteamPackageManifest.json"
        package_manifest.write_text(
            json.dumps(
                {
                    "schema": 4,
                    "runtime": "CookedGame",
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
                }
            ),
            encoding="utf-8",
        )

        artifact_paths = sorted(REQUIRED_TOOLS)
        kit_artifacts = []
        for index, relative in enumerate(artifact_paths):
            path = root / Path(*PurePosixPath(relative).parts)
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(f"tool-{index}".encode("ascii"))
            kit_artifacts.append(
                {"path": relative, "bytes": path.stat().st_size, "sha256": sha256(path)}
            )
        kit_manifest = root / "W11SteamAcceptanceKitManifest.json"
        kit_manifest.write_text(
            json.dumps(
                {
                    "schema": 3,
                    "kind": "W11SteamAcceptanceKit",
                    "runtime": "CookedGame",
                    "platform": "Win64",
                    "packageManifest": "Package/W11SteamPackageManifest.json",
                    "packageManifestSha256": sha256(package_manifest),
                    "supportedScenarios": SUPPORTED_SCENARIOS,
                    "fullInventorySealed": True,
                    "launchContract": "RunnerOnlyUserDirOutsidePackage",
                    "verificationContract": "PreLaunchAndPostExitFullInventory",
                    "artifactCount": len(kit_artifacts),
                    "mutablePaths": ["Saved/"],
                    "artifacts": kit_artifacts,
                }
            ),
            encoding="utf-8",
        )
        if verify(kit_manifest) != (len(REQUIRED_TOOLS), 3):
            raise KitManifestError("self-test did not validate the intact portable kit")

        intact_manifest = kit_manifest.read_text(encoding="utf-8")
        missing_launch_contract = json.loads(intact_manifest)
        del missing_launch_contract["launchContract"]
        kit_manifest.write_text(json.dumps(missing_launch_contract), encoding="utf-8")
        try:
            verify(kit_manifest)
        except KitManifestError as error:
            if "launch contract" not in str(error):
                raise
        else:
            raise KitManifestError("self-test accepted a kit without the launch contract")
        kit_manifest.write_text(intact_manifest, encoding="utf-8")

        missing_verification_contract = json.loads(intact_manifest)
        del missing_verification_contract["verificationContract"]
        kit_manifest.write_text(json.dumps(missing_verification_contract), encoding="utf-8")
        try:
            verify(kit_manifest)
        except KitManifestError as error:
            if "pre-launch and post-exit" not in str(error):
                raise
        else:
            raise KitManifestError("self-test accepted a kit without dual inventory verification")
        kit_manifest.write_text(intact_manifest, encoding="utf-8")

        tool = root / "Tools" / "RunW11SteamAcceptance.ps1"
        original_tool = tool.read_bytes()
        tool.write_bytes(b"tampered")
        try:
            verify(kit_manifest)
        except KitManifestError as error:
            if "mismatch" not in str(error):
                raise
        else:
            raise KitManifestError("self-test accepted a tampered runner")
        tool.write_bytes(original_tool)

        unexpected = root / "extra.exe"
        unexpected.write_bytes(b"not-sealed")
        try:
            verify(kit_manifest)
        except KitManifestError as error:
            if "unexpected" not in str(error):
                raise
        else:
            raise KitManifestError("self-test accepted an unexpected immutable kit file")
        unexpected.unlink()

        saved_evidence = root / "Saved" / "evidence.log"
        saved_evidence.parent.mkdir(parents=True, exist_ok=True)
        saved_evidence.write_text("runtime evidence\n", encoding="utf-8")
        if verify(kit_manifest) != (len(REQUIRED_TOOLS), 3):
            raise KitManifestError("self-test rejected the declared mutable Saved directory")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path, nargs="?")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    try:
        if args.self_test:
            self_test()
            print(
                "W11_STEAM_ACCEPTANCE_KIT_SELF_TEST_PASSED "
                "PortableLayout=1 NestedPackage=1 ToolTamperRejected=1 "
                "UnexpectedFileRejected=1 MutableSavedAllowed=1 FullInventory=1 "
                "LaunchContractRejected=1 VerificationContractRejected=1 Scenarios=5 "
                "EnvironmentPreflight=1"
            )
            return 0
        if args.manifest is None:
            raise KitManifestError("a kit manifest path is required")
        manifest_path = args.manifest.resolve()
        artifacts, containers = verify(manifest_path)
        manifest_schema = json.loads(
            manifest_path.read_text(encoding="utf-8-sig")
        ).get("schema")
    except (KitManifestError, PackageManifestError) as error:
        print(f"W11_STEAM_ACCEPTANCE_KIT_FAILED: {error}")
        return 1
    print(
        "W11_STEAM_ACCEPTANCE_KIT_PASSED "
        f"Runtime=CookedGame Platform=Win64 ToolArtifacts={artifacts} "
        f"Containers={containers} NestedPackageHash=1 "
        f"Scenarios={5 if manifest_schema >= 2 else 1} "
        f"FullInventory={1 if manifest_schema >= 3 else 0} "
        f"LaunchContract={'RunnerOnly' if manifest_schema >= 3 else 'Legacy'} "
        f"VerificationContract={'DualInventory' if manifest_schema >= 3 else 'Legacy'} "
        f"EnvironmentPreflight={'1' if manifest_schema >= 3 else '0'}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
