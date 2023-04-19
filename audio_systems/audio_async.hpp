#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

#undef main

#include <atomic>
#include <mutex>
#include <vector>
#include "boost/circular_buffer.hpp"

namespace audioSystem
{
    class AudioAsync
    {
    public:
        //AudioAsync(int len_ms);
        //~AudioAsync();

        bool init(int capture_id, int sample_rate, int len_ms);

        // start capturing audio via the provided SDL callback
        // keep last len_ms seconds of audio in a circular buffer
        bool resume();
        bool pause();
        bool clear();

        // callback to be called by SDL
        void callback(uint8_t* stream, int len);

        // get audio data from the circular buffer
        void get(int ms, std::vector<float>& audio);
        std::vector<float> loadAudioFile(const char* filename);

    private:
        SDL_AudioDeviceID m_dev_id_in = 0;

        int m_len_ms = 0;
        int m_sample_rate = 0;

        std::atomic_bool m_running;
        std::mutex m_mutex;

        //std::vector<float> m_audio;
        std::vector<float> m_audio_new;
        boost::circular_buffer<float> m_audio_buffer;
        size_t m_audio_pos = 0;
        size_t m_audio_len = 0;
    };
}

