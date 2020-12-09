
#define ACQ_CLK_PIN 13  //Acquisition clock output here for external measurement
#define N_SAMPLES 600



typedef enum {
  TRIG_AUTO = 0,
  TRIG_NORM = 1,
  TRIG_STOP = 2,
} ETrigMode;

typedef enum {
  EDGE_RISING = 0,
  EDGE_FALLING = 1,
  EDGE_BOTH = 2,
} ETrigEdge;

struct {
  uint8_t sync[4] = { 0x00, 0x00, 0xFF, 0xFF };
  uint8_t triggered = 0;
  uint8_t a0[N_SAMPLES];
  uint8_t a1[N_SAMPLES];
} out;

struct {
  uint8_t sync = 0;
  uint8_t trig_mode = AUTO;        // dont use enum here to ensure 8bit
  uint8_t trig_level = 0x8F;
  uint8_t trig_chan = 0;
  uint8_t trig_edge = EDGE_RISING; // dont use enum here to ensure 8bit
  uint8_t spl_div = 0;
  uint8_t sgen_div = 0;
} cfg;


uint8_t adc_read( uint8_t channel )
{
  // Kanal waehlen, ohne andere Bits zu beeinflußen
  ADMUX = (ADMUX & ~(0x1F)) | (channel & 0x1F);
  ADCSRA |= (1<<ADSC);            // trigger "single conversion"
  while (ADCSRA & (1<<ADSC) ) {   // wait ready
  }
  return ADCH;                    // left justified 8 bit result (see ADLAR)
}

void adc_init() {
  ADMUX = (1<<REFS0) | (1<<ADLAR); // ref to AVcc, left-justified output (for 8 bit)
  ADCSRA = (1<<ADPS2) | (1<<ADEN); // Presc=16, conversion rate 76.9kHz, enable
  adc_read(0); // dummy read required after init
}

void sgen_set(uint8_t fdiv) {
  // TODO
}


void send_packet() {  
  Serial.write((char*)&out, sizeof(out));  
}

bool recv_config() {
  if (Serial.available() >= sizeof(cfg)) {
    Serial.readBytes((char*)&cfg, sizeof(cfg));
    if (cfg.sync != 0) {
      // something went wrong: force client to reset by messing up his sync
      Serial.write(0xAA);
      send_packet();
    } else {
      sgen_set(cfg.sgen_div);
      return true;
    }
  }  
  return false;
}

void toggle_acq_clock_output() {
  static uint8_t state = HIGH;  
  digitalWrite(ACQ_CLK_PIN, state = !state);
}

void wait_trigger() {    
  uint8_t lvl = 0x8F;
  uint8_t cur = adc_read(cfg.trig_chan);
  uint8_t last = cur;
  
  while (1) {
    last = cur;
    cur = adc_read(cfg.trig_chan);

    if (recv_config()) {
      // config changed, this took some time - reset to avoid jitter      
      cur = adc_read(cfg.trig_chan);
      continue;
    }

    bool rising = (last < cfg.trig_level) && (cur >= cfg.trig_level);
    bool falling = (last > cfg.trig_level) && (cur <= cfg.trig_level);
    if (rising && (cfg.trig_edge == EDGE_RISING || cfg.trig_edge == EDGE_BOTH))
      break;
    if (falling && (cfg.trig_edge == EDGE_FALLING || cfg.trig_edge == EDGE_BOTH))
      break;
  }
}

void capture_run() {  
  for (uint16_t i=0; i<N_SAMPLES; ++i) {
    // divide actual sampling rate by repeating measurement at position
    uint8_t div_cnt = cfg.spl_div;
    do {
      out.a0[i] = adc_read(0);
      out.a1[i] = adc_read(1);
      toggle_acq_clock_output();
    } while (div_cnt-- > 0);
  }
}

void setup() {
  adc_init();
  pinMode(ACQ_CLK_PIN, OUTPUT);
  Serial.setTimeout(10000);
  Serial.begin(115200);
}

void loop() {
  wait_trigger();
  capture_run();
  send_packet();
}