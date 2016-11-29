#include <Automaton.h>
#include <Adafruit_MCP4725.h>
#include <dac_type.h>
#include <SPI.h>


char cmd_buffer[80];
Atm_command cmd;
Atm_timer oneMilli;
Atm_step waveGen;

#define SERIAL_PORT Serial
#define PROJ_NAME "walnut"
#define DAC_NUM_CHAN 13
#define TIMER_PERIOD 100 //ms
#define ONE_HUNDRED 100
//#define PI 3.1416 //Arduino has it already.

// pins used for the connection with the actuators
// the other you need are controlled by the SPI library):
 /* Using default SPI setting
 CS       : pin 10
 MOSI     : pin 11
 MISO     : pin 12
 SCK      : pin 13
 */

//SPI Configuration
SPISettings SPI_settings(1000000, MSBFIRST, SPI_MODE0);

//Adafruit_MCP4725 dac[DAC_NUM_CHAN-1];

dac_chan_type DAC_LIST[DAC_NUM_CHAN];

#define WAVE_POINTS_NUMBER  64

const unsigned char DAC_SPI_Wave_Points[WAVE_POINTS_NUMBER] =

{
  0,   0,   2,   5,  10,  15,  21,  29,
37,  46,  56,  67,  78,  90, 102, 115,
127, 139, 152, 164, 176, 187, 198, 208,
217, 225, 233, 239, 244, 249, 252, 253,
255, 253, 252, 249, 244, 239, 233, 225,
217, 208, 198, 187, 176, 164, 152, 139,
127, 115, 102,  90,  78,  67,  56,  46,
37,  29,  21,  15,  10,   5,   0,   0
};


enum { CMD_INIT, CMD_ERASENV, CMD_HIGH, CMD_LOW, CMD_READ, CMD_AREAD, CMD_AWRITE,
       CMD_MODE_INPUT, CMD_MODE_OUTPUT, CMD_MODE_PULLUP,
       CMD_SET_DAC,
       BAD_COMMAND=-1};

const char cmdlist[] =
  "init eraseNV high low read aread awrite mode_input mode_output mode_pullup set_dac";

void cmd_callback( int idx, int v, int up ) {
  int pin = atoi( cmd.arg( 1 ) );
  switch ( v ) {
    case CMD_ERASENV:
      Serial.println("SPI DAC Erase NV Content!");
      break;
    case CMD_INIT:
      SPIDacInit();
      Serial.println("SPI DAC initilization");
      break;
    case CMD_HIGH:
      digitalWrite( pin, HIGH );
      break;
    case CMD_LOW:
      digitalWrite( pin, LOW );
      break;
    case CMD_READ:
      SERIAL_PORT.println( digitalRead( pin ) );
      break;
    case CMD_AREAD:
      SERIAL_PORT.println( analogRead( pin ) );
      break;
    case CMD_AWRITE:
      analogWrite( A14, atoi( cmd.arg( 1 ) ) );
      break;
    case CMD_MODE_INPUT:
      pinMode( pin, INPUT );
      break;
    case CMD_MODE_OUTPUT:
      pinMode( pin, OUTPUT );
      break;
    case CMD_MODE_PULLUP:
      pinMode( pin, INPUT_PULLUP );
      break;
    case CMD_SET_DAC:
      if ( ( atoi( cmd.arg(1) ) > 0 ) &&
         atoi( cmd.arg(1) ) < DAC_NUM_CHAN &&
         atoi( cmd.arg(2) ) < 101 &&
         atoi( cmd.arg(3) ) < 2 ) //1 or 0
      {  //idx,           addr (do not change)  on/off    waveform#
         DAC_LIST[atoi(cmd.arg(1))].state = atoi( cmd.arg(3));
         DAC_LIST[atoi(cmd.arg(1))].scale = atoi( cmd.arg(2));
      } else {
            //dac.setVoltage( 0, false );
         SERIAL_PORT.print("Wrong Channel Number or Voltage out of range(0 .. 100), do nothing!");
      }
      break;
    case BAD_COMMAND:
      if ( not strcmp( cmd.arg(0), "" ))
      {
        break;
      } else {
        SERIAL_PORT.print( cmd.arg(0) );
        //SERIAL_PORT.println(cmd.arg(1));
        //SERIAL_PORT.println(cmd.arg(2));
        //SERIAL_PORT.println(cmd.arg(3));
        SERIAL_PORT.println(" is a bad command");
        break;
      }
  }
  SERIAL_PORT.print("\r");
  SERIAL_PORT.print(PROJ_NAME);
  SERIAL_PORT.print(">");
  return;
}

