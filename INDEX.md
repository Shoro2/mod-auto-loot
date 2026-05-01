# INDEX — mod-auto-loot

Einstiegspunkt für KI-Tools. Lies zuerst diese Datei, dann nach Bedarf die unten gelisteten.

## Files in diesem Repo

| Datei | Größe | Zweck |
|-------|------:|-------|
| `INDEX.md` | <1 KB | diese Datei — Navigation |
| `CLAUDE.md` | ~3 KB | **Was** ist dieses Modul, welche Rolle, welche IDs/DB-Tabellen |
| `data_structure.md` | ~2 KB | exakte Folder/File-Auflistung |
| `functions.md` | ~5 KB | **Wie** funktioniert das Modul: Hooks, Mechanik, Konfig |
| `log.md` | wächst | minimaler Commit-Log (eine Zeile pro Commit) |
| `todo.md` | klein | offene Aufgaben mit Priorität |

## Cross-Repo

- Projekt-Übersicht & Konventionen: [`share-public/AI_GUIDE.md`](https://github.com/Shoro2/share-public/blob/main/AI_GUIDE.md)
- Cross-Repo-Historie: [`share-public/claude_log.md`](https://github.com/Shoro2/share-public/blob/main/claude_log.md)
- Detaillierte AzerothCore-Architektur: [`share-public/docs/02-architecture.md`](https://github.com/Shoro2/share-public/blob/main/docs/02-architecture.md)
- KI-Workflow & Doku-Konvention: [`share-public/docs/08-ai-workflow.md`](https://github.com/Shoro2/share-public/blob/main/docs/08-ai-workflow.md)

## Quick Facts

- AzerothCore-Modul für **WoW 3.3.5a**
- Funktion: AOE-Loot im 10-yd-Radius bei `OnPlayerUpdate`
- **Kein** AIO, keine Lua-Scripts, kein DB-Schema — rein C++.
- Kritisch: ruft `sScriptMgr->OnPlayerLootItem()` auf → andere Module (mod-paragon-itemgen, mod-loot-filter) hängen sich daran.
