#ifndef SPARSE_H
#define SPARSE_H

#include "../../global.h"
#include "../../common/model/map.h"
#include "../../common/model/mixer.h"

class SparseModel
{
  enum {CNUM=256*256,NLB=8,SM1=2,SM2=4};
  public:
    SparseModel(int membits,int mrate);
    int GetMemSize(){return (CtxMap.GetMemSize()+sizeof(tbl));};
    void AddByte(int val);
    void Predict(int *mixer,int &mix_cnt,int ctx);
    void Update(int bit);
  protected:
    uint16_t tbl[SM1][CNUM];
    uint16_t *s[SM1];
    HistProbMapping Map[SM1];
    int lb[NLB];
    int rate;
    uint32_t mask;
    ContextMap CtxMap;
};

#endif
