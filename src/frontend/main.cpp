#include "..\global.h"
#include "..\common\rangecoder.h"

#include "..\codec\cm.h"
#include "..\common\timer.h"


enum {SCM_ENCODE,SCM_OPTIMIZE,SCM_DECODE,SCM_OPTIMIZE_FAST,SCM_OPTIMIZE_SLOW};

class CmdLine
{
  static const uint32_t upd_rate=1<<14;
  public:
    CmdLine();
    void ProcessParam(int argc,char **argv);
    int ProcessFiles();
    int Encode(TCMParam *Param,int verbose,uint32_t maxbytes);
    void Decode();
    void CloseInput(){if (file_in!=NULL) fclose(file_in);};
    void CloseOutput(){if (file_out!=NULL) fclose(file_out);};
    bool OpenInput();
    bool OpenOutput();
    //void LoadProfile();
    void PrintParam(TCMParam *Param);
    TCMParam CMParam;
    void LoadParam(bool dump);
    int cmode;
    int max_fraction;
    int max_runs;
    int tot_inputbytes;
  private:
    void StoreByte(uint8_t *hdr,uint8_t val){*hdr=val;};
    void StoreWord(uint8_t *hdr,uint16_t val){*hdr=val&0xff;*(hdr+1)=(val>>8)&0xff;};
    uint8_t GetByte(uint8_t *hdr){return *hdr;};
    uint16_t GetWord(uint8_t *hdr){return (*hdr)+(*(hdr+1)<<8);};
    void PackOptions(uint8_t *hdr,int &idx);
    void UnpackOptions(uint8_t *hdr);
    uint32_t FileSize(FILE *file);
    void FillSepComma(char *val,int *data,int scale,int maxlen);
    void PrintProcess(double cr,int inbytes,int outbytes,int totbytes);
    void Put32LH(uint32_t v);
    uint32_t Get32LH();

    char str_in[MAXFNAME],str_out[MAXFNAME],profile_str[MAXFNAME];
    FILE *file_in,*file_out;
};

CmdLine::CmdLine()
:cmode(SCM_ENCODE),file_in(NULL),file_out(NULL)
{
  strcpy(str_in,"");strcpy(str_out,"");
  strcpy(profile_str,"profile.txt");
  max_fraction=100;
  max_runs=100;
};

void CmdLine::Put32LH(uint32_t v)
{
  fputc(v & 0xff,file_out);v>>=8;
  fputc(v & 0xff,file_out);v>>=8;
  fputc(v & 0xff,file_out);v>>=8;
  fputc(v & 0xff,file_out);v>>=8;
}

uint32_t CmdLine::Get32LH()
{
  uint32_t v=0;
  v+=uint32_t(fgetc(file_in));
  v+=uint32_t(fgetc(file_in))<<8;
  v+=uint32_t(fgetc(file_in))<<16;
  v+=uint32_t(fgetc(file_in))<<24;
  return v;
}

uint32_t CmdLine::FileSize(FILE *file)
{
  uint32_t na = ftell(file);
  fseek(file,0,SEEK_END);
  uint32_t ne = ftell(file);
  fseek(file,na,SEEK_SET);
  return ne;
}

void ToUpper(char *str)
{
  char *tstr=str;
  while (*tstr!='\0') {
    *tstr = toupper(*tstr);
    tstr++;
  }
}

void CmdLine::ProcessParam(int argc,char **argv)
{
  bool first=true;
  char tmp[MAXFNAME];
  for (int i=1;i<argc;i++)
  {
    strcpy(tmp,argv[i]);
    if (toupper(tmp[0])=='-')
    {
      ToUpper(tmp);
      if (strcmp(tmp+1,"E")==0) cmode=SCM_ENCODE;
      else if (strcmp(tmp+1,"D")==0) cmode=SCM_DECODE;
      else if (toupper(tmp[1])=='O')
      {
         cmode=SCM_OPTIMIZE;
         if (strlen(tmp+2)>0) max_fraction=clamp(atoi(tmp+2),0,100);
      } else if ((tmp[1]=='P') || (tmp[1]=='p'))
      {
        strcpy(profile_str,tmp+2);
      } else if (toupper(tmp[1])=='R')
      {
        max_runs=clamp(atoi(tmp+2),0,1000);
      } else if (strcmp(tmp+1,"MX")==0)
      {
        CMParam.maximize=true;
      } else printf("  warning: unknown option '%s'\n",tmp);
    } else {
      if (first) {strcpy(str_in,tmp);first=false;}
      else strcpy(str_out,tmp);
    }
  }
}

