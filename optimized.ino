// Define the analog input pins
#define AUDIO_IN A0 // Audio input pin
#define FREQ_IN A1 // Frequency adjustment pin
#define QUANT_IN A2 // Frequency quantization pin
#define Q_IN A3 // Q factor adjustment pin

// Define the DAC output pin
#define AUDIO_OUT 25 // Audio output pin

// Define the sampling frequency and resolution
#define FS 44100 // Sampling frequency in Hz
#define RES 10 // Resolution in bits

// Define the filter parameters
#define N 2 // Filter order
#define BW 0.0595 // Bandwidth in octaves (one semitone)
#define PI 3.14159 // Pi constant

// Declare the filter coefficients and states
float b[N+1]; // Feedforward coefficients
float a[N+1]; // Feedback coefficients
float x[N+1]; // Input states
float y[N+1]; // Output states

// Declare the frequency and Q variables
float f; // Center frequency in Hz
float Q; // Q factor

// Declare the quantization variable
int q; // Number of quantization levels

// Helper for float mapping
float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Declare the input and output variables
int in; // Input value
int out; // Output value

void setup() {
  // Set the analog input pins to input mode
  pinMode(AUDIO_IN, INPUT);
  pinMode(FREQ_IN, INPUT);
  pinMode(QUANT_IN, INPUT);
  pinMode(Q_IN, INPUT);

  // Set the DAC output pin to output mode
  pinMode(AUDIO_OUT, OUTPUT);

  // Initialize the filter states to zero
  for (int i = 0; i <= N; i++) {
    x[i] = 0;
    y[i] = 0;
  }
}

void loop() {
  // Read the analog input values
  in = analogRead(AUDIO_IN);
  f = fmap(analogRead(FREQ_IN), 0, 4095, 20, 20000); // Map the frequency from 20 Hz to 20 kHz
  q = fmap(analogRead(QUANT_IN), 0, 4095, 2, 256); // Map the quantization levels from 2 to 256
  Q = fmap(analogRead(Q_IN), 0, 4095, 0.5, 10); // Map the Q factor from 0.5 to 10

  // Quantize the input value
  in = round(in / (1024 / q)) * (1024 / q);

  // Calculate the filter coefficients
  float w0 = 2 * PI * f / FS; // Angular frequency
  float alpha = sin(w0) / (2 * Q); // Alpha parameter
  float norm = 1 / (1 + alpha); // Normalization factor
  b[0] = alpha * norm; // b0 coefficient
  b[1] = 0; // b1 coefficient
  b[2] = -alpha * norm; // b2 coefficient
  a[0] = 1; // a0 coefficient
  a[1] = -2 * cos(w0) * norm; // a1 coefficient
  a[2] = (1 - alpha) * norm; // a2 coefficient

  // Apply the filter
  // y[0] = b[0]*x[0] + b[1]*x[1] + b[2]*x[2] - a[1]*y[1] - a[2]*y[2]
  // a[0] is already 1, so we don't divide
  float current_y = 0;
  for (int i = 0; i <= N; i++) {
    current_y += b[i] * x[i];
  }
  for (int i = 1; i <= N; i++) {
    current_y -= a[i] * y[i];
  }

  // Shift the filter states
  for (int i = N; i > 0; i--) {
    x[i] = x[i-1];
    y[i] = y[i-1];
  }

  // Update the filter states
  x[0] = in - 2048; // Center input signal (12-bit ADC)
  y[0] = current_y;

  // Scale the output value
  out = constrain((int)y[0] + 128, 0, 255);

  // Write the output value to the DAC
  dacWrite(AUDIO_OUT, out);
}
