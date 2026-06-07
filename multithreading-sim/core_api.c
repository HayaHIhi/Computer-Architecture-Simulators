/* 046267 Computer Architecture - Spring 2020 - HW #4 */


#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>

#define BLOCKED 0
#define FINE_GRAINED 1
#define READY -2
#define HALT -1
#define LOAD 1
#define STORE 2

typedef struct parameters
{
    uint32_t *instruction_num;// an array that holds in each index the number of instructions in each thread
    int total_cycles;
    int running_total_threads;
    int *thread_state;
    int load_latency;
    int store_latency;
    int switch_latency;
    int threads_num;
    int *reg_file;
} MTData;

MTData *blockedMT;
MTData *fineGrainedMT;

MTData *init(int sort, MTData *params)
{
    params = (MTData *)malloc(sizeof(*params));
    if (params == NULL)
    {
        return NULL;
    }
    params->total_cycles = 0;
    params->running_total_threads = SIM_GetThreadsNum();
    params->load_latency = SIM_GetLoadLat();
    params->store_latency = SIM_GetStoreLat();
    params->switch_latency = SIM_GetSwitchCycles();
    params->threads_num = SIM_GetThreadsNum();
    params->instruction_num = (uint32_t *)malloc(sizeof(uint32_t) * (params->threads_num));
    if (params->instruction_num == NULL)
    {
        free(params);
        return NULL;
    }
    for (int i = 0; i < (params->threads_num); i++)
    {
        params->instruction_num[i] = 0;
    }
    params->thread_state = (int *)malloc(sizeof(int) * 2 * (params->threads_num));
    if (params->thread_state == NULL)
    {
        free(params->instruction_num);
        free(params);
        return NULL;
    }
    for (int i = 0; i < (params->threads_num); i++)
    {
        params->thread_state[i] = 0;
        params->thread_state[i + (params->threads_num)] = READY;
    }
    params->reg_file = (int *)malloc(sizeof(int) * REGS_COUNT * (params->threads_num));
    if (params->reg_file == NULL)
    {
        free(params->instruction_num);
        free(params->thread_state);
        free(params);
        return NULL;
    }
    for (int i = 0; i < REGS_COUNT * (params->threads_num); i++)
    {
        params->reg_file[i] = 0;
    }
    return params;
}