bool CmdLine::OpenInput()
{
  if ( (file_in = fopen(str_in,"rb"))==NULL) {printf("could not open:  '%s'\n",str_in);return false;}
  else
  {
    tot_inputbytes=FileSize(file_in);
    return true;
  }
}

bool CmdLine::OpenOutput()
{
  if ( (file_out = fopen(str_out,"wb"))==NULL){printf("could not create: '%s'\n",str_out);return false;}
  else return true;
}

int RemoveTrail(char *cout,char *cin)
{
  char *out=cout;
  char *in=cin;
  while (*in!='\0')
  {
    if (*in == '#') break;
    else if ((*in != ' ') && (*in != 13) && (*in != 10)) *(out++)=*in;
    in++;
  }
  *out='\0';
  return out-cout;
}

void CmdLine::FillSepComma(char *val,int *data,int scale,int maxlen)
{
  int idx,x;
  char t[256];
  char *s=val;
  idx=x=0;
  bool stop=false;
  while (true)
  {
     char *c=strchr(s,',');
     if (c==NULL)
     {
       c=s+strlen(s);
       stop=true;
     }
     int len=c-s;strncpy(t,s,len);t[len]='\0';
     x=clamp(atoi(t),0,scale);

     data[idx]=x;
     idx++;
     if (stop) break;
     s=c+1;
  }
  while (idx<maxlen) data[idx++]=x;
}

