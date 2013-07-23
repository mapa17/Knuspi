volatile bool sendMsg = false;
volatile uint8_t cnt = 0;
volatile uint8_t sendBufferSize = 3;
volatile uint8_t nEcho = 5;

volatile uint8_t new_adc_sample=0;
const uint8_t log2_n_samples = 4;
const uint8_t max_n_samples = 0x01 << 4; //Fuse 16 sample values
volatile uint8_t n_samples=0;
volatile uint16_t accum=0;
volatile uint8_t adc_sample = 0;

// Standard arduino setup function ---------------------------------------------
void setup() {
    Serial.begin(115200);
    setup_timer1();
    setup_adc();
}

// Interrupt service routine for timer1 overflow -------------------------------
ISR(TIMER1_OVF_vect)
{
  sendMsg = true;
  cnt ++;
}

void setup_timer1() {
    cli();
    TCCR1A = 0;
    TCCR1B = 0; // normal mode

    TCCR1B |= _BV( CS11 ) | _BV( CS10 ); // clock prescaler 64

    TIMSK1 = _BV(TOIE1); // enable interrupt on timer1
    sei();
}

ISR(ADC_vect)
{
    accum += ADCH;
    n_samples++;
   
    if (n_samples >= max_n_samples) {
        adc_sample = 0x0FF & (accum >> log2_n_samples); // Clamp sample to [0, 255]

        accum = 0;
        n_samples = 0;

        new_adc_sample=1;
    }
}

// Setup analog sampling -------------------------------------------------------
void setup_adc() {
    cli();

    ADMUX = 0; // Use ADC0.
    ADMUX |= (1 << REFS0); // Use AVcc as the reference.
    ADMUX |= (1 << ADLAR); // Set right adjust -> reading ADCH after
                              // convertion will read the higher eight
                              // bits only ( i.e dividing the result by 4 ).

    ADCSRA |= (1 << ADATE); // Set free running mode
    ADCSRA |= (1 << ADEN); // Enable the ADC
    ADCSRA |= (1 << ADIE); // Enable Interrupts

    ADCSRA |= (1 << ADSC); // Start the ADC conversion

    sei();
}

// Standard Arduino loop function. This gets repeatedly called at high rate ----
void loop()
{
    static char d;
    if ( Serial.available() >= 2 )
    {
        nEcho = Serial.read();
        sendBufferSize = Serial.read();

        for (uint8_t i=0; i < nEcho; i++) {
          Serial.write('@'+i);
        }                   
    }
    
    if( sendMsg )
    {
      sendMsg = false;
      //Serial.println(cnt); 
    }
    
    if( new_adc_sample )
    {
      new_adc_sample = 0;
      
      for (uint8_t i=0; i < sendBufferSize; i++) {
        Serial.write('0'+i);
      }           
      //Serial.write( buf, 7 );
    }
}

