#include "cache.h"

cache::cache()
{
    for (int i = 0; i < L1_CACHE_SETS; i++)
        L1[i].valid = false;
    for (int i = 0; i < L2_CACHE_SETS; i++)
        for (int j = 0; j < L2_CACHE_WAYS; j++)
            L2[i][j].valid = false;
    for (int i = 0; i < VICTIM_SIZE; i++)
        victim[i].valid = false;

    this->myStat.hitL1 = 0;
    this->myStat.hitL2 = 0;
    this->myStat.hitVic = 0;
    this->myStat.missL1 = 0;
    this->myStat.missL2 = 0;
    this->myStat.missVic = 0;
    this->myStat.accL1 = 0;
    this->myStat.accL2 = 0;
    this->myStat.accVic = 0;
}

void cache::controller(bool MemR, bool MemW, int* data, int adr, int* myMem)
{
    int index = getIndex(adr);
    // check L1
    if (checkL1(MemR, MemW, adr)) {
        // load
        if (MemR) {
            this->myStat.hitL1++;
            this->myStat.accL1++;
        }
        // store
        if (MemW) {
            L1[index].data = *data;
            storeInMem(adr, data, myMem);
        }
    }
    else {
        // check victim
        int way = checkVictim(MemR, MemW, adr);
        if (way != -1) {
            // load
            if (MemR) {
                victimToL1(adr, way, data);
                this->myStat.missL1++;
                this->myStat.accL1++;
                this->myStat.hitVic++;
                this->myStat.accVic++;
            }
            // store
            if (MemW) {
                victim[way].data = *data;
                storeInMem(adr, data, myMem);
            }
        }
        else {
            // check L2
            int way = checkL2(MemR, MemW, adr);
            if (way != -1) {
                // load
                if (MemR) {
                    L2ToL1(adr, way, data);
                    this->myStat.missL1++;
                    this->myStat.accL1++;
                    this->myStat.missVic++;
                    this->myStat.accVic++;
                    this->myStat.hitL2++;
                    this->myStat.accL2++;
                }
                // store
                if (MemW) {
                    storeInMem(adr, data, myMem);
                }
            }
            // check main memory
            else {
                // load
                if (MemR) {
                    memToL1(adr, data, myMem);
                    this->myStat.missL1++;
                    this->myStat.accL1++;
                    this->myStat.missVic++;
                    this->myStat.accVic++;
                    this->myStat.missL2++;
                    this->myStat.accL2++;
                }
                // store
                if (MemW) {
                    storeInMem(adr, data, myMem);
                }
            }
        }
    }
}


bool cache::checkL1(bool MemR, bool MemW, int adr)
{
    int index = getIndex(adr);
    if (adr/4 == L1[index].memAdr/4 && L1[index].valid) {
        return true;
    }
    return false;
}

int cache::checkL2(bool MemR, bool MemW, int adr)
{
    int index = getIndex(adr);
    for (int i = 0; i < L2_CACHE_WAYS; i++) {
        if (adr/4 == L2[index][i].memAdr/4 && L2[index][i].valid) {
            return i;
        }
    }
    return -1;
}

int cache::checkVictim(bool MemR, bool MemW, int adr)
{
    for (int i = 0; i < VICTIM_SIZE; i++) {
        if (adr/4 == victim[i].memAdr/4 && victim[i].valid) {
            return i;
        }
    }
    return -1;
}

void cache::storeInMem(int adr, int* data, int* myMem)
{
    myMem[adr] = *data & 0xff;
}

void cache::victimToL1(int adr, int way, int* data)
{
    int index = getIndex(adr);
    cacheBlock evictL1 = L1[index];
    L1[index] = victim[way];
    victim[way].valid = false;
    
    if (evictL1.valid) {
        int lru = 0;
        int availableWay = -1;
        for (int i = 0; i < VICTIM_SIZE; i++) {
            // count LRU position
            if (victim[i].valid) {
                lru++;
            }
            // first available way for evicted L1 block
            else if (availableWay == -1) {
                availableWay = i;
            }
        }
        
        // update victim LRU
        for (int i = 0; i < VICTIM_SIZE; i++) {
            if (victim[i].valid && victim[i].lru_position > victim[way].lru_position) {
                victim[i].lru_position--;
            }
        }
        evictL1.lru_position = lru;
        victim[availableWay] = evictL1;
    }
}

