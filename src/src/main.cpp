#include <SPI.h>
#include <SD.h>
#include <I2S.h>

// Samples
#define AUDIO_BUFFER_SIZE 1024
// Bytes
#define FILE_BUFFER_SIZE AUDIO_BUFFER_SIZE * 2

// Volume
#define VOL_MAX   15
#define VOL_NOM   10
#define VOL_DLY   200

// Pins
#define EN1       9
#define EN2       10
#define EN3       3
#define EN4       46
#define LEDA      11
#define LEDB      12
#define VOLDN     13
#define VOLUP     14
#define AUX1      15
#define AUX2      16
#define AUX3      17
#define AUX4      18
#define AUX5      8
#define I2S_SD    4
#define I2S_DATA  5
#define I2S_BCLK  6
#define I2S_LRCLK 7
#define SDCLK     36
#define SDMOSI    35
#define SDMISO    37
#define SDCS      34
#define SDDET     33

int bufferSize;
File wavFile;

uint8_t volume = 8;

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  pinMode(VOLDN, INPUT_PULLUP);
  pinMode(VOLUP, INPUT_PULLUP);
  pinMode(I2S_SD, OUTPUT);
  digitalWrite(I2S_SD, 1);  // Enable amplifier

  pinMode(LEDA, OUTPUT);
  pinMode(LEDB, OUTPUT);
  digitalWrite(LEDA, 0);
  digitalWrite(LEDB, 0);

  if (!SD.begin()) {
    Serial.println("SD card initialization failed!");
    return;
  }

  I2S.setSckPin(I2S_BCLK);
  I2S.setFsPin(I2S_LRCLK);
  I2S.setDataPin(I2S_DATA);
/*
  Serial.println(I2S.getSckPin());
  Serial.println(I2S.getFsPin());
  Serial.println(I2S.getDataPin());
  Serial.println(I2S.getDataInPin());
  Serial.println(I2S.getDataOutPin());
*/

  I2S.setBufferSize(AUDIO_BUFFER_SIZE);
/*
  bufferSize = I2S.getBufferSize();
  Serial.print("Buffer size: ");
  Serial.println(bufferSize);
*/

  if(!I2S.begin(I2S_PHILIPS_MODE, 44100, 16))
  {
      Serial.println("Failed to initialize I2S!");
      while (1)
      {
        digitalWrite(LEDA, 1);  digitalWrite(LEDB, 0);  delay(150);
        digitalWrite(LEDA, 0);  digitalWrite(LEDB, 0);  delay(150);
        digitalWrite(LEDA, 1);  digitalWrite(LEDB, 0);  delay(150);
        digitalWrite(LEDA, 0);  digitalWrite(LEDB, 0);  delay(150);
        digitalWrite(LEDA, 0);  digitalWrite(LEDB, 1);  delay(150);
        digitalWrite(LEDA, 0);  digitalWrite(LEDB, 0);  delay(150);
        digitalWrite(LEDA, 0);  digitalWrite(LEDB, 1);  delay(150);
        digitalWrite(LEDA, 0);  digitalWrite(LEDB, 0);  delay(150);
      }
  }

  // FIXME: read volume from NVM

}

void play(const char *wavName)
{
  int i;
  size_t bytesRead;
  uint8_t fileBuffer[FILE_BUFFER_SIZE];
  int16_t sampleValue;
  unsigned long pressTime;
  uint32_t byteCount = 0, wavDataSize = 0;

  Serial.print("Playing... ");
  Serial.println(wavName);

  wavFile = SD.open(wavName, FILE_READ);
  wavFile.seek(40);   // Seek to length
  wavFile.read((uint8_t*)&wavDataSize, 4);  // Read length - WAV is little endian, only works if uC is also little endian
  while(wavFile.available() && (byteCount < wavDataSize))
  {
    // Process volume
    bool holdoffDone = (millis() - pressTime) > VOL_DLY;
    if(holdoffDone)
    {
      // Turn off LED if past delay time
      digitalWrite(LEDB, 0);
    }

    if(!digitalRead(VOLUP) && holdoffDone)
    {
      pressTime = millis();
      if(volume < VOL_MAX)
        volume++;
      // FIXME: Add NVM volume write
      Serial.println(volume);
      digitalWrite(LEDB, 1);
    }
    else if(!digitalRead(VOLDN) && holdoffDone)
    {
      pressTime = millis();
      if(volume > 0)
        volume--;
      // FIXME: Add NVM volume write
      Serial.println(volume);
      digitalWrite(LEDB, 1);
    }

    // Audio buffer samples are in 16-bit chunks, so multiply by two to get # of bytes to read
    bytesRead = wavFile.read(fileBuffer, min((uint32_t)FILE_BUFFER_SIZE, wavDataSize-byteCount));
    byteCount += bytesRead;
    for(i=0; i<bytesRead; i+=2)
    {
      // File is read on a byte basis, so convert into int16 samples, and step every 2 bytes
      sampleValue = *((int16_t *)(fileBuffer+i));
      sampleValue = sampleValue * volume / 10;
      // Write twice (left & right)
      I2S.write(sampleValue);
      I2S.write(sampleValue);
    }
  }
  wavFile.close();
}

void loop()
{
  play("/y-mono44k.wav");
  I2S.flush();
  I2S.end();
}
