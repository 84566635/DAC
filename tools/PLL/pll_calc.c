#include <stdint.h>
#include <stdio.h>
#include <math.h>

#define CKIN ((44100.0d*32.0d*6.0d)/1000000.0d)

#define abs1(a) (((a)>=0.0d)?(a):-(a))
void calc(double freqIn, double freq)
{
  int n/*1-4095*/,m/*1-511*/,p/*1-127*/;
  int nt=0/*1-4095*/,mt=0/*1-511*/,pt=0/*1-127*/;
  double err = 1.0;
  for(n=1; n< 4096; n++)
    for(m=1; m< 512; m++)
      for(p=1; p< 128; p++)
        {
          double vco = (freqIn*(double)n)/((double)(m));
          if( vco > 100.0 && vco < 200.0)
            {
              double tfreq = vco/((double)(p));
              double nerr = abs1(tfreq-freq)/freq;
              if(nerr < err)
                {
                  nt=n;
                  mt=m;
                  pt=p;
                  err=nerr;
                }
            }
        }
  double vco = (freqIn*(double)nt)/((double)(mt));
  double tfreq = vco/((double)(pt));
  printf("fIn=%3f n=%4d m=%3d p=%3d err=%f%% freq=%f vco=%f\n", freqIn, nt, mt, pt, 100.0*err, tfreq, vco);
}


static double getErr(double n, double q, double freq)
{
  double tfreq = (1000000.0d*(double)n)/((double)(q))/2.0d;
  double nerr = abs1(tfreq-freq)/freq;
  return nerr;
}
#define ENCODE(arg_n, arg_q, arg_d) (int)(((arg_n)<<16) | ((arg_q)<<8) |((arg_d)<<0))
double mdiv[] = {1,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32};
static void calcSAI(double freq)
{
  int n/*192-432*/,q/*2-15*/,d,mc;
  int nt=0,qt=0,dt=0,mct=0;
  double err = 10000.0;
  //for(mc=0; mc < 32; mc++)
  for(d = 1; d <= 32; d++)
    for(n=192; n< 433; n++)
      for(q=2; q< 16; q++)
        {
          double tfreq = ((1000000.0d*(double)n)/((double)(q)))/((double)d)/mdiv[mc];
          double nerr = abs1(tfreq-freq)/freq;
          if(nerr < 0.0005d) printf("n=%3d q=%2d d=%2d mdiv=%f err=%f%% freq=%.0fHz fdiff=%.0fHz fSMPL=%.0fHz cfg: 0x%08x \n", n, q, d, mdiv[mc], 100.0*nerr, tfreq, tfreq-freq, tfreq/256.0d, ENCODE(n,q,d));

          if(nerr < err)
            {
              nt=n;
              qt=q;
              err=nerr;
              dt = d;
            }

        }
  double tfreq = ((1000000.0d*(double)nt)/((double)(qt)))/((double)dt/mdiv[mct]);
  printf("Best setting: n=%3d q=%2d d=%2d mdiv=%f err=%f%% freq=%.0fHz fdiff=%.0fHz fSMPL=%.0fHz cfg: 0x%08x \n", nt, qt, dt, mdiv[mct], 100.0*err, tfreq, tfreq-freq, tfreq/256.0d, ENCODE(nt,qt,dt));
}

#define FOUT44100 (512*0.0441d)
#define FOUT48000 (512*0.048d)

int main(void)
{
  calcSAI(11289600);
  calcSAI(12288000);
  calcSAI(11289600*2);
  calcSAI(12288000*2);

  //  printf("%f %f %f %f \n", 100*getErr(271,12, 11289600), 100*getErr(246,10, 12288000),100*getErr(361,8, 11289600*2),100*getErr(246,5, 12288000*2));

  calc(CKIN, FOUT44100);
  calc(CKIN, FOUT48000);
  calc(CKIN*48.0d/44.1d, FOUT48000);

  calc(141.12d/16, FOUT44100);
  calc(141.12d/16, FOUT48000);
  calc((141.12d/16)*48.0d/44.1d, FOUT48000);

}
