#!/usr/bin/env python3

import os
import sys
import subprocess
import argparse
import glob
import fnmatch
from pathlib import Path

EXTENSION_LANG = {
    '.ts': 'ts',
    '.js': 'js',
    '.py': 'python',
    '.cpp': 'cpp',
    '.hpp': 'cpp',
    '.h': 'c',
    '.c': 'c',
    '.java': 'java',
    '.cs': 'csharp',
    '.html': 'html',
    '.css': 'css',
    '.json': 'json',
    '.sh': 'bash',
    '.md': 'markdown',
}

def get_md_language(extension: str) -> str:
    return EXTENSION_LANG.get(extension.lower(), '')

def should_omit_path(path: Path, omitted_patterns: list[str]) -> bool:
    path_str = str(path)
    return any(fnmatch.fnmatch(path_str, pattern) for pattern in omitted_patterns)

def dump_file(file_path: Path, output, base: Path = None):
    extension = file_path.suffix
    if extension not in EXTENSION_LANG:
        return
    lang = get_md_language(extension)
    rel_path = file_path.relative_to(base) if base else file_path
    output.write(f"- `{rel_path}`\n")
    output.write(f"```{lang}\n")
    try:
        with file_path.open('r', encoding='utf-8', errors='ignore') as f:
            output.write(f.read())
    except Exception as e:
        output.write(f"// Error reading file: {e}")
    output.write("\n```\n\n")

def dump_directory(path: Path, output, omitted_patterns: list[str]):
    output.write(f"\n## {path.name}\n\n")
    for root, _, files in os.walk(path):
        root_path = Path(root)
        if should_omit_path(root_path, omitted_patterns):
            continue
        for file in files:
            file_path = root_path / file
            if should_omit_path(file_path, omitted_patterns):
                continue
            dump_file(file_path, output, path)

def copy_to_clipboard(file_path: Path):
    try:
        if sys.platform == "darwin":
            subprocess.run(["pbcopy"], input=file_path.read_bytes(), check=True)
        elif sys.platform.startswith("linux"):
            subprocess.run(["xclip", "-selection", "clipboard"], input=file_path.read_bytes(), check=True)
        elif sys.platform == "win32":
            subprocess.run("clip", input=file_path.read_bytes(), shell=True, check=True)
        print(f"> Fichier copié dans le presse-papiers ({file_path})")
    except Exception as e:
        print(f"[!] Impossible de copier dans le presse-papiers : {e}")

def main():
    parser = argparse.ArgumentParser(description="Dump code files into a markdown document.")
    parser.add_argument("paths", nargs="+", help="Directories, files, or glob patterns to process")
    parser.add_argument("-o", "--omit", nargs="*", default=[], help="Glob patterns to omit (e.g. '**/*.spec.ts')")
    parser.add_argument("-c", "--copy", action="store_true", help="Copy the result to clipboard")
    args = parser.parse_args()

    resolved_paths = set()
    for pattern in args.paths:
        matched = glob.glob(pattern, recursive=True)
        if not matched:
            print(f"[!] No match for pattern: {pattern}")
        resolved_paths.update(Path(p) for p in matched)

    out_file = Path("dump.md")
    with out_file.open("w", encoding='utf-8') as out:
        out.write("# FILES\n")
        for p in sorted(resolved_paths):
            if should_omit_path(p, args.omit):
                continue
            if p.is_dir():
                dump_directory(p, out, args.omit)
            elif p.is_file():
                dump_file(p, out)
            else:
                print(f"Skipping: {p} is not a valid file or directory.")

    if args.copy:
        copy_to_clipboard(out_file)

if __name__ == "__main__":
    main()
