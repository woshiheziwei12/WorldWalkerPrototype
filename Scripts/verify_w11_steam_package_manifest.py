#!/usr/bin/env python3
"""Verify a W11 Steam acceptance package without trusting its build machine paths."""

import argparse
import hashlib
import json
import tempfile
from pathlib import Path, PurePosixPath


class PackageManifestError(RuntimeError):
    pass


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def safe_artifact_path(package_root: Path, relative: str) -> Path:
    logical = PurePosixPath(relative)
    if logical.is_absolute() or ".." in logical.parts or not logical.parts:
        raise PackageManifestError(f"unsafe artifact path: {relative!r}")
    resolved = (package_root / Path(*logical.parts)).resolve()
    try:
        resolved.relative_to(package_root.resolve())
    except ValueError as error:
        raise PackageManifestError(f"artifact escapes package root: {relative!r}") from error
    return resolved


def verify(
    manifest_path: Path,
    require_full_inventory: bool = False,
    require_process_target: bool = False,
) -> tuple[int, int]:
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    except (OSError, json.JSONDecodeError) as error:
        raise PackageManifestError(f"cannot read manifest: {error}") from error

    schema = manifest.get("schema")
    if schema not in (2, 3, 4):
        raise PackageManifestError(f"unsupported schema: {manifest.get('schema')!r}")
    if require_full_inventory and schema < 3:
        raise PackageManifestError("formal acceptance requires schema 3+ full-inventory sealing")
    if require_process_target and schema != 4:
        raise PackageManifestError("formal acceptance requires schema 4 sealed process targeting")
    if schema >= 3 and manifest.get("fullInventorySealed") is not True:
        raise PackageManifestError(f"schema {schema} package does not declare full-inventory sealing")
    if schema == 4:
        if manifest.get("processTargetSealed") is not True:
            raise PackageManifestError("schema 4 package does not declare sealed process targeting")
        if manifest.get("bootstrapExecutable") != "Windows/WorldWalkerPrototype.exe":
            raise PackageManifestError("schema 4 bootstrap executable is not the canonical package launcher")
        if manifest.get("runtimeExecutable") != (
            "Windows/WorldWalkerPrototype/Binaries/Win64/WorldWalkerPrototype.exe"
        ):
            raise PackageManifestError("schema 4 runtime executable is not the canonical wait target")
    if manifest.get("runtime") != "CookedGame" or manifest.get("platform") != "Win64":
        raise PackageManifestError("manifest is not a Win64 CookedGame package")
    if manifest.get("acceptanceSteamAppIdEmbedded") is not False:
        raise PackageManifestError("the acceptance project App ID must not be embedded")
    if manifest.get("packagedDefaultPlatformService") != "Null":
        raise PackageManifestError("package default platform service must remain Null")
    if str(manifest.get("packagedDefaultSteamDevAppId")) != "480":
        raise PackageManifestError("package must record the rejected public default App ID 480")

    artifacts = manifest.get("artifacts")
    if not isinstance(artifacts, list) or not artifacts:
        raise PackageManifestError("manifest has no artifacts")
    package_root = manifest_path.parent
    paths = set()
    container_count = 0
    for artifact in artifacts:
        relative = artifact.get("path")
        if not isinstance(relative, str) or relative in paths:
            raise PackageManifestError(f"invalid or duplicate artifact path: {relative!r}")
        paths.add(relative)
        path = safe_artifact_path(package_root, relative)
        if not path.is_file():
            raise PackageManifestError(f"missing artifact: {relative}")
        if path.stat().st_size != artifact.get("bytes"):
            raise PackageManifestError(f"size mismatch: {relative}")
        if sha256(path) != str(artifact.get("sha256", "")).lower():
            raise PackageManifestError(f"SHA-256 mismatch: {relative}")
        if path.suffix.lower() in (".pak", ".utoc", ".ucas"):
            container_count += 1

    required_executables = {
        "Windows/WorldWalkerPrototype.exe",
        "Windows/WorldWalkerPrototype/Binaries/Win64/WorldWalkerPrototype.exe",
    }
    if not required_executables.issubset(paths):
        raise PackageManifestError("bootstrap or runtime executable is missing from the hash set")
    if container_count != manifest.get("contentContainerCount") or container_count < 3:
        raise PackageManifestError("content container count does not match the manifest")
    if any(PurePosixPath(item).name.lower() == "steam_appid.txt" for item in paths):
        raise PackageManifestError("package unexpectedly contains steam_appid.txt")
    if schema >= 3:
        if manifest.get("artifactCount") != len(artifacts):
            raise PackageManifestError("artifact count does not match the full inventory")
        actual_paths = {
            path.relative_to(package_root).as_posix()
            for path in package_root.rglob("*")
            if path.is_file() and path.resolve() != manifest_path.resolve()
        }
        if actual_paths != paths:
            missing = sorted(paths - actual_paths)
            unexpected = sorted(actual_paths - paths)
            raise PackageManifestError(
                f"full inventory mismatch: missing={missing} unexpected={unexpected}"
            )
    return len(artifacts), container_count


