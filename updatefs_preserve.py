"""Upload the LittleFS web files while preserving runtime scripts and scenes.

PlatformIO's uploadfs replaces the entire LittleFS image. This wrapper first
downloads the current device filesystem, copies the runtime-owned JSON files
into data/, builds and uploads the new filesystem, then restores the original
project data files.

Runtime-owned files:
    /scripts.json
    /scenes.json
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

# Only these files are runtime-owned and must survive an uploadfs.
RUNTIME_PATHS = (
    Path("scripts.json"),
    Path("scenes.json"),
)


def is_valid_json_file(path):
    """Return True if the file exists, is non-empty, and contains valid JSON."""
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

    subprocess.run(
        command,
        cwd=PROJECT_DIR,
        check=True,
    )


def copy_path(source, destination):
    """Copy a file or directory, replacing the destination if necessary."""
    if source.is_dir():
        shutil.copytree(
            source,
            destination,
            dirs_exist_ok=True,
        )
    else:
        destination.parent.mkdir(
            parents=True,
            exist_ok=True,
        )
        shutil.copy2(
            source,
            destination,
        )


def preserve_runtime_files():
    """Copy device runtime JSON files into data/ temporarily."""

    original_dir = Path(
        tempfile.mkdtemp(prefix="updatefs-original-")
    )

    restore_plan = []

    try:
        print(
            f"→ Temporary backup of project data: {original_dir}",
            flush=True,
        )

        for relative_path in RUNTIME_PATHS:

            device_path = DEVICE_FS_DIR / relative_path

            # ---------------------------------------------------------
            # Device file does not exist
            # ---------------------------------------------------------
            if not device_path.exists():
                print(
                    f"  [SKIP] Device path not present: /{relative_path}",
                    flush=True,
                )
                continue

            # ---------------------------------------------------------
            # Device file exists but is invalid
            # ---------------------------------------------------------
            if not is_valid_json_file(device_path):
                print(
                    f"  [ERROR] /{relative_path} is missing, empty, "
                    f"or invalid JSON!",
                    flush=True,
                )

                print(
                    f"          Refusing to overwrite project "
                    f"version of /{relative_path}.",
                    flush=True,
                )

                raise RuntimeError(
                    f"Invalid device runtime file: /{relative_path}"
                )

            # ---------------------------------------------------------
            # Preserve current project version
            # ---------------------------------------------------------
            project_path = DATA_DIR / relative_path
            original_path = original_dir / relative_path

            if project_path.exists():
                copy_path(
                    project_path,
                    original_path,
                )

                restore_plan.append(
                    (
                        project_path,
                        original_path,
                        True,
                    )
                )
            else:
                restore_plan.append(
                    (
                        project_path,
                        None,
                        False,
                    )
                )

            # ---------------------------------------------------------
            # Replace project runtime file with device version
            # ---------------------------------------------------------
            copy_path(
                device_path,
                project_path,
            )

            print(
                f"  [COPY] /{relative_path} -> {project_path}",
                flush=True,
            )

            print(
                f"  [VERIFY] {project_path} "
                f"({project_path.stat().st_size} bytes)",
                flush=True,
            )

        return original_dir, restore_plan

    except BaseException:

        restore_project_files(
            original_dir,
            restore_plan,
        )

        raise


def restore_project_files(original_dir, restore_plan):
    """Restore the developer's original data/ files."""

    print(
        "→ Restoring the original project data/ files...",
        flush=True,
    )

    for project_path, original_path, existed in reversed(
        restore_plan
    ):

        # Project file didn't exist before the update.
        if not existed:

            if project_path.is_dir():
                shutil.rmtree(project_path)

            elif project_path.exists():
                project_path.unlink()

        # Project file existed before the update.
        else:

            if project_path.is_dir():
                shutil.rmtree(project_path)

            elif project_path.exists():
                project_path.unlink()

            copy_path(
                original_path,
                project_path,
            )

        print(
            f"  [RESTORE] {project_path}",
            flush=True,
        )

    shutil.rmtree(
        original_dir,
        ignore_errors=True,
    )

    print(
        "  [CLEAN] Removed temporary project backup",
        flush=True,
    )


def main():

    print(
        "=== Preserve runtime LittleFS data ===",
        flush=True,
    )

    print(
        f"Project: {PROJECT_DIR}",
        flush=True,
    )

    print(
        "Runtime paths: "
        + ", ".join(
            "/" + str(path)
            for path in RUNTIME_PATHS
        ),
        flush=True,
    )

    # -------------------------------------------------------------
    # Download current filesystem
    # -------------------------------------------------------------

    print(
        "→ Downloading the current device filesystem...",
        flush=True,
    )

    run_platformio(
        "--target",
        "download_fs",
    )

    # -------------------------------------------------------------
    # Verify runtime files before touching data/
    # -------------------------------------------------------------

    print(
        "\n→ Checking device runtime files...",
        flush=True,
    )

    for relative_path in RUNTIME_PATHS:

        device_path = DEVICE_FS_DIR / relative_path

        if not device_path.exists():

            print(
                f"  [WARNING] /{relative_path} does not exist.",
                flush=True,
            )

        elif not is_valid_json_file(device_path):

            print(
                f"  [ERROR] /{relative_path} is empty or invalid JSON.",
                flush=True,
            )

            print(
                "  ✗ Aborting before build/upload.",
                flush=True,
            )

            return

        else:

            print(
                f"  [OK] /{relative_path}",
                flush=True,
            )

    original_dir = None
    restore_plan = []
    preserve_success = False

    try:

        # ---------------------------------------------------------
        # Copy runtime files from device -> data/
        # ---------------------------------------------------------

        original_dir, restore_plan = preserve_runtime_files()

        preserve_success = True

        # ---------------------------------------------------------
        # Build filesystem
        # ---------------------------------------------------------

        run_platformio(
            "--target",
            "buildfs",
        )

        # ---------------------------------------------------------
        # Upload filesystem
        # ---------------------------------------------------------

        run_platformio(
            "--target",
            "uploadfs",
        )

    except Exception as exc:

        print(
            f"\n✗ Update failed: {exc}",
            flush=True,
        )

    finally:

        if original_dir is not None:

            restore_project_files(
                original_dir,
                restore_plan,
            )

    if preserve_success:

        print(
            "\n✓ LittleFS updated successfully.",
            flush=True,
        )

        print(
            "  Runtime scripts.json and scenes.json were preserved.",
            flush=True,
        )

    else:

        print(
            "\n✗ LittleFS update was not completed.",
            flush=True,
        )


if __name__ == "__main__":
    main()
    