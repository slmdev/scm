#include "ppm.h"

PPMModel::PPMModel(int maxo,int memhash,int counter_rate)
:maxorder(maxo),cnt_rate(counter_rate),CtxMap(1<<memhash,maxorder-1,1000),
ByteHash(new uint32_t[maxorder+1]),CntPtr(new uint8_t*[maxorder+1]),HistMaps(new HistProbMapping[(maxorder+1)])
{
  memset(ByteHash,0,sizeof(uint32_t)*(maxorder+1));
}

PPMModel::~PPMModel()
{
    //printf("delete PPM\n");
    delete []ByteHash;
    delete []HistMaps;
    delete []CntPtr;
}

void PPMModel::UpdateNextByte(uint8_t lastbyte)
{
  int i;
  for (i=maxorder;i>0;i--) ByteHash[i]=h2x(ByteHash[i-1],lastbyte);
  CtxMap.Reset();
  for (i=2;i<=maxorder;i++) CtxMap.SetContext(ByteHash[i]);
  lb=lastbyte;
}

void PPMModel::Predict(int *mixer,int &model_cnt,int ctx)
{
  int i;
  int P[maxorder+1];

  CntPtr[0]=t0+ctx;
  CntPtr[1]=t1+((lb<<8)+ctx);
  for (i=2;i<=maxorder;i++) CntPtr[i]=CtxMap.GetState(i-2,ctx);

  usedorder=0;
  for (i=0;i<=maxorder;i++)
  {
    uint8_t *CS=CntPtr[i];
    int px=HistMaps[i].p1(*CS);
    if (!(*CS==0)) usedorder=i;
    P[i]=px;
  }
  memcpy(mixer,P,(maxorder+1)*sizeof(int));
  model_cnt+=(maxorder+1);
}

void PPMModel::Update(int bit)
{
  for (int i=0;i<=maxorder;i++)
  {
    if (*CntPtr[i]>0) HistMaps[i].Update(*CntPtr[i],bit,cnt_rate);
    *CntPtr[i]=nex(*CntPtr[i],bit);
  }
}
