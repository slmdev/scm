#ifndef MIX_H
#define MIX_H

#include "..\..\common\model\mixer.h"
#include "..\..\common\model\map.h"

class Mixer
{
  public:
    Mixer(int mix_rate,int chain_rate);
    int Predict();
    void Add(int p);
    void Update(int bit);
    void SetCtx(int cx,int range);
    int nm,m[64];
  protected:
    int cx;
    NMixLogistic<64> LMix[256*256*8];
    NMixLogistic<16> FinalMix;
    int nctx,mctx[8],mm[16];
    int mrate,crate,base;
};

#endif
