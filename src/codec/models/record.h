#ifndef RECORD_H
#define RECORD_H

#include "..\..\global.h"
#include "..\..\common\model\map.h"

class RecordModel {
  enum{bufsize=1<<15,bufmask=bufsize-1};
  public:
     RecordModel(int mem,int mrate);
     int GetMemSize(){return CtxMap.GetMemSize();};
     void AddByte(uint8_t lastbyte);
     void Predict(int *mixer,int &mix_cnt,int ctx);
     void Update(int bit);
     uint8_t buf[bufsize];
     int bpos,cycle_len;
     int rlen1,rlen2,rcount1,rcount2;
     int lpos[256][4];
     ContextMap CtxMap;
};

#endif