def self_test() -> None:
    with tempfile.TemporaryDirectory(prefix="w11_steam_package_") as temporary:
        root = Path(temporary)
        relative_paths = (
            "Windows/WorldWalkerPrototype.exe",
            "Windows/WorldWalkerPrototype/Binaries/Win64/WorldWalkerPrototype.exe",
            "Windows/WorldWalkerPrototype/Content/Paks/test.pak",
            "Windows/WorldWalkerPrototype/Content/Paks/test.utoc",
            "Windows/WorldWalkerPrototype/Content/Paks/test.ucas",
        )
        artifacts = []
        for index, relative in enumerate(relative_paths):
            path = safe_artifact_path(root, relative)
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(f"W11-test-{index}".encode("ascii"))
            artifacts.append(
                {
                    "path": relative,
                    "bytes": path.stat().st_size,
                    "sha256": sha256(path),
                }
            )
        manifest = root / "W11SteamPackageManifest.json"
        manifest.write_text(
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
                    "artifactCount": len(artifacts),
                    "artifacts": artifacts,
                }
            ),
            encoding="utf-8",
        )
        if verify(
            manifest, require_full_inventory=True, require_process_target=True
        ) != (5, 3):
            raise PackageManifestError("self-test did not validate the intact package")
        safe_artifact_path(root, relative_paths[-1]).write_bytes(b"tampered")
        try:
            verify(manifest)
        except PackageManifestError as error:
            if "mismatch" not in str(error):
                raise
        else:
            raise PackageManifestError("self-test accepted a tampered content container")
        safe_artifact_path(root, relative_paths[-1]).write_bytes(b"W11-test-4")
        manifest_data = json.loads(manifest.read_text(encoding="utf-8"))
        manifest_data["runtimeExecutable"] = "Windows/WorldWalkerPrototype.exe"
        manifest.write_text(json.dumps(manifest_data), encoding="utf-8")
        try:
            verify(manifest, require_process_target=True)
        except PackageManifestError as error:
            if "runtime executable" not in str(error):
                raise
        else:
            raise PackageManifestError("self-test accepted the bootstrap as the wait target")
        manifest_data["runtimeExecutable"] = (
            "Windows/WorldWalkerPrototype/Binaries/Win64/WorldWalkerPrototype.exe"
        )
        manifest.write_text(json.dumps(manifest_data), encoding="utf-8")
        unexpected = root / "Windows" / "runtime-generated.ini"
        unexpected.write_text("mutable", encoding="utf-8")
        try:
            verify(manifest, require_full_inventory=True)
        except PackageManifestError as error:
            if "unexpected" not in str(error):
                raise
        else:
            raise PackageManifestError("self-test accepted an unsealed extra package file")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path, nargs="?")
    parser.add_argument("--require-full-inventory", action="store_true")
    parser.add_argument("--require-process-target", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    try:
        if args.self_test:
            self_test()
            print(
                "W11_STEAM_PACKAGE_MANIFEST_SELF_TEST_PASSED "
                "IntactAccepted=1 TamperedContainerRejected=1 UnexpectedFileRejected=1 "
                "BootstrapWaitRejected=1 PathContainment=1 FullInventory=1 ProcessTarget=1"
            )
            return 0
        if args.manifest is None:
            raise PackageManifestError("a manifest path is required")
        artifacts, containers = verify(
            args.manifest.resolve(),
            require_full_inventory=args.require_full_inventory,
            require_process_target=args.require_process_target,
        )
    except PackageManifestError as error:
        print(f"W11_STEAM_PACKAGE_MANIFEST_FAILED: {error}")
        return 1
    print(
        "W11_STEAM_PACKAGE_MANIFEST_PASSED "
        f"Runtime=CookedGame Platform=Win64 Artifacts={artifacts} "
        f"Containers={containers} Hashes=SHA256 AcceptanceAppIdEmbedded=0 "
        f"Integrity={'FullInventory' if args.require_full_inventory else 'ManifestDeclared'} "
        f"ProcessTarget={'RuntimeExecutable' if args.require_process_target else 'ManifestDeclared'} "
        "PackagedDefault=Null/480"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
