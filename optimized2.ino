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

// Define the filter coefficients
// These are calculated using the bilinear transform method
// for a second-order bandpass filter with a center frequency of 1 kHz
// and a bandwidth of one semitone at FS = 44100 Hz
// You can use this tool to calculate different coefficients: [1](https://www.norwegiancreations.com/2016/03/arduino-tutorial-simple-high-pass-band-pass-and-band-stop-filtering/)
#define B0 0.001227 // Feedforward coefficient 0
#define B1 0 // Feedforward coefficient 1
#define B2 -0.001227 // Feedforward coefficient 2
#define A1 -1.995546 // Feedback coefficient 1
#define A2 0.995546 // Feedback coefficient 2

// Declare the filter variables
float x0, x1, x2; // Input samples
float y0, y1, y2; // Output samples
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
  x0 = x1 = x2 = 0;
  y0 = y1 = y2 = 0;
  fc = 1000; // Initial center frequency
  bw = fc / SEMITONE; // Initial bandwidth
  q = fc / bw; // Initial Q factor
  k = tan(PI * fc / FS); // Initial frequency warping constant
  b0 = B0; // Initial feedforward coefficient 0
  b1 = B1; // Initial feedforward coefficient 1
  b2 = B2; // Initial feedforward coefficient 2
  a1 = A1; // Initial feedback coefficient 1
  a2 = A2; // Initial feedback coefficient 2
}

void loop() {
  // Read the analog inputs
  int audio_in = analogRead(AUDIO_IN); // Read the audio input
  int freq_in = analogRead(FREQ_IN); // Read the frequency input
  int quant_in = analogRead(QUANT_IN); // Read the quantization input
  int q_in = analogRead(Q_IN); // Read the Q factor input

  // Map the analog inputs to the desired ranges
  float audio = map(audio_in, 0, 4095, -1, 1); // Map the audio input to [-1, 1]
  float freq = map(freq_in, 0, 4095, 20, 20000); // Map the frequency input to [20, 20000] Hz
  float quant = map(quant_in, 0, 4095, 0, 1); // Map the quantization input to [0, 1]
  float q_factor = map(q_in, 0, 4095, 0.5, 20); // Map the Q factor input to [0.5, 20]

  // Quantize the frequency input if needed
  if (quant > 0.5) {
    // Quantize the frequency to the nearest semitone
    freq = round(log(freq / 440) / log(SEMITONE)) * SEMITONE * 440;
  }

  // Update the filter parameters if the frequency or Q factor has changed
  if (freq != fc || q_factor != q) {
    // Update the center frequency and Q factor
    fc = freq;
    q = q_factor;

    // Update the bandwidth
    bw = fc / q;

    // Update the frequency warping constant
    k = tan(PI * fc / FS);

    // Update the filter coefficients using the bilinear transform method
    b0 = k * k / (k * k + k / q + 1);
    b1 = 2 * b0;
    b2 = b0;
    a1 = 2 * (k * k - 1) / (k * k + k / q + 1);
    a2 = (k * k - k / q + 1) / (k * k + k / q + 1);
  }

  // Apply the filter to the input sample
  x0 = audio; // Store the current input sample
  y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2; // Calculate the current output sample
  x2 = x1; // Shift the input samples
  x1 = x0;
  y2 = y1; // Shift the output samples
  y1 = y0;

  // Map the output sample to the DAC range
  int audio_out = map(y0, -1, 1, 0, 255);

  // Write the output sample to the DAC
  dacWrite(AUDIO_OUT, audio_out);

  // Print the input and output samples to the serial monitor
  Serial.print(audio);
  Serial.print(",");
  Serial.println(y0);
}
