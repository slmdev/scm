#include "cm.h"


CM::CM(BaseRangeCoder *RCoder,TCMParam *CMParam,bool verbose)
:Encoder(RCoder),Param(CMParam)
,myPPM(Param->maxorder,Param->memhash,Param->counter_limit),
myMixer(Param->final_mix_rate,Param->final_chain_rate)
{
  maxorder = Param->maxorder;

  myMatchModel=NULL;
  myWordModel=NULL;
  mySparseModel=NULL;
  myRecordModel=NULL;
  myIndirectModel=NULL;
  mySSEModel=NULL;

  if (Param->match_model) myMatchModel = new MatchModel(Param->match_win_size,Param->match_win_size-2,Param->match_model_rate);
  if (Param->word_model)  myWordModel = new WordModel(Param->word_model_mem,Param->word_model_limit);
  if (Param->sparse_model) mySparseModel = new SparseModel(Param->sparse_model_mem,Param->sparse_model_limit);
  if (Param->record_model) myRecordModel = new RecordModel(Param->record_model_mem,Param->record_model_limit);
  if (Param->indirect_model) myIndirectModel = new IndirectModel(Param->indirect_model_mem,Param->indirect_model_limit);
  if (Param->final_sse) mySSEModel = new SSEModel();

  if (verbose)
  {
    printf("cm: %i mb",myPPM.GetMemSize()>>20);
    if (myWordModel) printf(" wm: %i mb",myWordModel->GetMemSize()>>20);
    if (myMatchModel) printf(" mm: %i mb",myMatchModel->GetMemSize()>>20);
    if (mySparseModel) printf(" sp: %i mb",mySparseModel->GetMemSize()>>20);
    if (myRecordModel) printf(" rc: %i mb",myRecordModel->GetMemSize()>>20);
    if (myIndirectModel) printf(" id: %i mb",myIndirectModel->GetMemSize()>>20);
    if (mySSEModel) printf(" (sse: %i mb)",mySSEModel->GetMemSize()>>20);
    printf("\n");
  }
  l1byte=l2byte=l3byte=l4byte=0;
}

CM::~CM()
{
  if (myMatchModel) delete myMatchModel,myMatchModel=NULL;
  if (myWordModel) delete myWordModel,myWordModel=NULL;
  if (mySparseModel) delete mySparseModel,mySparseModel=NULL;
  if (myRecordModel) delete myRecordModel,myRecordModel=NULL;
  if (myIndirectModel) delete myIndirectModel,myIndirectModel=NULL;
  if (mySSEModel) delete mySSEModel,mySSEModel=NULL;
}

inline void CM::UpdByteHash()
{
  myPPM.UpdateNextByte(l1byte);
  if (Param->match_model)
  {
    myMatchModel->AddByte(l1byte);
    myMatchModel->FindLongestContext();
  }
  if (Param->word_model) myWordModel->AddByte(l1byte);
  if (Param->sparse_model) mySparseModel->AddByte(l1byte);
  if (Param->record_model) myRecordModel->AddByte(l1byte);
  if (Param->indirect_model) myIndirectModel->AddByte(l1byte);
}

int ilog2(uint32_t val)
{
  int n=0;
  while (val) {val>>=1;n++;};
  return n;
}

