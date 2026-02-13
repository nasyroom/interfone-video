/**************************************************
 * poulet 
 * Video 320x240 + PDM Audio
**************************************************/

#include <Arduino.h>
#include <ESPNowCam.h>
#include <drivers/CamXiao.h>
#include "driver/i2s.h"

CamXiao Camera;
ESPNowCam radio;

// ======== I2S AUDIO ========
#define I2S_NUM_RX           I2S_NUM_0
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_42
#define I2S_MIC_SERIAL_DATA  GPIO_NUM_41
#define SAMPLE_RATE          16000
#define AUDIO_BUF            256
#define MAX_PKT_SIZE         200  // bytes per audio packet

// ======== Camera Buffer ========
#define MAX_FRAME_SIZE       40000
uint8_t frameBuffer[MAX_FRAME_SIZE];

// ======== Audio setup ========
void setupMic() {
    i2s_config_t rx_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = AUDIO_BUF,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t rx_pins = {
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = I2S_MIC_SERIAL_CLOCK,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_MIC_SERIAL_DATA
    };

    i2s_driver_install(I2S_NUM_RX, &rx_config, 0, NULL);
    i2s_set_pin(I2S_NUM_RX, &rx_pins);
    i2s_set_clk(I2S_NUM_RX, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}

// ======== Send Audio ========
void sendAudio() {
    int16_t buf[AUDIO_BUF];
    size_t bytesRead = 0;
    i2s_read(I2S_NUM_RX, (void*)buf, sizeof(buf), &bytesRead, portMAX_DELAY);

    int remaining = bytesRead;
    uint8_t *ptr = (uint8_t*)buf;

    while (remaining > 0) {
        int chunk = remaining > MAX_PKT_SIZE ? MAX_PKT_SIZE : remaining;
        radio.sendData(ptr, chunk);
        ptr += chunk;
        remaining -= chunk;
        delayMicroseconds(500);
    }
}

// ======== Send Video ========
void sendFrame() {
    if (Camera.get()) {
        uint8_t* out_jpg = NULL;
        size_t out_jpg_len = 0;
        if (frame2jpg(Camera.fb, 20, &out_jpg, &out_jpg_len)) { // lower quality to save memory
            radio.sendData(out_jpg, out_jpg_len);
            free(out_jpg);
        }
        Camera.free();
    }
}

// ======== Setup ========
void setup() {
    Serial.begin(115200);
    Camera.config.frame_size = FRAMESIZE_QVGA;  // 320x240
    Camera.config.jpeg_quality = 20;            // lower for memory safety
    Camera.config.fb_count = 1;
    Camera.begin();

    const uint8_t RECEIVER_MAC[6] = {0x8C, 0xBF, 0xEA, 0x8E, 0x58, 0x24};
    radio.setTarget(RECEIVER_MAC);
    radio.init();

    setupMic();
    Serial.println("✅ Transmitter Ready");
}

// ======== Loop ========
void loop() {
    sendFrame();
    sendAudio();
}
