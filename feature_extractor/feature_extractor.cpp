#include "feature_extractor.hpp"

#include "Instrumentor.hpp"
#include "tokenizer.hpp"

#include <cmath>
#include <complex>
#include <corecrt_math_defines.h>
#include <stdexcept> // for runtime_error
#include <iostream>
#include <vector>
#include <cmath>
#include <cstring>
#include <fftw3.h>


featureExtractor::FeatureExtractor::FeatureExtractor(int feature_size, int sampling_rate, int hop_length, int chunk_length, int n_fft)
    : n_fft_(n_fft), hop_length_(hop_length), chunk_length_(chunk_length), n_samples_(chunk_length * sampling_rate),
      nb_max_frames(n_samples_ / hop_length), time_per_frame(hop_length / static_cast<float>(sampling_rate)),
      sampling_rate_(sampling_rate), mel_filters_(get_mel_filters(sampling_rate, n_fft, feature_size))
{
}



std::vector<std::vector<float>> featureExtractor::FeatureExtractor::get_mel_filters(int sr, int n_fft, int n_mels)
{
    // Initialize the weights
    std::vector<std::vector<float>> weights(n_mels, std::vector<float>(1 + n_fft / 2));

    // Center freqs of each FFT bin
    std::vector<float> fftfreqs(n_fft / 2 + 1);
    for (int i = 0; i < fftfreqs.size(); i++)
    {
        fftfreqs[i] = i * static_cast<float>(sr) / n_fft;
    }

    // 'Center freqs' of mel bands - uniformly spaced between limits
    const float min_mel = 0.0;
    const float max_mel = 45.245640471924965;
    std::vector<float> mels(n_mels + 2);
    for (int i = 0; i < mels.size(); i++)
    {
        mels[i] = min_mel + i * (max_mel - min_mel) / (n_mels + 1);
    }

    // Fill in the linear scale
    const float f_min = 0.0;
    const float f_sp = 200.0 / 3;
    std::vector<float> freqs(n_mels + 2);
    for (int i = 0; i < freqs.size(); i++)
    {
        freqs[i] = f_min + f_sp * mels[i];
    }

    // And now the nonlinear scale
    const float min_log_hz = 1000.0;                       // beginning of log region (Hz)
    const float min_log_mel = (min_log_hz - f_min) / f_sp; // same (Mels)
    const float logstep = std::log(6.4) / 27.0;            // step size for log region
    std::vector<bool> log_t(n_mels + 2);
    for (int i = 0; i < log_t.size(); i++)
    {
        log_t[i] = mels[i] >= min_log_mel;
    }
    for (int i = 0; i < freqs.size(); i++)
    {
        if (log_t[i])
        {
            freqs[i] = min_log_hz * std::exp(logstep * (mels[i] - min_log_mel));
        }
    }

    std::vector<float> fdiff = diff(freqs);
    std::vector<std::vector<float>> ramps = subtract_outer(freqs, fftfreqs);

    for(int i=0; i<n_mels; i++){
        // lower and upper slopes for all bins
        std::vector<double> lower(n_fft/2 + 1, 0.0);
        std::vector<double> upper(n_fft/2 + 1, 0.0);

        for(int j=0; j<=n_fft/2; j++){
            lower[j] = -ramps[i][j] / fdiff[i];
            upper[j] = ramps[i+2][j] / fdiff[i+1];
        }

        // .. then intersect them with each other and zero
        for(int j=0; j<=n_fft/2; j++){
            weights[i][j] = std::max(0.0, std::min(lower[j], upper[j]));
        }
    }   

    // Slaney-style mel is scaled to be approx constant energy per channel
    std::vector<double> enorm(n_mels, 0.0);
    for (int i = 0; i < n_mels; i++)
    {
        enorm[i] = 2.0 / (freqs[i + 2] - freqs[i]);
    }
    for (int i = 0; i < n_mels; i++)
    {
        for (int j = 0; j <= n_fft / 2; j++)
        {
            weights[i][j] *= enorm[i];
        }
    }

    return weights;
}

std::vector<std::vector<size_t>> featureExtractor::FeatureExtractor::get_prompt(Tokenizer tokenizer)
{
    std::vector<std::vector<size_t>> prompt;
    size_t id = tokenizer.token_to_id("<|startoftranscript|>");
    std::vector<size_t> in_prompt{id};
    prompt.push_back(in_prompt);
    return prompt;
}

std::vector<float> featureExtractor::FeatureExtractor::diff(std::vector<float> arr)
{
    std::vector<float> diff;

    for (int i = 1; i < arr.size(); i++)
    {
        diff.push_back(arr[i] - arr[i - 1]);
    }

    return diff;
}

std::vector<std::vector<float>> featureExtractor::FeatureExtractor::subtract_outer(std::vector<float> arr1, std::vector<float> arr2)
{
    std::vector<std::vector<float>> result;

    for (int i = 0; i < arr1.size(); i++)
    {
        std::vector<float> row;
        for (int j = 0; j < arr2.size(); j++)
        {
            row.push_back(arr1[i] - arr2[j]);
        }
        result.push_back(row);
    }

    return result;
}

