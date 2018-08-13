#ifndef _DIST_H
#define _DIST_H

#include "..\..\global.h"
#include "..\..\common\model\map.h"

class IndirectModel
{
  public:
     IndirectModel(int mem,int mrate);
     int GetMemSize(){return CtxMap.GetMemSize();};
     void AddByte(uint8_t lastbyte);
     void Predict(int *mixer,int &mix_cnt,int ctx);
     void Update(int bit);
     ContextMap CtxMap;
     uint32_t t1[256];
     uint16_t t2[1<<16];
     uint8_t l1,l2,l3;
};

#endif

