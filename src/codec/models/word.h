#ifndef _WORD_MODEL_H
#define _WORD_MODEL_H

#include "../../global.h"
#include "../../common/hashtable.h"
#include "../../common/hashfunc.h"
#include "../../common/model/map.h"
#include "../../common/model/mixer.h"

#define BSIZE (1024)
#define BMASK (BSIZE-1)

class WordModel
{
  public:
    WordModel(int membits,int mrate);
    int GetMemSize() {return CtxMap.GetMemSize();};
    void AddByte(int lastbyte);
    void Predict(int *mixer,int &mix_cnt,int ctx);
    void Update(int bit);
  protected:
    int tsize,rate,w0len,w1len;
    uint32_t Word0,Word1,Word2,Word3,Word4;
    ContextMap CtxMap;
    uint8_t lb,above;
    uint8_t buf[BSIZE];
    int bpos,n0pos,n1pos,col;
};

#endif
