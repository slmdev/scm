#include "mix.h"

Mixer::Mixer(int mix_rate,int chain_rate)
:mrate(mix_rate),crate(chain_rate)
{
   nm=nctx=base=0;
   FinalMix.Init(WSCALE);
}

int Mixer::Predict()
{
  for (int i=0;i<nctx;i++)
  {
     int p=LMix[mctx[i]].Predict(m,nm);
     m[nm++]=p;
     mm[i]=p;
  }
  return FinalMix.Predict(mm,nctx);
}

void Mixer::SetCtx(int cx,int range)
{
  mctx[nctx++]=base+cx;
  base+=range;
}

void Mixer::Add(int p)
{
   m[nm++]=p;
}

void Mixer::Update(int bit)
{
  for (int i=0;i<nctx;i++)
  {
     LMix[mctx[i]].update(bit,mrate);
  }
  FinalMix.update(bit,crate);
  nm=nctx=base=0;
}
