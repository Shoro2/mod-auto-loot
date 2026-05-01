# Change Log — mod-auto-loot

> Minimaler Commit-Log. Eine Zeile pro Änderung mit Verweis auf den Commit.
> Format: `YYYY-MM-DD — <type>: <kurzbeschreibung> (<short-sha>)`

## 2026

- 2026-03-21 — refactor: Extract loot storage into helper function ([1f9b162](https://github.com/Shoro2/mod-auto-loot/commit/1f9b162152b6221b3a3991877ceae0b1351a4cce)) — auto-looted Items lösen jetzt `OnPlayerLootItem` aus, sodass mod-paragon-itemgen Bonus-Stats vergeben kann.
- 2026-03-06 — refactor: Update PlayerScript hooks and API calls ([9aeb55d](https://github.com/Shoro2/mod-auto-loot/commit/9aeb55d17ff44f91c5781d234be9d9fad9bdf540)) — `OnLogin` → `OnPlayerLogin`, `RemoveFlag(UNIT_DYNAMIC_FLAGS,...)` → `RemoveDynamicFlag(...)`, `OnAfterGObjLoot` entfernt (Hook existiert nicht mehr).
- 2026-03-06 — chore: update name ([8ac50ad](https://github.com/Shoro2/mod-auto-loot/commit/8ac50ad7ec2f5c20e0ec3e2dbb1da5cf6ab34b1e))
- 2026-03-06 — chore: init ([78b171b](https://github.com/Shoro2/mod-auto-loot/commit/78b171b62c8ba5dc0811c52a95450c210e94e568))
- 2026-03-06 — chore: Initial commit ([553bf64](https://github.com/Shoro2/mod-auto-loot/commit/553bf642efcc2b7384399ac59d931f6618544010))

## Konvention

Neue Einträge oben anhängen. Ausführliche Beschreibung gehört in den Commit-Body bzw. den projekt-zentralen Log unter `share-public/claude_log.md`.