void CmdLine::LoadParam(bool dump)
{
  FILE *pFile;
  pFile = fopen(profile_str,"r");
  char s[256],t[256];
  if (!pFile) printf("\n'%s' not found! using default params.\n",profile_str);
  else {
    while (1) {
      fgets(t,256,pFile);
      if (!feof(pFile))
      {
        int slen=RemoveTrail(s,t);
        if (slen>0)
        {
         char *substr = strchr(s,'=');
         char key[256],val[256];

         if (substr!=NULL)
         {
           int len_key = substr-s;
           int len_val = strlen(s)-len_key-1;
           strncpy(key,s,len_key);key[len_key]=0;ToUpper(key);
           strncpy(val,substr+1,len_val);val[len_val]=0;ToUpper(val);
           if (strcmp(key,"MAXORDER")==0) CMParam.maxorder=clamp(atoi(val),1,32);
           else if (strcmp(key,"MEMHASH")==0) CMParam.memhash=clamp(atoi(val),10,30);
           else if (strcmp(key,"COUNTER_LIMIT")==0) CMParam.counter_limit=clamp(atoi(val),0,PSCALE);
           else if (strcmp(key,"WORD_MODEL")==0) CMParam.word_model=((strcmp(val,"TRUE")==0) || (strcmp(val,"1")==0))?true:false;
           else if (strcmp(key,"WORD_MODEL_MEM")==0) CMParam.word_model_mem=clamp(atoi(val),10,29);
           else if (strcmp(key,"WORD_MODEL_LIMIT")==0) CMParam.word_model_limit=clamp(atoi(val),0,PSCALE);
           else if (strcmp(key,"SPARSE_MODEL")==0) CMParam.sparse_model=((strcmp(val,"TRUE")==0) || (strcmp(val,"1")==0))?true:false;
           else if (strcmp(key,"SPARSE_MODEL_MEM")==0) CMParam.sparse_model_mem=clamp(atoi(val),1,31);
           else if (strcmp(key,"SPARSE_MODEL_LIMIT")==0) CMParam.sparse_model_limit=clamp(atoi(val),0,PSCALE);
           else if (strcmp(key,"RECORD_MODEL")==0) CMParam.record_model=((strcmp(val,"TRUE")==0) || (strcmp(val,"1")==0))?true:false;
           else if (strcmp(key,"RECORD_MODEL_MEM")==0) CMParam.record_model_mem=clamp(atoi(val),1,31);
           else if (strcmp(key,"RECORD_MODEL_LIMIT")==0) CMParam.record_model_limit=clamp(atoi(val),0,PSCALE);
           else if (strcmp(key,"INDIRECT_MODEL")==0) CMParam.indirect_model=((strcmp(val,"TRUE")==0) || (strcmp(val,"1")==0))?true:false;
           else if (strcmp(key,"INDIRECT_MODEL_MEM")==0) CMParam.indirect_model_mem=clamp(atoi(val),1,31);
           else if (strcmp(key,"INDIRECT_MODEL_LIMIT")==0) CMParam.indirect_model_limit=clamp(atoi(val),0,PSCALE);
           else if (strcmp(key,"MATCH_MODEL")==0) CMParam.match_model=((strcmp(val,"TRUE")==0) || (strcmp(val,"1")==0))?true:false;
           else if (strcmp(key,"MATCH_WIN_SIZE")==0) CMParam.match_win_size=clamp(atoi(val),1,31);
           else if (strcmp(key,"MATCH_MODEL_RATE")==0) CMParam.match_model_rate=clamp(atof(val),0.0001,1.0)*PSCALE;
           else if (strcmp(key,"FINAL_MIX_RATE")==0) CMParam.final_mix_rate=clamp(atof(val),0.0001,1.0)*WSCALE;
           else if (strcmp(key,"FINAL_CHAIN_RATE")==0) CMParam.final_chain_rate=clamp(atof(val),0.0001,1.0)*WSCALE;
           else if (strcmp(key,"FINAL_SSE")==0) CMParam.final_sse=clamp(atoi(val),0,2);
           else if (strcmp(key,"FINAL_SSE_LIMIT")==0) CMParam.final_sse_limit=clamp(atoi(val),0,PSCALE);
           else if (strcmp(key,"FINAL_SSE_MIX_RATE")==0) CMParam.final_sse_mix_rate=clamp(atof(val),0.0001,1.0)*WSCALE;
           else printf("unknown option: '%s'\n",s);
           //printf("'%s' '%s'\n",key,val);
         }
       }
      } else break;
    }
  }
  if (dump) PrintParam(&CMParam);
}

void CmdLine::PrintParam(TCMParam *Param)
{
  printf("  maximum order:  %i\n",Param->maxorder);
  printf("  counter:        rate %i\n",Param->counter_limit);
  /*if (Param->mixer_chain)
  {
    printf("  mixer rate:     ");
    for (int i=0;i<Param->maxorder;i++) printf("%0.4f ",float(Param->mix_rate[i])/float(WSCALE));
    printf("\n");
    printf("  mixer init:     ");
    for (int i=0;i<Param->maxorder;i++) printf("%0.4f ",float(Param->mix_init[i])/float(WSCALE));
    printf("\n");
  }*/
  printf("  final:          mix rate %0.4f  chain rate %0.4f\n",float(Param->final_mix_rate)/float(WSCALE),float(Param->final_chain_rate)/float(WSCALE));
  if (Param->final_sse)     printf("  final sse:      limit %i  mix rate %0.4f\n",Param->final_sse_limit,float(Param->final_sse_mix_rate)/float(WSCALE));
  if (Param->match_model)   printf("  match model:    rate %0.4f  window %i kb\n",float(Param->match_model_rate)/float(PSCALE),1<<(Param->match_win_size-10));
  if (Param->word_model)    printf("  word model:     limit %i\n",Param->word_model_limit);
  if (Param->sparse_model)  printf("  sparse model:   limit %i\n",Param->sparse_model_limit);
  if (Param->record_model)  printf("  record model:   limit %i\n",Param->record_model_limit);
  if (Param->indirect_model)printf("  indirect model: limit %i\n",Param->indirect_model_limit);
  printf("\n");
}

