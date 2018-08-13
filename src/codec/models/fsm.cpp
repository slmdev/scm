#include "fsm.h"

FSM::FSM(int num_states,int num_lowbits)
:maxstates(num_states),lowbits(num_lowbits)
{
  //maxstates=(1<<(num_lowbits+1))+(1<<num_highbits);

  numlowstates=(1<<(num_lowbits+1))-1;
  myStates = new tState[maxstates];
  memset(myStates,0,maxstates*sizeof(tState));
  InitLowStates(myStates);
  InitHighStates(num_lowbits);
  FindNextStatesLow();
  FindNextStatesHigh();
  //DumpTable();
}

FSM::~FSM()
{
  delete []myStates;
}

int FSM::GetP1(int state)
{
  int n0=myStates[state].n0;
  int n1=myStates[state].n1;
  int p1=(((n1)+1)*PSCALE)/((n0+n1)+2);
  return p1;
}

int FSM::NextState(int state,int bit)
{
   return myStates[state].next[bit];
}

void FSM::CountBits1(int val,int &n0,int &n1)
{
   n0=n1=0;
   while (val) {
     if (val&1) n1++;
     else n0++;
     val>>=1;
   };
}

void FSM::Dez2Bin(char *s,int val)
{
  for (int i=0;i<8;i++)
  {
    if ((val>>i) & 1) s[7-i]='1';
    else s[7-i]='0';
  }
  s[8]='\0';
}

void FSM::AddLowState(int state1,int n0,int n1)
{
  int state=state1-1;
  myStates[state].n0=n0;
  myStates[state].n1=n1;
  myStates[state].lb=state>0?state1&1:-1;

  int s=(state1>>1)-1;
  if (s>=0)
  {
    if (state1&1) myStates[s].next[1]=state;
    else myStates[s].next[0]=state;
  }
}

void FSM::DumpTable()
{
   for (int i=0;i<maxstates;i++)
   {
       printf("%i n0:%i n1:%i  s0:%i  s1:%i  lb: %i\n",i,myStates[i].n0,myStates[i].n1,myStates[i].next[0],myStates[i].next[1],myStates[i].lb);
   }
}

void FSM::InitLowStates(tState *States)
{
  int i=1;
  while (true)
  {
     int n0,n1;
     CountBits1(i,n0,n1);n1--;
     /*char s[9];
     Dez2Bin(s,i);
     printf("%s\n",s);*/
     AddLowState(i,n0,n1);
     i++;
     if (i>=1<<(lowbits+1)) break;
  }
}

void FSM::AddHighStates(int &state,int count,int maxstate)
{
  for (int i=0;i<=count;i++)
  {
    int n1=i;
    int n0=count-i;
    if (n0>0)
    {
      myStates[state].n0=n0;
      myStates[state].n1=n1;
      myStates[state].lb=0;
      state++;
      if (state>=maxstate) return;
    }
    if (n1>0)
    {
      myStates[state].n0=n0;
      myStates[state].n1=n1;
      myStates[state].lb=1;
      state++;
      if (state>=maxstate) return;
    }
  }
}

void FSM::InitHighStates(int lowbits)
{
  int count=lowbits+1;
  int i=count;
  int state=numlowstates;
  while (true)
  {
    AddHighStates(state,i,maxstates);
    if (state>=maxstates) break;
    i++;
  }
}

int FSM::FindNextState(int start,int state,int n0,int n1,int lb)
{
  for (int i=start;i<maxstates;i++)
  {
      if (myStates[i].n0==n0 && myStates[i].n1==n1 && myStates[i].lb==lb) return i;
  }

  int best_state=-1;
  double best_diff=1.0;
  double p=double(n1+1)/double(n0+n1+2);

  for (int i=start;i<=state;i++)
  {
     int n0t=myStates[i].n0;
     int n1t=myStates[i].n1;
     double pt=double(n1t+1)/double(n0t+n1t+2);
     double d=abs(p-pt);
     if ((d<=best_diff) && (lb==myStates[i].lb))
     {
       best_state=i;
       best_diff=d;
     }
  }
  if (best_state==-1) printf(" warning: state %i  s0:%i s1:%i",state,myStates[state].next[0],myStates[state].next[1]);
  return best_state;
}


void FSM::FindNextStatesLow()
{
  for (int i=(numlowstates-1)>>1;i<numlowstates;i++)
  {
    myStates[i].next[0]=FindNextState(numlowstates,i,myStates[i].n0+1,myStates[i].n1,0);
    myStates[i].next[1]=FindNextState(numlowstates,i,myStates[i].n0,myStates[i].n1+1,1);
  }
}

void FSM::FindNextStatesHigh()
{
  for (int i=numlowstates;i<maxstates;i++)
  {
    myStates[i].next[0]=FindNextState(numlowstates,i,myStates[i].n0+1,myStates[i].n1>4?myStates[i].n1/2+1:myStates[i].n1,0);
    myStates[i].next[1]=FindNextState(numlowstates,i,myStates[i].n0>4?myStates[i].n0/2+1:myStates[i].n0,myStates[i].n1+1,1);
  }
}

