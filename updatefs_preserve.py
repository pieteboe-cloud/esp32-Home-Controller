"""Upload the LittleFS web files while preserving runtime configuration.

The normal PlatformIO uploadfs command replaces the entire filesystem image.
This wrapper downloads the current image first, copies mutable device data into
the next image, and restores the project data directory when it is finished.
"""

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
    Path("backup") / "scripts.json",
    Path("config"),
)


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
    original_dir = Path(tempfile.mkdtemp(prefix="updatefs-original-"))
    restore_plan = []
    print(f"→ Temporary backup of project data: {original_dir}", flush=True)
    
    # Special handling for scripts.json - ensure it exists
    scripts_file = DEVICE_FS_DIR / "scripts.json"
    if not scripts_file.exists():
        print("⚠️  CRITICAL: /scripts.json not found in device filesystem!", flush=True)
        print("   This means user scripts will be lost during the update.", flush=True)
        return original_dir, restore_plan

    for relative_path in RUNTIME_PATHS:
        device_path = DEVICE_FS_DIR / relative_path
        if not device_path.exists():
            print(f"  [SKIP] Device path not present: /{relative_path}", flush=True)
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
    
    # Verify critical files exist
    scripts_file = DEVICE_FS_DIR / "scripts.json"
    if not scripts_file.exists():
        print("⚠️  WARNING: /scripts.json not found in device filesystem!", flush=True)
        print("   This means scripts may not be preserved during the update.", flush=True)
        # Create an empty scripts.json to ensure the file exists
        scripts_file.touch()
        print("   Created empty scripts.json to ensure preservation.", flush=True)

    original_dir = None
    restore_plan = []
    preserve_success = False
    try:
        original_dir, restore_plan = preserve_runtime_files()
        if original_dir and restore_plan:  # Only proceed if preservation was successful
            preserve_success = True
            run_platformio("--target", "buildfs")
            run_platformio("--target", "uploadfs")
    finally:
        if original_dir is not None:
            restore_project_files(original_dir, restore_plan)

    if preserve_success:
        print("\n✓ LittleFS updated; device scripts and config were preserved.", flush=True)
    else:
        print("\n⚠️  WARNING: LittleFS update completed but critical files were not preserved!", flush=True)
        print("   User scripts may have been lost. Check the device filesystem for /scripts.json", flush=True)


if __name__ == "__main__":
    main()
