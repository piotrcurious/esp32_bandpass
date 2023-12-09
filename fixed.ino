// Define the analog input pins
#define AUDIO_IN A0
#define FREQ_IN A1
#define QUANT_IN A2
#define RATE_IN A3

// Define the DAC output pin
#define AUDIO_OUT 25

// Define the filter parameters
#define Q 10 // Quality factor
#define BW 0.0595 // Bandwidth in octaves (one semitone)

// Define the minimum and maximum values for frequency, quantization and sampling rate
#define FREQ_MIN 20 // Hz
#define FREQ_MAX 20000 // Hz
#define QUANT_MIN 1 // Hz
#define QUANT_MAX 100 // Hz
#define RATE_MIN 8000 // Hz
#define RATE_MAX 44100 // Hz

// Define the scaling factors for the analog inputs
#define FREQ_SCALE ((FREQ_MAX - FREQ_MIN) / 4095.0)
#define QUANT_SCALE ((QUANT_MAX - QUANT_MIN) / 4095.0)
#define RATE_SCALE ((RATE_MAX - RATE_MIN) / 4095.0)

// Declare the filter coefficients and state variables
float b0, b1, b2, a1, a2; // Coefficients
float x1, x2, y1, y2; // State variables

// Declare the input and output variables
float x, y; // Input and output samples
int x_raw, y_raw; // Raw analog values

// Declare the frequency, quantization and sampling rate variables
float freq, quant, rate; // Actual values
int freq_raw, quant_raw, rate_raw; // Raw analog values

// Declare the timer variable
hw_timer_t * timer = NULL;

// The interrupt service routine for the timer
void IRAM_ATTR onTimer() {
  // Read the raw analog values for frequency, quantization and sampling rate
  freq_raw = analogRead(FREQ_IN);
  quant_raw = analogRead(QUANT_IN);
  rate_raw = analogRead(RATE_IN);

  // Scale and constrain the raw values to get the actual values
  freq = constrain(freq_raw * FREQ_SCALE + FREQ_MIN, FREQ_MIN, FREQ_MAX);
  quant = constrain(quant_raw * QUANT_SCALE + QUANT_MIN, QUANT_MIN, QUANT_MAX);
  rate = constrain(rate_raw * RATE_SCALE + RATE_MIN, RATE_MIN, RATE_MAX);

  // Quantize the frequency to the nearest multiple of quant
  freq = round(freq / quant) * quant;

  // Calculate the filter coefficients using the bilinear transform method
  // Reference: [Arduino Tutorial: Simple High-pass, Band-pass and Band-stop Filtering](https://www.norwegiancreations.com/2016/03/arduino-tutorial-simple-high-pass-band-pass-and-band-stop-filtering/)
  float omega = 2 * PI * freq / rate; // Angular frequency
  float alpha = sin(omega) / (2 * Q); // Alpha parameter
  float beta = cos(omega); // Beta parameter
  float gamma = 1 + alpha; // Gamma parameter
  b0 = alpha / gamma; // Feedforward coefficient 0
  b1 = 0; // Feedforward coefficient 1
  b2 = -alpha / gamma; // Feedforward coefficient 2
  a1 = -2 * beta / gamma; // Feedback coefficient 1
  a2 = (1 - alpha) / gamma; // Feedback coefficient 2

  // Read the raw analog value for the audio input
  x_raw = analogRead(AUDIO_IN);

  // Scale and center the raw value to get the input sample
  x = (x_raw - 2048) / 2048.0;

  // Apply the filter to the input sample using the difference equation
  // Reference: [Digital Filters for Everyone](https://forum.arduino.cc/t/digital-high-pass-or-band-pass-filter-for-arduino/934118)
  y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

  // Update the state variables
  x2 = x1;
  x1 = x;
  y2 = y1;
  y1 = y;

  // Scale and shift the output sample to get the raw analog value
  y_raw = (y * 2048) + 2048;

  // Write the raw analog value to the DAC output
  dacWrite(AUDIO_OUT, y_raw);
}

void setup() {
  // Initialize the serial monitor
  Serial.begin(115200);

  // Initialize the analog inputs
  analogReadResolution(12); // Set the resolution to 12 bits
  analogSetAttenuation(ADC_11db); // Set the attenuation to 11 dB

  // Initialize the DAC output
  dacWrite(AUDIO_OUT, 2048); // Set the initial value to the mid-point

  // Initialize the timer
  timer = timerBegin(0, 80, true); // Use timer 0 with prescaler 80 (1 MHz)
  timerAttachInterrupt(timer, &onTimer, true); // Attach the interrupt service routine
  timerAlarmWrite(timer, 1000, true); // Set the alarm value to 1000 (1 kHz)
  timerAlarmEnable(timer); // Enable the alarm
}

void loop() {
  // Print the frequency, quantization and sampling rate values to the serial monitor
  Serial.print("Frequency: ");
  Serial.print(freq);
  Serial.print(" Hz, Quantization: ");
  Serial.print(quant);
  Serial.print(" Hz, Sampling rate: ");
  Serial.print(rate);
  Serial.println(" Hz");
  delay(1000); // Wait for 1 second
}
