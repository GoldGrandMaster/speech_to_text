#pragma once
#include <cstdint>
#include <vector>

namespace audioSystem
{
    class AudioResampler
    {
    public:
        AudioResampler(int in_rate, int out_rate, int channels);

        ~AudioResampler();

        int Resample(const float* input, int num_samples, float* output, int max_output_samples);
        static constexpr int s_frac_table_size = 65537;
        static float s_frac_table[s_frac_table_size];

    private:
        int m_src_rate;
        int m_dst_rate;
        int m_channels;
        float* m_data;
        float* m_resampled;
        float m_frac;

        int m_buf_pos = 0;
    };

    class AudioDecoder
    {
    public:
        float s_frac_table[AudioResampler::s_frac_table_size];

        static void InitializeFracTable();

    public:
        static std::vector<float> DecodeAudio(const char*& input_path);
    };

}