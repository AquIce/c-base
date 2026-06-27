#!/usr/bin/env python3

import argparse
import subprocess
import shutil
import os
import sys

BUILD_DIR = "build"


# ----------------------------
# core helpers
# ----------------------------

def run(cmd, cwd=None):
    print(">", " ".join(cmd))
    subprocess.check_call(cmd, cwd=cwd)


def clean():
    if os.path.exists(BUILD_DIR):
        print(f"Removing {BUILD_DIR}/")
        shutil.rmtree(BUILD_DIR)
    else:
        print("Nothing to clean.")


# ----------------------------
# build profiles
# ----------------------------

def apply_profile(cmd, profile):
    """
    Maps high-level build profiles → CMake definitions
    """

    if profile == "dev":
        cmd += [
            "-DCMAKE_BUILD_TYPE=Debug",
            "-DBASE_BUILD_TESTS=ON",
            "-DBASE_BUILD_EXAMPLES=ON",
            "-DBASE_ENABLE_SANITIZERS=ON",
        ]

    elif profile == "sanitize":
        cmd += [
            "-DCMAKE_BUILD_TYPE=Debug",
            "-DBASE_BUILD_TESTS=ON",
            "-DBASE_BUILD_EXAMPLES=OFF",
            "-DBASE_ENABLE_SANITIZERS=ON",
        ]

    elif profile == "release":
        cmd += [
            "-DCMAKE_BUILD_TYPE=Release",
            "-DBASE_BUILD_TESTS=OFF",
            "-DBASE_BUILD_EXAMPLES=OFF",
            "-DBASE_ENABLE_SANITIZERS=OFF",
        ]

    elif profile == "ci":
        cmd += [
            "-DCMAKE_BUILD_TYPE=Debug",
            "-DBASE_BUILD_TESTS=ON",
            "-DBASE_BUILD_EXAMPLES=OFF",
            "-DBASE_ENABLE_SANITIZERS=ON",
        ]

    else:
        raise ValueError(f"Unknown profile: {profile}")


# ----------------------------
# commands
# ----------------------------

def configure(args):
    cmd = [
        "cmake",
        "-S", ".",
        "-B", BUILD_DIR,
    ]

    apply_profile(cmd, args.profile)

    run(cmd)


def build():
    run(["cmake", "--build", BUILD_DIR])


def test():
    run([
        "ctest",
        "--test-dir", BUILD_DIR,
        "--output-on-failure"
    ])


def rebuild(args):
    clean()
    configure(args)
    build()


def ci(args):
    clean()
    args.profile = "ci"
    configure(args)
    build()
    test()


# ----------------------------
# CLI
# ----------------------------

def main():
    parser = argparse.ArgumentParser(description="Build system helper")

    sub = parser.add_subparsers(dest="cmd", required=True)

    # configure
    p_config = sub.add_parser("configure")
    p_config.add_argument(
        "--profile",
        default="dev",
        choices=["dev", "release", "sanitize", "ci"]
    )

    # build
    sub.add_parser("build")

    # test
    sub.add_parser("test")

    # clean
    sub.add_parser("clean")

    # rebuild
    p_rebuild = sub.add_parser("rebuild")
    p_rebuild.add_argument(
        "--profile",
        default="dev",
        choices=["dev", "release", "sanitize", "ci"]
    )

    # ci shortcut
    sub.add_parser("ci")

    args = parser.parse_args()

    if args.cmd == "clean":
        clean()

    elif args.cmd == "configure":
        configure(args)

    elif args.cmd == "build":
        build()

    elif args.cmd == "test":
        test()

    elif args.cmd == "rebuild":
        rebuild(args)

    elif args.cmd == "ci":
        ci(args)


if __name__ == "__main__":
    main()
