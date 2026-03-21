/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

 /*
 GAMEOBJECT_TYPE_DOOR                   = 0,
 GAMEOBJECT_TYPE_BUTTON                 = 1,
 GAMEOBJECT_TYPE_QUESTGIVER             = 2,
 GAMEOBJECT_TYPE_CHEST                  = 3,
 GAMEOBJECT_TYPE_BINDER                 = 4,
 GAMEOBJECT_TYPE_GENERIC                = 5,
 GAMEOBJECT_TYPE_TRAP                   = 6,
 GAMEOBJECT_TYPE_CHAIR                  = 7,
 GAMEOBJECT_TYPE_SPELL_FOCUS            = 8,
 GAMEOBJECT_TYPE_TEXT                   = 9,
 GAMEOBJECT_TYPE_GOOBER                 = 10,
 GAMEOBJECT_TYPE_TRANSPORT              = 11,
 GAMEOBJECT_TYPE_AREADAMAGE             = 12,
 GAMEOBJECT_TYPE_CAMERA                 = 13,
 GAMEOBJECT_TYPE_MAP_OBJECT             = 14,
 GAMEOBJECT_TYPE_MO_TRANSPORT           = 15,
 GAMEOBJECT_TYPE_DUEL_ARBITER           = 16,
 GAMEOBJECT_TYPE_FISHINGNODE            = 17,
 GAMEOBJECT_TYPE_SUMMONING_RITUAL       = 18,
 GAMEOBJECT_TYPE_MAILBOX                = 19,
 GAMEOBJECT_TYPE_DO_NOT_USE             = 20,
 GAMEOBJECT_TYPE_GUARDPOST              = 21,
 GAMEOBJECT_TYPE_SPELLCASTER            = 22,
 GAMEOBJECT_TYPE_MEETINGSTONE           = 23,
 GAMEOBJECT_TYPE_FLAGSTAND              = 24,
 GAMEOBJECT_TYPE_FISHINGHOLE            = 25,
 GAMEOBJECT_TYPE_FLAGDROP               = 26,
 GAMEOBJECT_TYPE_MINI_GAME              = 27,
 GAMEOBJECT_TYPE_DO_NOT_USE_2           = 28,
 GAMEOBJECT_TYPE_CAPTURE_POINT          = 29,
 GAMEOBJECT_TYPE_AURA_GENERATOR         = 30,
 GAMEOBJECT_TYPE_DUNGEON_DIFFICULTY     = 31,
 GAMEOBJECT_TYPE_BARBER_CHAIR           = 32,
 GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING  = 33,
 GAMEOBJECT_TYPE_GUILD_BANK             = 34,
 GAMEOBJECT_TYPE_TRAPDOOR               = 35
 */

#include "Log.h"
#include "ScriptMgr.h"
#include "Config.h"
#include "Chat.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "Item.h"

enum AutoLootString
{
    AOE_ACORE_STRING_MESSAGE = 50000,
    AOE_ITEM_IN_THE_MAIL = 50001
};

// Store item and fire OnPlayerLootItem so other modules (e.g. paragon-itemgen)
// can process the item.  Returns the created Item* or nullptr on failure.
static Item* StoreLootAndNotify(Player* player, uint32 itemId, uint32 count, ObjectGuid lootSource)
{
    ItemPosCountVec dest;
    if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count) != EQUIP_ERR_OK)
        return nullptr;

    Item* newItem = player->StoreNewItem(dest, itemId, true);
    if (!newItem)
        return nullptr;

    player->SendNewItem(newItem, count, true, false);
    sScriptMgr->OnPlayerLootItem(player, newItem, count, lootSource);
    return newItem;
}

class AutoLoot_Player : public PlayerScript
{
public:
    AutoLoot_Player() : PlayerScript("AutoLoot_Player", {PLAYERHOOK_ON_LOGIN, PLAYERHOOK_CAN_SEND_ERROR_ALREADY_LOOTED, PLAYERHOOK_ON_UPDATE}) { }

    void OnPlayerLogin(Player* player) override
    {
        if (sConfigMgr->GetOption<bool>("AOELoot.Enable", true))
        {
            ChatHandler(player->GetSession()).PSendSysMessage(AOE_ACORE_STRING_MESSAGE);
        }
    }

