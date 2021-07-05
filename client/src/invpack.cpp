/*
 * =====================================================================================
 *
 *       Filename: invpack.cpp
 *        Created: 11/11/2017 01:03:43
 *    Description:
 *
 *        Version: 1.0
 *       Revision: none
 *       Compiler: gcc
 *
 *         Author: ANHONG
 *          Email: anhonghe@gmail.com
 *   Organization: USTC
 *
 * =====================================================================================
 */

#include "dbcomid.hpp"
#include "invpack.hpp"
#include "pngtexdb.hpp"
#include "fflerror.hpp"
#include "sdldevice.hpp"
#include "dbcomrecord.hpp"

extern PNGTexDB *g_itemDB;
void InvPack::add(SDItem item)
{
    fflassert(item);
    for(auto &bin: m_packBinList){
        if((bin.item.itemID == item.itemID) && (bin.item.seqID == item.seqID)){
            if(bin.item.count + item.count <= SYS_INVGRIDMAXHOLD){
                bin.item.count += item.count;
                return;
            }
            else{
                throw fflerror("adding overlapping item exceeds SYS_INVGRIDMAXHOLD: %s", to_cstr(item.str()));
            }
        }
    }

    // when reaching here
    // means we need a new grid for the item

    Pack2D pack2D(w());
    for(auto &bin: m_packBinList){
        pack2D.put(bin.x, bin.y, bin.w, bin.h);
    }

    auto addedBin = makePackBin(item);
    pack2D.add(addedBin);
    m_packBinList.push_back(addedBin);
}

void InvPack::add(SDItem item, int x, int y)
{
    fflassert(item);
    auto itemBin = makePackBin(item);

    if(!(x >= 0 && x + itemBin.w <= to_d(w()) && y >= 0)){
        add(item);
        return;
    }

    Pack2D pack2D(w());
    for(auto &bin: m_packBinList){
        pack2D.put(bin.x, bin.y, bin.w, bin.h);
    }

    if(pack2D.occupied(x, y, itemBin.w, itemBin.h, true)){
        add(item);
        return;
    }

    itemBin.x = x;
    itemBin.y = y;
    m_packBinList.push_back(itemBin);
}

int InvPack::update(SDItem item)
{
    fflassert(item);
    if(item.seqID <= 0){
        throw fflerror("udpate item with zero seqID: %s", to_cstr(item.str()));
    }

    for(auto &bin: m_packBinList){
        if((bin.item.itemID == item.itemID) && (bin.item.seqID == item.seqID)){
            const int changed = to_d(item.count) - to_d(bin.item.count);
            bin.item = std::move(item);
            return changed;
        }
    }

    const int changed = to_d(item.count);
    add(std::move(item));
    return changed;
}

size_t InvPack::remove(uint32_t itemID, uint32_t seqID, size_t count)
{
    for(auto &bin: m_packBinList){
        if((bin.item.itemID == itemID) && (bin.item.seqID == seqID)){
            if(bin.item.count <= count){
                const auto doneCount = bin.item.count;
                std::swap(bin, m_packBinList.back());
                m_packBinList.pop_back();
                return doneCount;;
            }
            else{
                bin.item.count -= count;
                return count;
            }
        }
    }
    return 0;
}

std::tuple<int, int> InvPack::getPackBinSize(uint32_t itemID)
{
    if(auto texPtr = g_itemDB->retrieve(DBCOM_ITEMRECORD(itemID).pkgGfxID | 0X01000000)){
        const auto [itemPW, itemPH] = SDLDeviceHelper::getTextureSize(texPtr);
        return
        {
            (itemPW + (SYS_INVGRIDPW - 1)) / SYS_INVGRIDPW,
            (itemPH + (SYS_INVGRIDPH - 1)) / SYS_INVGRIDPH,
        };
    }
    throw fflerror("can't find size: itemID = %llu", to_llu(itemID));
}

PackBin InvPack::makePackBin(SDItem item)
{
    const auto [w, h] = getPackBinSize(item.itemID);
    return
    {
        .item = std::move(item),
        .x = -1,
        .y = -1,
        .w =  w,
        .h =  h,
    };
}

void InvPack::setGrabbedItem(SDItem item)
{
    if(item.itemID){
        fflassert(item);
        m_grabbedItem = std::move(item);
    }
    else{
        m_grabbedItem = {};
    }
}

void InvPack::setGold(int gold)
{
    if(gold < 0){
        throw fflerror("invalid argument: gold = %d", gold);
    }
    m_gold = (size_t)(gold);
}

void InvPack::addGold(int gold)
{
    if(const auto newGold = to_d(m_gold) + gold; newGold >= 0){
        m_gold = (size_t)(newGold);
    }
    throw fflerror("invalid argument: gold = %d", gold);
}

void InvPack::setInventory(const SDInventory &sdInv)
{
    m_packBinList.clear();
    for(const auto &item: sdInv.getItemList()){
        add(item);
    }
}
