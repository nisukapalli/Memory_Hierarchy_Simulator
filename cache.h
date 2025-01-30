#include <iostream>
#include <bitset>
#include <stdio.h>
#include <cstdlib>
#include <string>
using namespace std;

#define L1_CACHE_SETS 16
#define L2_CACHE_SETS 16
#define VICTIM_SIZE 4
#define L2_CACHE_WAYS 8
#define MEM_SIZE 4096
#define BLOCK_SIZE 4 // bytes per block
#define DM 0
#define SA 1

struct cacheBlock
{
    int memAdr;
    int tag; // you need to compute offset and index to find the tag.
    int lru_position; // for SA only
    int data; // the actual data stored in the cache/memory
    bool valid;
};

struct Stat
{
    int hitL1;
    int hitL2;
    int hitVic;
    int missL1;
    int missL2;
    int missVic;
    int accL1;
    int accL2;
    int accVic;
};

class cache {
private:
    cacheBlock L1[L1_CACHE_SETS]; // 1 set per row.
    cacheBlock L2[L2_CACHE_SETS][L2_CACHE_WAYS]; // x ways per row
    cacheBlock victim[VICTIM_SIZE];

    Stat myStat;

public:
    cache();
    void controller(bool MemR, bool MemW, int* data, int adr, int* myMem);
    int getBlockOffset(int adr) {return adr & 0x3; }
    int getIndex(int adr) { return (adr >> 2) & 0xf; }
    int getTag(int adr) { return adr >> 6; }
    bool checkL1(bool MemR, bool MemW, int adr);
    int checkL2(bool MemR, bool MemW, int adr);
    int checkVictim(bool MemR, bool MemW, int adr);
    void storeInMem(int adr, int* data, int* myMem);
    void victimToL1(int adr, int way, int* data);
    void L2ToL1(int adr, int way, int* data);
    void memToL1(int adr, int* data, int* myMem);
    double getHitL1() { return (double) myStat.hitL1; }
    double getHitL2() { return (double) myStat.hitL2; }
    double getHitVic() { return (double) myStat.hitVic; }
    double getMissL1() { return (double) myStat.missL1; }
    double getMissL2() { return (double) myStat.missL2; }
    double getMissVic() { return (double) myStat.missVic; }
    double getAccL1() { return (double) myStat.accL1; }
    double getAccL2() { return (double) myStat.accL2; }
    double getAccVic() { return (double) myStat.accVic; }
};
