#ifndef TIMER_H
#define TIMER_H

class Timer {
  public:
    void StartTimer()
    {
        tstart=clock();
    };
    void StopTimer()
    {
        tsec = (clock()-tstart)/double(CLOCKS_PER_SEC);
    };
    double GetSecTime(){return tsec;};
  protected:
    double tstart,tsec;
};

#endif
