# Changelog

## Unreleased
- Example case made more meaningful.
- Changed module name to arritmic3d and imported as a3d.

## [3.0b6.dev1] - 2026-02-04

### Changed
- Build infrastructure improvements to set up CI/CD pipelines with github actions.
- Improved management of library restitution models.
- Changed versioning scheme to use semantic versioning with beta indicators.

### Added
- Restitution models are now distributed with the main package.
- Restitution models are saved with the case files after a simulation run for reproducibility.

### Contributors
- Ignacio García-Fernández

## [3.0b5] - 2026-01-30

### Added
- Updated build infrastructure to generate a Python module installable via pip.
- Release file for GitHub Actions, enabling automatic publication to PyPI.
- `tools` directory to store utility scripts.
- Additional checks for eigenvalue search.

### Changed
- Main files moved to the `arritmic` folder to facilitate module building.
- License updated to GPLv3.
- Publish step now uses Test PyPI.
- Python test files moved to the `test` directory.

### Removed
- Old build/install infrastructure removed.

### Fixed
- Typo correction in references.

### Contributors
- Ignacio García-Fernández

## [3.0b4] - 2026-01-25

### Added
- Activations: new management and definitions for activation regions and lists, including support for external files with node lists (commits ce9424f, 1bbda7b, b8a7c60).
- CLI / UX: centralized CLI overrides, organized argument groups and improved help examples (commits d548acb, 6e98189, 81cf4db).
- build_slab and examples: slab generation tools and example scripts with activation support (commits 612d02b, f7d45a1, 3481654).
- Restitution surfaces: integration and use of restitution surfaces and new configuration options (commits 1ba2b79, 6bce13f).
- Save/Load: SaveState/LoadState support for Node, Tissue and related structures; serialization utilities (commits e8eb90f, f194ca6, 34aaffb).
- Tests and utilities: extraction/validation scripts and improvements in tests/fixtures (commit 9dd808c).

### Changed
- Naming and semantics: STIMULATION_SITES renamed to ACTIVATION_REGION and related renames for consistency (commits 364465c, 97708a0, 26866a6).
- Path and config handling: resolve file paths to absolute, simplify config management and save the used config in output for reproducibility (commits 4cab369, 40a0507, 60fb4c5, c62a457).
- API / CLI refactor: reorganized CLI into argument groups, moved main run logic into functions and improved parsing error handling (commits 6e98189, 8a00a54, 87f3a63).
- Slab / inputs: changed ndivs → nnodes and simplified input/config/output handling for examples (commits 2e6a97e, 4d72855).
- General refactor and cleanup: import reorganization and code simplification (commits da512c0, 93d74e1).

### Fixed
- Activation and scheduling bugs: fixed CL calculation after last beat and unified scheduling logic for PROTOCOL and ACTIVATE_NODES (commits ffb5518, c625b82, 3428b33).
- Documentation and examples: fixed PROTOCOL list formatting, typos and conversion-related issues (commits f92d383, 4eee2d1, 73e202d).
- Path/IO issues: fixed JSON path interpretation and output directory handling (commits 98b0045, dfefe71).
- Build/platform fixes: ARM and Python 3.12 compilation fixes and Makefile adjustments (commits bcb32f2, fb0aa9b, 833e4b1).
- Miscellaneous minor bug fixes and parameter corrections (e.g. f8c6080, 0e305b5, 0f706af).

### Contributors
- Ignacio García-Fernández
- Fernando Barber
- Giada Romitti

## [3.0b3] - 2025-11-17

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

## [3.0b2] - 2025-09-24

### Added
- Developed tools for case initialization and conversion.
- Code cleanup.
- Extended version of the viewer (beta)

### Fixed
- Fixed activation bugs.

## [3.0b1] - 2025-06-12

### Added
- Initial migration of arritmic3D to C++/Python.
- Activation model