void CmdLine::PackOptions(uint8_t *hdr,int &idx)
{
  idx=1;
  int opt=int(CMParam.word_model) + (int(CMParam.sparse_model)<<1) + (int(CMParam.match_model<<2))+(int(CMParam.record_model<<3))+(int(CMParam.indirect_model<<4))+(int(CMParam.final_sse)<<5) + (int(CMParam.maximize)<<6);
  StoreByte(hdr+idx,CMParam.maxorder-1);idx++;
  StoreByte(hdr+idx,opt);idx++;
  StoreByte(hdr+idx,CMParam.memhash);idx++;
  StoreWord(hdr+idx,CMParam.counter_limit);idx+=2;
  StoreWord(hdr+idx,CMParam.final_mix_rate);idx+=2;
  StoreWord(hdr+idx,CMParam.final_chain_rate);idx+=2;
  if (CMParam.match_model)
  {
    StoreByte(hdr+idx,CMParam.match_win_size);idx+=1;
    StoreWord(hdr+idx,CMParam.match_model_rate);idx+=2;
  }
  if (CMParam.word_model)
  {
    StoreByte(hdr+idx,CMParam.word_model_mem);idx+=1;
    StoreWord(hdr+idx,CMParam.word_model_limit);idx+=2;
  }
  if (CMParam.sparse_model)
  {
    StoreWord(hdr+idx,CMParam.sparse_model_mem);idx+=2;
    StoreWord(hdr+idx,CMParam.sparse_model_limit);idx+=2;
  }
  if (CMParam.record_model)
  {
    StoreWord(hdr+idx,CMParam.record_model_mem);idx+=2;
    StoreWord(hdr+idx,CMParam.record_model_limit);idx+=2;
  }
  if (CMParam.indirect_model)
  {
    StoreWord(hdr+idx,CMParam.indirect_model_mem);idx+=2;
    StoreWord(hdr+idx,CMParam.indirect_model_limit);idx+=2;
  }
  if (CMParam.final_sse)
  {
    StoreWord(hdr+idx,CMParam.final_sse_limit);idx+=2;
    StoreWord(hdr+idx,CMParam.final_sse_mix_rate);idx+=2;
  }
  StoreByte(hdr,uint8_t(idx));
}

void CmdLine::UnpackOptions(uint8_t *hdr)
{
  int idx=0;
  CMParam.maxorder=GetByte(hdr+idx)+1;idx++;
  uint8_t opt=GetByte(hdr+idx);idx++;
  CMParam.word_model=opt&1;
  CMParam.sparse_model=(opt>>1)&1;
  CMParam.match_model=(opt>>2)&1;
  CMParam.record_model=(opt>>3)&1;
  CMParam.indirect_model=(opt>>4)&1;
  CMParam.final_sse=(opt>>5)&1;
  CMParam.maximize=(opt>>6)&1;
  CMParam.memhash=GetByte(hdr+idx);idx++;
  CMParam.counter_limit=GetWord(hdr+idx);idx+=2;
  CMParam.final_mix_rate=GetWord(hdr+idx);idx+=2;
  CMParam.final_chain_rate=GetWord(hdr+idx);idx+=2;
  if (CMParam.match_model)
  {
    CMParam.match_win_size=GetByte(hdr+idx);idx++;
    CMParam.match_model_rate=GetWord(hdr+idx);idx+=2;
  }
  if (CMParam.word_model)
  {
    CMParam.word_model_mem=GetByte(hdr+idx);idx+=1;
    CMParam.word_model_limit=GetWord(hdr+idx);idx+=2;
  }
  if (CMParam.sparse_model)
  {
    CMParam.sparse_model_mem=GetWord(hdr+idx);idx+=2;
    CMParam.sparse_model_limit=GetWord(hdr+idx);idx+=2;
  }
  if (CMParam.record_model)
  {
    CMParam.record_model_mem=GetWord(hdr+idx);idx+=2;
    CMParam.record_model_limit=GetWord(hdr+idx);idx+=2;
  }
  if (CMParam.indirect_model)
  {
    CMParam.indirect_model_mem=GetWord(hdr+idx);idx+=2;
    CMParam.indirect_model_limit=GetWord(hdr+idx);idx+=2;
  }
  if (CMParam.final_sse)
  {
    CMParam.final_sse_limit=GetWord(hdr+idx);idx+=2;
    CMParam.final_sse_mix_rate=GetWord(hdr+idx);idx+=2;
  }
}

