#include <ADC.h>
#include <SPI.h>
#include <DMAChannel.h>
#include <Wire.h>
#include "kinetis.h"  
#include <AnalogBufferDMA.h>
#define LED_PIN 13

// DOCS for Delay
// https://www.nxp.com/docs/en/supporting-information/Programmable-Delay-Block-Training.pdf
// https://www.nxp.com/docs/en/application-note/AN4822.pdf 
// USES ADC PDB0_CH0DLY0:
// https://forum.pjrc.com/threads/63411-Periodic-Noise-Spikes-When-Using-Teensy-3-5-ADC?highlight=PDB0_CH0DLY0
// https://forum.pjrc.com/threads/61263-Triggering-ADC-with-PDB-in-Teensy-3-6?highlight=PDB0_CH0DLY0


// DOCS for PulseOut
// https://github.com/teebr/DMAServo/blob/master/DMAServo.cpp <-- shows how to route a DMA transfer event to a GPIO pin change
// 


// **WORK IN PROGRESS
// Outputs to DAC and ADC using PDB with different delays so that the 
// DAC-driven system perturbation can settle before the ADC reads

// TEENSY ADC runs at about 187.5Khz sample rate
// DAC Is Much faster, about 300kHz on T3.2
#define DAC_PIN A14

// idegani: Modified/Improved DMA DAC Generation. Initial low-res version here:
// https://forum.pjrc.com/threads/28101-Using-the-DAC-with-DMA-on-Teensy-3-1
DMAChannel dma_dac;

//elapsedMicros elapsed;
bool fast_binary_tx = true;
int read_byte;
int capturing = 0x0;

// DEBUG MODE DMA pin toggles:
//https://forum.pjrc.com/archive/index.php/t-17532.html
// https://www.pjrc.com/teensy/schematic32.png <-- Teensy 3.2 pin decoder schematic
// volatile uint32_t &GPIO_ADDR = GPIOC_PTOR; <-- Don't use this direct method, instead rely on core_pins.h
bool dbg_mode = false;
bool dbg_init = false;
DMAChannel dma_dbg_pdb(false);
DMAChannel dma_dbg_adc_start(false);
DMAChannel dma_dbg_adc_complete(false);

// define the DMA destination port toggle registers
#define DMA_DBG_PDB_PIN             4
#define DMA_DBG_ADC_START_PIN       5
#define DMA_DBG_ADC_COMPLETE_PIN    6
#define FTM_TEST_PIN                7

#define DMA_DBG_PDB_DEST            CORE_PIN4_PORTTOGGLE
#define DMA_DBG_ADC_START_DEST      CORE_PIN5_PORTTOGGLE
#define DMA_DBG_ADC_START_CFG       CORE_PIN5_CONFIG
#define DMA_DBG_ADC_COMPLETE_DEST   CORE_PIN6_PORTTOGGLE
#define FTM_TEST_PIN_CFG            CORE_PIN7_CONFIG


// allocate a single 32-bit write register value for each pin to complete the toggle on the right bit
static volatile uint32_t __attribute__((aligned(32))) pdb_trig_toggle = CORE_PIN4_BITMASK;
static volatile uint32_t __attribute__((aligned(32))) adc_start_toggle = CORE_PIN5_BITMASK;
static volatile uint32_t __attribute__((aligned(32))) adc_complete_toggle = CORE_PIN6_BITMASK;

