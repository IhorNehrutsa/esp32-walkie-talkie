#include "I2SMEMSSampler.h"
#include "soc/i2s_reg.h"
#include "HardwareSerial.h"

//#define SELF_LISTENING

I2SMEMSSampler::I2SMEMSSampler(
    i2s_port_t i2s_port,
    i2s_pin_config_t &i2s_pins,
    i2s_config_t i2s_config,
    int raw_samples_size,
    bool fixSPH0645) : I2SSampler(i2s_port, i2s_config)
{
    m_i2sPins = i2s_pins;
    m_fixSPH0645 = fixSPH0645;
    m_raw_samples_size = raw_samples_size;
    m_raw_samples = (int32_t *)malloc(sizeof(int32_t) * raw_samples_size);
}

I2SMEMSSampler::~I2SMEMSSampler()
{
  free(m_raw_samples);
}

void I2SMEMSSampler::configureI2S()
{
    if (m_fixSPH0645)
    {
        // FIXES for SPH0645
#if CONFIG_IDF_TARGET_ESP32
        REG_SET_BIT(I2S_TIMING_REG(m_i2sPort), BIT(9));
        REG_SET_BIT(I2S_CONF_REG(m_i2sPort), I2S_RX_MSB_SHIFT);
#endif
    }

    i2s_set_pin(m_i2sPort, &m_i2sPins);
}

#ifdef SELF_LISTENING
uint32_t millis_rd = 0;
uint32_t _millis_rd = 0;
#endif

int I2SMEMSSampler::read(int16_t *samples, int count)
{
    // read from i2s
    size_t bytes_read = 0;
    if (count>m_raw_samples_size)
    {
        count = m_raw_samples_size; // Buffer is too small
    }
    i2s_read(m_i2sPort, m_raw_samples, sizeof(m_raw_samples[0]) * count, &bytes_read, portMAX_DELAY);//pdMS_TO_TICKS(40));//

#ifdef SELF_LISTENING
    esp_err_t err;
    if (bytes_read > 0) {
        millis_rd = millis();
        size_t bytes_written;
        err = i2s_write(m_i2sPort, m_raw_samples, bytes_read, &bytes_written, portMAX_DELAY);
        Serial.print("err=");
        Serial.print(err);
        Serial.print(" millis_rd - _millis_rd=");
        Serial.print(millis_rd - _millis_rd);
        _millis_rd = millis_rd;
        Serial.print(" bytes_read=");
        Serial.print(bytes_read);
        Serial.print(" bytes_written=");
        Serial.println(bytes_written);
        /*
        for (auto i = 0; i < (bytes_read / sizeof(m_raw_samples[0])); i++) {
            Serial.print(m_raw_samples[i]);
            Serial.print(' ');
        }
        Serial.println();
        */
    }
#endif

    int samples_read = bytes_read / sizeof(int32_t);
    for (int i = 0; i < samples_read; i++)
    {
        int32_t temp = m_raw_samples[i] >> 11;
        samples[i] = (temp > INT16_MAX) ? INT16_MAX : (temp < -INT16_MAX) ? -INT16_MAX : (int16_t)temp;
    }
    return samples_read;
}
