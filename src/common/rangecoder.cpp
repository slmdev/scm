#include "RangeCoder.h"

// carryless rangecoder
void RangeCoder::Init()
{
  low     = code  = 0;
  range   = uint32_t(-1);
  if (decode==1) DO(NUM) (code <<=8) += fgetc(file);
}

void RangeCoder::Stop()
{
  if (decode==0) DO(NUM) fputc(low>>24,file),low<<=8;
}

void RangeCoder::EncodeSymbol(uint32_t cumfreq,uint32_t freq,uint32_t totfreq)
{
  low   += cumfreq * (range /= totfreq);
  range *= freq;
  RANGE_ENC_NORMALIZE
}

void RangeCoder::DecodeSymbol(uint32_t cumfreq,uint32_t freq)
{
  low   += cumfreq*range;
  range *= freq;
  RANGE_DEC_NORMALIZE
}

inline uint32_t RangeCoder::DecProb(uint32_t totfreq)
{
  uint32_t tmp=(code-low) / (range /= totfreq);
  return tmp;
}

void RangeCoder::EncodeBitOne(uint32_t p1,const int bit)
{
  const uint32_t rnew=SCALE_RANGE;
  bit ? low += rnew,range-=rnew:range=rnew;
  RANGE_ENC_NORMALIZE
}

int RangeCoder::DecodeBitOne(uint32_t p1)
{
  const uint32_t rnew=SCALE_RANGE;
  int bit=(code-low>=rnew);
  bit ? low += rnew,range-=rnew:range=rnew;
  RANGE_DEC_NORMALIZE
  return bit;
}

// binary rangecoder
void RangeCoderSH::Init()
{
  range = 0xFFFFFFFF;
  lowc = FFNum = Cache = code = 0;
  if(decode==1) DO(NUM+1) (code <<=8) += fgetc(file);
}

void RangeCoderSH::Stop()
{
  if (decode==0) DO(NUM+1) ShiftLow();
}

void RangeCoderSH::EncodeBitOne(uint32_t p1,int bit)
{
  const uint32_t rnew=SCALE_RANGE;
  bit ? range-=rnew, lowc+=rnew : range=rnew;
  while(range<TOP) range<<=8,ShiftLow();
}

int RangeCoderSH::DecodeBitOne(uint32_t p1)
{
  const uint32_t rnew=SCALE_RANGE;
  int bit = (code>=rnew);
  bit ? range-=rnew, code-=rnew : range=rnew;
  while(range<TOP) range<<=8,(code<<=8)+=fgetc(file);
  return bit;
}

void RangeCoderSH::ShiftLow()
{
  uint32_t Carry = uint32_t(lowc>>32), low = uint32_t(lowc);
  if( low<Thres || Carry )
  {
     fputc( Cache+Carry, file);
     for (;FFNum != 0;FFNum--) fputc(Carry-1,file);
     Cache = low>>24;
   } else FFNum++;
  lowc = (low<<8);
}