// Maybe DMAMEM doesn't initialize values at compile time?
/*DMAMEM*/ static volatile uint16_t __attribute__((aligned(32))) walshtable_default[] = {
//  4095,2457,1638,1638,1638,3276,819,819,819,2457,1638,1638,1638,0,819,819,
//  3276,3276,2457,819,2457,2457,0,1638,1638,1638,819,2457,819,819,1638,0
4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 2457, 
2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 1638, 
1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 
1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 
1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 3276, 
3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 819, 
819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 
819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 
819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 2457, 
2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 1638, 
1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 
1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 
1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 819, 
819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 
819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 3276, 
3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 
3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 3276, 2457, 
2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 819, 
819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 2457, 
2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 
2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1638, 
1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 
1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 
1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 819, 
819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 2457, 
2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 2457, 819, 
819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 
819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 819, 1638, 
1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 1638, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static volatile uint16_t __attribute__((aligned(32))) *walshtable = (volatile uint16_t*)malloc(sizeof(walshtable_default));
bool scaled = false;

//DMAMEM static volatile uint16_t __attribute__((aligned(32))) walshtable_scaled[sizeof(walshtable)/sizeof(uint16_t)];

//DMAMEM static volatile uint16_t __attribute__((aligned(32))) walshtable_padded[2*sizeof(walshtable)/sizeof(uint16_t)];


//float walsh_period_usec = 1000; // 1000usec ==  1kHz <-- Very Slow for debugging
//float walsh_period_usec = 100;  // 100usec ==  10kHz <-- Very Slow for debugging
//float walsh_period_usec = 20;   // 1000usec ==  50kHz <-- Slow for debugging
//float walsh_period_usec = 6;    // 6usec == 166.66 kHz (187.5kHz is the ADC continuous limit (MED/MED) on Teensy 3.2)
//float walsh_period_usec = 5.8;  // 6usec == 180.0 kHz (187.5kHz is the ADC continuous limit on Teensy 3.2)
//float walsh_period_usec = 5;      // 5usec == 200 kHz // ADC on T3.2 will not trigger at this level or faster....
//float walsh_period_usec = 4;    // 4usec == 250 kHz
//float walsh_period_usec = 3;    // 3usec == 333.3 kHz
//float walsh_period_usec = 2.5;  // 2.5usec ==  400kHz <-- Nearing the upper limit of continuous ADC read on Teensy 4

//DAC Settling Time starting from clock Pulse: 800ns-1us
//LED Settling Time starting from clock pulse: 2.2-2.8us (including DAC settling time)

//ADC timings (12-bit, 1 average):
//VERY_HIGH_SPEED/VERY_HIGH_SPEED == 1.16us
//HIGH_SPEED/HIGH_SPEED           == 2.28us
//MEDIUM_SPEED/MEDIUM_SPEED       == 4.92us
//ADC timings (12-bit, 2 averages):
//VERY_HIGH_SPEED/VERY_HIGH_SPEED == 3.9us


// THORLABS METAL PCB LED has 40us settling time
// THORLABS Plastic LED has 20us settling time
// SML312BC4TT86 LED has ?? settling time


// one possibility with 5us period / 2.5us delay and High speed
//float walsh_period_usec = 5;
//ADC_CONVERSION_SPEED conversion_speed = ADC_CONVERSION_SPEED::HIGH_SPEED;
//ADC_SAMPLING_SPEED sampling_speed = ADC_SAMPLING_SPEED::HIGH_SPEED;
//float adc_delay_usec = 2.5; 

// another possibility with 5us period / 3.5us delay and very high speed
// SEEMS LIKE THE CLEANEST OPTION - BUT THE PHOTODIODE DOESN'T RESPOND WELL
float walsh_period_usec = 5;
ADC_CONVERSION_SPEED conversion_speed = ADC_CONVERSION_SPEED::VERY_HIGH_SPEED;
ADC_SAMPLING_SPEED sampling_speed = ADC_SAMPLING_SPEED::VERY_HIGH_SPEED;
float adc_delay_usec = 3.5; 

// another possibility with 10us period / 3.5us delay and very high speed

//float walsh_period_usec = 10; // THORLABS LED TEST
//ADC_CONVERSION_SPEED conversion_speed = ADC_CONVERSION_SPEED::VERY_HIGH_SPEED;
//ADC_SAMPLING_SPEED sampling_speed = ADC_SAMPLING_SPEED::VERY_HIGH_SPEED;
//float adc_delay_usec = 3.5; 

//float walsh_period_usec = 20; // THORLABS LED TEST
//ADC_CONVERSION_SPEED conversion_speed = ADC_CONVERSION_SPEED::VERY_HIGH_SPEED;
//ADC_SAMPLING_SPEED sampling_speed = ADC_SAMPLING_SPEED::VERY_HIGH_SPEED;
//float adc_delay_usec = 3.5; 

//float walsh_period_usec = 30; // 
//ADC_CONVERSION_SPEED conversion_speed = ADC_CONVERSION_SPEED::VERY_HIGH_SPEED;
//ADC_SAMPLING_SPEED sampling_speed = ADC_SAMPLING_SPEED::VERY_HIGH_SPEED;
//float adc_delay_usec = 3.5; 

//float walsh_period_usec = 500; // 
//ADC_CONVERSION_SPEED conversion_speed = ADC_CONVERSION_SPEED::HIGH_SPEED;
//ADC_SAMPLING_SPEED sampling_speed = ADC_SAMPLING_SPEED::HIGH_SPEED;
//float adc_delay_usec = 250; 
//


// another possibility with 4us period / 2.5us delay and very High speed
// Higher Sample rate, but stuck with the signal still whipping around
//float walsh_period_usec = 4;
//ADC_CONVERSION_SPEED conversion_speed = ADC_CONVERSION_SPEED::VERY_HIGH_SPEED;
//ADC_SAMPLING_SPEED sampling_speed = ADC_SAMPLING_SPEED::VERY_HIGH_SPEED;
//float adc_delay_usec = 2.5; 


// Multiple input & output objects use the Programmable Delay Block
// to set their sample rate.  They must all configure the same
// period to avoid chaos.

/*
  PDB_SC_TRGSEL(15)        Select software trigger
  PDB_SC_PDBEN             PDB enable
  PDB_SC_PDBIE             Interrupt enable
  PDB_SC_CONT       f       Continuous mode
  PDB_SC_DMAEN             DMA enable
  PDB_SC_PRESCALER(7)      Prescaler = 128
  PDB_SC_MULT(1)           Prescaler multiplication factor = 10
*/

#define PDB_TRG_FTM   PDB_SC_TRGSEL(8)  // PDB triggered by Flextimer
#define PDB_TRG_SFT   PDB_SC_TRGSEL(15) // PDB triggered by Software

#define PDB_CONFIG (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT | PDB_SC_PDBIE | PDB_SC_DMAEN)
#define PDB_CH0C1_BB   0x010000             // PDB Channel Pretrigger Enable Back-to-back mode, useful for 2 successive ADC channel reads
#define PDB_CH0C1_TOS    0x0100             // PDB Channel Pretrigger Output Select = 1 (assert when counter hits delay register + 1)
#define PDB_CH0C1_EN       0x01             // PDB Channel Pretrigger Enable
#define PDB0_PO_ENABLE     0x01             // PDB Channel Pulse-Out Enable
#define PDB0_PO_DISABLE    0x00             // PDB Channel Pulse-Out Disable
#define PDB_PO_DLY1   (ADC_PDB_DELAY<<16)   // Configure Pulse-Out delay 1 (MSB's of register)
#define PDB_PO_DLY2   (ADC_PDB_DELAY+500)   // Configure Pulse-Out delay 2 (LSB's of register)
#define PDB_PO_DLY    PDB_PO_DLY1 | PDB_PO_DLY2

// ADC Stuff
const int readPin_adc_0 = A8; // A0 == pin 14 (not to be confused with DAC's A14)
ADC *adc = new ADC(); // adc object
const uint32_t initial_average_value = 2048;

// how many samples to capture before triggering interrupt and flushing to PC
// for simplicity and to avoid sending partially filled buffers, this should be a power of 2
const uint32_t buffer_size = 512; 

// logical size of the transform (which equals the sequency/waveform length)
// for simplicity and to avoid padding the sequence, this should be a power of 2
uint32_t tx_buffer_size = sizeof(walshtable_default)/sizeof(uint16_t);

// if a logical tx_buffer is composed of multiple ADC buffers, this tracks the 
// chunk number and total number of chunks
uint32_t buffer_chunk_idx = 0;
uint32_t buffer_num_chunks = tx_buffer_size / buffer_size;
bool use_default_walshtable = true;

DMAMEM static volatile uint16_t __attribute__((aligned(32))) dma_adc_buff1[buffer_size];
DMAMEM static volatile uint16_t __attribute__((aligned(32))) dma_adc_buff2[buffer_size];
AnalogBufferDMA abdma1(dma_adc_buff1, buffer_size, dma_adc_buff2, buffer_size);

typedef struct SerialHeader {
  unsigned long timestamp;
  uint32_t item_count;
  uint32_t tx_buffer_size;
  uint32_t buffer_chunk_idx;
  uint32_t buffer_num_chunks;
  
} SerialHeader;

//static SerialHeader header;


void setup() {
  while (!Serial && millis() < 5000) ;
  //Serial.begin(115200); 
  Serial.begin(6000000); 
  Serial.setTimeout(5000);
  //dma.begin(true); // allocate the DMA channel

  // DAC PIN flutters around otherwise and the op amp buffers may charge the LED
  analogWrite(DAC_PIN, 0); // 
}


void start_capture(){
  capturing = 0x1;
  
  buffer_chunk_idx = 0;
  buffer_num_chunks = tx_buffer_size / buffer_size;
    
  // ADC needs to be set up before the PDB for some reason...
  setup_adc();
  // start the DAC
  setup_dac();
  // start the pdb peripheral to time the DAC/ADC
  start_pdb();


  if(dbg_mode){
    setup_dbg_dma();  
    //setup_ftm_dbg();
  }
}

void stop_capture(){
  capturing = 0x0;
  
  // disable the DAC and ADC
  disable_dac();
  disable_adc();
  // stop the PDB timer peripheral
  stop_pdb();

  if(dbg_mode){
    stop_dbg_dma();  
    //disable_ftm_dbg();
  }

}

// Concept from:
// https://github.com/teebr/DMAServo/blob/master/DMAServo.cpp
void setup_dbg_dma(){
  if(!dbg_init){
    // allocate debug channels
    dma_dbg_pdb.begin(true);
    //dma_dbg_adc_start.begin(true); // see AN4822
    dma_dbg_adc_complete.begin(true);

    pinMode(DMA_DBG_PDB_PIN,OUTPUT);
    pinMode(DMA_DBG_ADC_START_PIN,OUTPUT);
    pinMode(DMA_DBG_ADC_COMPLETE_PIN,OUTPUT);
    digitalWriteFast(DMA_DBG_PDB_PIN, LOW);
    digitalWriteFast(DMA_DBG_ADC_START_PIN, LOW);
    digitalWriteFast(DMA_DBG_ADC_COMPLETE_PIN, LOW);
    
    // trigger DMA toggle on a pin (4) when the pdb triggers
    // NOTE: could also trigger at transfers of dma_dac, i.e.
    //dma_dbg_pdb.triggerAtTransfersOf(dma_dac);
    //dma_dbg_pdb.triggerAtCompletionOf(dma_dac); // <-- need both, otherwise we skip an element?
  
    
    dma_dbg_pdb.sourceBuffer(&pdb_trig_toggle, sizeof(uint32_t));
    dma_dbg_pdb.destination(DMA_DBG_PDB_DEST);
    dma_dbg_pdb.triggerAtHardwareEvent(DMAMUX_SOURCE_PDB);
  
    // trigger DMA toggle on a pin (5) when the adc pre-triggers
    // need to route this via a flextimer, see AN4822 (TODO)
    //dma_dbg_adc_start.sourceBuffer(&adc_start_toggle, sizeof(uint32_t));
    //dma_dbg_adc_start.destination(DMA_DBG_ADC_START_DEST);
    //dma_dbg_adc_start.triggerAtHardwareEvent(FINDADC); // how to get pre-trigger of ADC0?
  
    // trigger DMA toggle on a pin (6) when the adc completes
    dma_dbg_adc_complete.sourceBuffer(&adc_complete_toggle, sizeof(uint32_t));
    dma_dbg_adc_complete.destination(DMA_DBG_ADC_COMPLETE_DEST);
    dma_dbg_adc_complete.triggerAtHardwareEvent(DMAMUX_SOURCE_ADC0); // trigger at ADC0 completed event -
    dbg_init = true;
  }

  dma_dbg_pdb.enable();
  //dma_dbg_adc_start.enable();
  dma_dbg_adc_complete.enable();
}

void stop_dbg_dma(){
  dma_dbg_pdb.disable();
  //dma_dbg_adc_start.disable();
  dma_dbg_adc_complete.disable();
  digitalWriteFast(DMA_DBG_PDB_PIN, LOW);
  digitalWriteFast(DMA_DBG_ADC_START_PIN, LOW);
  digitalWriteFast(DMA_DBG_ADC_COMPLETE_PIN, LOW);
}

void setup_ftm_dbg() {
    digitalWriteFast(DMA_DBG_ADC_START_PIN, LOW);
    // configure pin 5 for FTM PWM output (FTM0 channel 0)
    DMA_DBG_ADC_START_CFG = PORT_PCR_MUX(4) | PORT_PCR_DSE | PORT_PCR_SRE;

    //FTM_TEST_PIN
    
    FTM0_SC = 0; // disable FTM0
    FTM0_CNT = 0;
    FTM0_MOD = (uint32_t)((F_BUS / 1000000) * walsh_period_usec - 1); // SAME  AS PDB
    FTM0_CNTIN = 0;
    // channel 7 triggers DMA
//    FTM0_C0SC = FTM_CSC_CHIE | FTM_CSC_DMA | FTM_CSC_MSA | FTM_CSC_ELSA; // ch0 output compare, enable DMA trigger
    FTM0_C7SC = FTM_CSC_CHIE | FTM_CSC_DMA | FTM_CSC_MSA | FTM_CSC_ELSA; // ch0 output compare, enable DMA trigger
    // FTM0_C7V = (uint32_t)(PDB0_MOD_CYCLES * 0.5); // same as PDB0_CH1DLY0
    FTM0_C7V = 0; // set to zero for testing, needs ultimately to be same as PDB delay
//    FTM0_C0V = 0; // set to zero for testing, needs ultimately to be same as PDB delay
    FTM0_SC = FTM_SC_CLKS(1);// | FTM_SC_TOIE; // enable timer with busclock and interrupts -- same as  PDB....
  }

void disable_ftm_dbg(){
  //dma_dbg_adc_start.disable();
  //DMA_DBG_ADC_START_CFG = 0;
  FTM0_C7SC = 0; // disable channel 7;
  FTM0_SC = 0; // disable FTM0
}



void setup_adc(){
  //Serial.println("Setup ADC_0");

  adc->adc0->setAveraging(1); // set number of averages
  adc->adc0->setResolution(12); // set bits of resolution
  //adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED);
  //adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::HIGH_SPEED);


  // ADC timings (12-bit, 1 average)
  // VERY_HIGH_SPEED/VERY_HIGH_SPEED == 1.16us
  // HIGH_SPEED/HIGH_SPEED           == 2.28us
  // MEDIUM_SPEED/MEDIUM_SPEED       == 4.92us

  // ADC timings (12-bit, 2 averages)
  // VERY_HIGH_SPEED/VERY_HIGH_SPEED == 3.9us

  
  adc->adc0->setConversionSpeed(conversion_speed);
  adc->adc0->setSamplingSpeed(sampling_speed);
  adc->adc0->analogRead(readPin_adc_0); // performs various ADC setup stuff
  
  //Serial.printf("DMA Buffer Size: %d\n", buffer_size);
  abdma1.init(adc, ADC_0);
  abdma1.userData(initial_average_value); // save away initial starting average
  
  //trigger ADC with hardware, i.e. the PDB, and enable DMA
  ADC0_SC2 |= ADC_SC2_ADTRG | ADC_SC2_DMAEN;
  // or we could do it this way, using ADC library:
  //adc->adc0->setHardwareTrigger(); // this sets the register atomic::setBitFlag(adc_regs.SC2, ADC_SC2_ADTRG);
  //adc->adc0->enableDMA(); 
  
  adc->adc0->startSingleRead(readPin_adc_0); // call this to setup everything before the pdb starts, differential is also possible
}

void disable_adc(){  
  //Serial.println("Stopping ADC");
  //adc->adc0->stopPDB(); // don't do this as the DAC is also tied to PDB
  adc->adc0->setSoftwareTrigger(); // turn off hardware trigger
  adc->adc0->disableDMA(); // turn off hardware trigger
}

void start_pdb(){
  if (!(SIM_SCGC6 & SIM_SCGC6_PDB)){
      SIM_SCGC6 |= SIM_SCGC6_PDB; // enable PDB clock
      PDB0_IDLY = 0; // interrupt delay register
    
      // PDB_PERIOD has to be calculated depending on 
      // (1) desired output frequency
      // (2) selected cpu clock frequency 
      // For example: frequency = 1kHz and the LUT has 4096 entries 
      // (1)  4096 x 1000 = 4096000 triggers per second. 
      // (2) Teensy 3.2 at 96MHz = internal bus clock to 48MHz. 
      // Divide these 48MHz by 4096000 and you get 11.718 as the divider. 
      // Subtract 1 because the PDB includes also a full cycle for a 0 value
      //PDB0_MOD = 47-1; // modulus register, sets period to 1Khz
      //PDB0_MOD = 32768-1; // modulus register, sets period to 1hz (for debugging)
      //PDB0_MOD = 47-1; // modulus register, sets period to 1Khz
      //PDB0_MOD =128; // modulus register, sets pulse to about 3us (300kHz or so)
      // set modulus based on the period of each pulse in microseconds
      // this way we are indpendent of the bus speed
      uint32_t PDB0_MOD_CYCLES = (uint32_t)((F_BUS / 1000000) * walsh_period_usec - 1);
      PDB0_MOD = PDB0_MOD_CYCLES;
      
      // reset the counter value to zero to synchronize with other timers (i.e. FTM)
//      PDB0_CNT = 0; doesn't work, read-only register
      
      // set the ADC pre-trigger to ADC_PDB_DELAY microseconds after the PDB event
      // this will allow the DAC to settle to a new value
      PDB0_CH0DLY0 = (uint32_t)((F_BUS / 1000000) * adc_delay_usec - 1);

      // or as a percentage of PDB0_MOD_CYCLES
      //(uint32_t)(PDB0_MOD_CYCLES * 0.5); // start ADC 1/3 of the way through the signal change
      
      //PDB0_CH0DLY0 = (uint32_t)((F_BUS / 1000000) * ADC_PDB_DELAY_USEC - 1);
      
      PDB0_SC = PDB_CONFIG | PDB_SC_LDOK; // load registers from buffers
      PDB0_SC = PDB_CONFIG | PDB_SC_SWTRIG; // reset and restart
      
      //PDB0_CH0C1 = 0x0101; // channel n control register?
      //PDB0_CH0C1 = PDB_C1_EN (0x03) | PDB_C1_TOS(0x03) | PDB_C1_BB (0x02);
      PDB0_CH0C1 = PDB_CH0C1_TOS | PDB_CH0C1_EN; // enable pretrigger 0 (SC1A)
      
      // Pulse-out doesn't work according to the forums...
      // https://forum.pjrc.com/threads/45083-Generate-ESC-DSHOT-pulses-using-PDB-PulseOut-to-an-output-pin?p=147089&viewfull=1#post147089
      // may want to fiddle with this anyway just in case ...
      //PDB0_POEN = PDB0_PO_ENABLE;
      //PDB0_PO0DLY = PDB_PO_DLY
  }
}

void stop_pdb(){
    // erase the status control register
    PDB0_SC = 0;
    // turn off the PDB clock
    SIM_SCGC6 = SIM_SCGC6 & ~(SIM_SCGC6_PDB);
}

void setup_dac(){
  SIM_SCGC2 |= SIM_SCGC2_DAC0; // enable DAC clock
  DAC0_C0 = DAC_C0_DACEN | DAC_C0_DACRFS; // enable the DAC module, 3.3V reference
  
  // slowly ramp up to DC voltage, approx 1/4 second
  /*for (int16_t i=0; i<2048; i+=8) {
    *(int16_t *)&(DAC0_DAT0L) = i;
    delay(1);
  }*/

  // The DAC tends to overshoot past 3.3V on upswings, so scale the value to 0.75
  float scale =0.75; //0.25; // 0.6
  //int s_len = sizeof(walshtable_default)/sizeof(uint16_t);
  //int s_len = tx_buffer_size;
  
//  int w_len = sizeof(walshtable_padded)/sizeof(uint16_t);
//  for(int i = 0; i< w_len; i++){
//    if(i < w_len /2){
//        walshtable_padded[i] = (uint16_t)(scale*(float)walshtable[i]);
//    } else {
//      walshtable_padded[i] = 0;
//    }
//  }

  // scale only
  if(use_default_walshtable){
    for(uint32_t i = 0; i< tx_buffer_size; i++){
        walshtable[i] = (uint16_t)(scale*(float) walshtable_default[i]);
    }
    scaled = true;  
  }
  
  if(!scaled){
    for(uint32_t i = 0; i< tx_buffer_size; i++){
        walshtable[i] = (uint16_t)(scale*(float)walshtable[i]);
    }
    scaled = true;  
  }
  

  dma_dac.sourceBuffer(walshtable, tx_buffer_size*sizeof(uint16_t));
  //dma_dac.sourceBuffer(walshtable_padded, sizeof(walshtable_padded));
  dma_dac.destination(*(volatile uint16_t *)&(DAC0_DAT0L));
  dma_dac.triggerAtHardwareEvent(DMAMUX_SOURCE_PDB);
  dma_dac.enable();
}

void disable_dac(){
  dma_dac.disable();
  //(int16_t *)&(DAC0_DAT0L) = 0;  
  analogWrite(A14, 0);
}


static unsigned magic_start = 0xDABBAD00;
static unsigned chunk_start = 0xFEEDC0DE;

void ProcessAnalogADC(AnalogBufferDMA *pabdma, int8_t adc_num) {
  volatile uint16_t *pbuffer = pabdma->bufferLastISRFilled();
  volatile uint16_t *end_pbuffer = pbuffer + pabdma->bufferCountLastISRFilled();
  volatile uint32_t item_count = (uint32_t)pabdma->bufferCountLastISRFilled();
  unsigned long current_time = micros();
  uint16_t min_val = 0xffff;
  uint16_t max_val = 0;
  //unsigned long currentTime = millis();

  // https://forum.pjrc.com/threads/58365-Teensy-4-0-UART-RX-to-DMA
  // "if the return buffer is in high memory, then delete the cache"
  if ((uint32_t)pbuffer >= 0x20200000u)  arm_dcache_delete((void*)pbuffer, sizeof(dma_adc_buff1));

  if (fast_binary_tx){

    // TODO: change this to writing the header struct
    if(buffer_chunk_idx == 0){
      Serial.write((byte*)&magic_start, sizeof(magic_start));
    } else {
      Serial.write((byte*)&chunk_start, sizeof(chunk_start));
    }

    
    //Serial.printf("Time %d in bytes: |0x%.8x|\n", current_time,current_time);
    //Serial.printf("Size %d in bytes: |0x%.8x|\n", (uint32_t)item_count,(uint32_t)item_count);
    Serial.write((byte*)&current_time, sizeof(current_time));
    
    // number of logical samples in this chunk (for 16-bit, each sample is 2 bytes)
    Serial.write((byte*)&item_count, sizeof(item_count));
    // total number of items in the waveform
    Serial.write((byte*)&tx_buffer_size, sizeof(tx_buffer_size));
    // chunk index ( for multi-chunk waveforms)
    Serial.write((byte*)&buffer_chunk_idx, sizeof(buffer_chunk_idx));
    // number of chunks ( for multi-chunk waveforms)
    Serial.write((byte*)&buffer_num_chunks, sizeof(buffer_num_chunks));
    
    //Serial.printf("DEBUG: Writing %d bytes\n", pabdma->bufferCountLastISRFilled()*sizeof(uint16_t));
    Serial.write((byte*)pbuffer, item_count*sizeof(uint16_t));
    //Serial.println("DEBUG: End Writing");
    // newline character signifies a buffer chunk stop byte - client should explicitly look for this
    Serial.write((char)0);

    buffer_chunk_idx = (buffer_chunk_idx + 1)%buffer_num_chunks;
  }
  else {
    Serial.printf("Time %d\n", current_time);
    while (pbuffer < end_pbuffer) {
      Serial.printf("%d ", *pbuffer);
      if (*pbuffer < min_val) min_val = *pbuffer;
      if (*pbuffer > max_val) max_val = *pbuffer;
      pbuffer++;
    }
    Serial.println();
    Serial.printf("Min: %d | Max: %d\n", min_val,max_val);
  }
  pabdma->clearInterrupt();
}

void handle_fast_waveform(){
  // read the data and assume it is the same size as our existing buffer...
  Serial.readBytes((char *)walshtable, tx_buffer_size*sizeof(uint16_t));
  // no acknowledge message, in order to avoid interrupting the flow of data
}

void handle_new_waveform(){
   // PC UI is setting a different waveform
   if(capturing == 0x1){
    Serial.printf("Error - should not be changing waveform while a capture is in progress");
   }

   uint32_t new_tx_buffer_size;
   Serial.readBytes((char *)&new_tx_buffer_size, 4); 
   if (tx_buffer_size != new_tx_buffer_size){
     Serial.printf("New TX Buffer Size: %d\n", new_tx_buffer_size);
     // realloc the buffer
     Serial.printf("Reallocation: %d tx_buffer to %d bytes\n", tx_buffer_size*sizeof(uint16_t), new_tx_buffer_size*sizeof(uint16_t));
     walshtable = (volatile uint16_t *)realloc((void*)walshtable, new_tx_buffer_size*sizeof(uint16_t));
     tx_buffer_size = new_tx_buffer_size;
   }

  // read the new sequence into our buffer
  Serial.readBytes((char *)walshtable, new_tx_buffer_size*sizeof(uint16_t));
  Serial.printf("Received New Buffer, Size: %d\n", tx_buffer_size);
  
  /*for(int i =0; i < 8; i++){
    Serial.printf("%d = %d, ", i, walshtable[i]);
  }*/
  
  // make sure we get the protocol stop bit
  char stop_byte = Serial.read();
  if(stop_byte != '\n'){
    Serial.printf("Error: did not get expected stop byte: %c\n", stop_byte);
  }

  use_default_walshtable = false;
  scaled = false; 

  //  start_capture();
}

void loop() {
    if (abdma1.interrupted()) {
        ProcessAnalogADC(&abdma1, 0);
    }
    if (Serial.available() > 0){
      read_byte = Serial.read();
      if(read_byte == '0'){
       stop_capture();
      } else if (read_byte == '1'){
        start_capture();
      } else if (read_byte == 'w'){
        handle_new_waveform();
      } else if (read_byte == 'x'){
        handle_fast_waveform();
      }else if (read_byte == 'd'){
        dbg_mode = !dbg_mode;
        if(dbg_mode){
          Serial.printf("Debug mode enabled - connected to pins 4,5,6 to observe timing pulses\n");
          Serial.printf("Sample rate MOD is %d cycles\n",(uint32_t)((F_BUS / 1000000) * walsh_period_usec - 1));
        } else{
          Serial.println("Debug mode disabled");
        }      
      } else if (read_byte == 'b' && !capturing){
        fast_binary_tx = true;
        Serial.printf("Switched to fast binary mode transfer\n");
        //Serial.flush();
      } else if (read_byte == 't' && !capturing){
        //adc->adc0->stopContinuous();
        //delay(1);
        // NOTE: SLOW TEXT MODE WILL DROP MANY SAMPLES 
        // (string conversion not fast enough - only use for debugging)
        // the ADC interrupt will not trigger 4 out of 5 times
        // leading to a loss of most of the data
        fast_binary_tx = false;
        Serial.printf("WARNING: Switched to slow text mode transfer. Samples will be dropped @fs > 30kHz\n");
        //Serial.flush();
        //adc->adc0->startContinuous(readPin_adc_0);
      } 
    } 
}
