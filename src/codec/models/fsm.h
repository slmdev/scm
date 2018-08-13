#ifndef FSM_H
#define FSM_H

#include "../../global.h"
#include "../../common/model/model.h"

typedef struct
{
  int n0,n1,p1;
  int next[2];
  int lb;
} tState;

class FSM
{
  public:
    FSM(int num_states,int num_lowbits);
    int NextState(int state,int bit);
    int GetP1(int state);
    int GetLB(int state){return myStates[state].lb;};
    ~FSM();
  protected:
    void DumpTable();
    int FindNextState(int start,int state,int n0,int n1,int lb);
    void FindNextStatesLow();
    void FindNextStatesHigh();
    void AddLowState(int state,int n0,int n1);
    void AddHighStates(int &state,int count,int maxstate);
    void InitLowStates(tState *States);
    void InitHighStates(int lowbits);
    void Dez2Bin(char *s,int val);
    void CountBits1(int val,int &n0,int &n1);
    tState *myStates;
    int maxstates,numlowstates,lowbits;
};

#endif