inline void CM::UpdBitPtr()
{
  o1ctx=(l1byte<<8)+ctx;

  int t,len,mb;
  t=len=mb=0;

  if (Param->match_model)
  {
    int n=0;
    myMatchModel->Predict(&p1_match,n,ctx,BitPos);
    len=myMatchModel->GetMatchLen();
    mb=myMatchModel->match_bit;
    t=(len>0)+(len>=8)+(len>=32);
  }
  int mix1_ctx=((l2byte>>6)<<16)+(l1byte<<8)+ctx;

  if (!Param->maximize)
  {
    int mix0_ctx=(((t<<5)+myPPM.usedorder)<<4)+(((l1byte>>5)&7)<<1)+mb;
    myMixer.SetCtx(mix0_ctx,1<<13);
    myMixer.SetCtx(mix1_ctx,0);
  } else {
    //int mix1_ctx=((l2byte>>7)<<16)+(l1byte<<8)+ctx;
    //myMixer.SetCtx(mix1_ctx,1<<18);
    //myMixer.SetCtx(h2y(l1byte+(l2byte<<8),12)&((1<<16)-1)^ctx,1<<16);
    myMixer.SetCtx((myPPM.usedorder<<5)+(ilog2(len)<<1)+(BitPos==0),1<<8);
    myMixer.SetCtx((l1byte<<8)+ctx,1<<16);
    myMixer.SetCtx(ctx,256);
    myMixer.SetCtx(l2byte,1<<16);
    //myMixer.SetCtx((l3byte,1<<16);
    //(ilog2(len)<<1)+
    //myMixer.SetCtx((myPPM.usedorder<<4)+((l4byte>>6)<<2)+((BitPos==0)<<1)+(l1byte==l2byte),256);
    //myMixer.SetCtx(l2byte,256);
    //myMixer.SetCtx(l3byte,256);
    //myMixer.SetCtx((ilog2(len)<<1)+mb,512);
  }


  if (Param->final_sse)
  {
    int sse0_ctx=(((((l1byte>>6)&7)<<10)+(ctx<<2)+t)<<1)+mb;
    int sse1_ctx=(l1byte<<8)+ctx;
    int sse2_ctx=(h2y(l1byte+(l2byte<<8),12)&((1<<16)-1))^ctx;
    int sse3_ctx=(h2y(l1byte+(l2byte<<8)+(l3byte<<16),12)&((1<<16)-1))^ctx;

    mySSEModel->SetCtx(0,sse0_ctx);
    mySSEModel->SetCtx(1,sse1_ctx);
    mySSEModel->SetCtx(2,sse2_ctx);
    mySSEModel->SetCtx(3,sse3_ctx);
  }
}

int CM::Predict()
{
  UpdBitPtr();

  myPPM.Predict(myMixer.m,myMixer.nm,ctx);
  if (myMatchModel) myMixer.Add(p1_match);
  if (myWordModel) myWordModel->Predict(myMixer.m,myMixer.nm,ctx);
  if (mySparseModel) mySparseModel->Predict(myMixer.m,myMixer.nm,ctx);
  if (myRecordModel) myRecordModel->Predict(myMixer.m,myMixer.nm,ctx);
  if (myIndirectModel) myIndirectModel->Predict(myMixer.m,myMixer.nm,ctx);

  int p_out = myMixer.Predict();
  if (mySSEModel) p_out=mySSEModel->Predict(p_out,ctx);
  return clamp(p_out,1,PSCALEm);
}

void CM::Update(int bit)
{
  myPPM.Update(bit);
  if (myMatchModel) myMatchModel->Update(bit);
  if (myWordModel) myWordModel->Update(bit);
  if (mySparseModel) mySparseModel->Update(bit);
  if (myRecordModel) myRecordModel->Update(bit);
  if (myIndirectModel) myIndirectModel->Update(bit);
  if (mySSEModel) mySSEModel->Update(bit,Param->final_sse_limit,Param->final_sse_mix_rate);
  myMixer.Update(bit);
}

void CM::Encode(int val)
{
  ctx=1;
  UpdByteHash();

  for (BitPos=7;BitPos>=0;BitPos--)
   {
     const int bit=(val>>BitPos)&1;
     Coder->EncodeBitOne(Predict(),bit);
     Update(bit);
     ctx+=ctx+bit;
  }
  l4byte=l3byte;
  l3byte=l2byte;
  l2byte=l1byte;
  l1byte=val;
  //hist.AddFront(val);
}

int CM::Decode()
{
  ctx=1;
  UpdByteHash();
  int val=0;
  for (BitPos=7;BitPos>=0;BitPos--)
  {
     const int bit=Coder->DecodeBitOne(Predict());
     Update(bit);
     val+=(bit<<BitPos);
     ctx+=ctx+bit;
  }
  l4byte=l3byte;
  l3byte=l2byte;
  l2byte=l1byte;
  l1byte=val;
  //hist.AddFront(val);
  return val;
}