double GetRate(double t,uint32_t fs)
{
   return(t>0.0?fs/t:0.);
}

void CmdLine::PrintProcess(double ct,int inbytes,int outbytes,int totbytes)
{
  float c_rate=(totbytes>0)?double(inbytes)*100.0/double(totbytes):0.0;
  float c_time_rate=((ct>0.0)?double(inbytes)/ct:0.0)/1024;
  printf("  %i/%i: %5.1f%%  (%0.2f kb/s)\r",inbytes,outbytes,c_rate,c_time_rate);
}

int CmdLine::Encode(TCMParam *Param,int verbose,uint32_t maxbytes)
{
  Timer myTimer;
  myTimer.StartTimer();

  uint32_t inbytes,counter;
  inbytes=counter=0;

  Put32LH(tot_inputbytes);
  uint8_t hdr[256];
  int len;
  PackOptions(hdr,len);
  fwrite(hdr,1,len,file_out);
  RangeCoderSH Coder(file_out,0);
  //CM *MyCM=new CM(&Coder,&Param);


  CM *MyCM=new CM(&Coder,Param,verbose);
  Coder.Init();
  int c;
  while ((c=fgetc(file_in))!=EOF)
  {
    MyCM->Encode(c);
    inbytes++;
    if (inbytes-counter>upd_rate)
    {
        if (verbose)
        {
           myTimer.StopTimer();
           PrintProcess(myTimer.GetSecTime(),inbytes,FileSize(file_out),tot_inputbytes);counter=inbytes;
        }
        if ( (maxbytes>0)&&(inbytes>=maxbytes)) break;
    };
  }
  Coder.Stop();
  delete MyCM;

  uint32_t outbytes=FileSize(file_out);
  if (verbose)
  {
    myTimer.StopTimer();
    PrintProcess(myTimer.GetSecTime(),inbytes,outbytes,tot_inputbytes);
    float cr=float(outbytes)/float(inbytes);
    printf("\n %5.1f%%  (%0.3fbpb)\n",cr*100.0,cr*8.0);
  };
  return outbytes;
}

void CmdLine::Decode()
{
  uint32_t totbytes,outbytes,counter;
  outbytes=counter=0;

  totbytes=Get32LH(); // read file header

  uint8_t hdr[256];
  int hdr_len=fgetc(file_in);
  fread(hdr,1,hdr_len-1,file_in);
  UnpackOptions(hdr);
  RangeCoderSH Coder(file_in,1);
  PrintParam(&CMParam);
  CM *MyCM=new CM(&Coder,&CMParam,true);
  Coder.Init();
  while (outbytes<totbytes)
  {
    int val=MyCM->Decode();
    fputc(val,file_out);
    outbytes++;
    if (outbytes-counter>upd_rate){PrintProcess(0,outbytes,totbytes,totbytes);counter=outbytes;};
  }
  Coder.Stop();
  PrintProcess(0,outbytes,FileSize(file_out),totbytes);printf("\n");
  delete MyCM;
}

int CmdLine::ProcessFiles()
{
  int retval=0;
  printf("open:   '%s'\n",str_in);
  printf("create: '%s'\n",str_out);
  //printf("profile: %i \n",profile);
  //LoadParam();
  if (cmode==SCM_ENCODE)
  {
    if (OpenInput() && OpenOutput())
    {
      Encode(&CMParam,1,0);
      CloseInput();
      CloseOutput();
    }
  } else {
    if (OpenInput() && OpenOutput()) Decode();
    CloseInput();
    CloseOutput();
  }
  return retval;
}

