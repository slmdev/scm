#ifndef _PPM_H
#define _PPM_H

#include "..\..\global.h"
#include "fsm.h"
#include "..\..\common\hashtable.h"
#include "..\..\common\model\map.h"
#include "..\..\common\model\mixer.h"

class PPMModel
{
  enum{MIXCTX=256*256};
  public:
    PPMModel(int maxo,int memhash,int counter_rate);
    ~PPMModel();
    int GetMemSize(){return CtxMap.GetMemSize()+sizeof(t0)+sizeof(t1);};
    void UpdateNextByte(uint8_t lastbyte);
    void Predict(int *mixer,int &model_cnt,int ctx);
    void Update(int bit);
    int usedorder;
  protected:
    uint8_t t0[256],t1[256*256]; //o0+o1-tables
    int maxorder;
    int cnt_rate;
    uint8_t lb;
    ContextMap CtxMap;
    uint32_t *ByteHash;
    uint8_t **CntPtr;
    HistProbMapping *HistMaps;
};

#endif
