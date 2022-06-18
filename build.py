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
    if "VCINSTALLDIR" in os.environ or cfg.no_vcvars:
        return

    # get path of vs
    programfiles = os.getenv("ProgramFiles(x86)")
    vswhere = rf"{programfiles}\Microsoft Visual Studio\Installer\vswhere.exe"
    cmd = f'"{vswhere}" -prerelease -latest -property installationPath'
    vspath = subprocess.check_output(cmd).decode().strip()

    # call vcvars
    vcvars = rf"{vspath}\VC\Auxiliary\Build\vcvars64.bat"
    cmd = f'call "{vcvars}" && python '
    cmd += " ".join(sys.argv) + " --no-vcvars"
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
    use_ninja = cfg.use_ninja
    if use_ninja:
        if subprocess.call("where ninja", stdout=subprocess.DEVNULL) == 0:
            print("Found ninja")
        else:
            use_ninja = False

    cmd = (
        f"cmake "
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
    subprocess.check_output(f"windeployqt.exe {natrix_path} --no-opengl-sw")

    # check version
    import setver

    debug = cfg.build_type == "Debug"
    version = setver.install_version()
    if debug:
        version = f"{version}_debug"

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

    if not debug:
        setup_file_name = cfg.setup_file.split("\\")[-1]
        nfo_file = "result\\version.nfo"
        with open(nfo_file, "w+") as nfo:
            nfo.write(f"{setup_file_name}\n")

        rewrite_file = "result\\htaccess.txt"
        with open(rewrite_file, "w+") as hta:
            hta.write(
                rf"RewriteRule ^download/Natrix_Setup(.*)\.exe$ https://natrixlabs.ru/download/{setup_file_name} [L,R=302]"
            )


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
        "--no-ninja",
        dest="use_ninja",
        action="store_false",
        default=True,
        help="Do not use ninja",
    )

    parser.add_argument(
        "--no-vcvars",
        dest="no_vcvars",
        action="store_true",
    )

    return parser.parse_args()


if __name__ == "__main__":
    try:
        cfg = create_parser()

        if cfg.platform == "msvs":
            call_vcvars(cfg)

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
