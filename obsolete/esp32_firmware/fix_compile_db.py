#!/usr/bin/env python3
"""Add esp32_firmware.ino to compile_commands.json so clangd can analyze it."""
import json
import sys

path = "compile_commands.json"
with open(path, encoding="utf-8") as f:
    db = json.load(f)

for entry in db:
    if entry.get("file") == "esp32_firmware.ino.cpp":
        new_entry = dict(entry)
        new_entry["file"] = "esp32_firmware.ino"
        new_entry["command"] = entry["command"].replace(
            "esp32_firmware.ino.cpp", "esp32_firmware.ino"
        )
        db.append(new_entry)
        with open(path, "w", encoding="utf-8") as f:
            json.dump(db, f, indent=4)
        print("Added esp32_firmware.ino to compile_commands.json")
        sys.exit(0)

print("esp32_firmware.ino.cpp entry not found")
sys.exit(1)
