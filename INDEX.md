# INDEX — mod-auto-loot

Entry point for AI tools. Read this file first, then the ones listed below as needed.

## Files in this repo

| File | Size | Purpose |
|-------|------:|-------|
| `INDEX.md` | <1 KB | this file — navigation |
| `CLAUDE.md` | ~3 KB | **What** this module is, what role, which IDs/DB tables |
| `data_structure.md` | ~2 KB | exact folder/file listing |
| `functions.md` | ~5 KB | **How** the module works: hooks, mechanics, config |
| `log.md` | grows | minimal commit log (one line per commit) |
| `todo.md` | small | open tasks with priority |

## Cross-Repo

- Project overview & conventions: [`share-public/AI_GUIDE.md`](https://github.com/Shoro2/share-public/blob/main/AI_GUIDE.md)
- Cross-repo history: [`share-public/claude_log.md`](https://github.com/Shoro2/share-public/blob/main/claude_log.md)
- Detailed AzerothCore architecture: [`share-public/docs/02-architecture.md`](https://github.com/Shoro2/share-public/blob/main/docs/02-architecture.md)
- AI workflow & doc convention: [`share-public/docs/08-ai-workflow.md`](https://github.com/Shoro2/share-public/blob/main/docs/08-ai-workflow.md)

## Quick Facts

- AzerothCore module for **WoW 3.3.5a**
- Function: AOE loot in a 10-yd radius on `OnPlayerUpdate`
- **No** AIO, no Lua scripts, no DB schema — pure C++.
- Critical: calls `sScriptMgr->OnPlayerLootItem()` → other modules (mod-paragon-itemgen, mod-loot-filter) hook into this.
