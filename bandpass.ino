// Define the analog input pins
#define AUDIO_IN A0
#define FREQ_IN A1
#define QUANT_IN A2

// Define the DAC pin
#define AUDIO_OUT 25

// Define the sample rate and the filter order
#define SAMPLE_RATE 10000 // Hz
#define FILTER_ORDER 7

// Define the filter coefficients
// These are calculated using the Bessel filter design from https://github.com/MartinBloedorn/libFilter
// The filter is a band-pass with a center frequency of 1000 Hz and a bandwidth of 177 Hz (one semitone)
// The coefficients are scaled by 2^15 to avoid floating point arithmetic
const int16_t b[FILTER_ORDER + 1] = {0, 0, -64, 0, 448, 0, -1344, 0};
const int16_t a[FILTER_ORDER + 1] = {32768, 0, -131072, 0, 262144, 0, -327680, 0};

// Define the filter state variables
int16_t x[FILTER_ORDER + 1] = {0}; // input samples
int16_t y[FILTER_ORDER + 1] = {0}; // output samples

// Define the frequency range and quantization levels
#define FREQ_MIN 100 // Hz
#define FREQ_MAX 10000 // Hz
#define QUANT_MIN 1 // semitones
#define QUANT_MAX 12 // semitones

// Define the frequency mapping function
// This function maps the analog input value to a frequency value
// The frequency value is quantized to the nearest semitone or the specified quantization level
int16_t mapFreq(int16_t analogValue, int16_t quantValue) {
  // Map the analog value to a linear frequency value between FREQ_MIN and FREQ_MAX
  int16_t freq = map(analogValue, 0, 4095, FREQ_MIN, FREQ_MAX);
  
  // Map the quant value to a quantization level between QUANT_MIN and QUANT_MAX
  int16_t quant = map(quantValue, 0, 4095, QUANT_MIN, QUANT_MAX);
  
  // Calculate the frequency ratio of one semitone
  float ratio = pow(2, 1.0 / 12.0);
  
  // Calculate the base frequency of the lowest semitone in the range
  float base = FREQ_MIN / pow(ratio, QUANT_MAX - 1);
  
  // Calculate the number of semitones from the base frequency to the linear frequency
  float n = log(freq / base) / log(ratio);
  
  // Round the number of semitones to the nearest quantization level
  n = round(n / quant) * quant;
  
  // Calculate the quantized frequency value
  freq = base * pow(ratio, n);
  
  // Return the quantized frequency value
  return freq;
}

// Define the filter update function
// This function updates the filter state and returns the filtered output value
int16_t updateFilter(int16_t input) {
  // Shift the input samples to the right
  for (int i = FILTER_ORDER; i > 0; i--) {
    x[i] = x[i - 1];
  }
  
  // Store the new input sample
  x[0] = input;
  
  // Shift the output samples to the right
  for (int i = FILTER_ORDER; i > 0; i--) {
    y[i] = y[i - 1];
  }
  
  // Calculate the new output sample using the filter coefficients
  // Use long integers to avoid overflow
  int32_t sum = 0;
  for (int i = 0; i <= FILTER_ORDER; i++) {
    sum += (int32_t)b[i] * x[i] - (int32_t)a[i] * y[i];
  }
  
  // Scale the output sample back to 16 bits and store it
  y[0] = (int16_t)(sum >> 15);
  
  // Return the output sample
  return y[0];
}

// Define the timer interrupt handler
// This function is called at the sample rate and performs the audio processing
void IRAM_ATTR onTimer() {
  // Read the analog input values
  int16_t audioIn = analogRead(AUDIO_IN);
  int16_t freqIn = analogRead(FREQ_IN);
  int16_t quantIn = analogRead(QUANT_IN);
  
  // Map the audio input value to a signed 16-bit value
  audioIn = map(audioIn, 0, 4095, -32768, 32767);
  
  // Map the frequency input value to a quantized frequency value
  freqIn = mapFreq(freqIn, quantIn);
  
  // Update the filter coefficients using the frequency value
  // This is done by modulating the filter coefficients with a sine wave of the desired frequency
  // The modulation depth is proportional to the filter bandwidth
  // The modulation phase is shifted by pi/2 for the odd coefficients
  // This method is based on https://github.com/paulh002/band-passfilter-arduino-esp32
  float omega = 2 * PI * freqIn / SAMPLE_RATE; // angular frequency
  float depth = 0.1; // modulation depth
  for (int i = 0; i <= FILTER_ORDER; i++) {
    if (i % 2 == 0) {
      // Even coefficients
      b[i] = (int16_t)((1 + depth * sin(i * omega)) * 32768);
    } else {
      // Odd coefficients
      b[i] = (int16_t)((1 + depth * cos(i * omega)) * 32768);
    }
  }
  
  // Update the filter state and get the filtered output value
  int16_t audioOut = updateFilter(audioIn);
  
  // Map the output value to an unsigned 8-bit value
  audioOut = map(audioOut, -32768, 32767, 0, 255);
  
  // Write the output value to the DAC pin
  dacWrite(AUDIO_OUT, audioOut);
}

void setup() {
  // Set the analog input pins to 12-bit resolution
  analogReadResolution(12);
  
  // Set the DAC pin to 8-bit resolution
  dacWriteResolution(8);
  
  // Initialize the timer
  hw_timer_t * timer = NULL;
  
  // Create the timer
  timer = timerBegin(0, 80, true);
  
  // Attach the timer interrupt handler
  timerAttachInterrupt(timer, &onTimer, true);
  
  // Set the timer interval to match the sample rate
  timerAlarmWrite(timer, 1000000 / SAMPLE_RATE, true);
  
  // Start the timer
  timerAlarmEnable(timer);
}

void loop() {
  // Nothing to do here
}
