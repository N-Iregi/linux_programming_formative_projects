# setup.py
from setuptools import setup, Extension

c_python_pybaseline_module = Extension(
    'c_python_pybaseline',
    sources=['cpython_pybaseline.c'],
    extra_compile_args=['-O3']  # Enable maximum compiler optimization
)

setup(
    name='c_python_pybaseline',
    version='1.0',
    description='CPython C extension for data analysis',
    ext_modules=[c_python_pybaseline_module]
)
