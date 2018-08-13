#ifndef _COUNTER_H
#define _COUNTER_H

#include "model.h"

static struct tcnt_tbl {
  tcnt_tbl()
  {
    for (int n1=0;n1<256;n1++)
      for (int n0=0;n0<256;n0++)
      {
        tbl[n1][n0] =((n1+1)*PSCALE)/(n0+n1+2);
      }
  }
  int tbl[256][256];
} cnt_tbl;

// stationary 16-bit counter
class StatCounter
{
  public:
    uint8_t n0,n1;
    StatCounter(){n0=n1=0;};
    //int p1(){return ((int(n1)+1)*PSCALE)/(int(n0)+int(n1)+2);};
    int p1() {return cnt_tbl.tbl[n1][n0];}
    //bool Escape(){return (n0==0) && (n1==0);};
    bool Escape(){return ((int)n0+(int)n1==0);};
    void update(int bit,int add)
    {
      int in1=n1;
      int in0=n0;
      bit?in1+=add:in0+=add;
      if (in1>255 || in0>255) {in1>>=1;in0>>=1;};
      n0=in0;n1=in1;
    };
    void update(int bit) {
      int in1=n1;
      int in0=n0;
      if (bit) {
        in1++;
        if (in0>2) {in0=(in0+1)>>1;};
      } else {
        in0++;
        if (in1>2) {in1=(in1+1)>>1;};
      };
      if (in1>255 || in0>255) {in1>>=1;in0>>=1;};
      n0=in0;n1=in1;
    };
};

class Prob16Counter
{
  public:
    uint16_t p1;
    Prob16Counter():p1(0){};
  protected:
    int idiv(int val,int s) {return (val+(1<<(s-1)))>>s;};
    int idiv_signed(int val,int s){return val<0?-(((-val)+(1<<(s-1)))>>s):(val+(1<<(s-1)))>>s;};
};

// Linear Counter, p=16 bit
class LinearCounter16 : public Prob16Counter
{
  public:
    LinearCounter16():Prob16Counter(){};
    //p'=(1-w0)*p+w0*((1-w1)*bit+w1*0.5)
    #define wh(w) ((w*PSCALEh+PSCALEh)>>PBITS)
    void update(int bit,const int w0,const int w1)
    {
      int h=(w0*wh(w1))>>PBITS;
      int p=idiv((PSCALE-w0)*p1,PBITS);
      p+=bit?w0-h:h;
      p1=clamp(p,1,PSCALEm);
    };
    //p'+=L*(bit-p)
    void update(int bit,int L)
    {
      int err=(bit<<PBITS)-p1;
      // p1 should be converted to "int" implicit anyway?
      int px = int(p1) + idiv_signed(L*err,PBITS);
      p1=clamp(px,1,PSCALEm);
    }
};

static struct tdiv_tbl
{
  tdiv_tbl()
  {
    for (int i=0;i<PSCALE;i++)
    {
      tbl[i]=PSCALE/(i+3);
    }
  }
  int& operator[](int i)  {return tbl[i];};
  int tbl[PSCALE];
} div_tbl;

class LinearCounterLimit: public Prob16Counter
{
  uint16_t counter;
  public:
    LinearCounterLimit():Prob16Counter(){counter=0;};
    void update(int bit,int limit)
    {
      if (counter<limit) counter++;
      #if 0
        int dp=((bit<<PBITS)-p1)/int(counter+3);
      #else
        int dp=bit?((PSCALE-p1)*div_tbl[counter])>>PBITS:-((p1*div_tbl[counter])>>PBITS);
        //int dp=(((bit<<PBITS)-p1)*div_tbl[counter]+PSCALEh)>>PBITS;
      #endif
      p1=clamp(p1+dp,1,PSCALEm);
    };
};

/*class LinearCounterLimit32
{
  uint32_t x;
  public:
    LinearCounterLimit32(){x=0;};
    void Init(uint32_t p)
    {
       x=p<<(32-PBITS);
    }
    int p1()
    {
       return x>>(32-PBITS);
    }
    void update(int bit,int limit)
    {
      int cnt=x&1023;
      int p=x>>14;
      if (cnt<uint32_t(limit)) cnt++;
      int dp=((bit<<18)-p)/(cnt+3);
      p=clamp(p+dp,1,(1<<18)-1);
      x=(uint32_t(p)<<14)+cnt;
    };
};*/


#endif
