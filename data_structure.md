# File and directory structure — mod-auto-loot

> Static inventory of the repo. Maintain this when adding/removing files.

## Tree

```
mod-auto-loot/
├── .editorconfig                      # AzerothCore standard: 4 spaces, LF, UTF-8
├── .gitattributes                     # Line endings, diff treatment
├── .gitignore                         # Build artifacts
├── .github/
│   └── (PR templates / actions if present)
├── conf/
│   └── mod_auto_loot.conf.dist        # Module configuration (template)
├── data/
│   └── (SQL files if added)
├── src/
│   ├── mod_auto_loot.cpp              # Main logic: PlayerScript "AutoLoot_Player"
│   └── mod_auto_loot_loader.cpp       # Loader: AddSC_AutoLoot()
├── include.sh                         # SQL path registration (currently empty)
├── pull_request_template.md           # GitHub PR template
├── log.md                             # Commit log of this repo (modular)
├── data_structure.md                  # This file
├── functions.md                       # Mechanics and function reference
└── CLAUDE.md                          # Detailed content doc
```

## File purposes (alphabetical)

| File | Purpose |
|-------|-------|
| `conf/mod_auto_loot.conf.dist` | Configuration options (`AOELoot.Enable`, `AOELoot.MailEnable`) |
| `src/mod_auto_loot.cpp` | Complete PlayerScript implementation including tick loop for AOE loot, chest looting via Lockpicking, inventory/mail fallback |
| `src/mod_auto_loot_loader.cpp` | `AddSC_AutoLoot()` — loader entry point, registers the `AutoLoot_Player` class |
| `include.sh` | Used by AzerothCore during auto-update to read SQL paths (currently empty — no DB schema needed) |
| `pull_request_template.md` | Standard template for PRs |
| `log.md` | Commit history (see convention there) |
| `functions.md` | Mechanics doc: hooks, flow, loot iteration, chest logic |
| `CLAUDE.md` | Content-oriented overview doc (what the module does, IDs, DB references) |

## Size notes (as of 2026-05-01)

- `src/mod_auto_loot.cpp` ~8.5 KB — readable in one piece
- `src/mod_auto_loot_loader.cpp` ~850 B
- `conf/mod_auto_loot.conf.dist` ~1.2 KB
- `include.sh` 0 B (empty)

All files are small enough for a single `Read` call.

## External dependencies

- **azerothcore-wotlk** (core): uses `PlayerScript`, `Loot`, `Item`, `Creature`, `GameObject`, `ChatHandler`, `Config`, `Log`, `ScriptedGossip`.
- **mod-paragon-itemgen** (optional): consumes the `OnPlayerLootItem` event fired by this module to assign bonus stats.
- **mod-loot-filter** (optional): also hooks into `OnPlayerLootItem` to apply automatic Sell/DE/Delete.

## What is not where?

- **No SQL files** — the module is pure C++ and needs no DB.
- **No Lua/AIO files** — no client UI.
- **No own build slot** — included in `modules/` via AzerothCore auto-detection.
