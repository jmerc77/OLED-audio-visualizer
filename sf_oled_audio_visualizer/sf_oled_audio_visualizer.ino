#include <TimerThree.h>

#include <SFE_MicroOLED.h>

#include <SPI.h>

#include "arduinoFFT.h"

#define PIN_DC    8
#define PIN_RESET 9
#define PIN_CS    10
const int audio_pin = A9;
const int sw = 64;
const int sh = 48;
const uint16_t samples = 128;//2 * screen_width
double vImag[samples], vReal[samples];
const double rainspdmin = 0.2;
const double slimespdmin = 0.2;
const double rainspdmax = 6.0;
const double slimespdmax = 3.0;
const double barspd = 0.5;
const uint8_t greys = 16;
const int uspersamp = 300;
uint8_t exponent;
volatile unsigned int samp = 0, t2 = 20000;
volatile float rainbuff[sw], rainbuff2[sw];
volatile uint8_t buff[sw][sh];
volatile int sel = 2, hitbottom = sw, y = 0;
elapsedMillis t = 0;
MicroOLED oled(PIN_RESET, PIN_DC, PIN_CS);
arduinoFFT FFT = arduinoFFT();
void setup() {
  analogReadRes(8);
  int seed = 0;
  for (int i = 0; i < 1000; i++)
  {
    seed += analogRead(audio_pin);
    delay(1);
  }
  randomSeed(seed);

  // put your setup code here, to run once:
  oled.begin();    // Initialize the OLED
  oled.clear(ALL); // Clear the display's internal memory
  oled.clear(PAGE);
  oled.display();
  //delay(1000);
  exponent = FFT.Exponent(samples);
  Timer3.initialize(uspersamp);
  Timer3.attachInterrupt(getsamp);
}
void getsamp()
{

  vReal[samp % samples] = analogRead(audio_pin) - 128;
  vImag[samp % samples] = 0;
  samp++;
  bool rst = false;
  int fftlow = 0, ffthigh = 0;
  if (samp % samples == 0)
  {
    if (t > t2)
    {
      sel = random(9);//(sel + 1) % 9;
      t2 = random(20000) + 15000;
      hitbottom = 0;
      t = 0;
    }
    switch (sel)
    {
      case 0:
        //rain
        FFT.Compute(vReal, vImag, samples, exponent, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, samples);
        oled.clear(PAGE);
        for (int i = 0; i < sw; i++)
        {
          rainbuff[i] = (rainbuff[i] + vReal[i] * rainspdmax / 511.0 + rainspdmin); //+rainspdmax/2.0;
          if (rainbuff[i] > sh - 1)rainbuff[i] -= sh - 1;
          rainbuff2[i] = rainbuff[i];
          oled.pixel(i, rainbuff[i]);
        }
        oled.display();
        break;
      case 1:
        //bars
        FFT.Compute(vReal, vImag, samples, exponent, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, samples);
        oled.clear(PAGE);
        for (int i = 0; i < sw; i++)
        {
          int bar = sh - 1 - map(constrain(vReal[i], 0, 511), 0, 511, 0, sh - 1);
          rainbuff2[i] = bar;
          rainbuff[i] += barspd;
          if (rainbuff[i] > bar)rainbuff[i] = bar;
          oled.pixel(i, rainbuff[i]);
          if (bar < sh - 1)oled.line(i, sh - 1, i, bar);
        }
        oled.display();
        break;
      case 2:
        //osc
        oled.clear(PAGE);
        for (int i = 0; i < sw; i++)
        {
          if (i > 0)
          {
            uint8_t point1 = sh - 1 - constrain(((vReal[i * 2 - 2] + 128) * (sh - 1) / 255.0), 0, sh - 1);
            uint8_t   point2 = sh - 1 - constrain(((vReal[i * 2] + 128) * (sh - 1) / 255.0), 0, sh - 1);
            oled.line(i - 1, point1, i, point2);
          }
          rainbuff[i] = sh - 1 - constrain(((vReal[i * 2] + 128) * (sh - 1) / 255.0), 0, sh - 1);
          rainbuff2[i] = rainbuff[i];
        }
        oled.display();
        break;
      case 3:
        //slime
        FFT.Compute(vReal, vImag, samples, exponent, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, samples);
        oled.clear(PAGE);
        rst = (hitbottom >= sw);
        hitbottom = 0;
        for (int i = 0; i < sw; i++)
        {
          if (rst)rainbuff2[i] = 0;
          if (rainbuff2[i] <= sh - 1)
          {
            rainbuff2[i] = (rainbuff2[i] + vReal[i] * slimespdmax / 511.0 + slimespdmin);
          }
          else
          {
            rainbuff2[i] = sh;
            hitbottom++;
          }
          if (i > 0)
          {
            if (rainbuff2[i] <= sh - 1 && rainbuff2[i - 1] <= sh - 1)
            {
              oled.line(i - 1, (uint8_t)floor(rainbuff2[i - 1]), i, (uint8_t)floor(rainbuff2[i]));
            }
            else if (rainbuff2[i - 1] > sh - 1 && rainbuff2[i] <= sh - 1)
            {
              oled.line(i - 1, sh - 1, i, (uint8_t)floor(rainbuff2[i]));
            }
            else if (rainbuff2[i] > sh - 1 && rainbuff2[i - 1] <= sh - 1)
            {
              oled.line(i - 1, (uint8_t)floor(rainbuff2[i - 1]), i, sh - 1);
            }
          }
          rainbuff[i] = rainbuff2[i];
        }
        oled.display();
        break;
      case 4:
        //osc and rain
        oled.clear(PAGE);
        for (int i = 0; i < sw; i++)
        {
          if (i > 0)
          {
            uint8_t point1 = sh - 1 - constrain(((vReal[i * 2 - 2] + 128) * (sh - 1) / 255.0), 0, sh - 1);
            uint8_t   point2 = sh - 1 - constrain(((vReal[i * 2] + 128) * (sh - 1) / 255.0), 0, sh - 1);
            oled.line(i - 1, point1, i, point2);
          }
          rainbuff2[i] = sh - 1 - constrain(((vReal[i * 2] + 128) * (sh - 1) / 255.0), 0, sh - 1);
        }
        FFT.Compute(vReal, vImag, samples, exponent, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, samples);
        for (int i = 0; i < sw; i++)
        {
          rainbuff[i] = (rainbuff[i] + vReal[i]  * rainspdmax / 511.0 + rainspdmin); //+rainspdmax/2.0;
          if (rainbuff[i] > sh - 1)rainbuff[i] -= sh - 1;
          oled.pixel(i, rainbuff[i]);
        }
        oled.display();
        break;
      case 5:
        //slime and rain
        FFT.Compute(vReal, vImag, samples, exponent, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, samples);
        oled.clear(PAGE);
        rst = (hitbottom >= sw);
        hitbottom = 0;
        for (int i = 0; i < sw; i++)
        {
          if (rst)rainbuff2[i] = 0;
          if (rainbuff2[i] <= sh - 1)
          {
            rainbuff2[i] = (rainbuff2[i] + vReal[i] * slimespdmax / 511.0 + slimespdmin);
          }
          else
          {
            rainbuff2[i] = sh;
            hitbottom++;
          }
          if (i > 0)
          {
            if (rainbuff2[i] <= sh - 1 && rainbuff2[i - 1] <= sh - 1)
            {
              oled.line(i - 1, (uint8_t)floor(rainbuff2[i - 1]), i, (uint8_t)floor(rainbuff2[i]));
            }
            else if (rainbuff2[i - 1] > sh - 1 && rainbuff2[i] <= sh - 1)
            {
              oled.line(i - 1, sh - 1, i, (uint8_t)floor(rainbuff2[i]));
            }
            else if (rainbuff2[i] > sh - 1 && rainbuff2[i - 1] <= sh - 1)
            {
              oled.line(i - 1, (uint8_t)floor(rainbuff2[i - 1]), i, sh - 1);
            }
          }
          rainbuff[i] = (rainbuff[i] + vReal[i]  * rainspdmax / 511.0 + rainspdmin); //+rainspdmax/2.0;
          if (rainbuff[i] > sh - 1)rainbuff[i] -= sh - 1;
          oled.pixel(i, rainbuff[i]);
        }
        oled.display();
        break;
      case 6:
        //midi
        FFT.Compute(vReal, vImag, samples, exponent, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, samples);
        //compute min/max
        fftlow = vReal[0];
        ffthigh = vReal[0];
        for (int i = 1; i < sw; i++)
        {
          if (vReal[i] > ffthigh)ffthigh = vReal[i];
          if (vReal[i] < fftlow)fftlow = vReal[i];
        }
        //put in buffer
        for (int i = 0; i < sw; i++)
        {
          buff[i][y] = (uint8_t)map(vReal[i], fftlow, ffthigh, 0, greys - 1); //>midithresh;
        }
        //update y pos
        y = (y + sh - 1) % sh;
        break;
      case 7:
        //jaw
        FFT.Compute(vReal, vImag, samples, exponent, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, samples);
        oled.clear(PAGE);
        for (int i = 0; i < sw; i++)
        {
          int bar = map(constrain(vReal[i], 0, 511), 0, 511, 0, sh - 1);
          rainbuff2[i] = bar;
          if (i % 2 == 0)
          {
            bar = sh - 1 - bar;
            rainbuff[i] += barspd;
            if (rainbuff[i] > bar)rainbuff[i] = bar;
            oled.pixel(i, rainbuff[i]);
            if (bar < sh - 1)oled.line(i, sh - 1, i, bar);
          }
          else
          {
            rainbuff[i] -= barspd;
            if (rainbuff[i] < bar)rainbuff[i] = bar;
            oled.pixel(i, rainbuff[i]);
            if (bar > 0)oled.line(i, 0, i, bar);
          }
        }
        oled.display();
        break;
      case 8:
        //bouncy
        FFT.Compute(vReal, vImag, samples, exponent, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, samples);
        oled.clear(PAGE);
        for (int i = 0; i < sw; i++)
        {
          if (buff[i][0])
          {
            rainbuff[i] = (rainbuff[i] - vReal[i]  * rainspdmax / 511.0 - rainspdmin); //+rainspdmax/2.0;
            if (rainbuff[i] < 0)
            {
              rainbuff[i] = -rainbuff[i];
              buff[i][0] = false;
            }
          }
          else
          {
            rainbuff[i] = (rainbuff[i] + vReal[i]  * rainspdmax / 511.0 + rainspdmin); //+rainspdmax/2.0;
            if (rainbuff[i] > sh - 1)
            {
              rainbuff[i] = sh - 1 - rainbuff[i] + sh - 1;
              buff[i][0] = true;
            }
          }
          rainbuff2[i] = rainbuff[i];
          oled.pixel(i, rainbuff[i]);
        }
        oled.display();
        break;
    }
  }
  if (sel == 6)
  {
    if (samp % greys == 0)
    {
      oled.clear(PAGE);
      for (int i = 0; i < sh; i++)
      {
        for (int i2 = 0; i2 < sw; i2++)
        {
          if (buff[i2][(y + i + 1) % sh] > 0)oled.pixel(i2, i, WHITE, NORM);
        }
      }
    }
    else
    {
      for (int i = 0; i < sh; i++)
      {
        for (int i2 = 0; i2 < sw; i2++)
        {
          if (buff[i2][(y + i + 1) % sh] == samp % greys)oled.pixel(i2, i, BLACK, NORM);
        }
      }
    }
    oled.display();
  }
}
void loop() {
  // put your main code here, to run repeatedly:
}