typedef struct
{
  int *param;
  int min,max;
} Component;

bool BitSet(int val,int bit) {return((val>>(bit-1))&1);};
int SetBit(int val,int bit) {return(val|(1<<(bit-1)));};

class SimulatedAnnealing
{
  public:
    SimulatedAnnealing(CmdLine *CommandLine,int OptSet,double init_prob,double cooling_rate);
    void StoreBestState(TCMParam *Dst){CopyState(Dst,&BestState);};
    void FullOptimize(int mode,int maxruns);
    int GetBest(){return ebest;};
    void Init(bool random_start);
  protected:
    void ChangeSingleComponent(int idx,double radius);
    void Run(int maxruns,bool full_search,double radius);
    void CreateStartVector(TCMParam *Param);
    int CreateVector(TCMParam *Param,Component *Vector);
    int GetEnergy(TCMParam *Param);
    double randfloat(){return (double(rand())+0.5)/double(RAND_MAX+1);};
    int randint(int min,int max){return (rand()%(max-min+1))+min;};
    void CreateProposal(TCMParam *Param,bool full_search,double radius);
    void CopyState(TCMParam *Dst,TCMParam *Src);
    CmdLine *Cmd;
    TCMParam BestState,ActualState;

    Component Vector[256];
    int accepted,rejected;
    int ebest,eactual,total_runs;
    int Set;
    double init_prob_accepted,alpha;
    double sum_dE,temp;
};

SimulatedAnnealing::SimulatedAnnealing(CmdLine *CommandLine,int OptSet,double init_prob,double cooling_rate)
:Cmd(CommandLine),Set(OptSet),init_prob_accepted(init_prob),alpha(cooling_rate)
{
}
int SimulatedAnnealing::GetEnergy(TCMParam *Param)
{
  uint32_t size=0;
  if (Cmd->OpenInput() && Cmd->OpenOutput())
  {
    int maxbytes=0;
    if (Cmd->max_fraction<100) maxbytes=int(double(Cmd->tot_inputbytes)*(double(Cmd->max_fraction)/100.0)+0.5);
    size=Cmd->Encode(Param,0,maxbytes);
    Cmd->CloseInput();
    Cmd->CloseOutput();
  } else printf("  warning: error open/create file\n");
  //numruns++;
  return size;
}

void SimulatedAnnealing::CopyState(TCMParam *Dst,TCMParam *Src)
{
  memcpy(Dst,Src,sizeof(TCMParam));
}

int SimulatedAnnealing::CreateVector(TCMParam *Param,Component *Vector)
{
  int n=0;
  if (Param->word_model && BitSet(Set,1))
  {
    Vector[n].param=&Param->word_model_limit;
    Vector[n].min=0;Vector[n].max=PSCALEm;
    n++;
  };
  if (Param->match_model && BitSet(Set,2))
  {
    Vector[n].param=&Param->match_model_rate;
    Vector[n].min=0;Vector[n].max=PSCALEm;
    n++;
  };
  if (BitSet(Set,3))
  {
     Vector[n].param=&Param->counter_limit;
     Vector[n].min=0;Vector[n].max=PSCALEm;
     n++;
  }
  if (BitSet(Set,4))
  {
    Vector[n].param=&Param->final_mix_rate;
    Vector[n].min=0;Vector[n].max=WSCALE;
    n++;
    Vector[n].param=&Param->final_chain_rate;
    Vector[n].min=0;Vector[n].max=WSCALE;
    n++;
  }
  if (Param->final_sse && BitSet(Set,5))
  {
    Vector[n].param=&Param->final_sse_limit;
    Vector[n].min=0;Vector[n].max=PSCALEm;
    n++;
    Vector[n].param=&Param->final_sse_mix_rate;
    Vector[n].min=0;Vector[n].max=WSCALE;
    n++;
  }
  if (Param->sparse_model && BitSet(Set,6))
  {
    Vector[n].param=&Param->sparse_model_limit;
    Vector[n].min=0;Vector[n].max=PSCALEm;
    n++;
  }
  if (Param->record_model && BitSet(Set,7))
  {
    Vector[n].param=&Param->record_model_limit;
    Vector[n].min=0;Vector[n].max=PSCALEm;
    n++;
  }
  if (Param->indirect_model && BitSet(Set,8))
  {
    Vector[n].param=&Param->indirect_model_limit;
    Vector[n].min=0;Vector[n].max=PSCALEm;
    n++;
  }
  return n;
}

