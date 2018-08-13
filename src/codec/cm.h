#ifndef _CM5_H
#define _CM5_H


#include <iostream>
#include "codec.h"
#include "models\match.h"
#include "models\word.h"
#include "models\ppm.h"
#include "models\fsm.h"
#include "models\sparse.h"
#include "models\record.h"
#include "models\indirect.h"
#include "models\mix.h"
#include "models\sse.h"
#include "..\common\histbuffer.h"
#include "..\common\model\counter.h"
#include "..\common\model\mixer.h"
#include "..\common\model\map.h"
#include "..\common\hashtable.h"

//enum {MAXMODEL=8};

class TCMParam
{
  public:
  enum {MAXORDER=32};
  TCMParam()
  {
   memhash  = 26;
   maxorder = 6;
   //counter_rate = 32;

   match_model=final_sse=word_model=sparse_model=record_model=indirect_model=false;

   match_win_size=18;
   match_model_rate=0.008*PSCALE;

   counter_limit=100;

   maximize=false;

   word_model_mem=23;
   word_model_limit=100;

   sparse_model_mem=22;
   sparse_model_limit=100;

   record_model_mem=22;
   record_model_limit=100;

   indirect_model_mem=22;
   indirect_model_limit=100;

   final_mix_rate=0.001*WSCALE;
   final_chain_rate=0.005*WSCALE;
   final_sse_limit = 100;
   final_sse_mix_rate=0.005*WSCALE;
  };

  bool match_model,word_model,sparse_model,final_sse,record_model,indirect_model;
  bool maximize;
  int maxorder;
  int memhash;
  int counter_limit;
  int match_win_size,match_model_rate;
  int sparse_model_mem,sparse_model_limit;
  int word_model_mem,word_model_limit;
  int record_model_mem,record_model_limit;
  int indirect_model_mem,indirect_model_limit;
  int final_mix_rate,final_chain_rate;
  int final_sse_limit,final_sse_mix_rate;
};

class CM : public Encoder {
  public:
    CM(BaseRangeCoder *RCoder,TCMParam *CMParam,bool verbose);
    ~CM();
    void Encode(int val);
    int Decode();
    int GetBytesMatched(){return myMatchModel?myMatchModel->BytesMatched:0;};
  protected:
    int Predict();
    void Update(int bit);
    void UpdByteHash();
    inline void UpdBitPtr();

    int maxorder;
    int ctx,BitPos,o1ctx;

    TCMParam *Param;
    uint8_t l1byte,l2byte,l3byte,l4byte;
    WordModel *myWordModel;
    MatchModel *myMatchModel;
    SparseModel *mySparseModel;
    RecordModel *myRecordModel;
    IndirectModel *myIndirectModel;
    SSEModel *mySSEModel;
    PPMModel myPPM;
    Mixer myMixer;
    int p1_match;
};

#endif
