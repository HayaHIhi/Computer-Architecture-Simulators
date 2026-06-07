/* 046267 Computer Architecture - Winter 20/21 - HW #2                  */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#define ALIGN 0

typedef struct cacheType
{
    unsigned *tag;
    unsigned *lru;
    bool *valid;
    bool *dirty;
    unsigned cacheSize; // input is: <log2(size)>
    unsigned blockSize; // input is: <block log2(size)>
    unsigned setSize;
    unsigned Nways;   // input is: log2(# of ways)
    unsigned time;    // input is: <# of cycles>
    unsigned memTime; // input is: <# of cycles>
    bool writeAlloc;

} *cache;

typedef struct statsType
{
    double L1MissRate;
    double L2MissRate;
    double avgAccTime;
    int numHitL1;
    int numMissL1;
    int numHitL2;
    int numMissL2;
    int totalTime;
} *Stats;

cache L1;
cache L2;
Stats stats;

unsigned int bitExtracted(unsigned number, unsigned length, unsigned start);
unsigned setCulc(unsigned long int address, int setSize, int blockSize);
unsigned tagCulc(unsigned long int address, int setSize, int blockSize);
int searchL1(unsigned setL1, unsigned tagL1);
int searchL2(unsigned setL2, unsigned tagL2);
void updateLRUL1(unsigned setL1, int lineL1);
void updateLRUL2(unsigned setL2, int lineL2);
int updateL1(unsigned setL1, unsigned tagL1);
int updateL2(unsigned setL2, unsigned tagL2);

int Cache_init(unsigned MemCyc, unsigned BSize, unsigned L1Size, unsigned L2Size, unsigned L1Assoc,
            unsigned L2Assoc, unsigned L1Cyc, unsigned L2Cyc, unsigned WrAlloc)
{
    int i;
    int L1Size_pow2 = pow(2,L1Size);
    int L2Size_pow2 = pow(2,L2Size);

    L1 = (struct cacheType *)malloc(sizeof(struct cacheType));
    if (L1 == NULL)
        return -1;
    L2 = (struct cacheType *)malloc(sizeof(struct cacheType));
    if (L2 == NULL)
    {
        free(L1);
        return -1;
    }
    L1->tag = (unsigned *)malloc(sizeof(unsigned) * L1Size_pow2);
    if (L1->tag == NULL)
    {
        free(L2);
        free(L1);
        return -1;
    }
    L2->tag = (unsigned *)malloc(sizeof(unsigned) * L2Size_pow2);
    if (L2->tag == NULL)
    {
        free(L1->tag);
        free(L2);
        free(L1);
        return -1;
    }
    L1->lru = (unsigned *)malloc(sizeof(unsigned) * L1Size_pow2);
    if (L1->lru == NULL)
    {
        free(L2->tag);
        free(L1->tag);
        free(L2);
        free(L1);
        return -1;
    }
    L2->lru = (unsigned *)malloc(sizeof(unsigned) * L2Size_pow2);
    if (L2->lru == NULL)
    {
        free(L1->lru);
        free(L2->tag);
        free(L1->tag);
        free(L2);
        free(L1);
        return -1;
    }
    L1->valid = (bool *)malloc(sizeof(bool) * L1Size_pow2);
    if (L1->valid == NULL)
    {
        free(L2->lru);
        free(L1->lru);
        free(L2->tag);
        free(L1->tag);
        free(L2);
        free(L1);
        return -1;
    }
    L2->valid = (bool *)malloc(sizeof(bool) * L2Size_pow2);
    if (L2->valid == NULL)
    {
        free(L1->valid);
        free(L2->lru);
        free(L1->lru);
        free(L2->tag);
        free(L1->tag);
        free(L2);
        free(L1);
        return -1;
    }
    L1->dirty = (bool *)malloc(sizeof(bool) * L1Size_pow2);
    if (L1->dirty == NULL)
    {
        free(L2->valid);
        free(L1->valid);
        free(L2->lru);
        free(L1->lru);
        free(L2->tag);
        free(L1->tag);
        free(L2);
        free(L1);
        return -1;
    }
    L2->dirty = (bool *)malloc(sizeof(bool) * L2Size_pow2);
    if (L2->dirty == NULL)
    {
        free(L1->dirty);
        free(L2->valid);
        free(L1->valid);
        free(L2->lru);
        free(L1->lru);
        free(L2->tag);
        free(L1->tag);
        free(L2);
        free(L1);
        return -1;
    }

    stats = (struct statsType *)malloc(sizeof(struct statsType));
    if (stats == NULL)
    {
        free(L2->dirty);
        free(L1->dirty);
        free(L2->valid);
        free(L1->valid);
        free(L2->lru);
        free(L1->lru);
        free(L2->tag);
        free(L1->tag);
        free(L2);
        free(L1);
        return -1;
    }

    stats->avgAccTime = 0;
    stats->L1MissRate = 0;
    stats->L2MissRate = 0;
    stats->numHitL1 = 0;
    stats->numHitL2 = 0;
    stats->numMissL1 = 0;
    stats->numMissL2 = 0;
    stats->totalTime = 0;

    L1->cacheSize = pow(2, L1Size);
    L1->blockSize = pow(2, BSize);
    // L1->setSize = L1Size / (BSize * L1Assoc); //if it was size values and not log2
    L1->setSize = pow(2, L1Size - (BSize + L1Assoc));
    L1->Nways = pow(2, L1Assoc);
    L1->time = L1Cyc;
    L1->writeAlloc = WrAlloc;
    L1->memTime = MemCyc;

    L2->cacheSize = pow(2, L2Size);
    L2->blockSize = pow(2, BSize);
    // L2->setSize = L2Size / (BSize * L2Assoc);//if it was size values and not log2
    L2->setSize = pow(2, L2Size - (BSize + L2Assoc));
    L2->Nways = pow(2, L2Assoc);
    L2->time = L2Cyc;
    L2->writeAlloc = WrAlloc;
    L2->memTime = MemCyc;
    int L1_size = (int)(L1->cacheSize);
    int L2_size = (int)(L2->cacheSize);

    for (i = 0; i < L1_size; i++)
    {
        L1->valid[i] = false;
        L1->lru[i] = i % ((int)pow(2, L1Assoc)); // Initial State like in the lecture
    }
    for (i = 0; i < L2_size; i++)
    {
        L2->valid[i] = false;
        L2->lru[i] = i % ((int)pow(2, L2Assoc)); // Initial State like in the lecture
    }
    // printf("\nL1->cacheSize = %u, L1->blockSize = %u, L1->setSize = %u, L1->Nways = %u\n", L1->cacheSize, L1->blockSize, L1->setSize, L1->Nways);
    // printf("\nL2->cacheSize = %u, L2->blockSize = %u, L2->setSize = %u, L2->Nways = %u\n", L2->cacheSize, L2->blockSize, L2->setSize, L2->Nways);
    return 0;
}

