typedef struct
{
   char addr;
   bool state; //on/off
   char waveform; // 0, flat; 1, sinewave
   char scale; //this is percentage, default 100
} dac_chan_type;
//dac_chan_type DAC_LIST[2];
