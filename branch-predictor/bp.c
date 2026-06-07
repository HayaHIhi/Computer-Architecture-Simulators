/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include "math.h"
#include <stdlib.h>
#include <stdio.h>

//defines
#define ALIGN 2 //Address aligned to 4 -> tag start at bit 2;
#define TARGETSIZE 30 
#define VALIDBIT 1
#define ST 3
#define WT 2
#define WNT 1
#define SNT 0
//enum stateMachine {ST = 3, WT = 2, WNT = 1, SNT = 0};

typedef struct BTBType{
	unsigned* tag; 
	unsigned* target;
	unsigned* localHist;
	unsigned globalHist;
	//enum stateMachine* fsm;
	unsigned* fsm;
	bool isGlobalHist;
	bool isGlobalTable;
	unsigned btbSize;
	unsigned historySize;
	unsigned tagSize;
	unsigned fsmState;
	int Shared;
	bool* valid;
} *BTB;

//Local functions
unsigned bitExtracted(unsigned number, unsigned length, unsigned start);
unsigned shareType(int typeShare, unsigned history, uint32_t pc, unsigned historySize);

//Global Variables: BTB Table, Stats;
BTB btb;
SIM_stats* stats;


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	int i;
	btb = (struct BTBType*) malloc(sizeof(struct BTBType));
	if (btb == NULL) return -1;
	btb->tag = malloc(sizeof(unsigned) * btbSize);
	if (btb->tag == NULL){
		free(btb);
		return -1;
	}
	btb->target = malloc(sizeof(unsigned) * btbSize);
	if (btb->target == NULL){
		free(btb->tag);
		free(btb);
		return -1;
	}
	stats = (SIM_stats*) malloc(sizeof(SIM_stats));
	if (stats == NULL){
		free(btb->target);
		free(btb->tag);
		free(btb);
		return -1;
	}
	stats->br_num = 0;
	stats->flush_num = 0;

	btb->valid = malloc(sizeof(bool) * btbSize);
	if (btb->valid == NULL){
		free(stats);
		free(btb->target);
		free(btb->tag);
		free(btb);
		return -1;
	}
	if(isGlobalTable){
		if(isGlobalHist){//stats for Global FSM && Global history
			stats->size = btbSize * (tagSize + TARGETSIZE + VALIDBIT) + historySize + 2 * (pow(2, historySize));
		}else{//stats for Global FSM && local history
			stats->size = btbSize * (tagSize + historySize +  TARGETSIZE + VALIDBIT)   + 2 * (pow(2, historySize)); //is correct???
		}
		//btb->fsm = malloc(sizeof(enum stateMachine) * (pow(2, historySize)));
		btb->fsm = malloc(sizeof(unsigned) * (pow(2, historySize)));
		if (btb->fsm == NULL){
			free(btb->valid);
			free(stats);
			free(btb->target);
			free(btb->tag);
			free(btb);
			return -1;
		}
		for(i=0; i<pow(2, historySize); i++){
			btb->fsm[i] = fsmState;
			//debug
			//printf("\nintitial: fsm state = %u, fsm num = %d\n", btb->fsm[i], i );
		}
	}else{
		if(isGlobalHist){//stats for Local FSM && Global history
			stats->size = btbSize * (tagSize +  TARGETSIZE + VALIDBIT + 2 * (pow(2, historySize)))  + historySize;
		}else{//stats for Local FSM && Local history
			stats->size = btbSize * (tagSize + historySize +  TARGETSIZE + VALIDBIT + 2 * (pow(2, historySize)));
		}
		//btb->fsm = malloc(sizeof(enum stateMachine) * (pow(2, historySize)) * btbSize);
		btb->fsm = malloc(sizeof(unsigned) * (pow(2, historySize)) * btbSize);
		if (btb->fsm == NULL){
			free(btb->valid);
			free(stats);
			free(btb->target);
			free(btb->tag);
			free(btb);
			return -1;
		}
		for(i=0; i<((pow(2, historySize)) * btbSize); i++){
			btb->fsm[i] = fsmState;
		}
	}
	if(isGlobalHist){
		btb->globalHist = 0;
		btb->localHist = NULL;
	}else{
		btb->globalHist = -1;
		btb->localHist = malloc(sizeof(unsigned) * btbSize);
		if (btb->localHist == NULL){
			free(btb->fsm);
			free(btb->valid);
			free(stats);
			free(btb->target);
			free(btb->tag);
			free(btb);
			return -1;
		}
		for(i=0; i<btbSize; i++){
			btb->localHist[i] = 0;
		}
	}

	btb->btbSize = btbSize;
	btb->fsmState = fsmState;
	btb->historySize = historySize;
	btb->isGlobalHist = isGlobalHist;
	btb->isGlobalTable = isGlobalTable;
	btb->Shared = Shared;
	btb->tagSize = tagSize;
	for(i=0; i<btbSize; i++){
		btb->valid[i] = false;
	}
	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	unsigned tag = bitExtracted(pc, btb->tagSize, ALIGN);
	unsigned line = bitExtracted(pc, log2(btb->btbSize), ALIGN);
	unsigned lineFSM;
	//printf("\nBP_predict: line = %X, btb->tag[line] = %X, tag = %X, btb->valid[line] = %d\n",line, btb->tag[line], tag, btb->valid[line]);
	if((tag == btb->tag[line]) && (btb->valid[line] == true)){ //is the pc in the table?
		if(btb->isGlobalTable){
			if(btb->isGlobalHist){  //global history + global table
				lineFSM = shareType(btb->Shared, btb->globalHist, pc, btb->historySize);
			//debug
			// printf("\nBP_predict, GH + GFSM\n");
			//printf("\nBP_predict: line = %X, lineFSM = %X, tag = %X, fsm state = %u\n", line,lineFSM, tag, btb->fsm[lineFSM]);
				if(btb->fsm[lineFSM] > 1){ //FSM State: St, WT
					*dst = btb->target[line];
					return true;
				}else{
					*dst = pc+4;
					return false;
				}
			}else{ //global fsm + local history
				lineFSM = shareType(btb->Shared, btb->localHist[line], pc, btb->historySize);
			//debug
			//printf("\nBP_predict, LH + GFSM\n");
			//printf("\nP_predict: fsm state = %u, fsm num = %d, history= %X\n", btb->fsm[lineFSM], lineFSM,btb->localHist[line] );
				if(btb->fsm[lineFSM] > 1){ //FSM State: St, WT
					*dst = btb->target[line];
					return true;
				}else{
					*dst = pc+4;
					return false;
				}

			}
		}else{
			if(btb->isGlobalHist){  //local fsm + global hisoty
			lineFSM = (line * pow(2, btb->historySize)) + btb->globalHist;
			//debug
			//printf("BP_predict, GH + LFSM\n");
				if(btb->fsm[lineFSM] > 1){ //FSM State: St, WT //accecing the array with pointer arithmetic
					*dst = btb->target[line];
					return true;
				}else{
					*dst = pc+4;
					return false;
				}
			}else{ //local history + local table
			lineFSM = (line * pow(2, btb->historySize)) + btb->localHist[line];
			//lineFSM = (line * pow(2, btb->historySize)) + shareType(btb->Shared, btb->localHist[line], pc, btb->historySize);
			//debug
			//printf("\nBP_predict, LH + LFSM\n");
			//unsigned temp = line * pow(2, btb->historySize);
			//printf("\nBP_predict: line = %X, lineFSM = %X + %X, tag = %X, fsm state = %u\n", line, temp, btb->localHist[line], tag, btb->fsm[lineFSM]);
				if(btb->fsm[lineFSM] > 1){ //FSM State: St, WT //accecing the array with pointer arithmetic
					*dst = btb->target[line];
					return true;
				}else{
					*dst = pc+4;
					return false;
				}
			}
		}
	}else{
		//debug
		// printf("\nBP_predict, Not Valid\n");
		//L FSM
		// lineFSM = (line * pow(2, btb->historySize)) + btb->localHist[line];
		// printf("\nBP_predict: line = %X, lineFSM = %X, tag = %X, fsm state = %u\n", line, lineFSM, tag, btb->fsm[lineFSM]);
		//G FSM
		//unsigned temp = shareType(btb->Shared, btb->globalHist, pc, btb->historySize);
		//printf("\nBP_predict: line = %X, lineFSM = %X, tag = %X, fsm state = %u\n", line,temp, tag, btb->fsm[temp]);
		*dst = pc+4;
		return false;
	}
	
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	unsigned tag = bitExtracted(pc, btb->tagSize, ALIGN);
	unsigned line = bitExtracted(pc, log2(btb->btbSize), ALIGN);
	unsigned lineFSM;
	
	if((btb->valid[line] == false) || (btb->tag[line] != tag)){ //new btb line
		
		if(btb->isGlobalHist == false){ //local history
			btb->localHist[line] = 0;
		}
		if(btb->isGlobalTable == false){//local fsm
			int i;
			for(i = line * pow(2, btb->historySize); i < line * pow(2, btb->historySize) + pow(2, btb->historySize); i++){
				btb->fsm[i] = btb->fsmState;
				// if(i = 0xE00){
				// 	printf("\nline = %X, fsm = %u\n", line, btb->fsm[i] );
				// }
				//printf("\nnew pc: fsm = %u, num = %d\n", btb->fsm[i], i);
			}
		}
		//unsigned temp = line * pow(2, btb->historySize);
		//printf("\nnew pc: line = %X, history = %X, fsm[0] = %u, lineFSM = %X\n", line, btb->localHist[line], btb->fsm[temp], temp);
	}
	btb->tag[line] =  tag;
	btb->target[line] = targetPc;
	btb->valid[line] = true;
	//printf("\nBP_update line = %X, pc=%X, valid = %d\n", line, pc, btb->valid[line]);
	stats->br_num++;
	if(taken){
		if(targetPc != pred_dst){
			stats->flush_num++;
		}
	}else{
		if(pc+4 != pred_dst){
			stats->flush_num++;
		}	
	}
	//debug
	//printf("\nBP_update: line = %d, tag = %d, valid = %d\nlog2(btb->btbSize) = %f, pc(hex) = %X\n", line, tag, btb->valid[line], log2(btb->btbSize), pc);
	
	if(btb->isGlobalHist){
		if(btb->isGlobalTable){  //global history + global table
			lineFSM = shareType(btb->Shared, btb->globalHist, pc, btb->historySize);
			//debug
			//printf("BP_update: line = %d, lineFSM = %d, tag = %d\n", line, lineFSM, tag);

			//
			if(taken){
				btb->globalHist = (bitExtracted(btb->globalHist  << 1, btb->historySize, 0)) + 1;
				if(btb->fsm[lineFSM] != ST){
					btb->fsm[lineFSM]++;
				}	
			}else{
				btb->globalHist = bitExtracted(btb->globalHist  << 1, btb->historySize, 0);
				if(btb->fsm[lineFSM] != SNT){
					btb->fsm[lineFSM]--;
				}
			}

		}else{ //global history + local table ??
			lineFSM = (line * pow(2, btb->historySize)) + btb->globalHist;
			if(taken){
				btb->globalHist = (bitExtracted(btb->globalHist  << 1, btb->historySize, 0)) + 1;
				if(btb->fsm[lineFSM] != ST){
					btb->fsm[lineFSM]++;
				}	
			}else{
				btb->globalHist = bitExtracted(btb->globalHist  << 1, btb->historySize, 0);
				if(btb->fsm[lineFSM] != SNT){
					btb->fsm[lineFSM]--;
				}
			}
		}
	}else{
		if(btb->isGlobalTable){  //local history + global table
			lineFSM = shareType(btb->Shared, btb->localHist[line], pc, btb->historySize);

			if(taken){
				btb->localHist[line] = (bitExtracted(btb->localHist[line]  << 1, btb->historySize, 0)) + 1;
				if(btb->fsm[lineFSM] != 3){
					btb->fsm[lineFSM]++;
				}	
			}else{
				btb->localHist[line] = bitExtracted(btb->localHist[line]  << 1, btb->historySize, 0);
				if(btb->fsm[lineFSM] != 0){
					btb->fsm[lineFSM]--;
				}
			}
			//printf("fsm state = %u, fsm line = %X, history = %X",btb->fsm[lineFSM], lineFSM, btb->localHist[line]);
		}else{ //local history + local table
			lineFSM  = (line * pow(2, btb->historySize)) + btb->localHist[line];
			if(taken){
				btb->localHist[line] = (bitExtracted(btb->localHist[line]  << 1, btb->historySize, 0)) + 1;
				if(btb->fsm[lineFSM] != 3){
					btb->fsm[lineFSM]++;
				}	
			}else{
				btb->localHist[line] = bitExtracted(btb->localHist[line]  << 1, btb->historySize, 0);
				if(btb->fsm[lineFSM] != 0){
					btb->fsm[lineFSM]--;
				}
			}
		}
	}
	return;
}