int Cache_write(unsigned long int address)
{
    unsigned setL1 = setCulc(address, L1->setSize, L1->blockSize);
    unsigned tagL1 = tagCulc(address, L1->setSize, L1->blockSize);
    int lineL1 = searchL1(setL1, tagL1);
    // printf("\nCache_write");
    // printf("\naddress = %lX, setL1 = %X, tagL1 = %X, lineL1=%d\n", address, setL1, tagL1, lineL1);
    stats->totalTime += L1->time;
    if (lineL1 >= 0)
    {
        L1->dirty[lineL1] = 1;
        stats->numHitL1++;
        updateLRUL1(setL1, lineL1);
        return 0;
    }
    else
    {
        stats->numMissL1++;
        if(L1->writeAlloc == true){
            lineL1 = updateL1(setL1, tagL1);
            L1->dirty[lineL1] = true;
        }
        unsigned setL2 = setCulc(address, L2->setSize, L2->blockSize);
        unsigned tagL2 = tagCulc(address, L2->setSize, L2->blockSize);
        int lineL2 = searchL2(setL2, tagL2);
        // printf("setL2 = %X, tagL2 = %X, lineL2=%d\n", setL2, tagL2, lineL2);
        stats->totalTime += L2->time; // time to access to L2
        if (lineL2 >= 0)
        {
            L2->dirty[lineL2] = 1;
            stats->numHitL2++;
            updateLRUL2(setL2, lineL2);
            return 0;
        }
        stats->numMissL2++;
        if(L2->writeAlloc == true){
            lineL2 = updateL2(setL2, tagL2);
            L2->dirty[lineL2] = true;
        }
        stats->totalTime += L2->memTime; // time to access to mem
        // printf("mem\n");
        return 0;
    }
    return 0;
}