void SimulatedAnnealing::CreateStartVector(TCMParam *Param)
{
  int dim=CreateVector(Param,Vector);
  for (int i=0;i<dim;i++)
  {
  int *p=Vector[i].param;
  int min_param=Vector[i].min;
  int max_param=Vector[i].max;

  int proposal=randint(min_param,max_param);
  *p=proposal;
  }
}

void SimulatedAnnealing::ChangeSingleComponent(int idx,double radius)
{
   int *p=Vector[idx].param;
   int min_param=Vector[idx].min;
   int max_param=Vector[idx].max;

   int r=(int)(radius*double(max_param-min_param)+0.5);
   *p = randint(max(min_param,*p-r),min(max_param,*p+r));
}

void SimulatedAnnealing::CreateProposal(TCMParam *Param,bool full_search,double radius)
{
  //int *p=&Param->final_mix_rate;
  int dim=CreateVector(Param,Vector);
  if (dim<1) return;

 if (full_search) for (int i=0;i<dim;i++) ChangeSingleComponent(i,radius);
 else ChangeSingleComponent(randint(0,dim-1),radius);
}

void SimulatedAnnealing::Init(bool random_start=false)
{
  srand(time(0));
  if (random_start) CreateStartVector(&Cmd->CMParam);
  ebest = GetEnergy(&Cmd->CMParam);
  eactual=ebest;
  CopyState(&BestState,&Cmd->CMParam);
  total_runs=0;
  sum_dE=temp=0.0;
  printf(" SA InitState: %i bytes alpha=%0.4f)\n",ebest,alpha);
}

void SimulatedAnnealing::Run(int maxruns,bool full_search,double radius)
{
  accepted=rejected=0;
  CopyState(&ActualState,&BestState);

  sum_dE=0;
  int num_runs=0;
  while (num_runs < maxruns)
  {

     TCMParam NewState;
     CopyState(&NewState,&ActualState);
     CreateProposal(&NewState,full_search,radius);

     int enew=GetEnergy(&NewState);

     bool proposal_accepted=false;

     num_runs++;
     total_runs++;

     int dE = enew - eactual;
     if (num_runs<=2)
     {
         sum_dE+=abs(dE);
         double tE=sum_dE/double(num_runs);
         temp=-tE/log(init_prob_accepted);
     }

     if (dE > 0)
     {
       double pAccept=exp(-double(dE/temp));
       if (randfloat()<pAccept) proposal_accepted=true;
     } else proposal_accepted=true;

     if (proposal_accepted)
     {
       CopyState(&ActualState,&NewState);
       eactual=enew;
       accepted++;
     } else rejected++;

     // we found new optimum
     double rate=accepted+rejected>0?double(accepted)/double(accepted+rejected):0.0;
     printf("[%i] [rate: %4.1f%%], temp: %0.4f\r",total_runs,rate*100.0,temp);

     if (enew < ebest)
     {
       CopyState(&BestState,&NewState);
       ebest=enew;
       printf("\n best [%i]: %i (radius: %0.4f)\n",full_search,ebest,radius);
     }

     temp=temp*alpha;
     if (temp<1e-8) break;
  }
}

void SimulatedAnnealing::FullOptimize(int mode,int maxruns)
{
   printf(" Max runs: %i\n",maxruns);
   if (mode==SCM_OPTIMIZE_FAST)
   {
     //Run(maxruns/5,true,0.10);
     Run(maxruns/5,false,0.05);
     Run((maxruns*2)/5,false,0.01);
     Run((maxruns*2)/5,false,0.005);
   } else if (mode==SCM_OPTIMIZE_SLOW)
   {
     Run(50,false,0.10);
   }
}

