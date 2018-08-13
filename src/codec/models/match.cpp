#include "match.h"

MatchModel::MatchModel(int winbits,int hash_bits,int mrate)
:wsize(1<<winbits),wmask(wsize-1),hbits(hash_bits),hsize(1<<hbits),hmask(hsize-1),LenMap(256*256)
{
  window=new uint8_t[wsize];
  hash2 = new uint32_t[1<<8];
  hash48 = new uint32_t[hsize];
  rate=mrate;
  Reset();
}

MatchModel::~MatchModel()
{
   delete []window;
   delete []hash2;
   delete []hash48;
}


void MatchModel::Reset()
{
  int i;
  memset(window,0,wsize*sizeof(uint8_t));
  for (i=0;i<1<<8;i++)hash2[i] = -1;
  for (i=0;i<hsize;i++)hash48[i] = -1;
  match_byte=last_byte=match_bit=0;
  curpos=0;
  MatchLen=MatchPos=MatchContext=0;
  BytesMatched=0;
  //not_matched=false;
  InitMap();
}

// can it get more stupid :p

uint32_t MatchModel::GetUWord(int pos)
{
  uint8_t a=MODWINDOW(pos);
  uint8_t b=MODWINDOW(pos+1);
  uint32_t u=uint32_t(a)+(uint32_t(b)<<8);
  return u;
}

uint32_t MatchModel::GetULong(int pos)
{
  uint8_t a=MODWINDOW(pos);
  uint8_t b=MODWINDOW(pos+1);
  uint8_t c=MODWINDOW(pos+2);
  uint8_t d=MODWINDOW(pos+3);
  uint32_t u=uint32_t(a)+(uint32_t(b)<<8)+(uint32_t(c)<<16)+(uint32_t(d)<<24);
  return u;
}

void MatchModel::FindLongestContext()
{
  const int maxlen=255;

  uint32_t u4=GetULong(curpos-4);
  uint32_t u8=GetULong(curpos-8);
  uint32_t h2=MODWINDOW(curpos-1);

  uint32_t h4=h1y(u4,12) & hmask;
  uint32_t h8=h1y(u4^(u8<<19)^(u8>>19),12) & hmask;

  int MatchLen2,MatchLen4,MatchLen8;
  MatchLen2=MatchLen4=MatchLen8=0;

  if (MatchLen>0)
  {
    MatchPos=wmod(MatchPos+1);
    match_byte=window[MatchPos];
    if (MatchLen<maxlen) MatchLen++;
  } else
  {
    //not_matched=false;
    MatchLen=0;
    MatchPos=-1;
    match_byte=0;

    int MatchPos2=hash2[h2];
    int MatchPos4=hash48[h4];
    int MatchPos8=hash48[h8];

    MatchLen2=MatchAgainstContext(curpos,MatchPos2,maxlen);
    MatchLen4=MatchAgainstContext(curpos,MatchPos4,maxlen);
    MatchLen8=MatchAgainstContext(curpos,MatchPos8,maxlen);

    if (MatchLen2>MatchLen){MatchLen=MatchLen2;MatchPos=MatchPos2;};
    if (MatchLen4>MatchLen){MatchLen=MatchLen4;MatchPos=MatchPos4;};
    if (MatchLen8>MatchLen){MatchLen=MatchLen8;MatchPos=MatchPos8;};

    if (MatchPos!=-1) match_byte=MODWINDOW(MatchPos);
  }
  hash2[h2]=curpos;
  hash48[h4]=curpos;
  hash48[h8]=curpos;
}

int MatchModel::MatchAgainstContext(int curpos,int matchpos,int maxlen)
{
  int mlen=0;
  if (matchpos>=0)
  {
    while ((MODWINDOW(curpos-mlen-1)==MODWINDOW(matchpos-mlen-1)) && (mlen<maxlen)) mlen++;
  }
  return mlen;
}

void MatchModel::AddByte(uint8_t c)
{
    if (MatchLen>0) BytesMatched++;
    window[curpos]=c;
    curpos=wmod(curpos+1);
    last_byte=c;
}

/*uint8_t MatchModel::GetMispredicted()
{
  int c0=0;
  if (MatchLen==0 && MatchPos!=-1)
  {
      c0=MODWINDOW(MatchPos);
  }
  return c0;
}*/

void MatchModel::InitMap()
{
  for (int ctx=0;ctx<256;ctx++)
  {
     for (int t=1;t<=32;t++)
     {
        int ctx1=(ctx)+(t<<9);
        int p0=1;
        int p1=PSCALEm;
        LenMap.Init(ctx1,p0);
        LenMap.Init(ctx1+(1<<8),p1);
     }
  }
}

int MatchModel::GetMatchContext(int ctx)
{
  if (MatchLen==0) return ctx;
  else
  {
     int t=0;
     if (MatchLen<16) t=MatchLen;
     else t=((MatchLen-16)/16)+17;
     return (last_byte)+(t<<9)+(match_bit<<8);
  }
}

void MatchModel::Predict(int *mixer,int &mix_cnt,int ctx,int BitPos)
{
  if (ctx!=(match_byte+256)>>(BitPos+1))
  {
     MatchLen=0;match_byte=0;
  }
  match_bit=(match_byte>>BitPos)&1;
  MatchContext=GetMatchContext(ctx);
  mixer[mix_cnt++]=LenMap.p1(MatchContext);
}

void MatchModel::Update(int bit)
{
  LenMap.update(MatchContext,bit,rate);
  //not_matched=(bit!=match_bit);
}
