"""
arritmic3d - Fast eikonal solver for electrophysiology simulation

A high-performance C++ eikonal solver with Python bindings for
electrophysiology simulations in cardiac tissue.
"""

__version__ = "3.0b6"
__author__ = "CoMMLabUV"
__license__ = "LGPL-3.0-or-later"

# Import the compiled C++ module
try:
    from ._core import CardiacTissue, SystemEventType
    # Expose arritmic3d and test_case lazily (avoid exposing submodules in package namespace)
    def __getattr__(name):
        if name == "arritmic3d":
            from .arritmic3D import run_arritmic3D as arritmic3d
            return arritmic3d
        if name == "test_case":
            from .arritmic3D import run_test_case as test_case
            return test_case
        if name == "build_slab":
            from .arr3D_build_slab import build_slab
            return build_slab
        raise AttributeError(f"module {__name__!r} has no attribute {name!r}")

    def __dir__():
        return sorted(list(globals().keys()) + ["arritmic3d", "test_case", "build_slab"])
except ImportError as e:
    raise ImportError(
        "Cannot import C++ extension '_core'. "
        "Make sure the package was built correctly.\n"
        "Try rebuilding with: pip install --force-reinstall --no-cache-dir .\n"
        f"Original error: {e}"
    ) from e

# Expose the public API
__all__ = [
    "CardiacTissue",
    "SystemEventType",
    "__version__",
    "arritmic3d",
    "test_case",
    "build_slab",
]
