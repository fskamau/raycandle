"""Build raycandle with an existing static raylib; no dependency downloads."""
import os
import shutil
import subprocess
from pathlib import Path

from setuptools import Distribution, find_packages, setup
from setuptools.command.build_py import build_py

ROOT = Path(__file__).resolve().parent


def native_configuration():
    env = os.environ.copy()
    opts = []
    if os.name != 'nt':
        if env.get('RAYLIB_ROOT'):
            opts.append('-DRAYLIB_ROOT=' + env['RAYLIB_ROOT'])
        return env, opts

    raylib_root = Path(env.get('RAYLIB_ROOT', r'C:\\raylib\\raylib\\src')).resolve()
    opts.append('-DRAYLIB_ROOT=' + raylib_root.as_posix())
    if not (raylib_root / 'raylib.h').is_file() or not (raylib_root / 'libraylib.a').is_file():
        raise RuntimeError(
            f'Expected raylib.h and static libraylib.a in {raylib_root}. '
            'Set RAYLIB_ROOT to the directory containing them.'
        )

    # Find the compiler shipped alongside the raylib Windows installation.
    install_root = raylib_root
    if raylib_root.name.lower() == 'src' and raylib_root.parent.name.lower() == 'raylib':
        install_root = raylib_root.parent.parent
    candidates = [
        install_root / 'w64devkit' / 'bin',
        install_root / 'mingw64' / 'bin',
        install_root / 'mingw' / 'bin',
        install_root / 'bin',
        install_root / 'w64devkit' / 'x86_64-w64-mingw32' / 'bin',
    ]
    gcc = next((d / 'gcc.exe' for d in candidates if (d / 'gcc.exe').is_file()), None)
    if gcc is None:
        gcc_path = shutil.which('gcc')
        gcc = Path(gcc_path) if gcc_path else None
    if gcc is None:
        raise RuntimeError(f'MinGW gcc.exe not found near {install_root} or on PATH.')
    bindir = gcc.parent
    make = next((bindir / n for n in ('mingw32-make.exe', 'make.exe') if (bindir / n).is_file()), None)
    if make is None:
        make_path = shutil.which('mingw32-make') or shutil.which('make')
        make = Path(make_path) if make_path else None
    if make is None:
        raise RuntimeError('MinGW make executable not found next to gcc or on PATH.')
    env['PATH'] = str(bindir) + os.pathsep + env.get('PATH', '')
    opts += ['-G', 'MinGW Makefiles', '-DCMAKE_C_COMPILER=' + gcc.as_posix(),
             '-DCMAKE_MAKE_PROGRAM=' + make.as_posix()]
    return env, opts


class BinaryDistribution(Distribution):
    def has_ext_modules(self):
        return True


class BuildPythonWithNative(build_py):
    def run(self):
        env, flags = native_configuration()
        # Separate from a manually configured Visual Studio build tree.
        build_dir = ROOT / 'build' / 'pip-native-mingw' if os.name == 'nt' else ROOT / 'build' / 'pip-native'
        subprocess.check_call(['cmake', '-S', str(ROOT), '-B', str(build_dir),
                               '-DCMAKE_BUILD_TYPE=Release', *flags], env=env)
        subprocess.check_call(['cmake', '--build', str(build_dir), '--config', 'Release', '--parallel', '2'], env=env)
        # CMake POST_BUILD stages the DLL/.so and cleaned header in source package.
        super().run()
        # Editable builds retain the source directory, which is staged above.


setup(
    name='raycandle',
    version='2.1.0',
    description='raylib-powered financial charts',
    packages=find_packages(),
    install_requires=['cffi>=1.15', 'numpy>=1.23', 'pandas>=1.5', 'pytz'],
    python_requires='>=3.9',
    package_data={'raycandle': ['raycandle.h', '*.so', '*.dll', '*.dylib', 'assets/*']},
    include_package_data=True,
    zip_safe=False,
    cmdclass={'build_py': BuildPythonWithNative},
    distclass=BinaryDistribution,
)