void cache::L2ToL1(int adr, int way, int* data)
{
    int index = getIndex(adr);
    cacheBlock evictL1 = L1[index];
    L1[index] = L2[index][way];
    L2[index][way].valid = false;
    
    if (evictL1.valid) {
        // check victim
        int lru = 0;
        int availableWay = -1;
        int evictWay = -1;
        for (int i = 0; i < VICTIM_SIZE; i++) {
            // count LRU position
            if (victim[i].valid) {
                lru++;
                // evict this way if victim is full
                if (victim[i].lru_position == 0) {
                    evictWay = i;
                }
            }
            // first available way for evicted L1 block
            else if (availableWay == -1) {
                availableWay = i;
            }
        }
        
        evictL1.lru_position = lru;
        
        // if there's an available way, insert evicted L1 block
        if (lru < VICTIM_SIZE) {
            victim[availableWay] = evictL1;
        }
        // evict from victim if full
        else {
            cacheBlock evictVictim = victim[evictWay];
            victim[evictWay] = evictL1;
            // update victim LRU
            for (int i = 0; i < VICTIM_SIZE; i++) {
                victim[i].lru_position--;
            }
            
            // check L2
            index = getIndex(evictVictim.memAdr);
            lru = 0;
            availableWay = -1;
            evictWay = -1;
            for (int i = 0; i < L2_CACHE_WAYS; i++) {
                // count LRU position
                if (L2[index][i].valid) {
                    lru++;
                }
                // first available way for evicted victim block
                else if (availableWay == -1) {
                    availableWay = i;
                }
            }
            
            // update L2 LRU
            for (int i = 0; i < VICTIM_SIZE; i++) {
                if (L2[index][i].valid && L2[index][i].lru_position > L2[index][way].lru_position) {
                    L2[index][i].lru_position--;
                }
            }
            evictVictim.lru_position = lru;
            L2[index][availableWay] = evictVictim;
        }
    }
}

void cache::memToL1(int adr, int* data, int* myMem)
{
    int index = getIndex(adr);
    int tag = getTag(adr);
    cacheBlock evictL1 = L1[index];
    L1[index] = cacheBlock();
    L1[index].memAdr = adr;
    L1[index].tag = tag;
    L1[index].data = myMem[adr];
    L1[index].valid = 1;
    
    if (evictL1.valid) {
        // check victim
        int lru = 0;
        int availableWay = -1;
        int evictWay = -1;
        for (int i = 0; i < VICTIM_SIZE; i++) {
            // count LRU position
            if (victim[i].valid) {
                lru++;
                // evict this way if victim is full
                if (victim[i].lru_position == 0) {
                    evictWay = i;
                }
            }
            // first available way for evicted L1 block
            else if (availableWay == -1) {
                availableWay = i;
            }
        }
        
        evictL1.lru_position = lru;
        
        // if there's an available way, insert evicted L1 block
        if (lru < VICTIM_SIZE) {
            victim[availableWay] = evictL1;
        }
        // evict from victim if full
        else {
            cacheBlock evictVictim = victim[evictWay];
            victim[evictWay] = evictL1;
            // update victim LRU
            for (int i = 0; i < VICTIM_SIZE; i++) {
                victim[i].lru_position--;
            }
            
            // check L2
            index = getIndex(evictVictim.memAdr);
            lru = 0;
            availableWay = -1;
            evictWay = -1;
            for (int i = 0; i < L2_CACHE_WAYS; i++) {
                // count LRU position
                if (L2[index][i].valid) {
                    lru++;
                    // evict this way if L2 is full
                    if (L2[index][i].lru_position == 0) {
                        evictWay = i;
                    }
                }
                // first available way for evicted victim block
                else if (availableWay == -1) {
                    availableWay = i;
                }
            }
            
            evictVictim.lru_position = lru;
            
            // if there's an available way, insert evicted victim block
            if (lru < L2_CACHE_WAYS) {
                L2[index][availableWay] = evictVictim;
            }
            // evict from L2 if full
            else {
                L2[index][evictWay] = evictVictim;
                // update L2 LRU
                for (int i = 0; i < L2_CACHE_WAYS; i++) {
                    L2[index][i].lru_position--;
                }
            }
        }
    }
}
