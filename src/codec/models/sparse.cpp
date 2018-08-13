#include "sparse.h"


SparseModel::SparseModel(int membits,int mrate)
:rate(mrate),CtxMap(1<<membits,2*SM2+10,rate)
{
   for (int i=0;i<SM1;i++) memset(tbl[i],0,sizeof(tbl[i]));
   memset(lb,0,sizeof(lb));
   mask=0;
}

void SparseModel::AddByte(int v)
{
  for (int i=NLB-1;i>0;i--) lb[i]=lb[i-1];
  lb[0]=v;

  CtxMap.Reset();
  CtxMap.SetContext(v+(lb[2]<<8));
  CtxMap.SetContext(v+(lb[3]<<8));

  //   i=0: {1,2},{1,3}
  //   i=1: {2,3},{2,4}
  //   i=2: {3,4},{3,5}
  //   i=3: {4,5},{4,6}
  for (int i=0;i<SM2;i++)
  {
    CtxMap.SetContext(lb[i+1]+(lb[i+2]<<8));
    CtxMap.SetContext(lb[i+1]+(lb[i+3]<<8));
  }

  #if 0
    const uint32_t c4=lb[0]+(lb[1]<<8)+(lb[2]<<16)+(lb[3]<<24);
    CtxMap.SetContext(c4&0x00E0E0E0);
    CtxMap.SetContext(c4&0xE0E0E0E0);
    CtxMap.SetContext(c4&0x00F0F0F0);
    CtxMap.SetContext(c4&0xF0F0F0F0);
    CtxMap.SetContext(c4&0x00F8F8F8);
    CtxMap.SetContext(c4&0xF8F8F8F8);
  #endif

  int t=0;
  if (v!=0)
  {
    if (isalpha(v)) t=1;
    else if (isdigit(v)) t=2;
    else if (ispunct(v)) t=3;
    else if (v<32) t=4;
    else if (v<64) t=5;
    else if (v<128) t=6;
    else t=7;
  }
  mask=(mask<<3)+t;
  CtxMap.SetContext(mask);
  CtxMap.SetContext(v+(mask<<8));
}

void SparseModel::Predict(int *mixer,int &mix_cnt,int ctx)
{
  for (int i=0;i<SM1;i++) s[i]=&tbl[i][(lb[i+1]<<8)+ctx];
  for (int i=0;i<SM1;i++)
  {
    int st=*s[i];
    int p=Map[i].p1(st);
    mixer[mix_cnt++]=p;
  }

  CtxMap.Predict(mixer,mix_cnt,ctx);
}

void SparseModel::Update(int bit)
{
  for (int i=0;i<SM1;i++)
  {
     Map[i].Update(*s[i],bit,rate);
     *s[i]=nex(*s[i],bit);
  }
  CtxMap.Update(bit);
}
