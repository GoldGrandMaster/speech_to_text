#pragma once

#include <queue>
#include <string>
#include <chrono>
#include <mutex>
#include <thread>
#include "audio_async.hpp"
#include "whisper_fast.hpp"

namespace whisper
{
    class WhisperStream
    {
    private:
        // string queue that updates
        std::queue<std::string> text_queue;
        std::mutex mtx;

        std::chrono::high_resolution_clock::time_point t_last;
        std::chrono::high_resolution_clock::time_point t_last_attempt;
        std::vector<float> pcmf32;
        std::vector<float> pcmf32_vad;
        std::thread t;
        WhisperFast whisper_fast;

        int detect_segment();
        int vad_simple(std::vector<float>& pcmf32, int sample_rate, int last_ms, float vad_thold, float freq_thold,
            bool verbose);
        void high_pass_filter(std::vector<float>& data, float cutoff, float sample_rate);

    public:
        WhisperStream();
        audioSystem::AudioAsync audio;
        void init();

        // function to get the latest string
        int get_last_transcribed(std::string& str);

        // async audio processing function that writes to the string queue
        int process_audio();
        // calls AI
        // mutex and extracts
    };

}
