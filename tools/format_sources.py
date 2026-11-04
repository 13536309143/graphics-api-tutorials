"""Format course-owned C++, GLSL/HLSL and CMake; never enter dependency trees."""
from pathlib import Path
import argparse
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCE_SUFFIXES = {".cpp", ".hpp", ".h", ".vert", ".frag", ".geom", ".comp", ".hlsl"}
EXCLUDED = {"third_party", "external", "build", "build-local", ".git", ".cache", "dist", "backups", ".venv", "venv"}


def source_files(directory):
    # Prune dependency/build folders before recursion, including submodule contents.
    for path in sorted(directory.iterdir()):
        if path.name in EXCLUDED or path.name.startswith("build-"):
            continue
        if path.is_dir() and not path.is_symlink():
            yield from source_files(path)
        elif path.is_file() and (path.suffix in SOURCE_SUFFIXES | {".cmake"} or path.name == "CMakeLists.txt"):
            yield path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Report differences without changing files")
    parser.add_argument("--clang-format", default="clang-format", help="Path to clang-format 19.1.7")
    args = parser.parse_args()
    clang = shutil.which(args.clang_format)
    if not clang:
        parser.error("Install tools first: python -m pip install -r tools/requirements-format.txt")
    version = subprocess.check_output([clang, "--version"], text=True)
    if "version 19.1.7" not in version:
        parser.error("Use clang-format 19.1.7 for reproducible formatting")
    try:
        import cmakelang
    except ImportError:
        parser.error("Install tools first: python -m pip install -r tools/requirements-format.txt")
    changed = []
    files = list(source_files(ROOT))
    for path in files:
        original = path.read_bytes()
        if path.suffix in SOURCE_SUFFIXES:
            # clang-format parses GLSL/HLSL using its C++-compatible layout grammar.
            assumed = path if path.suffix in {".cpp", ".hpp", ".h"} else path.with_suffix(".cpp")
            command = [clang, "--style=file:" + str(ROOT / ".clang-format"), "--assume-filename=" + str(assumed)]
        else:
            command = [sys.executable, "-m", "cmakelang.format", "--config-files", str(ROOT / ".cmake-format.json"), "--", "-"]
        formatted = subprocess.check_output(command, input=original).replace(b"\r\n", b"\n")
        if formatted != original:
            changed.append(path.relative_to(ROOT).as_posix())
            if not args.check:
                path.write_bytes(formatted)
    for name in changed:
        print(("Needs format: " if args.check else "Formatted: ") + name)
    print(f"Checked {len(files)} files; {len(changed)} {'need formatting' if args.check else 'updated'}.")
    return 1 if args.check and changed else 0


if __name__ == "__main__":
    raise SystemExit(main())
