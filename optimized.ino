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
float glob_b[N+1]; // Feedforward coefficients
float glob_a[N+1]; // Feedback coefficients
float glob_x[N+1]; // Input states
float glob_y[N+1]; // Output states

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
    glob_x[i] = 0;
    glob_y[i] = 0;
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
  float w0 = 2.0f * PI * f / FS; // Angular frequency
  float alpha = sin(w0) / (2.0f * Q); // Alpha parameter
  float cosw = cos(w0);
  float norm = 1.0f / (1.0f + alpha);
  glob_b[0] = alpha * norm; // b0 coefficient
  glob_b[1] = 0; // b1 coefficient
  glob_b[2] = -alpha * norm; // b2 coefficient
  glob_a[0] = 1.0f; // a0 coefficient
  glob_a[1] = -2.0f * cosw * norm; // a1 coefficient
  glob_a[2] = (1.0f - alpha) * norm; // a2 coefficient

  // Apply the filter
  float current_x = (in - 2048) / 2048.0f;
  float current_y = glob_b[0] * current_x + glob_b[1] * glob_x[1] + glob_b[2] * glob_x[2]
                  - glob_a[1] * glob_y[1] - glob_a[2] * glob_y[2];

  // Shift the filter states
  glob_x[2] = glob_x[1];
  glob_x[1] = current_x;
  glob_y[2] = glob_y[1];
  glob_y[1] = current_y;

  // Scale the output value
  out = constrain((int)(current_y * 127) + 128, 0, 255);

  // Write the output value to the DAC
  dacWrite(AUDIO_OUT, out);
}
