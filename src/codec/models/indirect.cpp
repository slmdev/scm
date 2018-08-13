#include "indirect.h"


IndirectModel::IndirectModel(int mem,int mrate)
:CtxMap(1<<mem,6,mrate)
{
  l1=l2=l3=0;
  memset(t1,0,256*sizeof(uint32_t));
  memset(t2,0,(1<<16)*sizeof(uint16_t));
}

void IndirectModel::AddByte(uint8_t lastbyte)
{
  l3=l2;
  l2=l1;
  l1=lastbyte;

  // update history for o1 and o2-context
  const uint32_t o12=l1+(l2<<8);

  uint32_t& r1=t1[l2];r1=(r1<<8)+l1;
  uint16_t& r2=t2[l2+(l3<<8)];r2=(r2<<8)+l1;

  uint32_t h0=l1+(t1[l1]<<8);
  uint32_t h1=o12+(t2[o12]<<16);

/*
cm.set(t);
    cm.set(t0);
    cm.set(ta);
    cm.set(t&0xff00);
    cm.set(t0&0xff0000);
    cm.set(ta&0xff0000);
    cm.set(t&0xffff);
    cm.set(t0&0xffffff);
    cm.set(ta&0xffffff);
    */
  CtxMap.Reset();
  CtxMap.SetContext(h0);
  CtxMap.SetContext(h1);
  CtxMap.SetContext(h0&0xff00);
  CtxMap.SetContext(h0&0xffff);
  //CtxMap.SetContext(h0&0xffffff);
  CtxMap.SetContext(h1&0xffffff);
  CtxMap.SetContext(h1&0xff0000);
}

void IndirectModel::Predict(int *mixer,int &mix_cnt,int ctx)
{
  CtxMap.Predict(mixer,mix_cnt,ctx);
}

void IndirectModel::Update(int bit)
{
  CtxMap.Update(bit);
}
