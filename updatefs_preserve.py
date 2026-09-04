"""Upload the LittleFS web files while preserving runtime configuration.

The normal PlatformIO uploadfs command replaces the entire filesystem image.
This wrapper downloads the current image first, copies mutable device data into
the next image, and restores the project data directory when it is finished.
"""

import json
import shutil
import subprocess
import tempfile
from pathlib import Path

PROJECT_DIR = Path(__file__).resolve().parent
DATA_DIR = PROJECT_DIR / "data"
DEVICE_FS_DIR = PROJECT_DIR / "unpacked_fs"
ENVIRONMENT = "esp32doit-devkit-v1"

# These files are created or changed by the running firmware. Web assets and IR
# databases remain controlled by the project source and are not overwritten.
RUNTIME_PATHS = (
    Path("scripts.json"),
    Path("scenes.json"),      # SceneManager writes here after the scripts.json migration
    Path("backup") / "scripts.json",
    Path("config"),
)


def is_valid_json_file(path):
    """Return True if the file exists, is non-empty, and parses as JSON."""
    if not path.exists() or path.stat().st_size == 0:
        return False
    try:
        with open(path, "r", encoding="utf-8") as f:
            json.load(f)
        return True
    except (json.JSONDecodeError, OSError):
        return False


def run_platformio(*args):
    """Run PlatformIO from the project directory and stop on errors."""
    command = ["pio", "run", "--environment", ENVIRONMENT, *args]
    print("\n→ Running:", " ".join(command), flush=True)
    subprocess.run(command, cwd=PROJECT_DIR, check=True)


def copy_path(source, destination):
    """Copy a file or directory, replacing the destination if necessary."""
    if source.is_dir():
        shutil.copytree(source, destination, dirs_exist_ok=True)
    else:
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)


def preserve_runtime_files():
    """Temporarily merge device-owned files into data/ and return a restore plan."""
    # created here (not in main) so any exception is reported with a valid backup dir
    original_dir = Path(tempfile.mkdtemp(prefix="updatefs-original-"))
    restore_plan = []
    try:
        print(f"→ Temporary backup of project data: {original_dir}", flush=True)

        # scripts.json is the firmware's primary mutable state and MUST be
        # present AND non-empty, otherwise user data would be lost in the update.
        # scenes.json is optional: empty/missing is fine because the firmware
        # falls back to scripts.json when scenes.json is empty.
        scripts_file = DEVICE_FS_DIR / "scripts.json"
        if not is_valid_json_file(scripts_file):
            print("⚠️  CRITICAL: /scripts.json missing or empty in device filesystem!", flush=True)
            print("   This means user scripts would be lost during the update. Aborting.", flush=True)
            return original_dir, []

        for relative_path in RUNTIME_PATHS:
            device_path = DEVICE_FS_DIR / relative_path
            if not device_path.exists():
                print(f"  [SKIP] Device path not present: /{relative_path}", flush=True)
                continue

            # Skip empty/invalid optional files so a 0-byte scenes.json does not
            # get baked into the new image (harmless but pointless).
            if device_path.is_file() and not is_valid_json_file(device_path):
                print(f"  [SKIP] /{relative_path} is empty or invalid JSON; not preserving", flush=True)
                continue

            project_path = DATA_DIR / relative_path
            original_path = original_dir / relative_path
            if project_path.exists():
                copy_path(project_path, original_path)
                restore_plan.append((project_path, original_path, True))
            else:
                restore_plan.append((project_path, None, False))

            copy_path(device_path, project_path)
            print(f"  [COPY] /{relative_path} -> {project_path}", flush=True)

            if project_path.is_file():
                print(f"  [VERIFY] {project_path} ({project_path.stat().st_size} bytes)", flush=True)
            else:
                file_count = sum(1 for item in project_path.rglob("*") if item.is_file())
                print(f"  [VERIFY] {project_path} ({file_count} files)", flush=True)

        return original_dir, restore_plan
    except BaseException:
        # Restore immediately if the merge itself fails mid-copy, so data/ is
        # never left containing a mix of device and project files.
        restore_project_files(original_dir, restore_plan)
        raise


def restore_project_files(original_dir, restore_plan):
    """Put the developer's original data/ files back after uploadfs."""
    print("→ Restoring the original project data/ files...", flush=True)
    for project_path, original_path, existed in reversed(restore_plan):
        if project_path.is_dir() and not existed:
            shutil.rmtree(project_path)
        elif project_path.exists() and not existed:
            project_path.unlink()
        elif existed:
            if project_path.is_dir():
                shutil.rmtree(project_path)
            else:
                project_path.unlink()
            copy_path(original_path, project_path)
        print(f"  [RESTORE] {project_path}", flush=True)

    shutil.rmtree(original_dir, ignore_errors=True)
    print("  [CLEAN] Removed temporary project backup", flush=True)


def main():
    print("=== Preserve runtime LittleFS data ===", flush=True)
    print(f"Project: {PROJECT_DIR}", flush=True)
    print(f"Runtime paths: {', '.join('/' + str(path) for path in RUNTIME_PATHS)}", flush=True)
    print("→ Downloading the current device filesystem...", flush=True)
    # If the serial port is busy or the download fails, uploadfs is never run.
    run_platformio("--target", "download_fs")

    # Informational check: scripts.json is critical, scenes.json is optional
    # (the firmware falls back to scripts.json when scenes.json is empty).
    if not is_valid_json_file(DEVICE_FS_DIR / "scripts.json"):
        print("⚠️  WARNING: /scripts.json missing or empty in device filesystem!", flush=True)
        print("   The update will be aborted before anything is written.", flush=True)

    original_dir = None
    restore_plan = []
    preserve_success = False
    try:
        original_dir, restore_plan = preserve_runtime_files()
        if original_dir and restore_plan:  # Only proceed if preservation was successful
            preserve_success = True
            run_platformio("--target", "buildfs")
            run_platformio("--target", "uploadfs")
        else:
            print("\n✗ Aborted: no runtime files were preserved; uploadfs was NOT run.", flush=True)
    finally:
        if original_dir is not None:
            restore_project_files(original_dir, restore_plan)

    if preserve_success:
        print("\n✓ LittleFS updated; device scripts, scenes and config were preserved.", flush=True)
    elif not original_dir or not restore_plan:
        # Aborted before uploadfs - nothing was written to the device, so no data can be lost.
        print("\n✗ Update aborted before uploadfs. Nothing was written; no data was lost.", flush=True)
        print("   Fix the critical-file problem above and run the script again.", flush=True)


if __name__ == "__main__":
    main()