// do the operations and update registers and memory situation
static void update(MTData *params, Instruction *cur_instruction, int cur_thread)
{
    int dest = cur_thread * REGS_COUNT + (cur_instruction->dst_index);
    int arg1 = cur_thread * REGS_COUNT + (cur_instruction->src1_index);
    int arg2 = cur_thread * REGS_COUNT + (cur_instruction->src2_index_imm);
    if (cur_instruction->opcode == CMD_HALT)
    {
        params->running_total_threads--;
        params->thread_state[cur_thread] = 0;
        params->thread_state[cur_thread + (params->threads_num)] = HALT;
    }
    else if (cur_instruction->opcode == CMD_ADD)
    {
        params->reg_file[dest] = params->reg_file[arg1] + params->reg_file[arg2];
        params->thread_state[cur_thread] = 0;
        params->thread_state[cur_thread + (params->threads_num)] = READY;
    }
    else if (cur_instruction->opcode == CMD_SUB)
    {
        params->reg_file[dest] = params->reg_file[arg1] - params->reg_file[arg2];
        params->thread_state[cur_thread] = 0;
        params->thread_state[cur_thread + (params->threads_num)] = READY;
    }
    else if (cur_instruction->opcode == CMD_ADDI)
    {
        params->reg_file[dest] = params->reg_file[arg1] + cur_instruction->src2_index_imm;
        params->thread_state[cur_thread] = 0;
        params->thread_state[cur_thread + (params->threads_num)] = READY;
    }
    else if (cur_instruction->opcode == CMD_SUBI)
    {
        params->reg_file[dest] = params->reg_file[arg1] - cur_instruction->src2_index_imm;
        params->thread_state[cur_thread] = 0;
        params->thread_state[cur_thread + (params->threads_num)] = READY;
    }
    else if (cur_instruction->opcode == CMD_LOAD)
    {
        int32_t temp = 0;
        if (cur_instruction->isSrc2Imm == false)
        {
            SIM_MemDataRead(params->reg_file[arg1] + params->reg_file[arg2], &temp);
        }
        else
        {
            SIM_MemDataRead(params->reg_file[arg1] + cur_instruction->src2_index_imm, &temp);
        }
        params->reg_file[dest] = temp;
        params->thread_state[cur_thread] = LOAD;
        params->thread_state[cur_thread + (params->threads_num)] = 0; // only one LOAD or STORE can run in each thread
    }
    else if (cur_instruction->opcode == CMD_STORE)
    {
        if (cur_instruction->isSrc2Imm == false)
        {
            SIM_MemDataWrite(params->reg_file[dest] + params->reg_file[arg2], params->reg_file[arg1]);
        }
        else
        {
            SIM_MemDataWrite(params->reg_file[dest] + cur_instruction->src2_index_imm, params->reg_file[arg1]);
        }
        params->thread_state[cur_thread] = STORE;
        params->thread_state[cur_thread + (params->threads_num)] = 0; // only one LOAD or STORE can run in each thread
    }
}
Instruction *init_inst()
{
    Instruction *cur_instruction = (Instruction *)malloc(sizeof(*cur_instruction));
    if (cur_instruction == NULL)
    {
        return NULL;
    }
    cur_instruction->opcode = 0;
    cur_instruction->dst_index = 0;
    cur_instruction->src1_index = 0;
    cur_instruction->src2_index_imm = 0;
    cur_instruction->isSrc2Imm = false;
    return cur_instruction;
}
void tick(MTData *params)
{
    for (int i = 0; i < (params->threads_num); i++)
    {
        if (params->thread_state[i] == LOAD)
        {
            if (params->thread_state[i + (params->threads_num)] == params->load_latency)
            {
                params->thread_state[i] = 0;                             // get the count ready for the coming LOAD in the thread, if was
                params->thread_state[i + (params->threads_num)] = READY; // the stall (because of the LOAD) is done
            }
            else
            {
                params->thread_state[i + (params->threads_num)]++; // still in stall
            }
        }
        if (params->thread_state[i] == STORE)
        {
            if (params->thread_state[i + (params->threads_num)] == params->store_latency)
            {
                params->thread_state[i] = 0;                             // get the count ready for the coming STORE in the thread, if was
                params->thread_state[i + (params->threads_num)] = READY; // the stall (because of the STORE) is done
            }
            else
            {
                params->thread_state[i + (params->threads_num)]++; // still in stall
            }
        }
    }
}

bool readyThread(int cur_thread, int i)
{
    if (fineGrainedMT->thread_state[fineGrainedMT->threads_num + ((i + cur_thread) % (fineGrainedMT->threads_num))] == READY)
    {
        return true;
    }
    if (fineGrainedMT->thread_state[((i + cur_thread) % (fineGrainedMT->threads_num))] == STORE)
    {
        if (fineGrainedMT->thread_state[fineGrainedMT->threads_num + ((i + cur_thread) % (fineGrainedMT->threads_num))] == fineGrainedMT->store_latency)
        {
            return true;
        }
    }
    if (fineGrainedMT->thread_state[((i + cur_thread) % (fineGrainedMT->threads_num))] == LOAD)
    {
        if (fineGrainedMT->thread_state[fineGrainedMT->threads_num + ((i + cur_thread) % (fineGrainedMT->threads_num))] == fineGrainedMT->load_latency)
        {
            return true;
        }
    }
    return false;
}

