// Define the analog input pins
#define AUDIO_IN A0 // Audio input pin
#define FREQ_IN A1 // Frequency input pin
#define QUANT_IN A2 // Quantization input pin
#define Q_IN A3 // Q input pin

// Define the DAC output pin
#define AUDIO_OUT 25 // Audio output pin

// Define some constants
#define SAMPLE_RATE 44100 // Sampling rate in Hz
#define SEMITONE 1.0594630943592953 // Ratio of a semitone
#define MAX_FREQ 20000 // Maximum frequency in Hz
#define MIN_FREQ 20 // Minimum frequency in Hz
#define MAX_Q 10 // Maximum Q factor
#define MIN_Q 0.1 // Minimum Q factor

// Declare some global variables
float centerFreq; // Center frequency of the bandpass filter
float bandwidth; // Bandwidth of the bandpass filter
float Q; // Q factor of the bandpass filter
float b0, b1, b2, a1, a2; // Filter coefficients
float x1, x2, y1, y2; // Filter states

void setup() {
  // Initialize the serial monitor
  Serial.begin(115200);

  // Initialize the DAC output
  dacWrite(AUDIO_OUT, 0);

  // Initialize the filter variables
  centerFreq = 0;
  bandwidth = 0;
  Q = 0;
  b0 = b1 = b2 = a1 = a2 = 0;
  x1 = x2 = y1 = y2 = 0;
}

void loop() {
  // Read the analog inputs
  int audioIn = analogRead(AUDIO_IN); // Audio input value (0-4095)
  int freqIn = analogRead(FREQ_IN); // Frequency input value (0-4095)
  int quantIn = analogRead(QUANT_IN); // Quantization input value (0-4095)
  int qIn = analogRead(Q_IN); // Q input value (0-4095)

  // Map the analog inputs to the desired ranges
  float audio = map(audioIn, 0, 4095, -1, 1); // Audio input signal (-1 to 1)
  float freq = map(freqIn, 0, 4095, MIN_FREQ, MAX_FREQ); // Frequency input signal (MIN_FREQ to MAX_FREQ)
  float quant = map(quantIn, 0, 4095, 0, 12); // Quantization input signal (0 to 12)
  Q = map(qIn, 0, 4095, MIN_Q, MAX_Q); // Q input signal (MIN_Q to MAX_Q)

  // Quantize the frequency input to the nearest semitone
  freq = round(log(freq / MIN_FREQ) / log(SEMITONE)) * SEMITONE * MIN_FREQ;

  // Calculate the bandwidth of the bandpass filter
  bandwidth = freq * (pow(SEMITONE, 0.5) - pow(SEMITONE, -0.5));

  // Calculate the filter coefficients using the bilinear transform method
  // Reference: [Arduino Tutorial: Simple High-pass, Band-pass and Band-stop Filtering](https://www.norwegiancreations.com/2016/03/arduino-tutorial-simple-high-pass-band-pass-and-band-stop-filtering/)
  float omega = 2 * PI * centerFreq / SAMPLE_RATE;
  float alpha = sin(omega) / (2 * Q);
  float cosw = cos(omega);
  float norm = 1 / (1 + alpha);
  b0 = alpha * norm;
  b1 = 0;
  b2 = -alpha * norm;
  a1 = -2 * cosw * norm;
  a2 = (1 - alpha) * norm;

  // Apply the filter to the audio input signal
  // Reference: [Arduino-signal-filtering-library](https://jeroendoggen.github.io/Arduino-signal-filtering-library/)
  float y = b0 * audio + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
  x2 = x1;
  x1 = audio;
  y2 = y1;
  y1 = y;

  // Map the filter output to the DAC range
  int audioOut = map(y, -1, 1, 0, 255);

  // Write the filter output to the DAC pin
  dacWrite(AUDIO_OUT, audioOut);

  // Print some information to the serial monitor
  Serial.print("Audio In: ");
  Serial.print(audioIn);
  Serial.print("\tFrequency: ");
  Serial.print(freq);
  Serial.print("\tBandwidth: ");
  Serial.print(bandwidth);
  Serial.print("\tQ: ");
  Serial.print(Q);
  Serial.print("\tAudio Out: ");
  Serial.println(audioOut);
}
