// Define the analog input pins
#define AUDIO_IN A0
#define FREQ_IN A1
#define QUANT_IN A2

// Define the DAC pin
#define AUDIO_OUT 25

// Define the sample rate and the filter order
#define SAMPLE_RATE 10000 // Hz
#define FILTER_ORDER 7

// Define the filter parameters
#define Q_FACTOR 10.0

// Define the filter coefficients (will be updated dynamically)
int32_t b[FILTER_ORDER + 1] = {0};
int32_t a[FILTER_ORDER + 1] = {32768, 0};

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
  float freq = (float)map(analogValue, 0, 4095, FREQ_MIN, FREQ_MAX);
  
  // Map the quant value to a quantization level between QUANT_MIN and QUANT_MAX
  int16_t quant = map(quantValue, 0, 4095, QUANT_MIN, QUANT_MAX);
  
  // Calculate the frequency ratio of one semitone
  float ratio = pow(2.0f, 1.0f / 12.0f);
  
  // Calculate the base frequency of the lowest semitone in the range
  float base = (float)FREQ_MIN / pow(ratio, (float)QUANT_MAX - 1.0f);
  
  // Calculate the number of semitones from the base frequency to the linear frequency
  float n = log(freq / base) / log(ratio);
  
  // Round the number of semitones to the nearest quantization level
  n = round(n / (float)quant) * (float)quant;
  
  // Calculate the quantized frequency value
  float quantizedFreq = base * pow(ratio, n);
  
  // Return the quantized frequency value
  return (int16_t)quantizedFreq;
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
  // Use long long integers to avoid overflow
  int64_t sum = 0;
  for (int i = 0; i <= FILTER_ORDER; i++) {
    sum += (int64_t)b[i] * x[i];
  }
  for (int i = 1; i <= FILTER_ORDER; i++) {
    sum -= (int64_t)a[i] * y[i];
  }
  
  // Scale the output sample back to 16 bits and store it
  // Assuming a[0] is 32768 (2^15)
  y[0] = (int16_t)(sum / a[0]);
  
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
  
  // Calculate the filter coefficients using the bilinear transform (2nd order BP)
  // We'll reuse the coefficients logic for a standard 2nd order filter
  // but for simplicity here, we'll implement it as a 2nd order within the 7th order array
  float omega = 2.0 * PI * freqIn / SAMPLE_RATE;
  float alpha = sin(omega) / (2.0 * Q_FACTOR);
  float cosw = cos(omega);
  float norm = 32768.0 / (1.0 + alpha);

  b[0] = (int32_t)(alpha * norm);
  b[1] = 0;
  b[2] = (int32_t)(-alpha * norm);
  a[0] = 32768;
  a[1] = (int32_t)(-2.0 * cosw * norm);
  a[2] = (int32_t)((1.0 - alpha) * norm);

  // Reset higher order terms for now to ensure stability
  for(int i=3; i<=FILTER_ORDER; ++i) { b[i] = 0; a[i] = 0; }
  
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
