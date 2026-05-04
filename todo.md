# TODOs — mod-auto-loot

> Open tasks for this module. Record completed TODOs in `log.md` and remove them here.
> Format: `[ ]` open, `[x]` done (before cleanup), priority in parentheses.

## Functional gaps

- [ ] **(low)** Mining/herbalism auto-loot: before 2026-03-06, node loot ran via `OnAfterGObjLoot`, but that hook was removed from AzerothCore. A workaround would have to be built around an active `CastSpell` path per skill — currently not implemented.
- [ ] **(low)** Skinning integration: the Skinnable flag is even removed during cleanup. Real skinning automation would need `HasSkill(393)` + `CastSpell(Skinning)` + a loot run.
- [ ] **(low)** Reagent routing into mod-endless-storage: looted Trade Goods/gems currently end up in the inventory, not directly in storage. An optional path could write straight to `custom_endless_storage` — useful when the player actively uses both.

## Performance / robustness

- [ ] **(low)** Throttle / cooldown between auto-loot passes: currently per frame tick — potentially expensive on big AOE pulls in crowded fortresses. A simple per-player `lastRunMs` check could halve that.

## Documentation

- [ ] no open items.

## Convention

Do NOT cross out completed items — instead remove them from this file and document them as a commit entry in `log.md`.