void CORE_BlockedMT()
{
    blockedMT = init(BLOCKED, blockedMT);
    int cur_thread = 0;
    Instruction *cur_instruction = init_inst();
    if (init_inst() == NULL)
    {
        return;
    }
    while (blockedMT->running_total_threads)
    {
        blockedMT->total_cycles++; // update this when we have a hold or an operation is ready in the thread
        tick(blockedMT);
        if (blockedMT->thread_state[cur_thread + (blockedMT->threads_num)] == READY)
        {
            SIM_MemInstRead(blockedMT->instruction_num[cur_thread], cur_instruction, cur_thread); // reading the instruction
            blockedMT->instruction_num[cur_thread]++;
            update(blockedMT, cur_instruction, cur_thread);
        }
        else
        {                  // there is a STORE or LOAD
            int temp = -1; // switch thread
            for (int i = 1; i <= (blockedMT->threads_num); i++)
            {
                if (blockedMT->thread_state[blockedMT->threads_num + ((i + cur_thread) % (blockedMT->threads_num))] == READY)
                { // get the most recent ready thread
                    temp = (i + cur_thread) % (blockedMT->threads_num);
                    break;
                }
            }
            if (temp != -1)
            { // if found ready operation in another thread we do switch
                cur_thread = temp;
                for (int i = 0; i < blockedMT->switch_latency; i++)
                {
                    if (i != 0)
                    {
                        blockedMT->total_cycles++;
                        tick(blockedMT);
                    }
                }
            }
        }
    }
}


void CORE_FinegrainedMT()
{
    fineGrainedMT = init(FINE_GRAINED, fineGrainedMT);
    int cur_thread = 0;
    Instruction *cur_instruction = init_inst();
    if (init_inst() == NULL)
    {
        return;
    }
    while (fineGrainedMT->running_total_threads)
    {
        fineGrainedMT->total_cycles++; // update this when we have a hold or an operation is ready in the thread
        tick(fineGrainedMT);
        if (fineGrainedMT->thread_state[cur_thread + (fineGrainedMT->threads_num)] == READY)
        {
            SIM_MemInstRead(fineGrainedMT->instruction_num[cur_thread], cur_instruction, cur_thread); // reading the instruction
            fineGrainedMT->instruction_num[cur_thread]++;
            update(fineGrainedMT, cur_instruction, cur_thread);
        }

        int temp = -1; // switch thread
        for (int i = 1; i <= (fineGrainedMT->threads_num); i++)
        {
            if (readyThread(cur_thread, i))
            { // get the most recent ready thread
                temp = (i + cur_thread) % (fineGrainedMT->threads_num);
                break;
            }
        }
        if (temp != -1)
        { // if found ready operation in another thread we do switch
            cur_thread = temp;
        }
    }
}

double CORE_BlockedMT_CPI()
{
    double counter;
    for (int i = 0; i < (blockedMT->threads_num); i++)
    {
        counter = counter + (blockedMT->instruction_num[i]);
    }
    double result = ((double)(blockedMT->total_cycles)) / counter;
    free(blockedMT->instruction_num);
    free(blockedMT->reg_file);
    free(blockedMT->thread_state);
    free(blockedMT);
    return result;
}

double CORE_FinegrainedMT_CPI()
{
    double counter;
    for (int i = 0; i < (fineGrainedMT->threads_num); i++)
    {
        counter = counter + (fineGrainedMT->instruction_num[i]);
    }
    double result = ((double)(fineGrainedMT->total_cycles)) / counter;
    free(fineGrainedMT->instruction_num);
    free(fineGrainedMT->reg_file);
    free(fineGrainedMT->thread_state);
    free(fineGrainedMT);
    return result;
}

void CORE_BlockedMT_CTX(tcontext *context, int threadid)
{
    for (int i = 0; i < REGS_COUNT; i++)
    {
        context->reg[i + threadid * REGS_COUNT] = blockedMT->reg_file[i + threadid * REGS_COUNT];
    }
}

void CORE_FinegrainedMT_CTX(tcontext *context, int threadid)
{
    for (int i = 0; i < REGS_COUNT; i++)
    {
        context->reg[i + threadid * REGS_COUNT] = fineGrainedMT->reg_file[i + threadid * REGS_COUNT];
    }
}
