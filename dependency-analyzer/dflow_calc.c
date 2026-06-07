#include "dflow_calc.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
typedef struct
{
    int dest;
    bool isDependent;
    int maxDepth;
    unsigned int latency;
    int dep1;
    int dep2;
} InstData;

unsigned int numOfInstructions;

int max_depth(int num1, int num2)
{
    if (num1 > num2)
    {
        return num1;
    }
    return num2;
}

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts)
{
    InstData **inst_arr = (InstData **)malloc(sizeof(InstData *) * (numOfInsts));
    if (inst_arr == PROG_CTX_NULL)
    {
        return PROG_CTX_NULL;
    }
    numOfInstructions = numOfInsts;

    for (unsigned int i = 0; i < numOfInsts; i++)
    {
        inst_arr[i] = (InstData *)malloc(sizeof(InstData));
        if (inst_arr[i] == PROG_CTX_NULL)
        {
            for (unsigned int k = 0; k < i; k++)
            {
                free(inst_arr[k]);
            }
            free(inst_arr);
            return PROG_CTX_NULL;
        }
        inst_arr[i]->dest = progTrace[i].dstIdx;
        inst_arr[i]->isDependent = false;
        inst_arr[i]->maxDepth = 0;
        inst_arr[i]->latency = opsLatency[progTrace[i].opcode];
        inst_arr[i]->dep1 = -1;
        inst_arr[i]->dep2 = -1;
        // checking dependency
        for (int j = (i - 1); j >= 0; j--)
        {
            if ((inst_arr[j]->dest == progTrace[i].src1Idx) && (inst_arr[i]->dep1 == -1))
            {
                inst_arr[i]->dep1 = j;
                inst_arr[j]->isDependent = true;
            }
            if ((inst_arr[j]->dest == progTrace[i].src2Idx) && (inst_arr[i]->dep2 == -1))
            {
                inst_arr[i]->dep2 = j;
                inst_arr[j]->isDependent = true;
            }
        }
        // checking the deepest depth for inst i
        int dep1 = inst_arr[i]->dep1;
        int dep2 = inst_arr[i]->dep2;
        
        if ((dep1 >= 0) && (dep2 >= 0))
        {
            int arg1 = (inst_arr[dep1]->maxDepth) + (inst_arr[dep1]->latency);
            int arg2 = (inst_arr[dep2]->maxDepth) + (inst_arr[dep2]->latency);
            inst_arr[i]->maxDepth = max_depth(arg1, arg2);
        }
        else if (dep1 >= 0)
        {
            inst_arr[i]->maxDepth = (inst_arr[dep1]->maxDepth) + (inst_arr[dep1]->latency);
        }
        else if (dep2 >= 0)
        {
            inst_arr[i]->maxDepth = (inst_arr[dep2]->maxDepth) + (inst_arr[dep2]->latency);
        }
    }

    return inst_arr;
}

void freeProgCtx(ProgCtx ctx)
{
    InstData **inst_arr = ctx;
    for (unsigned int i = 0; i < numOfInstructions; i++)
    {
        free(inst_arr[i]);
    }
    free(inst_arr);
}

int getInstDepth(ProgCtx ctx, unsigned int theInst)
{
    if (ctx == NULL)
    {
        return -1;
    }
    InstData **inst_arr = ctx;
    if (theInst < 0 || theInst >= numOfInstructions)
    {
        return -1;
    }
    return inst_arr[theInst]->maxDepth;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst)
{
    if (ctx == NULL)
    {
        return -1;
    }
    InstData **inst_arr = ctx;
    if (theInst < 0 || theInst >= numOfInstructions)
    {
        return -1;
    }
    if (src1DepInst == NULL || src2DepInst == NULL)
    {
        return -1;
    }

    *src1DepInst = inst_arr[theInst]->dep1;
    *src2DepInst = inst_arr[theInst]->dep2;
    return 0;
}

int getProgDepth(ProgCtx ctx)
{
    if (ctx == NULL)
    {
        return -1;
    }
    InstData **inst_arr = ctx;
    int current_max = 0;
    for (int i = 0; i < numOfInstructions; i++)
    {
        if (current_max <= (inst_arr[i]->maxDepth) + inst_arr[i]->latency)
        {
            current_max = inst_arr[i]->maxDepth+inst_arr[i]->latency;
        }
    }
    return current_max;
}
