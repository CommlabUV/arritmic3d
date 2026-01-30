"""
arritmic3d - Fast eikonal solver for electrophysiology simulation

A high-performance C++ eikonal solver with Python bindings for
electrophysiology simulations in cardiac tissue.
"""

__version__ = "3.0.0-rev5"
__author__ = "CoMMLabUV"
__license__ = "LGPL-3.0-or-later"

# Import the compiled C++ module
try:
    from ._core import CardiacTissue, SystemEventType
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
]
