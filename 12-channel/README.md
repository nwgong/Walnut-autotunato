# Fist release note

- Here's the first code drop for your 12 Channel board with all CS functionality implemented. 
- Two files are here: one is the main, and please remember copy the header file to it's library location, since I have to change the dac_types structure yet again. 

# Here's a quick setup: 

### Pin Mapping: 

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

- A new field is added to the typedef, which is a CS pin. Please wire accordingly to your 12 Channel CS pins for the DACs. 

i.e. DAC_LIST[11], is consider to be connected to Teensy Pin #5, and using DAC0 output. 
DAC_LIST[12], is consider to be connected to Teensy Pin #5, and using DAC1 output. 

for the channels shared on the same PIN, the first (odd number) uses DAC0, the even number uses DAC1.

### Remarks: 
Please find attached code with the following features:

0. Code cleaned up, a lot of intermediate functions are taken out, code is much nicer to read.

1. 12 Channel enabled and tested. 
Since there's no hardware 12 channel board in my hand yet, I could make sure that all the code will be drop and works.  But, I checked each individual CS line, they are all correctly set. And, at the same time, NO other CS is set as well. 

2. Added in a timer function to explicitly set all channels to 0 volt when not enabled. 
Also, to ensure the maximum safety, a timer of 100mS is fired to pull down any unused output back to 0 Volt to make sure there's no pin left floating.
Since this, if your Teensy still have LED attached to SCK, you would notice the flashing of the LED as well as constant traffic on the SPI/SCK line now. 

3. In the command set, channel 0 is take out to avoid confusion (the DAC_LIST[0] is still reserved for internal DAC though, just not callable.) 
so, 
walnut>set_dac 1~12 is legit from now, attempt to input "set_dac 0" will get an error. 

4. The SetVoltage function is intact from this update to ensure the most consistency of the code behavior. You should NOT experience any change of the Sine Wave generation when you type in a command. 

Hope all this helps, and please let me know when you get a chance to test them out. I hope you can get a dry run sometime during the day Saturday since I am gonna be away early Sunday morning and would not be back till next weekend. My access to computer will be limited. Please fully test them and/or have Jing to test (to avoid hardware damage, you can dis-connect the actuators from the DAC temporarily) 