void SPIChipSelect(unsigned char chan) {
  SPIUnselectAll();
  if ( chan > 0 and chan < DAC_NUM_CHAN) {
    digitalWrite(DAC_LIST[chan].csPin, LOW);
    delayMicroseconds(2);
  } else {
    SERIAL_PORT.println("channel number out of range 1~12");
  }
}

void SPIUnselectAll() {
  for (int i = 1; i < DAC_NUM_CHAN; i++) { //remember start from 1, 0 is internal DAC.
    digitalWrite(DAC_LIST[i].csPin, HIGH);
  } //make sure turn off every channel first.
}

void step_callback_spi ( int idx, int v, int up ) {
  for ( short i = 1; i < DAC_NUM_CHAN; i++ ) { //ignore DAC 0, we don't use internal DAC anymore.
     if ( DAC_LIST[i].state ) {
        SPIChipSelect(DAC_LIST[i].addr);
        //SPIDacInit();
        //
        for ( int j = 0; j < 64*150; j++) {
          SPISetVoltage(DAC_LIST[i].addr, (DAC_SPI_Wave_Points[j % 64])*DAC_LIST[i].scale/ONE_HUNDRED);
          if ( (j % 64) != 0 ) {
            delayMicroseconds(45); //to make the total set time 50uS.
          } else {
            delayMicroseconds(5000); //Take a break each full wave.
          }
        }
        SPISetVoltage(DAC_LIST[i].addr, 0x00);
        //all done, set the state back to 0
        DAC_LIST[i].state = 0;
        SPIUnselectAll();
     }  else {
        //all the other 11 channels
        //let's explicitly select the chip and set it to 0 voltage.
        SPIChipSelect(DAC_LIST[i].addr);
        SPISetVoltage(DAC_LIST[i].addr, 0x00);
        SPIUnselectAll();
     }
  }
}

void SPISetVoltage(char chan, unsigned char voltage) {
  switch ( chan ) {
    case 1:
      SPI.transfer(0x08); //  Set DAC 1 // 
      break;
    case 2:
      SPI.transfer(0x00); // Set DAC 0 // 
      break;
    case 3:
      SPI.transfer(0x08); //  Set DAC 1 // 
      break;
    case 4:
      SPI.transfer(0x00); //  Set DAC 0 //
      break;
    case 5:
      SPI.transfer(0x08); //  Set DAC 1 //
      break;
    case 6:
      SPI.transfer(0x00); // Set DAC 0 //
      break;
    case 7:
      SPI.transfer(0x08); //  Set DAC 1 //
      break;
    case 8:
      SPI.transfer(0x00); //  Set DAC 0 //
      break;
    case 9:
      SPI.transfer(0x08); //  Set DAC 1 //
      break;
    case 10:
      SPI.transfer(0x00); // Set DAC 0 //
      break;
    case 11:
      SPI.transfer(0x08); //  Set DAC 1 //
      break;
    case 12:
      SPI.transfer(0x00); //  Set DAC 0 //
      break;
  }

  //SPI.transfer(0x08); //  Set DAC 1 // #2 on the stripe
  SPI.transfer(0x00); //  send a 0x00
  SPI.transfer(voltage); //  Select value
}

void SPIDacInit() {
  //all 6 DAC chips.
  for (int i = 0 ; i< 6; i++) {
    SPIChipSelect(DAC_LIST[i*2+1].addr); //DAC_LIST 1, 3, 5, 7, 9, 11.
     //First DAC0 Vref (0x08) setting to 0x0f.
    SPI.transfer(0x40); //  set 0x08 << 3 ==0x40, 
    SPI.transfer(0x00); //  
    SPI.transfer(0x0f); //  

    //DAC1 Vref (0x18) setting to 0x0f.
    SPI.transfer(0xC0); //  Set 018 << 3 == 0xC0
    SPI.transfer(0x00); // 
    SPI.transfer(0x0f); // 

    //Second Output voltage (0x00) to 0
    SPI.transfer(0x00); //  Set 0x00 << 3 == 0x00
    SPI.transfer(0x00); // 
    SPI.transfer(0x00); //  voltage.

    //DAC1 Output voltage (0x01) to 0  
    SPI.transfer(0x08); //  Set 0x01 << 3 == 0x08
    SPI.transfer(0x00); // 
    SPI.transfer(0x00); //  voltage
    delay(1);
    //SPIChipSelect() would automatically disable all other channel,
    //so, do not need to do the UnselectAll()
  }
}

