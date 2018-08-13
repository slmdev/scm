#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include "..\global.h"

/*template <class T>
class HElement
{
  public:
    HElement(){checksum=0;}
    T val;
    uint8_t checksum;
};*/

#define h1x(h,v) (((h)*227)+(v))
#define h2x(h,v) ((((h)+(v))+512)*773)
#define h3x(h,v) (((h)+(v))*987660757L)
#define h4x(h,v) (((h)+(v)+1)*987660757L*16)

#define h1y(v,k) (((v)>>k)^(v))
#define h2y(v,k) (((v)*2654435761UL)>>(k))

template <class T>
class HashTable
{
  int size;
  public:
    int memsize;
    HashTable(int sz=1<<27)
    :size(sz),table(new T[sz])
    {
      memsize=size*sizeof(T);
      memset(table,0,memsize);
    };
    ~HashTable(){delete []table;};
    T& operator[](uint32_t h)
    {
      int idx=h&(size-1);
      return table[idx];
    };
    T *table;
};

#pragma pack(1)
class HElement {
  public:
    HElement() {chksum=state=0;}
    uint8_t chksum;
    //uint16_t state;
    uint8_t state;
    //StatCounter elem;
};
#pragma pack()

#define M (8)
class HashTable2
{
  const int hmask;
  public:
    int memsize;
    HashTable2(int sz=1<<27)
    :hmask(sz-1),table(new HElement[sz])
    {
      memsize=sz*sizeof(HElement);
      memset(table,0,memsize);
      //printf("  hashtable:     %im elem (%i)\n",sz/1000000,sizeof(HElement));
    }
    ~HashTable2(){delete []table;}
    /*StatCounter& operator[](uint32_t h)
    {
      int i;
      int msum=0xffff;
      const uint8_t chk=(h>>24)&0xff;

      HElement *MinElem=NULL;
      for (i=0;i<M;i++)
      {
        HElement *candidate = table + ((h+i) & hsize);
        const int tsum=(int)candidate->elem.n0+(int)candidate->elem.n1;
        if (tsum==0) {candidate->chksum=chk;return candidate->elem;}
        else if (candidate->chksum==chk) {return candidate->elem;}
        if (tsum<=msum) {MinElem=candidate;msum=tsum;};
      }
      MinElem->chksum=chk;
      return MinElem->elem;
    };*/

    uint8_t &operator[](uint32_t h)
    {
      int i;
      int ms=0xffff;
      const uint8_t chk=(h>>24)&0xff;

      HElement *MinElem=NULL;
      for (i=0;i<M;i++)
      {
        HElement *candidate = table + ((h+i) & hmask);
        int s=candidate->state;
        if (s==0) {candidate->chksum=chk;return candidate->state;}
        else if (candidate->chksum==chk) {return candidate->state;}
        if (s<ms) {MinElem=candidate;ms=s;};
      }
      MinElem->chksum=chk;
      return MinElem->state;
    };
    HElement *table;
};

#endif
