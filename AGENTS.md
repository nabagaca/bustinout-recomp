# Project knowledge

Start at [docs/okf/index.md](docs/okf/index.md) for architecture and task-specific source links. Consult the relevant concept before changing a subsystem; source remains authoritative and historical playtest reports are labelled separately. Update affected knowledge when implementation changes invalidate it, then run `python docs/okf/validate.py` (requires PyYAML).

Do not edit generated guest code. Use title overrides in `src/game/` or justified corrections in `config/game.toml`. Never commit game assets, XBEs, ISOs or generated game code.
