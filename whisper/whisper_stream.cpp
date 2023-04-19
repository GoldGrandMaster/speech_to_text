#include "whisper_stream.hpp"

#include "audio_async.hpp"
#include "Instrumentor.hpp"
#include "whisper_fast.hpp"

#include <future>
#include <iostream>

int whisper::WhisperStream::detect_segment()
{
    //mtx.lock();
    
    text_queue.push(whisper_fast.generate(pcmf32));
    //mtx.unlock();
    return 0;
}

// TODO: improve VAD algorithm
int whisper::WhisperStream::vad_simple(std::vector<float> &pcmf32, int sample_rate, int last_ms, float vad_thold,
                                       float freq_thold, bool verbose)
{
    const int n_samples = pcmf32.size();
    const int n_samples_last = (sample_rate * last_ms) / 1000;

    if (n_samples_last >= n_samples)
    {
        // not enough samples - assume no speech
        return 0;
    }

    if (freq_thold > 0.0f)
    {
        high_pass_filter(pcmf32, freq_thold, sample_rate);
    }

    float energy_all = 0.0f;
    float energy_last = 0.0f;

    for (int i = 0; i < n_samples; i++)
    {
        energy_all += fabsf(pcmf32[i]);

        if (i >= n_samples - n_samples_last)
        {
            energy_last += fabsf(pcmf32[i]);
        }
    }

    energy_all /= n_samples;
    energy_last /= n_samples_last;

    if (verbose)
    {
        fprintf(stderr, "%s: energy_all: %f, energy_last: %f, vad_thold: %f, freq_thold: %f\n", __func__, energy_all,
                energy_last, vad_thold, freq_thold);
    }
    
    if (energy_all > 0.0007)
    {
        return 1;
    }

    if (energy_last < vad_thold * energy_all)
    {
        return 2;
    }

    return 0;
}

void whisper::WhisperStream::high_pass_filter(std::vector<float> &data, float cutoff, float sample_rate)
{
    const float rc = 1.0f / (2.0f * M_PI * cutoff);
    const float dt = 1.0f / sample_rate;
    const float alpha = dt / (rc + dt);

    float y = data[0];

    for (size_t i = 1; i < data.size(); i++)
    {
        y = alpha * (y + data[i] - data[i - 1]);
        data[i] = y;
    }
}

whisper::WhisperStream::WhisperStream()
	: whisper_fast("C:/dev/Resources/Models/whisper-tiny.en-ct2")
{
    
}

void whisper::WhisperStream::init()
{
    if (!audio.init(-1, 16000, 29000))
    {
        fprintf(stderr, "%s: audio.init() failed!\n", __func__);
        return;
    }
    audio.resume();

    t_last = std::chrono::high_resolution_clock::now();
}

int whisper::WhisperStream::get_last_transcribed(std::string &str)
{
    //std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
    //if (lock.owns_lock()) {
    if (!text_queue.empty())
    {
        str = text_queue.front();
        text_queue.pop();
    }
    else
    {
        str = "";
    }
    return 0;
    //}
}

int whisper::WhisperStream::process_audio()
{
    const auto t_now = std::chrono::high_resolution_clock::now();
    const auto t_diff_attempt = std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_last_attempt).count();
    if (t_diff_attempt > 200)
    {
        const auto t_diff = std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_last).count();
        //std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
        audio.get(1000, pcmf32_vad);
        auto vad_state = vad_simple(pcmf32_vad, 16000, 700, 0.6f, 100.0f, false);
        if (vad_state == 1)
        {
            audio.get(t_diff, pcmf32);
            auto future_segment = std::async(std::launch::async, &WhisperStream::detect_segment, this);
        }
        else if (vad_state == 2)
        {
            t_last = std::chrono::high_resolution_clock::now();
            audio.get(t_diff, pcmf32);
            auto future_segment = std::async(std::launch::async, &WhisperStream::detect_segment, this);
        }
        t_last_attempt = t_now;
        return 1;
    }
    return 2;
}
