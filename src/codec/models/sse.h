#ifndef SSE_H
#define SSE_H

#include "..\..\common\model\map.h"

class SSEModel
{
  public:
    SSEModel();
    int GetMemSize(){return sizeof(SSE_O0)+sizeof(SSE_O1)+sizeof(SSE_O2)+sizeof(SSE_O3);}
    int Predict(int p,int cx);
    void Update(int bit,int sse_limit,int sse_mix);
    void SetCtx(int i,int cx)
    {
      Context[i]=cx;
    };
    SSENL<32>SSE_O0[1<<14];
    SSENL<32>SSE_O1[1<<16];
    SSENL<32>SSE_O2[1<<16];
    SSENL<32>SSE_O3[1<<16];
    Mix2Logistic SSEMix[1<<8];
    NMixLogistic<8> FinalMix;
    int Context[4];
    int ctx;
};

#endif
