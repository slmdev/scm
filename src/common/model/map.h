#ifndef _MAP_H
#define _MAP_H

#include "..\hashtable.h"
#include "counter.h"
#include "mixer.h"

/*
 SSE: functions of context history
 maps a probability via (linear-)quantization to a new probability
*/
/*template <int NB>
class SSE {
  uint16_t p_quant,px;
  public:
    enum {mapsize=1<<NB};
    SSE ()
    {
      for (int i=0;i<=mapsize;i++) // init prob-map that SSE.p1(p)~p
      {
        int v=((i*PSCALE)>>NB);
        v = clamp(v,1,PSCALEm);
        Map[i].p1=v;
      }
    }
    int p1(int p1) // linear interpolate beetween bins
    {
      p_quant=p1>>(PBITS-NB);
      int p_mod=p1&(mapsize-1); //int p_mod=p1%map_size;
      int pl=Map[p_quant].p1;
      int ph=Map[p_quant+1].p1;
      px=(pl+((p_mod*(ph-pl))>>NB));
      return px;
    }
    void update2(int bit,int rate) // update both bins
    {
      Map[p_quant].update(bit,rate);
      Map[p_quant+1].update(bit,rate);
    }
    void update4(int bit,int rate) // update four nearest bins
    {
      if (p_quant>0) Map[p_quant-1].update(bit,rate>>1);
      Map[p_quant].update(bit,rate);
      Map[p_quant+1].update(bit,rate);
      if (p_quant<mapsize-1) Map[p_quant+2].update(bit,rate>>1);
    }
    void update1(int bit,int rate) // update artifical bin
    {
      LinearCounter16 tmp;
      tmp.p1=px;
      tmp.update(bit,rate);
      int pm=tmp.p1-px;
      int pt1=Map[p_quant].p1+pm;
      int pt2=Map[p_quant+1].p1+pm;
      Map[p_quant].p1=clamp(pt1,1,PSCALEm);
      Map[p_quant+1].p1=clamp(pt2,1,PSCALEm);
    }
  protected:
    LinearCounter16 Map[(1<<NB)+1];
};

template <int NB>
class SSEV2 {
  uint16_t p_quant,px;
  uint8_t last_bit;
  public:
    enum {mapsize=1<<NB};
    SSEV2 ()   {Reset();}
    void Reset()
    {
      InitMap();
      last_bit=0;
    }
    void InitMap()
    {
      for (int i=0;i<=mapsize;i++) // init prob-map that SSE.p1(p)~p
      {
        int v=((i*PSCALE)>>NB);
        v = clamp(v,1,PSCALEm);
        Map[0][i].p1=v;
        Map[1][i].p1=v;
      }
    }
    int p1(int p1) // linear interpolate beetween bins
    {
      p_quant=p1>>(PBITS-NB);
      int p_mod=p1&(mapsize-1); //int p_mod=p1%map_size;
      int pl=Map[last_bit][p_quant].p1;
      int ph=Map[last_bit][p_quant+1].p1;
      px=(pl+((p_mod*(ph-pl))>>NB));
      return px;
    }
    void update1(int bit,int rate) // update artifical bin
    {
      LinearCounter16 tmp;
      LinearCounter16 *tMap=Map[last_bit];
      tmp.p1=px;
      tmp.update(bit,rate);
      int pm=tmp.p1-px;
      int pt1=tMap[p_quant].p1+pm;
      int pt2=tMap[p_quant+1].p1+pm;
      tMap[p_quant].p1=clamp(pt1,1,PSCALEm);
      tMap[p_quant+1].p1=clamp(pt2,1,PSCALEm);
      last_bit=bit;
    }
  protected:
    LinearCounter16 Map[2][(1<<NB)+1];
};*/

class StationaryMap
{
  int size;
  public:
    StationaryMap(int sz=256*256)
    :size(sz),Map(new LinearCounter16[size])
    {
      for (int i=0;i<size;i++)
      {
         Map[i].p1 = PSCALEh;
      }
    };
    int GetMemSize(){return (sizeof(*Map)*size);};
    ~StationaryMap() {delete []Map;};
    void Init(int state,int p){Map[state].p1=p;}
    int p1(int state) {return Map[state].p1;}
    void update(int state,int bit,int rate)
    {
       Map[state].update(bit,rate);
    }
  protected:
    LinearCounter16 *Map;
};

