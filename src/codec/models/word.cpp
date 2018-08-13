#include "word.h"

WordModel::WordModel(int membits,int mrate)
:tsize(1<<membits),rate(mrate),CtxMap(tsize,12,mrate)
{
   Word0=Word1=Word2=Word3=Word4=w0len=w1len=0;
   memset(buf,0,BSIZE);
   bpos=0;n0pos=n1pos=0;
}


void WordModel::AddByte(int lastbyte)
{
  lb=lastbyte;
  buf[bpos]=lastbyte;
  bpos=(bpos+1)&BMASK;

  if (isalpha(lastbyte) || lastbyte>=128)
  {
    int lbl=tolower(lastbyte);
    Word0=h2x(Word0,lbl);
    if (w0len<32) w0len++;
    //text0=text0*983+lbl;
  } else if (Word0)
  {
    Word4=Word3*271;
    Word3=Word2*269;
    Word2=Word1*263;
    Word1=Word0*257;
    Word0=0;
    w1len=w0len;
    w0len=0;
   };

   if (lastbyte==10)   {n1pos=n0pos;n0pos=(bpos-1)&BMASK;};
   col=min(255,(bpos-n0pos)&BMASK);
   above=buf[(n1pos+col)&BMASK];

   uint32_t h=Word0*263+lb;
   CtxMap.Reset();
   CtxMap.SetContext(Word0);
   CtxMap.SetContext(h);
   CtxMap.SetContext(h^Word1);
   CtxMap.SetContext(h^Word2);
   CtxMap.SetContext(Word0^Word1);
   CtxMap.SetContext(Word0^Word1^Word2);

   //CtxMap.SetContext(h^Word3);
   //CtxMap.SetContext(h^Word4);
   /*CtxMap.SetContext(text0&0xffff);
   CtxMap.SetContext(text0&0xfffff);*/

   CtxMap.SetContext(w0len+(col<<8));
   CtxMap.SetContext(w0len+(lb<<8));

   CtxMap.SetContext((col<<16)+(lb<<8)+above);
   CtxMap.SetContext((lb<<8)+above);
   CtxMap.SetContext((col<<8)+lb);
   CtxMap.SetContext(col);
}

void WordModel::Predict(int *mixer,int &mix_cnt,int ctx)
{
  CtxMap.Predict(mixer,mix_cnt,ctx);;
}

void WordModel::Update(int bit)
{
  CtxMap.Update(bit);
}


