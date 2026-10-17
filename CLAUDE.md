# CLAUDE.md — mod-auto-loot

> **Internal docs live in the vault, not here** (rule since 2026-10-18: this public repo keeps only
> README.md with the general info and installation, this CLAUDE.md, the license and the code).
> Index, data structure, functions, log and todo: share-public `docs/World of Warcraft/module-repos/mod-auto-loot/`
> (local checkout `C:\Users\Anwender\Documents\GitHub\share-public`). Update them there,
> in a vault worktree, in the same session as the code change.

> **Content-oriented**. What is this module, what does it do, which IDs/DB references exist?
> Mechanics details (hooks, function signatures, flow): see [`functions.md`](https://github.com/Shoro2/share-public/blob/main/docs/World%20of%20Warcraft/module-repos/mod-auto-loot/functions.md).
> File layout: see [`data_structure.md`](https://github.com/Shoro2/share-public/blob/main/docs/World%20of%20Warcraft/module-repos/mod-auto-loot/data_structure.md).
> Commit history: see [`log.md`](https://github.com/Shoro2/share-public/blob/main/docs/World%20of%20Warcraft/module-repos/mod-auto-loot/log.md).
> Project-wide context: see [`share-public/AI_GUIDE.md`](https://github.com/Shoro2/share-public/blob/main/AI_GUIDE.md).

## What is mod-auto-loot?

An AzerothCore module that automates **AOE looting** in a 10-yard radius. Dead creatures and (with Lockpicking skill) chests in the area are emptied automatically on the regular `OnPlayerUpdate` tick, without the player having to click each corpse individually. Full inventories can optionally be sent on by mail.

The module is small (one `.cpp` file with ~250 lines of main logic) and has **no DB schema at all**.

## Role in the overall project

```
Creature Death / Chest Open
        │
        ▼
mod-auto-loot   (proactively grabs loot in 10-yd radius)
        │
        ▼ sScriptMgr->OnPlayerLootItem()
        │
        ├─→ mod-paragon-itemgen  (assigns bonus stats / cursed marker)
        └─→ mod-loot-filter      (decides: Keep / Sell / DE / Delete)
```

The module is a **prerequisite** for mod-paragon-itemgen + mod-loot-filter as soon as the player "auto-loots" — without the `OnPlayerLootItem` hook, auto-looted items would not get stats and would not be filtered.

## What triggers auto-loot?

| Condition | Required |
|-----------|-------------|
| `AOELoot.Enable` config | `true` |
| Player not in a group | yes |
| ≥4 free inventory slots | yes |
| Dead creatures in the 10-yd radius | for creature loot |
| Chest (`GAMEOBJECT_TYPE_CHEST`) in the 10-yd radius **and** player has skill 186 (Lockpicking) | for chest loot |

## Item handling

| Item type (`MaxCount`) | Behavior |
|-----------------------|-----------|
| stackable (`MaxCount != 1`) | looted; on full inventory, sent on by mail (`AOELoot.MailEnable`) |
| unique (`MaxCount == 1`) | only looted if the player **does not yet own** the item |

Gold is accumulated across all creatures and sent to the client in a single `SMSG_LOOT_MONEY_NOTIFY` packet. The `LOOT_MONEY` achievement is counted along with it.

## IDs and constant values

| Resource | ID | Source |
|-----------|----|--------|
| acore_string "Auto-Loot enabled" | `50000` | `AOE_ACORE_STRING_MESSAGE` |
| acore_string "Item in mail" | `50001` | `AOE_ITEM_IN_THE_MAIL` |
| Spell "Pick Lock" | `2575` | Standard WoW spell, only referenced by us |
| Skill "Lockpicking" | `186` | Standard WoW skill |
| Loot range | `10.0f` yards | hard-coded |
| Min free slots | `4` | hard-coded |

## DB references

- **No own DB tables.**
- Only consumes:
  - `acore_world.acore_string` (for the two sysmessages 50000/50001)
  - `acore_world.item_template` (via `sObjectMgr->GetItemTemplate(itemid)` for the `MaxCount` check)

## Configuration

`conf/mod_auto_loot.conf.dist` (two options):

```ini
AOELoot.Enable = 1     # master toggle
AOELoot.MailEnable = 1 # mail items on full inventory
```

## What this module does NOT do

- No group loot, no master loot logic, no loot rolls.
- No mining/herbalism auto-loot (`OnAfterGObjLoot` hook removed).
- No skinning. The Skinnable flag is even removed once the corpse is empty.
- No own loot filter / no anti-spam — filtering is handled by mod-loot-filter on the downstream hook.

## Important for AI sessions

- No SQL migrations → changes are pure C++ refactorings.
- Module loader function: `AddSC_AutoLoot()` (against the convention `AddSC_*` instead of `Addmod_auto_lootScripts()` — works because the AzerothCore loader system accepts both patterns).
- Hook registration must use the correct `PLAYERHOOK_*` enum values (see `log.md` entry 2026-03-06).
- The `StoreLootAndNotify` helper must not be replaced with `player->AddItem()` — the `OnPlayerLootItem` hook is the interface to all other modules.
