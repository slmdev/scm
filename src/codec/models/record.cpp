#include "record.h"

RecordModel::RecordModel(int mem,int mrate)
:CtxMap(1<<mem,6,mrate)
{
  memset(buf,0,bufsize*sizeof(uint8_t));
  for (int i=0;i<256;i++)
  {
    lpos[0][i]=lpos[1][i]=lpos[2][i]=lpos[3][i]=0;
  }
  cycle_len=1;
  rlen1=2;
  rlen2=3;
  rcount1=0;
  rcount2=0;
  bpos=0;
}

void RecordModel::AddByte(uint8_t lastbyte)
{
  int (&lp)[4]=lpos[lastbyte];

  int dist3=(lp[2]-lp[3])&bufmask;
  int dist2=(lp[1]-lp[2])&bufmask;
  int dist1=(lp[0]-lp[1])&bufmask;
  int dist0=(bpos-lp[0])&bufmask;

  if (dist3==dist2 && dist2==dist1 && dist1==dist0 && dist0>3)
  {
    cycle_len=dist0;
    /*int r=dist0;
    if (r==rlen1) rcount1++;
    else if (r==rlen2) rcount2++;
    else if (rcount1>rcount2) rlen2=r,rcount2=1;
    else rlen1=r,rcount1=1;*/
  }

  /*if (rcount1>15 && cycle_len!=rlen1) cycle_len=rlen1,rcount1=rcount2=0;
  if (rcount2>15 && cycle_len!=rlen2) cycle_len=rlen2,rcount1=rcount2=0;*/

  for (int i=3;i>0;i--) lp[i]=lp[i-1];
  lp[0]=bpos;

  buf[bpos]=lastbyte;
  bpos=(bpos+1)&bufmask;

  int col=bpos%cycle_len;
  //printf("<%i>",col);

  int w=lastbyte;
  int n=buf[(bpos-cycle_len)&bufmask];
  int nn=buf[(bpos-(2*cycle_len))&bufmask];
  int nw=buf[(bpos-cycle_len-1)&bufmask];

  CtxMap.Reset();
  uint32_t h1=w+(n<<8)+(cycle_len<<16);
  uint32_t h2=n+(nn<<8)+(cycle_len<<16);
  uint32_t h3=n+(nw<<8)+(cycle_len<<16);
  uint32_t h4=w+(cycle_len<<8)+(col<<20);
  uint32_t h5=n+(cycle_len<<8)+(col<<20);
  uint32_t h6=(cycle_len)+(col<<12);
  CtxMap.SetContext(h1);
  CtxMap.SetContext(h2);
  CtxMap.SetContext(h3);
  CtxMap.SetContext(h4);
  CtxMap.SetContext(h5);
  CtxMap.SetContext(h6);
}

void RecordModel::Predict(int *mixer,int &mix_cnt,int ctx)
{
  CtxMap.Predict(mixer,mix_cnt,ctx);
}

void RecordModel::Update(int bit)
{
  CtxMap.Update(bit);
}
