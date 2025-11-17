# Changelog

## [Unreleased]

## [3.0.0-rc3] - 2025-11-17

### Added
- README updated with installation and usage instructions (commit d696ac7).
- Extended CLI help messages (commit a82d801).
- Makefile: automatic acquisition of Python include/link flags (commit 66612e5).
- Restitution models: added support for restitution surfaces and config files (multiple commits, e.g. 6ccc5e1, 78d5f09).
- Initialization-from-config added for spline/CV/APD modules and basic_tissue (commits 2022e41, 7f1e4ab).
- Tests and surface fixtures added/updated (test → surfaces, test2) and tissue size reporting in tests (commits d455d79, bd35caa).
- Argument parser refactored/moved to a reusable function (commit 6b617ab).
- New pacing/stimulation handling and related fields in scripts (several commits).
- Examples and helper scripts (install, slab generation, converters) updated/added.
- Use of std::filesystem in C++ for robust path handling (commit e57b924).

### Fixed
- Fixed incorrect requirements filename and removed version-specific requirements file (commits 031711e, c895cec).
- Prevented incorrect reactivation logic in nodes (ERP timing fix) (commit 8cb56f).
- getValue parameter order corrected; isnan substituted by is_novalue; getERP implemented (commit 43402e6).
- Padding and parameter order issues corrected (node_parameters) (commit 8cb56f).
- Commented out a line in `show` that caused a compilation error (commit e816bfe).
- Adjusted semantics of nx, ny, nz to represent number of nodes (not subdivisions) and adapted code accordingly (commit 34877e9).
- Miscellaneous fixes and refactors to adapt API, labels and naming to new restitution-model changes (many small commits).
- Various test and script corrections to work with the new models and interfaces.

### Contributors
- Fernando Barber
- Ignacio García-Fernández

## [3.0.0-rc2] - 2025-09-24

### Added
- Developed tools for case initialization and conversion.
- Code cleanup.
- Extended version of the viewer (beta)

### Fixed
- Fixed activation bugs.

## [3.0.0-rc1] - 2025-06-12

### Added
- Initial migration of arritmic3D to C++/Python.
- Activation model
