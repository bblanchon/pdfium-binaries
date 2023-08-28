#!/usr/bin/env python
#
# Simple replacement for conda-build
# Builds a package according to the conda package specification
# https://docs.conda.io/projects/conda/en/latest/user-guide/concepts/pkg-specs.html
# https://docs.conda.io/projects/conda-build/en/stable/resources/package-spec.html

import json
import re
import tarfile
from argparse import ArgumentParser
from hashlib import sha256
from pathlib import Path
from tempfile import TemporaryDirectory

CONDA_PLATFORMS = {
    "linux-32": ("linux", "x86"),
    "linux-64": ("linux", "x86_64"),
    "linux-aarch64": ("linux", "aarch64"),
    "linux-armv6l": ("linux", "armv6l"),
    "linux-armv7l": ("linux", "armv7l"),
    "linux-ppc64": ("linux", "ppc64"),
    "linux-ppc64le": ("linux", "ppc64le"),
    "linux-s390x": ("linux", "s390x"),
    "osx-arm64": ("osx", "arm64"),
    "osx-64": ("osx", "x86_64"),
    "win-32": ("win", "x86"),
    "win-64": ("win", "x86_64"),
    "win-arm64": ("win", "arm64"),
}

SOURCE_GLOBS = [
    "bin/*.dll",
    "lib/*.so",
    "lib/*.lib",
    "lib/*.dylib",
    "include/**/*.h",
]


def create_json_file(filename: str, data: dict):
    with open(filename, "w") as file:
        json.dump(data, file, indent=2)


def conda_build(
    package: str,
    platform: str,
    version: str,
    build: str,
    build_number: int,
    source_dir: Path,
    output_dir: Path,
    about_file: Path,
):
    assert "-" not in build, f"Invalid build {build!r}"
    assert build_number > 0, f"Invalid build number {build_number}"
    assert source_dir.exists(), f"Source directory {source_dir} does not exist"
    assert re.match(
        r"^\d+\.\d+\.\d+(?:.\d+)?$", version
    ), f"Invalid version {version!r}"

    output_file = output_dir / platform / f"{package}-{version}-{build_number}.tar.bz2"
    output_file.parent.mkdir(parents=True, exist_ok=True)

    assert platform in CONDA_PLATFORMS, f"Unknown platform {platform!r}"
    operating_system, architecture = CONDA_PLATFORMS[platform]

    files: list[Path] = []

    with tarfile.open(output_file, "w:bz2") as tar:
        for glob in SOURCE_GLOBS:
            for file in source_dir.glob(glob):
                tar.add(file, file.relative_to(source_dir))
                files.append(file)
        assert files, f"No files found for globs {SOURCE_GLOBS}"

        with TemporaryDirectory() as info_dir:
            create_json_file(
                f"{info_dir}/index.json",
                {
                    "arch": architecture,
                    "build_number": build_number,
                    "build": build,
                    "depends": [],
                    "name": package,
                    "platform": operating_system,
                    "version": version,
                },
            )

            Path(info_dir, "files").write_text(
                "\n".join(
                    str(file.relative_to(source_dir).as_posix()) for file in files
                )
            )

            create_json_file(
                f"{info_dir}/paths.json",
                {
                    "paths": [
                        {
                            "_path": str(file.relative_to(source_dir).as_posix()),
                            "path_type": "hardlink",
                            "sha256": sha256(file.read_bytes()).hexdigest(),
                            "size_in_bytes": file.stat().st_size,
                        }
                        for file in files
                    ],
                    "paths_version": 1,
                },
            )

            if about_file:
                assert about_file.exists(), f"About file {about_file} does not exist"
                tar.add(about_file, "info/about.json")

            license_file = source_dir / "LICENSE"
            if license_file.exists():
                tar.add(license_file, "info/license.txt")

            tar.add(info_dir, "info")

    return output_file


def main():
    parser = ArgumentParser(description="Build conda packages.")
    parser.add_argument("package", help="Package name")
    parser.add_argument("platform", help="Platform")
    parser.add_argument("version", help="Version")
    parser.add_argument("--source-dir", help="Source directory", type=Path, default=".")
    parser.add_argument("--output-dir", help="Output directory", type=Path, default=".")
    parser.add_argument("--about-file", help="Path to about.json", type=Path)
    parser.add_argument("--build", help="The build string", default="")
    parser.add_argument("--build-number", help="The build number", type=int, default=1)

    args = parser.parse_args()

    output_file = conda_build(**vars(args))

    print(f"Built package {output_file}")


if __name__ == "__main__":
    main()
