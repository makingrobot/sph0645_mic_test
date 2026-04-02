#include "ESP_I2S.h"
#include "wav_header.h"
#include "FS.h"
#include "SD.h"

const uint8_t I2S_SCK = 25;
const uint8_t I2S_WS = 26;
const uint8_t I2S_DIN = 27;

const uint8_t SD_CS = 14;
const uint8_t SD_CLK = 15;
const uint8_t SD_MISO = 16;
const uint8_t SD_MOSI = 17;

void setup() {

  // Create an instance of the I2SClass
  I2SClass i2s;

  // Initialize the serial port
  Serial.begin(115200);

  Serial.println("Initializing I2S bus...");

  // Set up the pins used for audio input
  i2s.setPins(I2S_SCK, I2S_WS, -1, I2S_DIN);

  // Initialize the I2S bus in standard mode
  if (!i2s.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT)) {
    Serial.println("Failed to initialize I2S bus!");
    return;
  }

  i2s.configureRX(16000, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO, I2S_RX_TRANSFORM_32_TO_16);

  Serial.println("I2S bus initialized.");
  Serial.println("Initializing SD card...");

  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  // Mount the SD card
  if (!SD.begin(SD_CS)) {
    Serial.println("Failed to initialize SD card!");
    return;
  }

  Serial.println("SD card initialized.");
  Serial.println("Recording 2 seconds of audio data...");

  // Create a file on the SD card
  SD.remove("/test001.wav");
  File file = SD.open("/test001.wav", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing!");
    return;
  }

  // 计算缓存
  uint32_t sample_rate = i2s.rxSampleRate();
  uint16_t sample_width = (uint16_t)i2s.rxDataWidth();
  uint16_t num_channels = (uint16_t)i2s.rxSlotMode();
  size_t buf_size = (sample_rate * (sample_width / 8)) * num_channels;
  Serial.printf("Record WAV: rate:%lu, bits:%u, channels:%u\n, buf_size:%u", sample_rate, sample_width, num_channels, buf_size);

  //uint8_t *wav_buf = (uint8_t *)malloc(rec_size + WAVE_HEADER_SIZE);
  uint8_t *wav_buf = (uint8_t *)heap_caps_malloc(buf_size + PCM_WAV_HEADER_SIZE, MALLOC_CAP_DEFAULT);
  if (wav_buf == NULL) {
    Serial.printf("Failed to allocate WAV buffer with size %u\n", buf_size);
    return;
  }

  uint8_t head_buf[PCM_WAV_HEADER_SIZE];
  // 写入文件头
  const pcm_wav_header_t wav_header = PCM_WAV_HEADER_DEFAULT(0, sample_width, sample_rate, num_channels);
  memcpy(head_buf, &wav_header, PCM_WAV_HEADER_SIZE);
  file.write(head_buf, PCM_WAV_HEADER_SIZE);

  int n = 0;
  int data_len = 0;
  int rec_second = 60;  //60s
  while (n++ < rec_second)
  {
    Serial.printf("Recording %ds ...\n", n);

    // 获取MIC数据
    size_t wav_size = i2s.readBytes((char *)wav_buf, buf_size);
    data_len += wav_size;

    // 写入数据
    file.write(wav_buf, wav_size);
  }
  free(wav_buf);
  Serial.println("Record ended.")

  // 更新文件头
  const pcm_wav_header_t wav_header2 = PCM_WAV_HEADER_DEFAULT(data_len, sample_width, sample_rate, num_channels);
  memcpy(head_buf, &wav_header2, PCM_WAV_HEADER_SIZE);
  file.seek(0, SeekSet);
  file.write(head_buf, PCM_WAV_HEADER_SIZE);
  file.close();

  Serial.println("Application complete.");
}

void loop() {}
