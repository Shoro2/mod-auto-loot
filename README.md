# mod-auto-loot

Automatic AOE looting for an [AzerothCore](https://www.azerothcore.org/) **WoW 3.3.5a (WotLK)** server. Part of the Forgotten Land server project.

The module empties dead creatures (and, with the Lockpicking skill, chests) within a 10-yard radius of the player automatically, with no clicking. Gold from all corpses in range is summed and credited in a single message, items are placed in the inventory, and overflow can optionally be forwarded to the player's mailbox. It is intentionally small (one source file, no custom tables). Derived from AzerothCore's [mod-aoe-loot](https://github.com/azerothcore/mod-aoe-loot).

## How it works

Every player update tick (throttled to a configurable interval, 250 ms by default) the module checks the trigger conditions below. If they hold, it scans the grid around the player for dead creatures, takes each corpse's gold and lootable items, stores the items in the bags as if the player had looted them by hand, and clears the corpse's loot so it no longer glows as lootable. Gold from all corpses of one pass is added up and credited in one go. Chests are handled the same way, but only for players who have the Lockpicking skill. If an item does not fit, it is mailed to the player (when mail overflow is on) instead of being lost; quest items that exceed what the quest needs are not mailed. Because items are stored through the regular loot path, other loot-related modules (stat enchanting, loot filters) still react to them.

## Features

- **AOE loot** in a 10-yard radius around the player.
- **Chest support** when the player has skill 186 (Lockpicking).
- **Gold aggregation**: all gold in range is summed and sent in one loot-money notification, with the "loot money" achievement criteria counted along.
- **Unique items**: items with `MaxCount = 1` are only looted if the player does not already own a copy.
- **Mail overflow** (optional): when the inventory is full, looted items are sent by mail (`AOELoot.MailEnable`). Quest-only drops that do not fit are not mailed.
- **Solo only**: skipped while the player is in a group, so group loot rules stay intact.
- Auto-looted items go through the normal loot-item hooks, so other loot modules (stat enchanting, loot filters) still see them.
- No DBC patch and no client addon.

## Trigger conditions

Auto-loot runs on the player's update tick (throttled, see `AOELoot.ScanIntervalMS`) when all of these hold:

| Condition | Required |
|-----------|----------|
| `AOELoot.Enable` is on | yes |
| Player is not in a group | yes |
| At least 4 free inventory slots | yes |
| Dead, lootable creatures within 10 yards | for creature loot |
| Chest within 10 yards and player has Lockpicking (skill 186) | for chest loot |

## Requirements

- AzerothCore WotLK. The module only uses standard upstream script hooks (`OnPlayerUpdate`, `OnPlayerCanSendErrorAlreadyLooted`, `OnPlayerLogin`), so the Shoro2 fork is not required.
- No other module, no client patch.

## Installation

1. Clone the module into the `modules/` directory of the server source:
   ```bash
   cd azerothcore-wotlk/modules
   git clone https://github.com/Shoro2/mod-auto-loot.git
   ```
2. Re-run CMake and rebuild the server.
3. SQL: `data/sql/db-world/base/aoe_loot_acore_string.sql` (two `acore_string` rows, ids 50000-50001, for the login notice and the mail message) is applied by the AzerothCore DB updater on the next world server start.
4. Copy `conf/mod_auto_loot.conf.dist` to your server's module config folder as `mod_auto_loot.conf`.
5. Restart the world server.

## Configuration

`conf/mod_auto_loot.conf.dist`:

- `AOELoot.Enable` - master toggle (default `1` in the shipped file).
- `AOELoot.MailEnable` - mail items that do not fit into the bags (default `1`).
- `AOELoot.ScanIntervalMS` - how often the corpse and chest scans run per player, in milliseconds (default `250`; `0` scans on every tick).

The 10-yard radius is fixed in the code.
