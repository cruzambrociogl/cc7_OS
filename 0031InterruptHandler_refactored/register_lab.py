#!/usr/bin/env python3
"""Register this lab in VS Code tasks.json and launch.json.

Place in the lab's root directory alongside build_and_run.sh and run:
    python register_lab.py

Adds QEMU Build, Beagle Deploy tasks and QEMU Run/Debug launch configs.
Safe to re-run — skips entries that already exist.

Note: JSONC comments in tasks.json/launch.json will be removed on first run.
"""

import json
import os
import re
import sys


def strip_jsonc(text):
    """Remove // and /* */ comments and trailing commas from JSONC."""
    result = []
    i = 0
    in_string = False
    while i < len(text):
        if text[i] == '"' and (i == 0 or text[i - 1] != '\\'):
            in_string = not in_string
            result.append(text[i])
            i += 1
        elif not in_string and text[i:i + 2] == '//':
            while i < len(text) and text[i] != '\n':
                i += 1
        elif not in_string and text[i:i + 2] == '/*':
            i += 2
            while i < len(text) - 1 and text[i:i + 2] != '*/':
                i += 1
            i += 2
        else:
            result.append(text[i])
            i += 1
    text = ''.join(result)
    text = re.sub(r',(\s*[}\]])', r'\1', text)
    return text


def read_jsonc(path):
    """Read a JSONC file, strip comments, return parsed data."""
    if not os.path.exists(path):
        return None
    with open(path, 'r', encoding='utf-8') as f:
        return json.loads(strip_jsonc(f.read()))


def write_json(path, data):
    """Write JSON with 4-space indent."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=4, ensure_ascii=False)
        f.write('\n')


def main():
    lab_dir = os.path.dirname(os.path.abspath(__file__))
    workspace_root = os.path.dirname(lab_dir)
    lab_folder = os.path.basename(lab_dir)

    # Extract numeric prefix (e.g., "003" from "003InterruptHandler_qemu")
    match = re.match(r'^(\d+)', lab_folder)
    if not match:
        print(f"ERROR: Folder '{lab_folder}' must start with a number (e.g., 005NewLab)")
        sys.exit(1)
    lab_num = match.group(1)

    vscode_dir = os.path.join(workspace_root, '.vscode')
    tasks_path = os.path.join(vscode_dir, 'tasks.json')
    launch_path = os.path.join(vscode_dir, 'launch.json')
    rel_path = lab_folder

    print(f"Lab:       {lab_folder}")
    print(f"Prefix:    {lab_num}")
    print(f"Workspace: {workspace_root}")
    print()

    # Check what scripts exist in this lab
    has_build = os.path.exists(os.path.join(lab_dir, 'build_and_run.sh'))
    has_deploy = os.path.exists(os.path.join(lab_dir, 'deploy_beagle.sh'))

    if not has_build:
        print("WARNING: build_and_run.sh not found — skipping QEMU tasks/launches")
    if not has_deploy:
        print("WARNING: deploy_beagle.sh not found — skipping Beagle task")

    # Cross-platform Stop QEMU command (detects OS at runtime, not registration time)
    stop_cmd = (
        "if command -v taskkill &>/dev/null; then taskkill /F /IM qemu-system-arm.exe; "
        "else killall qemu-system-arm; fi && echo 'QEMU stopped' || echo 'QEMU not running'"
    )

    # ---- Update tasks.json ----
    tasks_data = read_jsonc(tasks_path) or {"version": "2.0.0", "tasks": []}
    existing_labels = {t["label"] for t in tasks_data["tasks"]}
    added_tasks = []

    build_label = f"{lab_num} QEMU: Build"
    deploy_label = f"{lab_num} Beagle: Build & Deploy"

    new_tasks = []
    if has_build and build_label not in existing_labels:
        new_tasks.append({
            "label": build_label,
            "type": "shell",
            "command": "./build_and_run.sh qemu_build_only",
            "options": {"cwd": f"${{workspaceFolder}}/{rel_path}"},
            "group": "build",
            "problemMatcher": ["$gcc"]
        })
        added_tasks.append(build_label)

    if has_deploy and deploy_label not in existing_labels:
        new_tasks.append({
            "label": deploy_label,
            "type": "shell",
            "command": "./deploy_beagle.sh",
            "options": {"cwd": f"${{workspaceFolder}}/{rel_path}"},
            "problemMatcher": ["$gcc"],
            "presentation": {"reveal": "always", "panel": "dedicated", "focus": True}
        })
        added_tasks.append(deploy_label)

    # Insert new tasks before Stop QEMU (keep it last)
    stop_idx = next(
        (i for i, t in enumerate(tasks_data["tasks"]) if t["label"] == "Stop QEMU"),
        None
    )
    if stop_idx is not None:
        for i, task in enumerate(new_tasks):
            tasks_data["tasks"].insert(stop_idx + i, task)
    else:
        tasks_data["tasks"].extend(new_tasks)
        tasks_data["tasks"].append({
            "label": "Stop QEMU",
            "type": "shell",
            "command": stop_cmd,
            "problemMatcher": []
        })
        added_tasks.append("Stop QEMU")

    write_json(tasks_path, tasks_data)

    # ---- Update launch.json ----
    launch_data = read_jsonc(launch_path) or {"version": "0.2.0", "configurations": []}
    existing_names = {c["name"] for c in launch_data["configurations"]}
    added_launches = []

    run_name = f"{lab_num} QEMU: Run"
    debug_name = f"{lab_num} QEMU: Debug"

    base_config = {
        "type": "cortex-debug",
        "request": "launch",
        "servertype": "qemu",
        "cwd": f"${{workspaceFolder}}/{rel_path}",
        "executable": "bin/program.elf",
        "gdbPath": "arm-none-eabi-gdb",
        "qemuPath": "qemu-system-arm",
        "cpu": "arm926",
        "machine": "versatilepb",
        "qemuArgs": ["-nographic"],
        "preLaunchTask": build_label,
        "device": "ARM926EJ-S",
    }

    if has_build and run_name not in existing_names:
        launch_data["configurations"].append(
            {"name": run_name, **base_config, "stopAtEntry": False}
        )
        added_launches.append(run_name)

    if has_build and debug_name not in existing_names:
        launch_data["configurations"].append(
            {"name": debug_name, **base_config, "stopAtEntry": True}
        )
        added_launches.append(debug_name)

    write_json(launch_path, launch_data)

    # ---- Summary ----
    print("tasks.json:")
    for t in added_tasks:
        print(f"  + {t}")
    if not added_tasks:
        print("  (no changes)")

    print()
    print("launch.json:")
    for l in added_launches:
        print(f"  + {l}")
    if not added_launches:
        print("  (no changes)")

    if added_tasks or added_launches:
        print()
        print("Done! Reload VS Code window to pick up changes.")
    else:
        print()
        print(f"Lab {lab_num} already registered — nothing to do.")


if __name__ == "__main__":
    main()
