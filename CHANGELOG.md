# Changelog

All notable changes to Real Events are documented here.

## [0.1.0] - Release candidate

### Added
- Dynamic production event system.
- Production crisis event type.
- Production boom event type.
- 1–5 simultaneous event cities.
- Random event duration from 1–12 30-day months.
- Random event evaluation interval from 30–90 game days.
- Absolute-day tracking based on `Gametime / 256`.
- Per-campaign sidecar persistence.
- Town production hook.
- Private/Trader production hook.
- Tavern Informer rumour integration.
- Runtime hook byte validation.
- Development debug window and manual Lübeck event controls.

### Fixed
- Boom events originally failed to apply because the production helper returned early for every percentage `>= 100`; the condition was corrected so only exactly `100%` bypasses scaling.
- Debug window crash caused by `AppendText` format arguments being supplied in the wrong order.

### Changed
- Public project name changed from the development name EventManager/MoneyMod to **Real Events**.
- Persistence filenames now use `RealEvents_<campaign-hash>.dat`.
- Development hotkeys are disabled by default in public builds.

### Known limitations
- External INI configuration is not implemented yet.
- Informer event prose is English only.
- Compatibility is validated only against the currently tested executable/modloader build.