std::vector<std::vector<float>> featureExtractor::FeatureExtractor::from_wave(const std::vector<float> &waveform, bool center = true)
{
    Timer timer("timer from_wave");
    std::vector<std::vector<float>> frames;

    int half_window = (n_fft_ - 1) / 2 + 1;

    for (int i = 0; i < waveform.size() + 1; i += hop_length_)
    {
        std::vector<float> frame;

        if (center)
        {
            int start = i - half_window > 0 ? i - half_window : 0;
            int end = i + half_window < waveform.size() ? i + half_window : waveform.size();

            for (int j = start; j < end; j++)
            {
                frame.push_back(waveform[j]);
            }

            if (start == 0)
            {
                int padd_width_left = half_window - i;
                int padd_width_right = 0;
                std::vector<float> padded_frame(padd_width_left + frame.size() + padd_width_right);

                std::copy(frame.begin(), frame.end(), padded_frame.begin() + padd_width_left);

                for (int j = 0; j < padd_width_left; j++)
                {
                    padded_frame[j] = waveform[padd_width_left - j];
                }

                frame = padded_frame;
            }
            else if (end == waveform.size())
            {
                int padd_width_left = 0;
                int padd_width_right = i - waveform.size() + half_window;
                std::vector<float> padded_frame(padd_width_left + frame.size() + padd_width_right);

                std::copy(frame.begin(), frame.end(), padded_frame.begin() + padd_width_left);

                for (int j = 0; j < padd_width_right; j++)
                {
                    padded_frame[padded_frame.size() - j - 1] = waveform[waveform.size() - j - 1];
                }

                frame = padded_frame;
            }
        }
        else
        {
            if (i + n_fft_ > waveform.size())
            {
                frame.resize(n_fft_);
                std::copy(waveform.end() - n_fft_, waveform.end(), frame.begin());
                std::fill(frame.begin() + waveform.size() - i, frame.end(), 0.0);
            }
            else
            {
                std::copy(waveform.begin() + i, waveform.begin() + i + n_fft_, std::back_inserter(frame));
            }
        }

        frames.push_back(frame);
    }

    return frames;
}


std::vector<std::vector<std::complex<float>>> featureExtractor::FeatureExtractor::stft(std::vector<std::vector<float>> &frames,
                                                                                       std::vector<float> &window, int n_fft)
{

    /*
    auto num_fft_bins = 201;
    auto frame_size = frames[0].size();
    // Transpose data
    std::vector<std::vector<std::complex<float>>> transposed_data(num_fft_bins,
                                                                  std::vector<std::complex<float>>(frames.size()));
    std::vector<float> temp(frame_size);
    std::vector<std::complex<float>> fft_signal(frame_size);

    fftwf_plan plan = fftwf_plan_dft_r2c_1d(frame_size, temp.data(),
                                            reinterpret_cast<fftwf_complex *>(fft_signal.data()), FFTW_ESTIMATE);

    for (int i = 0; i < frames.size(); i++)
    {
        std::transform(frames[i].begin(), frames[i].end(), window.begin(), temp.begin(), std::multiplies<float>());

        //for (int j = 0; j < frame_size; j++)
        //{
        //    fft_signal[j] = std::complex<float>(temp[i], 0.0f);
        //}
        // TODO: implement fft here using the fftw3 lib
        fftwf_execute(plan);

        for (int j = 0; j < num_fft_bins; j++)
        {
            transposed_data[j][i] = fft_signal[j];
        }
    }
    fftwf_destroy_plan(plan);
    return transposed_data;
    */
    int frame_size = frames[0].size();
    int fft_size = n_fft;

    if (fft_size == 0)
    {
        fft_size = frame_size;
    }

    if (fft_size < frame_size)
    {
        throw std::invalid_argument("FFT size must greater or equal the frame size");
    }

    // number of FFT bins to store
    int num_fft_bins = (fft_size >> 1) + 1;

    std::vector<std::vector<std::complex<float>>> data(num_fft_bins, std::vector<std::complex<float>>(frames.size()));
    std::vector<float> fft_signal(fft_size);

    for (int f = 0; f < frames.size(); f++)
    {
        auto frame = frames[f];

        if (window.size() != frame_size)
        {
            throw std::invalid_argument("Window size must equal frame size");
        }

        for (int i = 0; i < frame_size; i++)
        {
            fft_signal[i] = frame[i] * window[i];
        }

        for (int i = frame_size; i < fft_size; i++)
        {
            fft_signal[i] = 0;
        }

        auto fft_result = fft(fft_signal);

        for (int i = 0; i < num_fft_bins; i++)
        {
            data[i][f] = fft_result[i];
        }
    }

    return data;
}


std::vector<std::complex<float>> featureExtractor::FeatureExtractor::fft(std::vector<float> &signal)
{
    int n = signal.size();
    std::vector<std::complex<float>> signal_cpx(n);

    for (int i = 0; i < n; i++)
    {
        signal_cpx[i] = std::complex<float>(signal[i], 0);
    }

    fft(signal_cpx);

    return signal_cpx;
}

