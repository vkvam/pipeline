# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

## [1.4] - 2019-12-10

### Added

- Basic support for reacting to terminal resizes.

### Fixed

- Fix for output not ending in a newline.

## [1.3] - 2019-11-29

### Changed

- Long lines wrap by default, rather than truncating.
- Line display length calculations are more accurate.

### Added

- Added -t/--truncate option to truncate long lines rather than wrapping (used
  to be the default behavior).

## [1.2] - 2019-11-25

### Changed

- Fixed potential crash in terminfo usage.
- Use the C locale to better support non-ASCII in the output.

## [1.1] - 2019-11-24

### Changed

- Make pipeline compatible with libreadline (tested on Ubuntu).

## [1.0] - 2019-11-23

### Added

- Initial release, tested mainly on MacOS with the default libedit.