void setup() {
  //These initialization can only stay here, not out-side!!! it took me 3 hours to figure it out!!!
  //structure: chan# state waveform scale(0~100) csPin#
  DAC_LIST[0]  = (dac_chan_type) {0, 0, 1, 100, 255};
  DAC_LIST[1]  = (dac_chan_type) {1, 0, 1, 100, 9};
  DAC_LIST[2]  = (dac_chan_type) {2, 0, 1, 100, 9};
  DAC_LIST[3]  = (dac_chan_type) {3, 0, 1, 100, 10};
  DAC_LIST[4]  = (dac_chan_type) {4, 0, 1, 100, 10};
  DAC_LIST[5]  = (dac_chan_type) {5, 0, 1, 100, 8};
  DAC_LIST[6]  = (dac_chan_type) {6, 0, 1, 100, 8};
  DAC_LIST[7]  = (dac_chan_type) {7, 0, 1, 100, 7};
  DAC_LIST[8]  = (dac_chan_type) {8, 0, 1, 100, 7};
  DAC_LIST[9]  = (dac_chan_type) {9, 0, 1, 100, 6};
  DAC_LIST[10] = (dac_chan_type) {10, 0, 1, 100, 6};
  DAC_LIST[11] = (dac_chan_type) {11, 0, 1, 100, 5};
  DAC_LIST[12] = (dac_chan_type) {12, 0, 1, 100, 5};
  

  //SPI settings on the
  SPI.begin();
  SPI.beginTransaction(SPI_settings);
  for (int i = 1; i< DAC_NUM_CHAN; i++) {
    pinMode(DAC_LIST[i].csPin, OUTPUT); //set all CS pins as OUTPUT.
    digitalWrite(DAC_LIST[i].csPin,HIGH); //set them high
  }
  
  *((unsigned int *) 0x4004b018) = 0x204;
  *((unsigned int *) 0x4004b014) = 0x204;
  

  *((unsigned int *) 0x4002c000) |= 1; // disable SPI to allow reconfig

  //*((unsigned int *)0x4002c00c) |= 0x0C0030; // set pdt = 3, dt = 3
  *((unsigned int *)0x4002c00c) |= 0x040030; // set pdt = 1, dt = 3

  //*((unsigned int *)0x4002c00c) |= 0x300F00; // set PASC, ASC to max
  *((unsigned int *)0x4002c00c) |= 0x100100; // set PASC=1, ASC =1

  //*((unsigned int *)0x4002c00c) |= 0xc03000; // set PSSCK = 3, cssck = 3
  *((unsigned int *)0x4002c00c) |= 0x401000; // set PSSCK = 1, cssck = 1

  *((unsigned int *)0x4002c000) &= 0xFFFFFFFE; // resume SPI
  
  SERIAL_PORT.begin( 9600 );

  analogWriteResolution( 12 );

  analogWrite(A14, 3000); //2.417 output default setting for Vref.
  // this comes from (2.5/3.3)*4096 = 3103 ->2.5 Vref
  // can be set with commend line walnut> awrite 3000

  SPIDacInit();

  waveGen.begin();
  for ( short i = 0; i <=7 ; i++ ) {
    waveGen.onStep( i, step_callback_spi ); //make all steps call the callback.
  }
  cmd.begin( SERIAL_PORT, cmd_buffer, sizeof( cmd_buffer ) )
    .list( cmdlist )
    .onCommand( cmd_callback )
    .separator( ",; " );

  oneMilli.begin( TIMER_PERIOD )
    .repeat( ATM_COUNTER_OFF )
    .onTimer( waveGen, Atm_step::EVT_STEP )
    .start();
}

void loop() {
  automaton.run();
}