    bool OnPlayerCanSendErrorAlreadyLooted(Player* /*player*/) override
    {
        return true;
    }

    void OnPlayerUpdate(Player* player, uint32 /*p_time*/) override
    {
        bool _enable = sConfigMgr->GetOption<bool>("AOELoot.Enable", true);

        if (player->GetGroup() || !_enable || player->GetFreeInventorySpace() < 4)
            return;

        float range = 10.0f;
        uint32 gold = 0;

        std::list<Creature*> creaturedie;
        player->GetDeadCreatureListInGrid(creaturedie, range);

        if (creaturedie.size()) {
            for (auto const& _creature : creaturedie)
            {
                Loot* loot = &_creature->loot;
                gold += loot->gold;
                loot->gold = 0;
                uint8 lootSlot = 0;
                uint32 maxSlot = loot->GetMaxSlotInLootFor(player);

                ObjectGuid creatureGuid = _creature->GetGUID();
                for (uint32 i = 0; i < maxSlot; ++i)
                {
                    if (LootItem* item = loot->LootItemInSlot(i, player))
                    {
                        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(item->itemid);

                        if (itemTemplate->MaxCount != 1)
                        {
                            if (StoreLootAndNotify(player, item->itemid, item->count, creatureGuid))
                            {
                                player->SendNotifyLootItemRemoved(lootSlot);
                                player->SendLootRelease(player->GetLootGUID());
                            }
                            else if (sConfigMgr->GetOption<bool>("AOELoot.MailEnable", true))
                            {
                                player->SendItemRetrievalMail(item->itemid, item->count);
                                ChatHandler(player->GetSession()).SendSysMessage(AOE_ITEM_IN_THE_MAIL);
                            }
                        }
                        else
                        {
                            if (!player->HasItemCount(item->itemid, 1))
                            {
                                StoreLootAndNotify(player, item->itemid, item->count, creatureGuid);
                            }
                            player->SendNotifyLootItemRemoved(lootSlot);
                            player->SendLootRelease(player->GetLootGUID());
                        }
                    }
                }

                if (!loot->empty())
                {
                    if (!_creature->IsAlive())
                    {
                        _creature->AllLootRemovedFromCorpse();
                        _creature->RemoveDynamicFlag(UNIT_DYNFLAG_LOOTABLE);
                        loot->clear();

                        if (_creature->HasUnitFlag(UNIT_FLAG_SKINNABLE))
                        {
                            _creature->RemoveUnitFlag(UNIT_FLAG_SKINNABLE);
                        }
                    }
                }
                else
                {
                    _creature->RemoveDynamicFlag(UNIT_DYNFLAG_LOOTABLE);
                    _creature->AllLootRemovedFromCorpse();
                }
            }

            player->ModifyMoney(gold);
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY, gold);
            WorldPacket data(SMSG_LOOT_MONEY_NOTIFY, 4 + 1);
            data << uint32(gold);
            data << uint8(1);
            player->GetSession()->SendPacket(&data);
        }

        GameObject* myObj = player->FindNearestGameObjectOfType(GAMEOBJECT_TYPE_CHEST, 10.f);

        if (myObj) {
            if (player->HasSkill(186)) {

                if (myObj->getLootState() == GO_READY && !myObj->IsInvisibleDueToDespawn()) {
                    player->CastSpell(myObj, 2575, true);
                    Loot* loot = &myObj->loot;
                    uint8 lootSlot = 0;
                    uint32 maxSlot = loot->GetMaxSlotInLootFor(player);

                    ObjectGuid objGuid = myObj->GetGUID();
                    for (uint32 i = 0; i < maxSlot; ++i)
                    {
                        if (LootItem* item = loot->LootItemInSlot(i, player))
                        {
                            uint32 itemcount = item->count;
                            if (StoreLootAndNotify(player, item->itemid, itemcount, objGuid))
                            {
                                player->SendNotifyLootItemRemoved(lootSlot);
                                player->SendLootRelease(player->GetLootGUID());
                                myObj->DespawnOrUnsummon();
                            }
                        }
                    }


                }
            }


        }


    }

};

void AddSC_AutoLoot()
{
    new AutoLoot_Player();
}
