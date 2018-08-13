#include "sse.h"

SSEModel::SSEModel()
{
  for (int i=0;i<256;i++)
  {
    SSEMix[i].Init(WSCALEh,WSCALEh);
  }
}

int SSEModel::Predict(int p,int cx)
{
  ctx=cx;
  int p_sse0=SSE_O0[Context[0]].Predict(p);
  int p_out0=SSEMix[ctx].Predict(p,p_sse0);
  int p_sse1=SSE_O1[Context[1]].Predict(p_out0);
  int p_sse2=SSE_O2[Context[2]].Predict(p_out0);
  int p_sse3=SSE_O3[Context[3]].Predict(p_out0);

  int m[5];
  m[0]=p;
  m[1]=p_sse0;
  m[2]=p_sse1;
  m[3]=p_sse2;
  m[4]=p_sse3;
  return FinalMix.Predict(m,5);
}

void SSEModel::Update(int bit,int sse_limit,int sse_mix)
{
    SSE_O0[Context[0]].Update(bit,sse_limit);
    SSE_O1[Context[1]].Update(bit,sse_limit);
    SSE_O2[Context[2]].Update(bit,sse_limit);
    SSE_O3[Context[3]].Update(bit,sse_limit);
    SSEMix[ctx].Update(bit,sse_mix);
    FinalMix.update(bit,0.0008*WSCALE);
}
