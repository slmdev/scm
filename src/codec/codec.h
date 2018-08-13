#ifndef _CODEC_H
#define _CODEC_H

#include "..\global.h"
#include "..\common\rangecoder.h"

class Encoder {
  public:
    Encoder(BaseRangeCoder *RCoder):Coder(RCoder){};
    virtual void Encode(int val)=0; // encode byte value
    virtual int Decode()=0;
  protected:
    BaseRangeCoder *Coder;
};

#endif
