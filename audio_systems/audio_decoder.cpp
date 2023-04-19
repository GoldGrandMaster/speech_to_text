#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <stdexcept>
#include <cstring>
#include "audio_decoder.hpp"
#include <fstream>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <vector>

// TODO: look into avresample from ffmpeg library if this resampler doesn't work
audioSystem::AudioResampler::AudioResampler(int in_rate, int out_rate, int channels)
{
    m_src_rate = in_rate;
    m_dst_rate = out_rate;
    m_channels = channels;
    m_data = new float[m_src_rate * m_channels];
    m_resampled = new float[m_dst_rate * m_channels];
    m_frac = 0.0f;
}

audioSystem::AudioResampler::~AudioResampler()
{
    delete[] m_data;
    delete[] m_resampled;
}

int audioSystem::AudioResampler::Resample(const float *input, int num_samples, float *output, int max_output_samples)
{
    const float src_rate = static_cast<float>(m_src_rate);
    const float dst_rate = static_cast<float>(m_dst_rate);
    const float ratio = dst_rate / src_rate;
    const int stride = m_channels;
    int samples_written = 0;

    while (num_samples > 0)
    {
        // Copy input to buffer.
        const int samples_to_copy = std::min(num_samples, m_src_rate - m_buf_pos);
        memcpy(m_data + m_buf_pos * stride, input, samples_to_copy * stride * sizeof(int16_t));
        m_buf_pos += samples_to_copy;
        input += samples_to_copy * stride;
        num_samples -= samples_to_copy;

        // Resample buffer.
        int resampled_samples = 0;
        while (m_buf_pos >= static_cast<int>(std::ceil((resampled_samples + 1) / ratio)))
        {
            const int output_index = resampled_samples * stride;
            for (int c = 0; c < m_channels; ++c)
            {
                const float *src = m_data + static_cast<int>(std::floor(output_index / ratio) + c);
                const int frac_index = static_cast<int>(m_frac * 65536);
                const float frac = audioSystem::AudioResampler::s_frac_table[frac_index];
                const float a = src[0];
                const float b = src[stride];
                const float resampled = a * (1.0f - frac) + b * frac;
                m_resampled[output_index + c] = resampled;
            }
            ++resampled_samples;
        }
        m_frac += resampled_samples / ratio - std::floor(resampled_samples / ratio);
        m_buf_pos -= resampled_samples;

        // Write resampled output.
        const int output_samples = std::min(max_output_samples - samples_written, resampled_samples);
        memcpy(output + samples_written * stride, m_resampled, output_samples * stride * sizeof(float));
        samples_written += output_samples;
        if (samples_written >= max_output_samples)
        {
            break;
        }
    }

    return samples_written;
}

float audioSystem::AudioResampler::s_frac_table[AudioResampler::s_frac_table_size];

void audioSystem::AudioDecoder::InitializeFracTable()
{
    for (int i = 0; i < audioSystem::AudioResampler::s_frac_table_size; ++i)
    {
        AudioResampler::s_frac_table[i] = static_cast<float>(i) / 65536;
    }
}

// TODO: complete the audio decoder based on the following code https://github.com/guillaumekln/faster-whisper/blob/master/faster_whisper/audio.py
std::vector<float> audioSystem::AudioDecoder::DecodeAudio(const char *&input_path)
{
    std::vector<float> test;
    return test;
}