struct tState3 {
  int n0,n1,lb;
  int next[2];
};

// fsm 3
class fsm3 {
  public:
    fsm3(int num_maxstates)
    :maxstates(num_maxstates)
    {
      States=new tState3[maxstates];
      memset(States,0,maxstates*sizeof(tState3));
      actual_state=0;
      NewState(0,0,-1);
      GenerateNextStates();
    }
    void Dump()
    {
       for (int i=0;i<maxstates;i++)
       {
           printf("%i  n0:%i  n1:%i  lb:%i  s0:%i  s1:%i\n",i,States[i].n0,States[i].n1,States[i].lb,States[i].next[0],States[i].next[1]);
       }
    }
    int NewState(int n0,int n1,int lb)
    {
       for (int i=0;i<actual_state;i++)
       {
           if (States[i].n0==n0 && States[i].n1==n1 && States[i].lb==lb) return i;
       }
       States[actual_state].n0=n0;
       States[actual_state].n1=n1;
       States[actual_state].lb=lb;
       actual_state++;
       return actual_state-1;
    }
    void GenerateNextStates()
    {
       for (int i=0;i<actual_state;i++)
       {
           // i is a state from which we have to generate next values
          int n0=States[i].n0;
          int n1=States[i].n1;
          States[i].next[0] = NewState(n0+1,n1>2?n1/2+1:n1,0);
          if (actual_state>=maxstates) break;

          States[i].next[1] = NewState(n0>2?n0/2+1:n0,n1+1,1);
          if (actual_state>=maxstates) break;
       }
    }
    ~fsm3()
    {
      delete []States;
    }
  protected:
    int maxstates,actual_state;
    tState3 *States;
};


int main(int argc,char **argv)
{
    /*double mine=1000000.0;
    int mins=0;
    for (int s=512;s<4096;s++)
    {
      SSENL<5>mySSE(s);
      double e=mySSE.MAE();
      if (e<=mine){mine=e;mins=s;};
    }
    printf("%i %0.4f\n",mins,mine);

    return 0;*/
    printf("scm 0.0.1d - lossless data compression - Sebastian Lehmann (2010-2011)\n");
    if (argc < 2)
    {
      printf("usage: scm [-options] input output\n\n");
      printf("  -e     encode input to output\n");
      printf("  -d     decode input to output\n");
      printf("  -p<n>  use profile file <s>, standard is <profile.txt>\n");
      printf("  -o<n%%> optimize model parameters on specific file\n");
      printf("         n specifies percentage of test, default=100\n");
    } else
    {
      CmdLine MyCmdLine;
      MyCmdLine.ProcessParam(argc,argv);
      if (MyCmdLine.cmode==SCM_OPTIMIZE)
      {
        MyCmdLine.LoadParam(false);
        int Set=0;
        Set=SetBit(Set,1); // word-model
        Set=SetBit(Set,2); // match-model
        Set=SetBit(Set,3); // counter
        Set=SetBit(Set,4); // final mix
        Set=SetBit(Set,5); // final sse
        Set=SetBit(Set,6); // sparse model
        Set=SetBit(Set,7); // record model
        Set=SetBit(Set,8); // indirect model
        SimulatedAnnealing SA(&MyCmdLine,Set,0.01,0.9);
        SA.Init();
        SA.FullOptimize(SCM_OPTIMIZE_FAST,MyCmdLine.max_runs);
        printf("\n\n  best value found: %i bytes\n",SA.GetBest());
        SA.StoreBestState(&MyCmdLine.CMParam);
        MyCmdLine.PrintParam(&MyCmdLine.CMParam);
        MyCmdLine.cmode=SCM_ENCODE;
        MyCmdLine.ProcessFiles();
      } else {
        if (MyCmdLine.cmode==SCM_ENCODE) MyCmdLine.LoadParam(true);
        MyCmdLine.ProcessFiles();
      }
    };
    return 0;
}
