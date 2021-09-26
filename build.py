#!python3
# coding:utf8

import os
import sys
import shutil
import subprocess
import pathlib
import argparse

# это основной метод сборки

# build
#    -c --clean       пересборка
#    --make-installer создать инсталятор
#    --install        установка

# build installer

############################################################################


def print_title(s):
    line = "#" * 120
    print(f"{line}\n# {s:^116} #\n{line}")


def print_subtitle(s):
    print(f"####### ", s)


############################################################################


def call_vcvars(cfg):
    if "VCINSTALLDIR" in os.environ or cfg.novcvars:
        return

    # get path of vs
    programfiles = os.getenv("ProgramFiles(x86)")
    vswhere = fr"{programfiles}\Microsoft Visual Studio\Installer\vswhere.exe"
    cmd = f'"{vswhere}" -prerelease -latest -property installationPath'
    vspath = subprocess.check_output(cmd).decode().strip()

    # call vcvars
    vcvars = fr"{vspath}\VC\Auxiliary\Build\vcvars64.bat"
    cmd = f'call "{vcvars}" && python '
    cmd += " ".join(sys.argv) + " --novcvars"
    rc = subprocess.call(cmd, shell=True)
    sys.exit(rc)


def build_natrix(cfg):
    print_title("Build natrix")

    cfg.build_path = f"build-{cfg.build_type.lower()}"
    if cfg.do_clean:
        shutil.rmtree(cfg.build_path)

    # conan
    print_subtitle("run conan")
    cmd = (
        "conan install "
        f'-s "build_type={cfg.build_type}" '
        f"-if {cfg.build_path} "
        "conanfile.txt "
    )
    print(cmd)
    subprocess.check_call(cmd, shell=False)

    # generate
    print_subtitle("cmake generate")
    use_ninja = subprocess.call("where ninja", stdout=subprocess.DEVNULL) == 0
    if use_ninja:
        print("Found ninja")

    cmd = (f"cmake "
           f"-S .  "
           f"-B {cfg.build_path} " 
           f"-DCMAKE_BUILD_TYPE={cfg.build_type} "
    )
    if cfg.platform == "mingw":
        cmd += f'-G "MinGW Makefiles" '
    elif use_ninja:
        cmd += f"-GNinja "

    if cfg.do_run_tests:
        cmd += f"-DNATRIX_BUILD_TESTS=1 "

    print(cmd)
    subprocess.check_call(cmd, shell=True)

    # build
    print_subtitle("cmake build")
    cmd = (
        f"cmake --build {cfg.build_path} "
        f"-j {os.cpu_count()} "
        f"--config {cfg.build_type} "
    )
    if not use_ninja:
        cmd += f"-- /p:CL_MPcount={os.cpu_count()}"
    print(cmd)
    subprocess.check_call(cmd, shell=True)


def run_tests(cfg):
    print_title("run tests")

    cmd = (
        f"cd {cfg.build_path} && ctest "
        f"-C {cfg.build_type} "
        f"-j{os.cpu_count()} "
        "--output-on-failure "
        "--no-compress-output "
    )
    print(cmd)
    subprocess.check_call(cmd, shell=True)


def make_installer(cfg):
    print_title("makensis")

    # create dirs
    deploy_path = f"{cfg.build_path}/deploy"
    if os.path.exists(deploy_path):
        shutil.rmtree(deploy_path)
    os.mkdir(deploy_path)

    if not os.path.exists("result"):
        os.mkdir("result")

    # create deply package
    natrix_path = f"{deploy_path}/natrix.exe"
    shutil.copy(f"{cfg.build_path}/bin/ui.exe", natrix_path)
    subprocess.check_output(
        f"windeployqt.exe {natrix_path} " "--no-compiler-runtime " "--no-opengl-sw"
    )

    # check version
    import setver

    version = setver.install_version()

    # create installer
    programfiles = os.getenv("ProgramFiles(x86)")
    logfile = "setup_maker.log"
    with open(logfile, "w+") as f:
        subprocess.call(
            rf"{programfiles}/nsis/makensis.exe /V4 /DVERSION={version} /DBIN={deploy_path} setup_maker.nsi",
            stdout=f,
        )

    # check result file
    cfg.setup_file = [
        l for l in open(logfile, "r").readlines() if l.startswith("Output:")
    ]
    cfg.setup_file = cfg.setup_file[0].split('"')[1]
    setup_file_size = os.stat(cfg.setup_file).st_size
    print(f"Output: {cfg.setup_file}")
    print(f"Size  : {setup_file_size}")


def install():
    subprocess.call(f"{cfg.setup_file} ", shell=True)


def create_parser():
    parser = argparse.ArgumentParser(prog="build")

    __platforms = ["msvs", "mingw"]
    parser.add_argument(
        "--tool",
        dest="platform",
        choices=__platforms,
        default=__platforms[0],
        help="Select build platform",
    )
    
    __build_types = ["Release", "Debug"]
    parser.add_argument(
        "-t",
        "--build-type",
        dest="build_type",
        choices=__build_types,
        default=__build_types[0],
        help="Select build type",
    )

    parser.add_argument(
        "--make-installer",
        dest="do_make_installer",
        action="store_true",
        default=False,
        help="Create installer",
    )

    parser.add_argument(
        "--install",
        dest="do_install",
        action="store_true",
        default=False,
        help="Install application",
    )

    parser.add_argument(
        "--run-tests",
        dest="do_run_tests",
        action="store_true",
        default=False,
        help="Run tests",
    )

    parser.add_argument(
        "-c",
        "--clean",
        "--rebuild",
        dest="do_clean",
        action="store_true",
        default=False,
        help="Clean before build",
    )

    parser.add_argument(
        "--novcvars",
        dest="novcvars",
        action="store_true",
    )

    ini = "".join(open("build.ini").readlines()).split()
    n = parser.parse_args(args=ini)
    return parser.parse_args(namespace = n)

if __name__ == "__main__":
    try:
        cfg = create_parser()
        
        if cfg.platform == "msvs":
            call_vcvars(cfg)

        cfg.do_make_installer = True
        cfg.do_run_tests = True

        from datetime import datetime

        start = datetime.today()

        # build
        build_natrix(cfg)

        # tests
        if cfg.do_run_tests:
            run_tests(cfg)

        # installer
        if cfg.do_make_installer:
            make_installer(cfg)

        elapsed = datetime.today() - start
        print_title(f"done (in {elapsed})")

        # install natrix
        if cfg.do_install:
            install()

    except Exception as a:
        print_title(f"failure :(")
        raise
