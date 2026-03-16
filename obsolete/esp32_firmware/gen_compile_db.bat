@echo off
REM Regenerate compile_commands.json and add .ino entry for clangd
pio run -t compiledb
python fix_compile_db.py