void featureExtractor::FeatureExtractor::fft(std::vector<std::complex<float>> &signal)
{
    int n = signal.size();

    if (n <= 1)
    {
        return;
    }

    std::vector<std::complex<float>> even(n / 2);
    std::vector<std::complex<float>> odd(n / 2);

    for (int i = 0; i < n / 2; i++)
    {
        even[i] = signal[2 * i];
        odd[i] = signal[2 * i + 1];
    }

    fft(even);
    fft(odd);

    for (int i = 0; i < n / 2; i++)
    {
        std::complex<float> t = std::complex<float>(std::polar(1.0, -2 * M_PI * i / n)) * odd[i];
        signal[i] = even[i] + t;
        signal[i + n / 2] = even[i] - t;
    }
}

std::vector<float> featureExtractor::FeatureExtractor::generate_window(int n_fft_)
{
    Timer timer("extract generate_window");
    std::vector<float> window(n_fft_ + 1);
    for (int i = 0; i < window.size(); i++)
    {
        window[i] = 0.5 - 0.5 * cos(2 * M_PI * i / (window.size() - 1));
    }
    window.pop_back();
    return window;
}

std::vector<std::vector<float>> featureExtractor::FeatureExtractor::apply_mel_filters(const std::vector<std::vector<float>> &magnitudes,
                                                                                      const std::vector<std::vector<float>> &mel_filters_)
{
    Timer timer("extract apply_mel_filers");
    const size_t n_mel_filters = mel_filters_.size();
    const size_t n_magnitudes = magnitudes.size();
    const size_t n_frames = magnitudes[0].size();
    std::vector<std::vector<float>> mel_spec(n_mel_filters, std::vector<float>(n_frames, 0.0f));

#pragma omp parallel for
    for (size_t i = 0; i < n_mel_filters; i++)
    {
        const float *mel_filter_ptr = &mel_filters_[i][0];
        float *mel_spec_ptr = &mel_spec[i][0];

        for (size_t j = 0; j < n_frames; j++)
        {
            const float *magnitudes_ptr = &magnitudes[0][j];
            float sum = 0.0f;

            for (size_t k = 0; k < n_magnitudes; k++)
            {
                sum += mel_filter_ptr[k] * magnitudes_ptr[k];
            }

            mel_spec_ptr[j] = sum;
        }
    }

    return mel_spec;
}


std::vector<std::vector<float>> featureExtractor::FeatureExtractor::apply_logarithm(const std::vector<std::vector<float>> &input)
{
    Timer timer("extract apply_log");
    std::vector<std::vector<float>> output(input.size(), std::vector<float>(input[0].size()));
    for (int i = 0; i < input.size(); i++)
    {
        for (int j = 0; j < input[i].size(); j++)
        {
            output[i][j] = log10(std::max(input[i][j], 1e-10f));
        }
    }
    return output;
}

void featureExtractor::FeatureExtractor::normalize(std::vector<std::vector<float>> &input)
{
    Timer timer("extract normalize");
    float log_spec_max = -INFINITY;
    for (int i = 0; i < input.size(); i++)
    {
        for (int j = 0; j < input[i].size(); j++)
        {
            if (input[i][j] > log_spec_max)
            {
                log_spec_max = input[i][j];
            }
        }
    }
    for (int i = 0; i < input.size(); i++)
    {
        for (int j = 0; j < input[i].size(); j++)
        {
            input[i][j] = std::max(input[i][j], log_spec_max - 8.0f);
        }
    }
    for (int i = 0; i < input.size(); i++)
    {
        for (int j = 0; j < input[i].size(); j++)
        {
            input[i][j] = (input[i][j] + 4.0f) / 4.0f;
        }
    }
}

std::vector<std::vector<float>> featureExtractor::FeatureExtractor::extract(std::vector<float> waveform, bool padding)
{
    Timer timer_feature("feature extract");
    if (padding)
    {
        waveform.resize(waveform.size() + n_samples_, 0);
    }

    auto window = generate_window(n_fft_);
    auto frames = from_wave(waveform);
    auto stft_result = stft(frames, window, n_fft_);
    auto magnitudes = stft_magnitudes(stft_result);

    auto mel_spec = apply_mel_filters(magnitudes, mel_filters_);
    auto log_spec = apply_logarithm(mel_spec);
    normalize(log_spec);

    return log_spec;
}

std::vector<std::vector<float>> featureExtractor::FeatureExtractor::stft_magnitudes(const std::vector<std::vector<std::complex<float>>> &stft)
{
    Timer timer("timer stft_magnitudes");
    const int num_frames = stft.size();
    const int num_fft_bins = stft[0].size();

    std::vector<std::vector<float>> magnitudes(num_frames, std::vector<float>(num_fft_bins - 1));

    for (int f = 0; f < num_frames; f++)
    {
        for (int b = 0; b < num_fft_bins - 1; b++)
        {
            magnitudes[f][b] = std::abs(stft[f][b]);
        }
    }

    return magnitudes;
}