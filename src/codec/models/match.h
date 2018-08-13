#ifndef _MATCH_H
#define _MATCH_H

#include "../../global.h"
#include "../../common/model/model.h"
#include "../../common/model/map.h"
#include "../../common/hashtable.h"

/*typedef struct
{
   uint32_t ctx;
   int wpos,lastlen;
} HashNode;*/

#define MODWINDOW(i) (window[(i)&wmask])

class MatchModel {
  public:
    MatchModel(int winbits,int hash_bits,int mrate);
    ~MatchModel();
    void Reset();
    void Predict(int *mixer,int &mix_cnt,int ctx,int BitPos);
    int GetMemSize(){return LenMap.GetMemSize()+hsize*4+(1<<16)+wsize;};
    void FindLongestContext();
    void AddByte(uint8_t c);
    void Update(int bit);
    inline int GetMatchLen(){return MatchLen;};
    //static inline uint32_t Hash4(uint32_t val,uint32_t k,uint32_t mask){return ( ((val>>k)^val) & mask);};
    //uint8_t GetMispredicted();
    int BytesMatched;
    int match_bit,MatchContext;
  protected:
    void InitMap();
    int GetMatchContext(int ctx);
    inline int wmod(int pos){return pos&wmask;};
    int MatchAgainstContext(int curpos,int matchpos,int maxlen);
    uint32_t GetULong(int pos);
    uint32_t GetUWord(int pos);
    uint8_t *window;
    uint32_t *hash2,*hash48;
    int wsize,wmask,hbits,hsize,hmask,curpos,MatchLen,MatchPos;
    int rate;
    uint8_t match_byte,last_byte;
    //bool not_matched;
    StationaryMap LenMap;
};

#endif
