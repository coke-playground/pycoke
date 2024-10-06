import os
import subprocess
import sys
from pathlib import Path

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    def __init__(self, name: str, *,
                 sourcedir: str = '.',
                 build_type: str = 'RelWithDebInfo',
                 cmake_parallel: int = 1,
                 cmake_verbose: bool = False) -> None:
        super().__init__(name, sources=[])
        self.sourcedir = os.fspath(Path(sourcedir).resolve())
        self.build_type = build_type
        self.cmake_parallel = cmake_parallel
        self.cmake_verbose = cmake_verbose


class CMakeBuild(build_ext):
    def build_extension(self, ext: CMakeExtension) -> None:
        # Must be in this form due to bug in .resolve() only fixed in Python 3.10+
        ext_fullpath = Path.cwd() / self.get_ext_fullpath(ext.name)
        extdir = ext_fullpath.parent.resolve()

        cmake_args = [
            f'-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}{os.sep}',
            f'-DPYTHON_EXECUTABLE={sys.executable}',
            f'-DCMAKE_BUILD_TYPE={ext.build_type}',
            f'-DPYCOKE_VERSION_INFO={self.distribution.get_version()}',
        ]

        build_args = []

        if ext.cmake_parallel > 1:
            build_args.append(f'-j{ext.cmake_parallel}')

        if ext.cmake_verbose:
            build_args.append(f'--verbose')

        build_temp = Path(self.build_temp) / ext.name
        if not build_temp.exists():
            build_temp.mkdir(parents=True)

        subprocess.run(
            ['cmake', ext.sourcedir, *cmake_args], cwd=build_temp, check=True
        )
        subprocess.run(
            ['cmake', '--build', '.', *build_args], cwd=build_temp, check=True
        )


setup(
    name='pycoke',
    version='0.1.0',
    author='kedixa',
    author_email='TODO',
    description='An example project to use coke with pybind11',
    long_description='',
    ext_modules=[
        CMakeExtension('pycoke.cpp_pycoke',
                       cmake_parallel=8,
                       cmake_verbose=False,
                       build_type='Release')
    ],
    cmdclass={'build_ext': CMakeBuild},
    zip_safe=False,
    packages=['pycoke'],
    package_data={
        'pycoke': ['*.pyi'],
    },
    include_package_data=True,
    python_requires='>=3.8',
)
