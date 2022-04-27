/**********************************************************************************
///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2019: Originally written by Nuke.YKT.
// A lite version of pqueue from Nblood adapted for Preview Mode
//
// This file is part of XMAPEDIT.
//
// XMAPEDIT is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License version 2
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
////////////////////////////////////////////////////////////////////////////////////
***********************************************************************************/

#ifndef __XMPPQ
#define __XMPPQ

#include <set>
#include <functional>
#include "common_game.h"
#define kPQueueSize 1024

template <typename T> struct queueItem
{
    uint32_t at0; // priority
    T at4; // data
    bool operator>(const queueItem& other) const
    {
        return at0 > other.at0;
    }
    bool operator<(const queueItem& other) const
    {
        return at0 < other.at0;
    }
    bool operator>=(const queueItem& other) const
    {
        return at0 >= other.at0;
    }
    bool operator<=(const queueItem& other) const
    {
        return at0 <= other.at0;
    }
    bool operator!=(const queueItem& other) const
    {
        return at0 != other.at0;
    }
    bool operator==(const queueItem& other) const
    {
        return at0 == other.at0;
    }
};

template<typename T> class PriorityQueue
{
public:
    virtual ~PriorityQueue() {}
    virtual uint32_t Size(void) = 0;
    virtual void Clear(void) = 0;
    virtual void Insert(uint32_t, T) = 0;
    virtual T Remove(void) = 0;
    virtual uint32_t LowestPriority(void) = 0;
    virtual void Kill(std::function<bool(T)> pMatch) = 0;
};

template<typename T> class StdPriorityQueue : public PriorityQueue<T>
{
public:
    std::multiset<queueItem<T>> stdQueue;
    ~StdPriorityQueue()
    {
        stdQueue.clear();
    }
    uint32_t Size(void) { return stdQueue.size(); };
    void Clear(void)
    {
        stdQueue.clear();
    }
    void Insert(uint32_t nPriority, T data)
    {
        stdQueue.insert({ nPriority, data });
    }
    T Remove(void)
    {
        dassert(stdQueue.size() > 0);
        T data = stdQueue.begin()->at4;
        stdQueue.erase(stdQueue.begin());
        return data;
    }
    uint32_t LowestPriority(void)
    {
        return stdQueue.begin()->at0;
    }
    void Kill(std::function<bool(T)> pMatch)
    {
        for (auto i = stdQueue.begin(); i != stdQueue.end();)
        {
            if (pMatch(i->at4))
                i = stdQueue.erase(i);
            else
                i++;
        }
    }
};
#endif