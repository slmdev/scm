#ifndef _MIXER_H
#define _MIXER_H

#include "model.h"
#include "domain.h"

static struct tdiv32tbl {// 32-bit 1/i (i=1..PSCALEm) div-tbl
  tdiv32tbl() {for (int i=1;i<PSCALE;i++) tbl[i]=0xffffffff/(uint32_t)(i);}
  uint32_t& operator[](int i)  {return tbl[i];};
  uint32_t tbl[PSCALE];
} div32tbl;

class StaticMix32 {
  public:
    static int Predict(int p1,int p2,int w)
    {
      int pm;
      pm = p1+idiv_signed((p2-p1)*w,WBITS);
      pm = clamp(pm,1,PSCALEm);
      return pm;
    }
  protected:
    static int idiv_signed(int val,int s){return val<0?-(((-val)+(1<<(s-1)))>>s):(val+(1<<(s-1)))>>s;};
};

// adaptive linear 2-input mix
// maximum weight precision 16-Bit
class Mix2
{
  public:
    virtual int Predict(int _p1,int _p2)=0;
    virtual void Update(int bit,int rate)=0;
};

class Mix2Linear : public Mix2
{
  public:
    Mix2Linear(){Init(WSCALEh);};
    void Init(int iw){w=iw;};
    //pm=(1-w)*p1+w*p2
    int Predict(int _p1,int _p2)
    {
      p1=_p1;p2=_p2;
      pm = p1+idiv_signed32((p2-p1)*w,WBITS);
      pm = clamp(pm,1,PSCALEm);
      return pm;
    }
    int w,p1,p2,pm;
  protected:
    inline int idiv_signed32(int val,int s){return val<0?-(((-val)+(1<<(s-1)))>>s):(val+(1<<(s-1)))>>s;};
    inline void upd_w(int d,int rate) {int wd=idiv_signed32(rate*d,PBITS);w=clamp(w+wd,0,int(WSCALE));};
};

class Mix2LeastSquares : public Mix2Linear  {
  public:
    // w_(i+1)=w_i + rate*(p2-p1)*e
    void Update(int bit,int rate)
    {
      int e=(bit<<PBITS)-pm;
      int d=idiv_signed32((p2-p1)*e,PBITS);
      upd_w(d,rate);
    }
};

class Mix2LeastCost : public Mix2Linear {
  public:
    void Update(int bit,int rate)
    {
      int d;
      if (bit) d=(((p2-p1)<<PBITS)*uint64_t(div32tbl[pm]))>>32;
      else d=(((p1-p2)<<PBITS)*uint64_t(div32tbl[PSCALE-pm]))>>32;
      /*if (bit) d=((p2-p1)<<PBITS)/pm;
      else d=((p1-p2)<<PBITS)/(PSCALE-pm);*/
      upd_w(d,rate);
    }
};

class Mix2Logistic : public Mix2
{
  enum {WRANGE=1<<19};
  int w[2];
  int x[2];
  int pd;
  public:
    int w0,w1;
    Mix2Logistic()
    {
      Init(0,0);
    }
    void Init(int w0,int w1) {w[0]=w0;w[1]=w1;};
    int Predict(int p0,int p1)
    {
      s64 sum=0;
      x[0]=myDomain.Fwd(p0);
      x[1]=myDomain.Fwd(p1);
      sum+=s64(w[0]*x[0]);
      sum+=s64(w[1]*x[1]);
      sum=idiv_signed64(sum,WBITS);
      pd=myDomain.Inv(sum);
      return pd;
    }
    void Update(int bit,int rate)
    {
      int err=(bit<<PBITS)-pd;
      for (int i=0;i<2;i++)
      {
        int de=idiv_signed32(x[i]*err,myDomain.dbits);
        upd_w(i,idiv_signed32(de*rate,myDomain.dbits));
      }
    };
  protected:
    inline int idiv_signed32(int val,int s){return val<0?-(((-val)+(1<<(s-1)))>>s):(val+(1<<(s-1)))>>s;};
    inline int idiv_signed64(s64 val,s64 s){return val<0?-(((-val)+(1<<(s-1)))>>s):(val+(1<<(s-1)))>>s;};
    inline void upd_w(int i,int wd){w[i]=clamp(w[i]+wd,-WRANGE,WRANGE-1);}
};

template <int N>
class fnLinearMix32
{
  public:
    double w[N],wsum,psum;
    double p[N],pd;
    int pm,n;

    fnLinearMix32()
    {
        for (int i=0;i<N;i++) w[i]=0.5;
    };
    int Predict(int *ip,int ni)
    {
      n=ni;
      for (int i=0;i<n;i++) p[i]=double(ip[i])/double(PSCALE);
      psum=wsum=0.0;
      for (int i=0;i<n;i++) {
        psum+=w[i]*p[i];
        wsum+=w[i];
      };
      wsum+=1e-8;
      //psum+=1e-8;
      pd = psum/wsum;
      pm = clamp(int(pd*PSCALE),1,PSCALEm);
      return pm;
    }
     void update(int bit,double rate)
     {
       double dw[n];

       if (bit) {
          for (int i=0;i<n;i++) dw[i]=(p[i]-pd)/(pd);
       } else {
          for (int i=0;i<n;i++) dw[i]=(p[i]-pd)/(pd-1);
       }
       for (int i=0;i<n;i++)
       {
         w[i]+=rate*dw[i];
         if (w[i]<0) w[i]=0.0;
         else if (w[i]>1.0) w[i]=1.0;
       }
     }
};

template <int N>
class NMixLogistic
{
  enum {WRANGE=1<<19};
  sint16_t x[N];
  sint16_t pd;
  uint8_t n;
  public:
    int w[N];
    NMixLogistic(){Init(0);};
    void Init(int iw){for (int i=0;i<N;i++) w[i]=iw;};
    int Predict(int *p,int num)
    {
      n=num;
      s64 sum=0;
      for (int i=0;i<n;i++)
      {
         x[i]=myDomain.Fwd(p[i]);
         sum+=s64(w[i]*x[i]);
      }
      sum=idiv_signed64(sum,WBITS);
      pd=myDomain.Inv(sum);
      return pd;
    }
    void update(int bit,int rate)
    {
      int err=(bit<<PBITS)-pd;
      for (int i=0;i<n;i++)
      {
         int de=idiv_signed32(x[i]*err,myDomain.dbits);
         upd_w(i,idiv_signed32(de*rate,myDomain.dbits));
      }
    };
  protected:
    inline int idiv_signed32(int val,int s){return val<0?-(((-val)+(1<<(s-1)))>>s):(val+(1<<(s-1)))>>s;};
    inline int idiv_signed64(s64 val,s64 s){return val<0?-(((-val)+(1<<(s-1)))>>s):(val+(1<<(s-1)))>>s;};
    inline void upd_w(int i,int wd){w[i]=clamp(w[i]+wd,-WRANGE,WRANGE-1);}
};

#endif
