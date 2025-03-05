from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext
from Cython.Build import cythonize

extensions = [
    Extension("test1", ["test1.pyx"])
]

setup(
    name = "test",
    # cmdclass = {'build_ext': build_ext},
    ext_modules = cythonize(extensions, language_level=3)
    )