// Maps a state to a probability
class HistProbMapping
{
  enum {NUMSTATES=256};
  public:
    HistProbMapping()
    :Map(new LinearCounterLimit[NUMSTATES])
    {
      for (int i=0;i<NUMSTATES;i++) Map[i].p1=StateProb::GetP1(i);
    };
    int GetMemSize(){return NUMSTATES*sizeof(*Map);};
    ~HistProbMapping()
    {
       //printf("delete hp\n");
       delete []Map;
    }
    inline int p1(uint8_t state)
    {
       return Map[state].p1;
    }
    void Update(int state,int bit,int rate)
    {
       Map[state].update(bit,rate);
    }
  protected:
    LinearCounterLimit *Map;
};

#define SPREADCONTEXT

class ContextMap {
  public:
    ContextMap(int size,int max_ctx,int update_limit)
    :numctx(max_ctx),Context(new uint32_t[numctx]),StateTbl(new uint8_t*[numctx]),ProbMap(new HistProbMapping[numctx]),htable(size),rate(update_limit)
    {
      Reset();
    }
    ~ContextMap()
    {
      delete []Context,Context=0;
      delete []ProbMap,ProbMap=0;
      delete []StateTbl,StateTbl=0;
    };
    void Reset(){nctx=0;};
    int GetMemSize(){return htable.memsize+numctx*ProbMap[0].GetMemSize();};
    void Predict(int *mixer,int &mix_cnt,int ctx)
    {
      for (int i=0;i<nctx;i++)
      {
         StateTbl[i]=&htable[Context[i]^(ctx<<24)^(ctx)];
         int p=ProbMap[i].p1(*StateTbl[i]);
         mixer[mix_cnt++]=p;
      }
    }
    bool Escaped(int i){return (*StateTbl[i]==0);};
    void Update(int bit)
    {
      for (int i=0;i<nctx;i++)
      {
        if (*StateTbl[i]>0) ProbMap[i].Update(*StateTbl[i],bit,rate);
        *StateTbl[i]=nex(*StateTbl[i],bit);
        //Mixer[i].Update(bit,0.001*WSCALE);
      }
    }
    uint8_t *GetState(int i,int ctx)
    {
      return &htable[Context[i]^ctx^(ctx<<24)];
    }
    void SetContext(uint32_t cx)
    {
      #ifdef SPREADCONTEXT
        cx=cx*961748941+nctx;
        cx=cx<<16|cx>>16;
        cx=cx*198491317+nctx;
      #endif
      Context[nctx]=cx;
      nctx++;
    }

  protected:
    int numctx;
    uint32_t *Context;
    uint8_t **StateTbl;
    HistProbMapping *ProbMap;
    HashTable2 htable;
    int rate;
    int nctx;
};

template <int N>
class SSENL
{
  //enum {szmap=1<<NB};
  public:
    int tscale,xscale;
    uint16_t p_quant;
    uint8_t lb;
    SSENL(int scale=myDomain.max)
    :tscale(scale),xscale((2*tscale)/(N-1))
    {
      if (xscale==0) xscale=1;
      for (int i=0;i<=N;i++)
      {
         int x=myDomain.Inv(i*xscale-tscale);
         Map[0][i].p1=x;
         Map[1][i].p1=x;
      }
      lb=0;
    };
    double MAE()
    {
       double sae=0.0;
       for (int i=0;i<PSCALE;i++)
       {
          sae+=double(abs(i-Predict(i)));
       }
       return sae/double(PSCALE);
    };
    int Predict(int p1)
    {
       int pq=min(2*tscale,max(0,myDomain.Fwd(p1)+tscale));

       p_quant=pq/xscale;
       /*if (p_quant>=(1<<NB))
       {
           printf("error: %i %i %i %i\n",p1,xscale,pq,p_quant);
           system("pause");
       }*/
       int p_mod=pq-(p_quant*xscale); //%xscale;

       int pl=Map[lb][p_quant].p1;
       int ph=Map[lb][p_quant+1].p1;

       int px=(pl*(xscale-p_mod)+ph*p_mod)/xscale;
       return px;
    };
    void Update(int bit,int rate)
    {
       Map[lb][p_quant].update(bit,rate);
       Map[lb][p_quant+1].update(bit,rate);
       lb=bit;
    };
  protected:
    LinearCounterLimit Map[2][N+1];
};

#endif
