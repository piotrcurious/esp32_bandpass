// Define the analog input pins
#define AUDIO_IN A0 // Audio input pin
#define FREQ_IN A1 // Frequency input pin
#define QUANT_IN A2 // Frequency quantization input pin
#define Q_IN A3 // Q factor input pin

// Define the DAC output pin
#define AUDIO_OUT 25 // Audio output pin

// Define the sampling frequency
#define FS 44100 // Sampling frequency in Hz

// Define the semitone ratio
#define SEMITONE 1.0594630943592953 // 12th root of 2

// Helper for float mapping
float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Define the filter coefficients
// These are calculated using the bilinear transform method
// for a second-order bandpass filter with a center frequency of 1 kHz
// and a bandwidth of one semitone at FS = 44100 Hz
// You can use this tool to calculate different coefficients: [1](https://www.norwegiancreations.com/2016/03/arduino-tutorial-simple-high-pass-band-pass-and-band-stop-filtering/)
#define B_COEFF0 0.001227 // Feedforward coefficient 0
#define B_COEFF1 0 // Feedforward coefficient 1
#define B_COEFF2 -0.001227 // Feedforward coefficient 2
#define A_COEFF1 -1.995546 // Feedback coefficient 1
#define A_COEFF2 0.995546 // Feedback coefficient 2

// Declare the filter variables
float filter_x0, filter_x1, filter_x2; // Input samples
float filter_y0, filter_y1, filter_y2; // Output samples
float fc; // Center frequency
float bw; // Bandwidth
float q; // Q factor
float k; // Frequency warping constant
float b0, b1, b2; // Feedforward coefficients
float a1, a2; // Feedback coefficients

void setup() {
  // Initialize the serial monitor
  Serial.begin(115200);

  // Initialize the DAC output
  dacWrite(AUDIO_OUT, 0);

  // Initialize the filter variables
  filter_x0 = filter_x1 = filter_x2 = 0;
  filter_y0 = filter_y1 = filter_y2 = 0;
  fc = -1.0f;
  q = -1.0f;
}

void loop() {
  // Read the analog inputs
  int audio_in = analogRead(AUDIO_IN); // Read the audio input
  int freq_in = analogRead(FREQ_IN); // Read the frequency input
  int quant_in = analogRead(QUANT_IN); // Read the quantization input
  int q_in = analogRead(Q_IN); // Read the Q factor input

  // Map the analog inputs to the desired ranges
  float audio = (audio_in - 2048) / 2048.0f; // Map the audio input to [-1, 1]
  float freq = fmap(freq_in, 0, 4095, 20, 20000); // Map the frequency input to [20, 20000] Hz
  float quant = fmap(quant_in, 0, 4095, 0, 1); // Map the quantization input to [0, 1]
  float q_factor = fmap(q_in, 0, 4095, 0.5, 20); // Map the Q factor input to [0.5, 20]

  // Quantize the frequency input if needed
  if (quant > 0.5) {
    // Quantize the frequency to the nearest semitone (base A4 = 440Hz)
    freq = pow(SEMITONE, round(log(freq / 440.0f) / log(SEMITONE))) * 440.0f;
  }

  // Update the filter parameters if the frequency or Q factor has changed
  if (std::abs(freq - fc) > 0.1 || std::abs(q_factor - q) > 0.01) {
    // Update the center frequency and Q factor
    fc = freq;
    q = q_factor;

    // Standard Bandpass Filter coefficients (Constant Peak Gain)
    // Reference: https://cycling74.com/forums/rbj-audio-eq-cookbook
    float omega = 2.0 * PI * fc / FS;
    // For bandpass, alpha can be defined by Q or bandwidth.
    // Let's use the standard Q-based alpha.
    float alpha = sin(omega) / (2.0 * q);
    float cosw = cos(omega);
    float norm = 1.0 / (1.0 + alpha);
    b0 = alpha * norm;
    b1 = 0;
    b2 = -alpha * norm;
    a1 = -2.0f * cosw * norm;
    a2 = (1.0f - alpha) * norm;
  }

  // Apply the filter to the input sample
  filter_x0 = audio; // Store the current input sample
  filter_y0 = b0 * filter_x0 + b1 * filter_x1 + b2 * filter_x2 - a1 * filter_y1 - a2 * filter_y2; // Calculate the current output sample

  // Update the filter states
  filter_x2 = filter_x1; // Shift the input samples
  filter_x1 = filter_x0;
  filter_y2 = filter_y1; // Shift the output samples
  filter_y1 = filter_y0;

  // Map the output sample to the DAC range
  int audio_out = (int)constrain(filter_y0 * 127.0f + 128.0f, 0, 255);

  // Write the output sample to the DAC
  dacWrite(AUDIO_OUT, audio_out);

  // Print the input and output samples to the serial monitor
  Serial.print(audio);
  Serial.print(",");
  Serial.println(filter_y0);
}