int Cache_read(unsigned long int address)
{
    unsigned setL1 = setCulc(address, L1->setSize, L1->blockSize);
    unsigned tagL1 = tagCulc(address, L1->setSize, L1->blockSize);
    int lineL1 = searchL1(setL1, tagL1);
    // printf("\nCache_read");
    // printf("\naddress = %lX, setL1 = %X, tagL1 = %X, lineL1=%d\n", address, setL1, tagL1, lineL1);
    stats->totalTime += L1->time; // time to access to L1
    if (lineL1 >= 0)
    {
        stats->numHitL1++;
        updateLRUL1(setL1, lineL1);
        return 0;
    }
    stats->numMissL1++;
    updateL1(setL1, tagL1);
    unsigned setL2 = setCulc(address, L2->setSize, L2->blockSize);
    unsigned tagL2 = tagCulc(address, L2->setSize, L2->blockSize);
    int lineL2 = searchL2(setL2, tagL2);
    // printf("setL2 = %X, tagL2 = %X, lineL2=%d\n", setL2, tagL2, lineL2);
    stats->totalTime += L2->time; // time to access to L2
    if (lineL2 >= 0)
    {
        stats->numHitL2++;
        updateLRUL2(setL2, lineL2);
        return 0;
    }
    
    stats->numMissL2++;
    updateL2(setL2, tagL2);
    stats->totalTime += L2->memTime; // time to access to mem
    // printf("mem\n");
    return 0;
}
//
int get_stats(double *L1MissRate, double *L2MissRate, double *avgAccTime)
{
    double total_L1 = stats->numHitL1 + stats->numMissL1;
    double total_l2 = stats->numHitL2 + stats->numMissL2;
    // if the #hit +#miss==0 what shouls i return?
    *L1MissRate = (double)(stats->numMissL1 / (total_L1));
    *L2MissRate = (double)(stats->numMissL2 / (total_l2));
    // double L1_HitRate = 1 - *L1MissRate;
    // double L2_HitRate = 1 - *L2MissRate;

    *avgAccTime = (((total_L1)*L1->time) + (stats->numMissL1 * L2->time) + stats->numMissL2 * L2->memTime) / (total_L1);
    free(stats);
    free(L2->dirty);
    free(L2->valid);
    free(L2->lru);
    free(L2->tag);
    free(L2);
    free(L1->dirty);
    free(L1->valid);
    free(L1->lru);
    free(L1->tag);
    free(L1);
    return 0;
}

// extract length number of bits, starting at bit start
unsigned bitExtracted(unsigned number, unsigned length, unsigned start)
{
    return (((1 << length) - 1) & (number >> (start)));
}

unsigned setCulc(unsigned long int address, int setSize, int blockSize)
{
    if(setSize > 1){
        return bitExtracted(address, log2(setSize), log2(blockSize));
    }else{
        return 0;
    }
    
}

unsigned tagCulc(unsigned long int address, int setSize, int blockSize)
{
    if(setSize > 1){
        return bitExtracted(address, 32 - log2(setSize) - log2(blockSize), log2(setSize) + log2(blockSize));
    }else{
        return bitExtracted(address, 32 - log2(blockSize), log2(blockSize));
    }
    
}

int searchL1(unsigned setL1, unsigned tagL1)
{
    int i = 0;
    // printf("\nsearchL1: for: setL1= %X, tagL1 = %X\n", setL1, tagL1);
    for (i = setL1 * L1->Nways; i < ((setL1 * L1->Nways) + L1->Nways); i++)
    {
        // printf("\n L1->valid[i] = %d, L1->tag[i] = %X\n", L1->valid[i], L1->tag[i]);
        if ((L1->valid[i] == true) && (L1->tag[i] == tagL1))
        {
            return i;
        }
    }
    return -1;
}

int searchL2(unsigned setL2, unsigned tagL2)
{
    int i = 0;
    // printf("\nsearchL2: for: setL2= %X, tagL2 = %X\n", setL2, tagL2);
    // printf("\nsetL2 * L2->Nways: %d, (int)L2->Nways = %d\n", setL2 * L2->Nways, (int)L2->Nways);
    for (i = setL2 * L2->Nways; i < ((setL2 * L2->Nways) + L2->Nways); i++)
    {
        // printf("\nin for of searchL2\n");
        // printf("\n L2->valid[i] = %d, L2->tag[i] = %X\n", L2->valid[i], L2->tag[i]);
        
        if ((L2->valid[i] == true) && (L2->tag[i] == tagL2))
        {
            return i;
        }
    }
    return -1;
}

void updateLRUL1(unsigned setL1, int lineL1)
{
    int i = 0;
    unsigned prev = L1->lru[lineL1];
    L1->lru[lineL1] = L1->Nways - 1;
    for (i = setL1 * L1->Nways; i < ((setL1 * L1->Nways) + L1->Nways); i++)
    {
        if ((i != lineL1) && (L1->lru[i] > prev))
        {
            L1->lru[i]--;
        }
    }
    return;
}

void updateLRUL2(unsigned setL2, int lineL2)
{
    int i = 0;
    unsigned prev = L2->lru[lineL2];
    L2->lru[lineL2] = L2->Nways - 1;
    for (i = setL2 * L2->Nways; i < ((setL2 * L2->Nways) + L2->Nways); i++)
    {
        if ((i != lineL2) && (L2->lru[i] > prev))
        {
            L2->lru[i]--;
        }
    }
    return;
}

