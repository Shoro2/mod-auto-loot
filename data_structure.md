# Datei- und Verzeichnisstruktur — mod-auto-loot

> Statisches Inventar des Repos. Bei Hinzufügen/Löschen von Files hier mitpflegen.

## Tree

```
mod-auto-loot/
├── .editorconfig                      # AzerothCore-Standard: 4 Spaces, LF, UTF-8
├── .gitattributes                     # Line-Endings, Diff-Treatment
├── .gitignore                         # Build-Artefakte
├── .github/
│   └── (PR-Templates / Actions falls vorhanden)
├── conf/
│   └── mod_auto_loot.conf.dist        # Modul-Konfiguration (Template)
├── data/
│   └── (SQL-Files falls hinzugefügt)
├── src/
│   ├── mod_auto_loot.cpp              # Hauptlogik: PlayerScript "AutoLoot_Player"
│   └── mod_auto_loot_loader.cpp       # Loader: AddSC_AutoLoot()
├── include.sh                         # SQL-Pfad-Registrierung (aktuell leer)
├── pull_request_template.md           # GitHub PR-Template
├── log.md                             # Commit-Log dieses Repos (modular)
├── data_structure.md                  # Diese Datei
├── functions.md                       # Mechanik- und Funktions-Referenz
└── CLAUDE.md                          # Detaillierte Inhalts-Doku
```

## Datei-Zwecke (alphabetisch)

| Datei | Zweck |
|-------|-------|
| `conf/mod_auto_loot.conf.dist` | Konfigurationsoptionen (`AOELoot.Enable`, `AOELoot.MailEnable`) |
| `src/mod_auto_loot.cpp` | Komplette PlayerScript-Implementierung inkl. Tick-Loop für AOE-Loot, Truhen-Looting via Lockpicking, Inventar-/Mail-Fallback |
| `src/mod_auto_loot_loader.cpp` | `AddSC_AutoLoot()` — Loader-Einsprung, registriert die `AutoLoot_Player`-Klasse |
| `include.sh` | wird von AzerothCore beim Auto-Update verwendet, um SQL-Pfade einzulesen (aktuell ohne Inhalt — kein DB-Schema nötig) |
| `pull_request_template.md` | Standard-Template für PRs |
| `log.md` | Commit-History (siehe Konvention dort) |
| `functions.md` | Mechanik-Doku: Hooks, Flow, Loot-Iteration, Truhen-Logik |
| `CLAUDE.md` | Inhaltsorientierte Gesamt-Doku (was tut das Modul, IDs, DB-Bezüge) |

## Größenhinweise (Stand: 2026-05-01)

- `src/mod_auto_loot.cpp` ~8.5 KB — komplett am Stück lesbar
- `src/mod_auto_loot_loader.cpp` ~850 B
- `conf/mod_auto_loot.conf.dist` ~1.2 KB
- `include.sh` 0 B (leer)

Alle Dateien sind klein genug für einen einzelnen `Read`-Aufruf.

## Externe Abhängigkeiten

- **azerothcore-wotlk** (Core): nutzt `PlayerScript`, `Loot`, `Item`, `Creature`, `GameObject`, `ChatHandler`, `Config`, `Log`, `ScriptedGossip`.
- **mod-paragon-itemgen** (optional): konsumiert das durch dieses Modul gefeuerte `OnPlayerLootItem`-Event, um Bonus-Stats zu vergeben.
- **mod-loot-filter** (optional): hängt sich ebenfalls in `OnPlayerLootItem` ein, um automatisches Sell/DE/Delete anzuwenden.

## Wo ist was nicht?

- **Keine SQL-Files** — Modul ist rein C++ ohne DB-Bedarf.
- **Keine Lua/AIO-Files** — keine Client-UI.
- **Kein eigener Build-Slot** — wird via AzerothCore Auto-Detection in `modules/` eingebunden.
