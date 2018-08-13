#ifndef _HISTBUFFER_H
#define _HISTBUFFER_H

template <class T,int N>
class HistBuffer
{
  public:
    HistBuffer():buf(new T[N]){Init();}
    void Init(){memset(buf,0,N*sizeof(T));}
    ~HistBuffer(){if(buf!=NULL)delete []buf,buf=0;}
    void AddFront(T val)
    {
       for (int i=N-1;i>0;i--) buf[i]=buf[i-1];
       buf[0]=val;
    }
    T& operator[](int i) {
      return buf[ (-i) - 1];
    }
  protected:
    T *buf;
};

#endif
