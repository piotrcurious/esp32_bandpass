#include <driver/dac.h>

// Define the analog input pins
#define AUDIO_IN 0 // Audio input pin
#define FREQ_IN 1 // Frequency adjustment pin
#define QUANT_IN 2 // Frequency quantization pin
#define Q_IN 3 // Q factor adjustment pin

// Define the DAC output pin
#define AUDIO_OUT 25 // Audio output pin
//#define AUDIO_OUT DAC_CH1 // Audio output pin

// Define the sample rate and the filter parameters
#define SAMPLE_RATE 10000 // Sample rate in Hz
#define SEMITONE 1.0594630943592953 // Ratio of a semitone
#define MIN_FREQ 20 // Minimum frequency in Hz
#define MAX_FREQ 2000 // Maximum frequency in Hz
#define MIN_Q 0.5 // Minimum Q factor
#define MAX_Q 10 // Maximum Q factor

// Define the filter coefficients
float b0, b1, b2, a1, a2; // Filter coefficients
//float filter_x1, filter_x2, filter_y1, filter_y2; // Filter states
float filter_x1 = 0 ;
float filter_x2 = 0 ;
float filter_y1 = 0 ; 
float filter_y2 = 0 ; 

// Define the timer number and the timer interval
#define TIMER_NUM 0 // Timer number (0-3)
#define TIMER_INTERVAL 0.1 // Timer interval in milliseconds


float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Initialize the filter
void initFilter() {
  // Reset the filter states
  filter_x1 = filter_x2 = filter_y1 = filter_y2 = 0;
  // Update the filter coefficients
  updateFilter();
}

// Update the filter coefficients
void updateFilter() {
  // Read the analog input values
  int freqVal = analogRead(FREQ_IN); // Frequency value (0-4095)
  int quantVal = analogRead(QUANT_IN); // Quantization value (0-4095)
  int qVal = analogRead(Q_IN); // Q factor value (0-4095)

  // Map the analog values to the filter parameters
  float freq = fmap(freqVal, 0, 4095, MIN_FREQ, MAX_FREQ); // Frequency in Hz
  float quant = fmap(quantVal, 0, 4095, 0, 12); // Quantization in semitones
  float Q = fmap(qVal, 0, 4095, MIN_Q, MAX_Q); // Q factor

  // Quantize the frequency to the nearest semitone
  freq = round(log(freq / MIN_FREQ) / log(SEMITONE)) * SEMITONE * MIN_FREQ;

  // Calculate the filter coefficients using the bilinear transform
  // Reference: [Arduino Tutorial: Simple High-pass, Band-pass and Band-stop Filtering](https://www.arduino.cc/reference/en/libraries/esp32timerinterrupt/)
  float omega = 2 * PI * freq / SAMPLE_RATE; // Angular frequency
  float alpha = sin(omega) / (2 * Q); // Alpha parameter
  float cosw = cos(omega); // Cosine of omega
  float norm = 1 + alpha; // Normalization factor
  b0 = alpha / norm; // B0 coefficient
  b1 = 0; // B1 coefficient
  b2 = -alpha / norm; // B2 coefficient
  a1 = -2 * cosw / norm; // A1 coefficient
  a2 = (1 - alpha) / norm; // A2 coefficient
}

// Process the audio input and output the filtered signal
void processAudio() {
  // Read the audio input value
  int x0 = analogRead(AUDIO_IN); // Audio input value (0-4095)

  // Apply the filter equation
  // y[n] = b0 * x[n] + b1 * x[n-1] + b2 * x[n-2] - a1 * y[n-1] - a2 * y[n-2]
  float y0 = b0 * x0 + b1 * filter_x1 + b2 * filter_x2 - a1 * filter_y1 - a2 * filter_y2; // Filter output value

  // Update the filter states
  filter_x2 = filter_x1; // Shift x[n-1] to x[n-2]
  filter_x1 = x0; // Shift x[n] to x[n-1]
  filter_y2 = filter_y1; // Shift y[n-1] to y[n-2]
  filter_y1 = y0; // Shift y[n] to y[n-1]

  // Map the filter output value to the DAC range
  int y = fmap(y0, -2048, 2047, 0, 255); // DAC output value (0-255)

  // Write the DAC output value
  dacWrite(AUDIO_OUT, y); // Write to the DAC pin
//  dac_output_voltage(AUDIO_OUT, y);

}

// Timer interrupt handler
void IRAM_ATTR onTimer() {
  // Process the audio input and output the filtered signal
  processAudio();
}
hw_timer_t *My_timer = NULL;

// Setup function
void setup() {
  // Initialize the filter
  initFilter();
  // Initialize the timer
  My_timer = timerBegin(TIMER_NUM, 80, true); // Timer number, prescaler (80MHz / 80 = 1MHz), count up
  timerAttachInterrupt(My_timer, &onTimer, true); // Timer struct, interrupt handler, edge triggered
  timerAlarmWrite(My_timer, TIMER_INTERVAL * 1000, true); // Timer struct, alarm value in microseconds, repeat
  timerAlarmEnable(My_timer); // Enable the timer
}

// Loop function
void loop() {
  // Update the filter coefficients
  updateFilter();
  // Do nothing else
}