int updateL1(unsigned setL1, unsigned tagL1)
{
    int lineL2, i = 0;
    unsigned int setL2, tagL2, addressL2;
    for (i = setL1 * L1->Nways; i < ((setL1 * L1->Nways) + L1->Nways); i++)
    {
        if (L1->valid[i] == false)
        {
            // printf("\nupdateL1: setL1 = %X, tagL1 = %X, line = %d\n", setL1, tagL1, i);
            L1->valid[i] = true;
            L1->dirty[i] = false;
            L1->tag[i] = tagL1;
            updateLRUL1(setL1, i);
            return i;
        }
    }
    for (i = setL1 * L1->Nways; i < ((setL1 * L1->Nways) + L1->Nways); i++)
    {
        // LRU Implementation like in the lecture -> evict way with counter == 0
        if (L1->lru[i] == 0)
        {
            if (L1->dirty[i] == true)
            {
                // writhe back to L2; effect LRU at L2?????? ; not costing time according to the pdf
                addressL2 = bitExtracted(L1->tag[i], 31 - log2(L1->setSize) - log2(L2->blockSize), 0) << (unsigned int)(log2(L1->blockSize) + log2(L1->setSize) - ALIGN);
                addressL2 += bitExtracted(setL1, log2(L1->setSize), 0) << (unsigned int)(log2(L1->blockSize) - ALIGN);
                setL2 = setCulc(addressL2, L2->setSize, L2->blockSize);
                tagL2 = tagCulc(addressL2, L2->setSize, L2->blockSize);
                lineL2 = searchL2(setL2, tagL2);
                L2->dirty[lineL2] = true;
                updateLRUL2(setL2, lineL2);
            }
            L1->valid[i] = true;
            L1->dirty[i] = false;
            L1->tag[i] = tagL1;
            updateLRUL1(setL1, i);
            return i;
        }
    }
    return -1;
}

int updateL2(unsigned setL2, unsigned tagL2)
{
    int i = 0, lineL1, lineL2D;
    unsigned setL1, tagL1, addressL1;
    unsigned setL2D, tagL2D, addressL2D;

    for (i = setL2 * L2->Nways; i < ((setL2 * L2->Nways) + L2->Nways); i++)
    {
        if (L2->valid[i] == false)
        {
            L2->valid[i] = true;
            L2->dirty[i] = false;
            L2->tag[i] = tagL2;
            updateLRUL2(setL2, i);
            return i;
        }
    }
    for (i = setL2 * L2->Nways; i < ((setL2 * L2->Nways) + L2->Nways); i++)
    {
        if (L2->lru[i] == 0)
        {
            if (L2->dirty[i] == true)
            {
                // write back to mem -> not costing time according to the pdf
                // do we need to do something?????
            }
            addressL1 = bitExtracted(L2->tag[i], 31 - log2(L2->setSize) - log2(L1->blockSize), 0) << (unsigned int)(log2(L1->blockSize) + log2(L2->setSize) - ALIGN);
            addressL1 += bitExtracted(setL2, log2(L2->setSize), 0) << (unsigned int)(log2(L1->blockSize) - ALIGN);
            setL1 = setCulc(addressL1, L1->setSize, L1->blockSize);
            tagL1 = tagCulc(addressL1, L1->setSize, L1->blockSize);
            lineL1 = searchL1(setL1, tagL1);
            if (lineL1 >= 0)
            {
                // need to remove from L1
                if (L1->dirty[lineL1] == true)
                {
                    // write back to L2; effect LRU at L2??????
                    addressL2D = bitExtracted(L1->tag[lineL1], 31 - log2(L1->setSize) - log2(L2->blockSize), 0) << (unsigned int)(log2(L1->blockSize) + log2(L1->setSize) - ALIGN);
                    addressL2D += bitExtracted(setL1, log2(L1->setSize), 0) << (unsigned int)(log2(L1->blockSize) - ALIGN);
                    setL2D = setCulc(addressL2D, L2->setSize, L2->blockSize);
                    tagL2D = tagCulc(addressL2D, L2->setSize, L2->blockSize);
                    lineL2D = searchL2(setL2D, tagL2D);
                    L2->dirty[lineL2D] = true;
                    updateLRUL2(setL2D, lineL2D);
                }
                L1->valid[lineL1] = false;
                updateLRUL1(setL1, lineL1);
            }
            L2->valid[i] = true;
            L2->dirty[i] = false;
            L2->tag[i] = tagL2;
            updateLRUL2(setL2, i);
            return i;
        }
    }
    return -1;
}

// int main_func(unsigned int op, char r_or_w)
// {
// }
