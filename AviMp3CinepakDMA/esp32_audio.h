#include "driver/i2s.h"

#include "MP3DecoderHelix.h"

#define GAIN_LEVEL 0.8

static unsigned long total_read_audio_ms = 0;
static unsigned long total_decode_audio_ms = 0;
static unsigned long total_play_audio_ms = 0;

static i2s_port_t _i2s_num;
static esp_err_t i2s_init(i2s_port_t i2s_num, uint32_t sample_rate,
                          int mck_io_num,   /*!< MCK in out pin. Note that ESP32 supports setting MCK on GPIO0/GPIO1/GPIO3 only*/
                          int bck_io_num,   /*!< BCK in out pin*/
                          int ws_io_num,    /*!< WS in out pin*/
                          int data_out_num, /*!< DATA out pin*/
                          int data_in_num   /*!< DATA in pin*/
)
{
    _i2s_num = i2s_num;

    esp_err_t ret_val = ESP_OK;

    i2s_config_t i2s_config;
    i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    i2s_config.sample_rate = sample_rate;
    i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    i2s_config.dma_buf_count = 12;
    i2s_config.dma_buf_len = 320;
    i2s_config.use_apll = false;
    i2s_config.tx_desc_auto_clear = true;
    i2s_config.fixed_mclk = 0;
    i2s_config.mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT;
    i2s_config.bits_per_chan = I2S_BITS_PER_CHAN_16BIT;

    i2s_pin_config_t pin_config;
    pin_config.mck_io_num = mck_io_num;
    pin_config.bck_io_num = bck_io_num;
    pin_config.ws_io_num = ws_io_num;
    pin_config.data_out_num = data_out_num;
    pin_config.data_in_num = data_in_num;

    ret_val |= i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
    ret_val |= i2s_set_pin(i2s_num, &pin_config);

    return ret_val;
}

static int _samprate = 0;
static void audioDataCallback(MP3FrameInfo &info, int16_t *pwm_buffer, size_t len, void *ref)
{
    unsigned long s = millis();
    if (_samprate != info.samprate)
    {
        log_i("bitrate: %d, nChans: %d, samprate: %d, bitsPerSample: %d, outputSamps: %d, layer: %d, version: %d",
              info.bitrate, info.nChans, info.samprate, info.bitsPerSample, info.outputSamps, info.layer, info.version);
        i2s_set_clk(_i2s_num, info.samprate /* sample_rate */, info.bitsPerSample /* bits_cfg */, (info.nChans == 2) ? I2S_CHANNEL_STEREO : I2S_CHANNEL_MONO /* channel */);
        _samprate = info.samprate;
    }
    for (int i = 0; i < len; i++)
    {
        pwm_buffer[i] = pwm_buffer[i] * GAIN_LEVEL;
    }
    size_t i2s_bytes_written = 0;
    i2s_write(_i2s_num, pwm_buffer, len * 2, &i2s_bytes_written, portMAX_DELAY);
    // log_i("len: %d, i2s_bytes_written: %d", len, i2s_bytes_written);
    total_play_audio_ms += millis() - s;
}

static libhelix::MP3DecoderHelix _mp3(audioDataCallback);
static void mp3_player_task(void *pvParam)
{
    unsigned long ms = millis();
    char *p;
    long w;
    do
    {
        ms = millis();

        p = audbuf;
        while (audbuf_remain > 0)
        {
            w = _mp3.write(p, audbuf_remain);
            // log_i("r: %d, w: %d\n", r, w);
            audbuf_remain -= w;
            p += w;
        }
        total_decode_audio_ms += millis() - ms;
        ms = millis();
        vTaskDelay(pdMS_TO_TICKS(1));
    } while (audbuf_read > 0);

    log_i("MP3 stop.");

    i2s_zero_dma_buffer(I2S_NUM_0);
    vTaskDelete(NULL);
}

static BaseType_t mp3_player_task_start(avi_t *a)
{
    _mp3.begin();

    return xTaskCreatePinnedToCore(
        mp3_player_task,
        "MP3 Player Task",
        2000,
        a,
        configMAX_PRIORITIES - 1,
        NULL,
        0);
}
