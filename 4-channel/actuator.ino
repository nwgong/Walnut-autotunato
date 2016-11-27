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
#define DAC_NUM_CHAN 5
#define TIMER_PERIOD 10 //ms
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
const int chipSelectPin1  =    10;
const int chipSelectPin2  =    9;
//SPI Configuration
SPISettings SPI_settings(1000000, MSBFIRST, SPI_MODE0);

Adafruit_MCP4725 dac[DAC_NUM_CHAN-1];

dac_chan_type DAC_LIST[5];

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


const PROGMEM uint16_t DACLookup_FullSine_6Bit[64] =
{
   0,   10,   39,   88,  156,  242,  345,  465,
   600,  749,  910, 1082, 1264, 1453, 1648, 1847,
  2048, 2248, 2447, 2642, 2831, 3013, 3185, 3346,
  3495, 3630, 3750, 3853, 3939, 4007, 4056, 4085,
  4095, 4085, 4056, 4007, 3939, 3853, 3750, 3630,
  3495, 3346, 3185, 3013, 2831, 2642, 2447, 2248,
  2048, 1847, 1648, 1453, 1264, 1082,  910,  749,
   600,  465,  345,  242,  156,   88,   39,   10
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
//    case CMD_DAC_READ:
//
//    break;
//
//    case CMD_DAC_WRITE:
//
//    break;
//
    case CMD_ERASENV:
      unLockWiper();
      for (int i = 0; i< 5; i++) EraseNVLOCK();
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
      if ( ( atoi( cmd.arg(1) ) >= 0 ) &&
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

void step_callback ( int idx, int v, int up ) {
  for (short j = 0; j < 64; j++ ) {
    for ( short i = 0; i < DAC_NUM_CHAN; i++ ) {
       //dac[i].setVoltage(pgm_read_word(&(DACLookup_FullSine_8Bit[counter % sizeof(DACLookup_FullSine_8Bit)])),false);
       //dac[i].setVoltage(pgm_read_word(&(DACLookup_FullSine_8Bit[j % 256])),false);
       //dac[i].setVoltage( (counter % 40)*100, false);
       //dac[i].setVoltage( (j % 40)*100, false);
       //analogWrite(A14, (j % 40)* 100);
       if ( ( i == 0 ) and ( DAC_LIST[0].state ) ) {
          analogWrite(A14, pgm_read_word(&(DACLookup_FullSine_6Bit[j % 64]))*DAC_LIST[i].scale/ONE_HUNDRED); //this is good, 200 uS for a full sine - 5K Hz.
       }  else if ( DAC_LIST[i].state ) {
          //dac[i-1].setVoltage(pgm_read_word(&(DACLookup_FullSine_6Bit[j % 64]))*DAC_LIST[i].scale/ONE_HUNDRED,false);
          //SPISetVoltage(DAC_LIST[i].addr, pgm_read_word(&(DAC_SPI_Wave_Points[j % 64]))*DAC_LIST[i].scale/ONE_HUNDRED);
          delayMicroseconds(45); //to make the total set time 50uS.
       //analogWrite(A14, abs(sin(float(j) / 256.0 * 2 * PI) * 3.3*1024)); //math is too slow, 10 ms for a full sine wave.
       //delay(2);
       //char buffer[50];
       //sprintf( buffer, "Time elapse: %02d:%02d", counter / 60, counter % 60 );
       //SERIAL_PORT.println( buffer );
       }
    }
  }
}

void setSPIChipSelect(char chan) {
  switch( chan ) {
  case 1:
    digitalWrite(chipSelectPin1, LOW);
    digitalWrite(chipSelectPin2, HIGH);
    break;
  case 2:
    digitalWrite(chipSelectPin1, LOW);
    digitalWrite(chipSelectPin2, HIGH);
    break;
  case 3:
    digitalWrite(chipSelectPin1, HIGH);
    digitalWrite(chipSelectPin2, LOW);
    break;
  case 4:
    digitalWrite(chipSelectPin1, HIGH);
    digitalWrite(chipSelectPin2, LOW);
    break;
  }
}

void step_callback_spi ( int idx, int v, int up ) {
  for ( short i = 0; i < DAC_NUM_CHAN; i++ ) {
     if ( ( i == 0 ) and ( DAC_LIST[0].state ) ) {
        //analogWrite(A14, pgm_read_word(&(DACLookup_FullSine_6Bit[j % 64]))*DAC_LIST[i].scale/ONE_HUNDRED); //this is good, 200 uS for a full sine - 5K Hz.
     }  else if ( DAC_LIST[i].state ) {
        setSPIChipSelect(DAC_LIST[i].addr);
        //SPIDacInit();
        //
        for ( int j = 0; j < 9600; j++) {
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
        digitalWrite(chipSelectPin1, HIGH);
        digitalWrite(chipSelectPin2, HIGH);
     }
  }
}

void SPISetVoltage(char chan, unsigned char voltage) {

  // 16FGV1.0 requires a configuration package to start streaming data
  switch ( chan ) {
    case 1:
      SPI.transfer(0x08); //  Set DAC 1 // #4 and #2 on stripe
      break;
    case 2:
      SPI.transfer(0x00); // Set DAC 0 // #3 and #1 on stripe
      break;
    case 3:
      SPI.transfer(0x08); //  Set DAC 1 // #4 and #2 on stripe
      break;
    case 4:
      SPI.transfer(0x00); //  Set DAC 0 // #4 and #2 on stripe
      break;
  }

  //SPI.transfer(0x08); //  Set DAC 1 // #2 on the stripe
  SPI.transfer(0x00); //  send a 0x00
  SPI.transfer(voltage); //  Select value
}

void unLockWiper() {
   //To reset the WiperTech registers at 0x0b.
   //DAC0 first.
  digitalWrite(chipSelectPin2, HIGH);
  digitalWrite(chipSelectPin1, LOW);
  delay(0.01);
  //this is for wiper, addr 0x0b, cmd 01,
  //0x0b = 0b00001011, 
  //0x0b<< 3 = 0x58, OR cmd 01
  //the result is: 0x59
  SPI.transfer(0x59); //
  delay(0.01);
  digitalWrite(chipSelectPin2, HIGH);
  digitalWrite(chipSelectPin1, HIGH);
}

void EraseNVLOCK() {
  
  //The Volatile LOCKs CL0 and CL1 First.
  digitalWrite(chipSelectPin2, HIGH);
  digitalWrite(chipSelectPin1, LOW);
  delay(0.01);
  //this is for DAC0, addr 0x00, cmd 01, 
  //0x00<< 3 = 0x00, OR cmd 01
  //the result is: 0x02
  SPI.transfer(0x02); //

  digitalWrite(chipSelectPin1, HIGH);
  delay(0.01);
  digitalWrite(chipSelectPin1, HIGH);
  digitalWrite(chipSelectPin1, LOW);
  //this is for DAC1, addr 0x01, cmd 01, 
  //0x01<< 3 = 0x08, OR cmd 010
  //0x0A
  SPI.transfer(0x0A); // 

  delay(0.01);
  digitalWrite(chipSelectPin2, HIGH);
  digitalWrite(chipSelectPin1, HIGH);

  //Now coming to the Non-Volitile part.
  //The Non-Volatile LOCKs DL0 and DL1 First.
  digitalWrite(chipSelectPin2, HIGH);
  digitalWrite(chipSelectPin1, LOW);
  delay(0.01);
  //this is for DAC0, addr 0x10, cmd 01, 
  //0x10<< 3 = 0x80, OR cmd 010
  //the result is: 0x82
  SPI.transfer(0x82); //

  digitalWrite(chipSelectPin1, HIGH);
  delay(0.01);
  digitalWrite(chipSelectPin1, HIGH);
  digitalWrite(chipSelectPin1, LOW);
  //this is for DAC1, addr 0x11, cmd 01, 
  //0x11<< 3 = 0x88, OR cmd 010
  //0x0A
  SPI.transfer(0x8A);

  delay(0.01);
  digitalWrite(chipSelectPin2, HIGH);
  digitalWrite(chipSelectPin1, HIGH);
}

void SPIDacInit() {
  //DAC 1
  digitalWrite(chipSelectPin1, HIGH);
  digitalWrite(chipSelectPin2, LOW);

  //First Vref (0x08) setting to 0x0f.
  SPI.transfer(0x40); //  set addr << 3, 
  SPI.transfer(0x00); //  Select
  SPI.transfer(0x0f); //  Select
 // SPI.transfer(0x0A); //  Select

  SPI.transfer(0xC0); //  Set addr << 3
  SPI.transfer(0x00); //  Select
  SPI.transfer(0x0f); //  Select
 // SPI.transfer(0x0A); //  Select

  SPI.transfer(0x00); //  Set DAC 0 output voltage to 0
  SPI.transfer(0x00); //  Set addr << 3, be 0x00
  SPI.transfer(0x00); //  voltage.

  SPI.transfer(0x08); //  Set DAC 1 output
  SPI.transfer(0x00); //  Set addr << 3, be 0x80
  SPI.transfer(0x00); //  voltage

  delay(1);
  digitalWrite(chipSelectPin2, HIGH);

  digitalWrite(chipSelectPin1, HIGH);
  delay(1);

  //DAC 1
  digitalWrite(chipSelectPin1, LOW);

  SPI.transfer(0x40); //  set addr << 3, so be 0x40.
  SPI.transfer(0x00); //  Select
  SPI.transfer(0x0f); //  Select

  SPI.transfer(0xC0); //  Set addr << 3, so 0xC0.
  SPI.transfer(0x00); //  Select
  SPI.transfer(0x0f); //  Select

  SPI.transfer(0x00); //  Set DAC 0 output voltage to 0
  SPI.transfer(0x00); //  Set addr << 3, might be 0x00
  SPI.transfer(0x00); //  voltage.

  SPI.transfer(0x08); //  Set DAC 1 output
  SPI.transfer(0x00); //  Set addr << 3, might be 0x80
  SPI.transfer(0x00); //  voltage

  delay(1);
  digitalWrite(chipSelectPin1, HIGH);
}
void SPIDacInit_error() {

  digitalWrite(chipSelectPin1, LOW);
  digitalWrite(chipSelectPin2, HIGH);
  SPI.transfer(0x08); //  set addr << 3, so, might have to be 0x40.
  SPI.transfer(0x00); //  Select
  SPI.transfer(0x0f); //  Select

  SPI.transfer(0x18); //  Set addr << 3, so, might be 0xC0.
  SPI.transfer(0x00); //  Select
  SPI.transfer(0x0f); //  Select

  SPI.transfer(0x00); //  Set DAC 0 output voltage to 0
  SPI.transfer(0x00); //  Set addr << 3, might be 0x00
  SPI.transfer(0x00); //  voltage.

  SPI.transfer(0x00); //  Set DAC 1 output
  SPI.transfer(0x10); //  Set addr << 3, might be 0x80
  SPI.transfer(0x00); //  voltage

  digitalWrite(chipSelectPin1, HIGH);

  digitalWrite(chipSelectPin1, HIGH);
  digitalWrite(chipSelectPin2, LOW);

  SPI.transfer(0x00); //  Select Same as the first DAC.
  SPI.transfer(0x08); //  Select
  SPI.transfer(0x0f); //  Select

  SPI.transfer(0x00); //  Select
  SPI.transfer(0x18); //  Select
  SPI.transfer(0x0f); //  Select

  SPI.transfer(0x00); //  Select
  SPI.transfer(0x00); //  Select
  SPI.transfer(0x00); //  Select

  SPI.transfer(0x00); //  Select
  SPI.transfer(0x10); //  Select
  SPI.transfer(0x00); //  Select

  digitalWrite(chipSelectPin2, HIGH);
}

void setup() {
  //These initialization can only stay here, not out-side!!! it took me 3 hours to figure it out!!!
  //structure: addr state waveform scale(0~100)
  DAC_LIST[0] = (dac_chan_type) {0, 0, 1, 100};
  DAC_LIST[1] = (dac_chan_type) {1, 0, 1, 100};
  DAC_LIST[2] = (dac_chan_type) {2, 0, 1, 100};
  DAC_LIST[3] = (dac_chan_type) {3, 0, 1, 100};
  DAC_LIST[4] = (dac_chan_type) {4, 0, 1, 100};

  //SPI settings on the
  SPI.begin();
  SPI.beginTransaction(SPI_settings);
  pinMode(chipSelectPin1, OUTPUT); // CS 1 pin set to OUTPUT
  pinMode(chipSelectPin2, OUTPUT); // CS 2 set to OUTPUT

  digitalWrite(chipSelectPin1, HIGH);
  digitalWrite(chipSelectPin2, HIGH);

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