void BP_GetStats(SIM_stats *curStats){
	//free all mallocs
	curStats->br_num = stats->br_num;
	curStats->flush_num = stats->flush_num;
	curStats->size = stats->size;

	free(btb->localHist);
	free(btb->fsm);
	free(btb->valid);
	free(stats);
	free(btb->target);
	free(btb->tag);
	free(btb);

	return;
}

//extract length number of bits, starting at bit start
unsigned bitExtracted(unsigned number, unsigned length, unsigned start){
	//debug
	// printf("\nnumber(hex) = %X, length = %d, start = %d\n", number, length, start);
	//printf("\number >> (start)(hex) = %X, (1 << length) - 1 = %X, ((1 << length) - 1) & (number >> (start)) = %X\n", number >> (start), (1 << length) - 1, ((1 << length) - 1) & (number >> (start)));
    return (((1 << length) - 1) & (number >> (start)));
}

//extract the correct FSM line, based on the type of share: LSB = 1, Mid = 2, no = 0;
unsigned shareType(int typeShare, unsigned history, uint32_t pc, unsigned historySize){
	switch (typeShare)
	{
	case 0: //no share
		return bitExtracted(history, historySize, 0);
		break;	
	case 1: //LSB
		return bitExtracted(history, historySize, 0) ^ bitExtracted(pc, historySize, ALIGN);
		break;
	case 2: //Mid
		return bitExtracted(history, historySize, 0) ^ bitExtracted(pc, historySize, 16);
		break;	
	default:
		return bitExtracted(history, historySize, 0);
		break;
	}
}
