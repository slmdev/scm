#ifndef _RANGECODER_H
#define _RANGECODER_H 1

#include "..\global.h"
#include "model\model.h"

#define DO(n) for (uint32_t _=0;_<n;_++)

#define RANGE_ENC_NORMALIZE  while ((low ^ (low+range))<TOP || (range<BOT && ((range= -(int)low & (BOT-1)),1))) fputc(low>>24,file),range<<=8,low<<=8;
#define RANGE_DEC_NORMALIZE  while ((low ^ (low+range))<TOP || (range<BOT && ((range= -(int)low & (BOT-1)),1))) (code<<=8)+=fgetc(file),range<<=8,low<<=8;

//#define SCALE_RANGE (((PSCALE-p1)*uint64_t(range)) >> PBITS) // 64 bit shift
#define SCALE_RANGE ((uint64_t(range)*((PSCALE-p1)<<(32-PBITS)))>>32) // faster, because we only take the high word from qword

// you should not do this because of slowdown
class BaseRangeCoder {
  public:
    BaseRangeCoder(FILE *iofile,int dec):file(iofile),decode(dec) {};
    virtual void Init()=0;
    virtual void Stop()=0;
    virtual void EncodeBitOne(uint32_t p1,int bit)=0;
    virtual int DecodeBitOne(uint32_t p1)=0;
    ~BaseRangeCoder(){};
  protected:
    FILE *file;
    int decode;
};

// Carryless RangeCoder
// derived from Dimitry Subbotin (public domain)
class RangeCoder : public BaseRangeCoder
{
  enum {NUM=4,TOP=0x01000000U,BOT=0x00010000U};
  public:
    RangeCoder(FILE *iofile,int dec):BaseRangeCoder(iofile,dec){};
    void Init();
    void Stop();
    void EncodeSymbol(uint32_t low,uint32_t freq,uint32_t tot);
    void DecodeSymbol(uint32_t low,uint32_t freq);
    void EncodeBitOne(uint32_t p1,int bit);
    int  DecodeBitOne(uint32_t p1);
    inline uint32_t DecProb(uint32_t totfreq);
  protected:
    uint32_t low,range,code;
};

// Binary RangeCoder with Carry and 64-bit low
// derived from rc_v3 by Eugene Shelwien
class RangeCoderSH : public BaseRangeCoder {
  enum { NUM=4,TOP=0x01000000U,Thres=0xFF000000U};
  public:
    RangeCoderSH(FILE *iofile,int dec):BaseRangeCoder(iofile,dec){};
    void Init();
    void Stop();
    void EncodeBitOne(uint32_t p1,int bit);
    int  DecodeBitOne(uint32_t p1);
  protected:
    void ShiftLow();
    uint32_t range,code,FFNum,Cache;
    uint64_t lowc;
};

#endif
