#include <TimerThree.h>

#include <SFE_MicroOLED.h>

#include <SPI.h>

#include "arduinoFFT.h"

#define PIN_DC    8
#define PIN_RESET 9
#define PIN_CS    10
const int audio_pin = A9;

//random effect
#define RAND_SEL
//random effect length
#define RAND_DUR
//default duration
#define DEFAULT_DUR 16000
//first effect
#define FIRST_EFFECT 10

#define NUM_EFFECTS 11
const int sw = 64;
const int sh = 48;
const uint16_t samples = 128;//2 * screen_width
double vImag[samples], vReal[samples];
const double rainspdmin = 0.2;
const double slimespdmin = 0.2;
const double rainspdmax = 16.0;
const double slimespdmax = 8.0;
const double barspd = 0.5;
const double fftvol = 0.25;
const uint8_t greys = 64;
const int uspersamp = 260;
uint8_t exponent;
volatile unsigned int samp = 0, t2 = DEFAULT_DUR;
volatile float rainbuff[sw], rainbuff2[sw];
volatile uint8_t buff[sw][sh];
volatile int sel = FIRST_EFFECT, hitbottom = sw, y = 0;
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
      //clr midi's buffer
      if(sel==0)
      {
        y = 0;
        for (int i = 0; i < sh; i++)
      {
        for (int i2 = 0; i2 < sw; i2++)
        {
          buff[i2][i]=0;
        }
      }
      }
      #ifdef RAND_SEL
      sel = random(NUM_EFFECTS);//(sel + 1) % 10;//pick new fx
      #else
      sel = (sel + 1) % NUM_EFFECTS;//pick new fx
      #endif
      #ifdef RAND_DUR
      t2 = random(DEFAULT_DUR) + (DEFAULT_DUR*3/2);//pick new duration
      #else
      t2 = DEFAULT_DUR;
      #endif
      hitbottom = 0;
      t = 0;
    }
    switch (sel)
    {
      case 0:
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
      case 1:
        //rain
        FFT.Compute(vReal, vImag, samples, exponent, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, samples);
        oled.clear(PAGE);
        for (int i = 0; i < sw; i++)
        {
          rainbuff[i] = (rainbuff[i] + vReal[i] * rainspdmax / 255.0 * fftvol + rainspdmin); //+rainspdmax/2.0;
          if (rainbuff[i] > sh - 1)rainbuff[i] -= sh - 1;
          rainbuff2[i] = rainbuff[i];
          oled.pixel(i, rainbuff[i]);
        }
        oled.display();
        break;
      case 2:
        //bars
        FFT.Compute(vReal, vImag, samples, exponent, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, samples);
        oled.clear(PAGE);
        for (int i = 0; i < sw; i++)
        {
          int bar = sh - 1 - map(constrain(vReal[i] * fftvol, 0, 255), 0, 255, 0, sh - 1);
          rainbuff2[i] = bar;
          rainbuff[i] += barspd;
          if (rainbuff[i] > bar)rainbuff[i] = bar;
          oled.pixel(i, rainbuff[i]);
          if (bar < sh - 1)oled.line(i, sh - 1, i, bar);
        }
        oled.display();
        break;
      case 3:
        //osc
        oled.clear(PAGE);
        for (int i = 0; i < samples; i+=2)
        {
          if (i/2 > 0)
          {
            uint8_t point1 = sh - 1 - constrain(((vReal[i - 2] + 128) * (sh - 1) / 255.0), 0, sh - 1);
            uint8_t   point2 = sh - 1 - constrain(((vReal[i] + 128) * (sh - 1) / 255.0), 0, sh - 1);
            oled.line(i/2 - 1, point1, i/2, point2);
          }
          rainbuff[i/2] = sh - 1 - constrain(((vReal[i] + 128) * (sh - 1) / 255.0), 0, sh - 1);
          rainbuff2[i/2] = rainbuff[i];
        }
        oled.display();
        break;
      case 4:
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
            rainbuff2[i] = (rainbuff2[i] + vReal[i] * slimespdmax / 255.0 * fftvol + slimespdmin);
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
      case 5:
        //osc and rain
        oled.clear(PAGE);
        //osc
        for (int i = 0; i < samples; i+=2)
        {
          if (i/2 > 0)
          {
            uint8_t point1 = sh - 1 - constrain(((vReal[i - 2] + 128) * (sh - 1) / 255.0), 0, sh - 1);
            uint8_t   point2 = sh - 1 - constrain(((vReal[i] + 128) * (sh - 1) / 255.0), 0, sh - 1);
            oled.line(i/2 - 1, point1, i/2, point2);
          }
          rainbuff2[i/2] = sh - 1 - constrain(((vReal[i] + 128) * (sh - 1) / 255.0), 0, sh - 1);
        }
        //rain
        FFT.Compute(vReal, vImag, samples, exponent, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, samples);
        for (int i = 0; i < sw; i++)
        {
          rainbuff[i] = (rainbuff[i] + vReal[i]  * rainspdmax / 255.0 * fftvol + rainspdmin); //+rainspdmax/2.0;
          if (rainbuff[i] > sh - 1)rainbuff[i] -= sh - 1;
          oled.pixel(i, rainbuff[i]);
        }
        oled.display();
        break;
      case 6:
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
            rainbuff2[i] = (rainbuff2[i] + vReal[i] * slimespdmax / 255.0 * fftvol + slimespdmin);
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
          rainbuff[i] = (rainbuff[i] + vReal[i]  * rainspdmax / 255.0 * fftvol + rainspdmin); //+rainspdmax/2.0;
          if (rainbuff[i] > sh - 1)rainbuff[i] -= sh - 1;
          oled.pixel(i, rainbuff[i]);
        }
        oled.display();
        break;
      case 7:
        //bouncy
        FFT.Compute(vReal, vImag, samples, exponent, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, samples);
        oled.clear(PAGE);
        for (int i = 0; i < sw; i++)
        {
          if (buff[i][0])
          {
            rainbuff[i] = (rainbuff[i] - vReal[i]  * rainspdmax / 255.0 * fftvol - rainspdmin); //+rainspdmax/2.0;
            if (rainbuff[i] < 0)
            {
              rainbuff[i] = -rainbuff[i];
              buff[i][0] = false;
            }
          }
          else
          {
            rainbuff[i] = (rainbuff[i] + vReal[i]  * rainspdmax / 255.0 * fftvol + rainspdmin); //+rainspdmax/2.0;
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
      case 8:
          //osc and bouncy
        oled.clear(PAGE);
        //osc
        for (int i = 0; i < samples; i+=2)
        {
          if (i/2 > 0)
          {
            uint8_t point1 = sh - 1 - constrain(((vReal[i - 2] + 128) * (sh - 1) / 255.0), 0, sh - 1);
            uint8_t   point2 = sh - 1 - constrain(((vReal[i] + 128) * (sh - 1) / 255.0), 0, sh - 1);
            oled.line(i/2 - 1, point1, i/2, point2);
          }
          rainbuff2[i/2] = sh - 1 - constrain(((vReal[i] + 128) * (sh - 1) / 255.0), 0, sh - 1);
        }
        //bouncy
        FFT.Compute(vReal, vImag, samples, exponent, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, samples);
        for (int i = 0; i < sw; i++)
        {
          if (buff[i][0])
          {
            rainbuff[i] = (rainbuff[i] - vReal[i]  * rainspdmax / 255.0 * fftvol - rainspdmin); //+rainspdmax/2.0;
            if (rainbuff[i] < 0)
            {
              rainbuff[i] = -rainbuff[i];
              buff[i][0] = false;
            }
          }
          else
          {
            rainbuff[i] = (rainbuff[i] + vReal[i]  * rainspdmax / 255.0 * fftvol + rainspdmin); //+rainspdmax/2.0;
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
      case 9:
        //jaw
        FFT.Compute(vReal, vImag, samples, exponent, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, samples);
        oled.clear(PAGE);
        for (int i = 0; i < sw; i++)
        {
          int bar = map(constrain(vReal[i] * fftvol, 0, 255), 0, 255, 0, sh/2 - 1);
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
      case 10:
        //jaw and osc
        //osc
        oled.clear(PAGE);
        for (int i = 0; i < samples; i+=2)
        {
          if (i/2 > 0)
          {
            uint8_t point1 = sh - 1 - constrain(((vReal[i - 2] + 128) * (sh - 1) / 255.0), 0, sh - 1);
            uint8_t   point2 = sh - 1 - constrain(((vReal[i] + 128) * (sh - 1) / 255.0), 0, sh - 1);
            oled.line(i/2 - 1, point1, i/2, point2);
          }
          //rainbuff[i] = sh - 1 - constrain(((vReal[i] + 128) * (sh - 1) / 255.0), 0, sh - 1);
          //rainbuff2[i] = rainbuff[i];
        }
        //jaw
        FFT.Compute(vReal, vImag, samples, exponent, FFT_FORWARD);
        FFT.ComplexToMagnitude(vReal, vImag, samples);
        for (int i = 0; i < sw; i++)
        {
          int bar = map(constrain(vReal[i] * fftvol, 0, 255), 0, 255, 0, sh/2 - 1);
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
      
    }
  }
  if (sel == 0)
  {
    if ((samp>>1) % greys == 0)
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
          if (buff[i2][(y + i + 1) % sh] == (samp>>1) % greys)oled.pixel(i2, i, BLACK, NORM);
        }
      }
    }
    oled.display();
  }
}
void loop() {
  // put your main code here, to run repeatedly:
}
