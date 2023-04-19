#pragma once
#include "tokenizer.hpp"

#include <complex>
#include <vector>

namespace featureExtractor
{
    class FeatureExtractor
    {
    private:
        int n_fft_;
        int hop_length_;
        int chunk_length_;
        int n_samples_;
        int sampling_rate_;
        std::vector<std::vector<float>> mel_filters_;
        std::vector<float> diff(std::vector<float> arr);
        std::vector<std::vector<float>> subtract_outer(std::vector<float> arr1, std::vector<float> arr2);
        std::vector<std::vector<float>> from_wave(const std::vector<float>& waveform, bool center);

        std::vector<std::vector<float>> stft_magnitudes(const std::vector<std::vector<std::complex<float>>>& stft);
        std::vector<std::vector<std::complex<float>>> stft(std::vector<std::vector<float>>& frames,
            std::vector<float>& window, int n_fft);
        std::vector<std::complex<float>> fft(std::vector<float>& signal);
        void fft(std::vector<std::complex<float>>& signal);
        std::vector<float> generate_window(int n_fft_);
        std::vector<std::vector<float>> apply_mel_filters(const std::vector<std::vector<float>>& magnitudes,
            const std::vector<std::vector<float>>& mel_filters_);
        std::vector<std::vector<float>> apply_logarithm(const std::vector<std::vector<float>>& input);
        void normalize(std::vector<std::vector<float>>& input);

    public:
        int nb_max_frames;
        float time_per_frame;
        FeatureExtractor(int feature_size = 80, int sampling_rate = 16000, int hop_length = 160, int chunk_length = 30,
            int n_fft = 400);
        std::vector<std::vector<float>> get_mel_filters(int sampling_rate, int n_fft, int n_mels);
        std::vector<std::vector<size_t>> get_prompt(Tokenizer tokenizer);

        std::vector<std::vector<float>> extract(std::vector<float> waveform, bool padding);

    };
}
