#include "audio_async.hpp"
#include "sndfile.h"

bool audioSystem::AudioAsync::init(int capture_id, int sample_rate, int len_ms)
{

    m_len_ms = len_ms;

    m_running = false;
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetHintWithPriority(SDL_HINT_AUDIO_RESAMPLING_MODE, "medium", SDL_HINT_OVERRIDE);

    {
        int nDevices = SDL_GetNumAudioDevices(SDL_TRUE);
        fprintf(stderr, "%s: found %d capture devices:\n", __func__, nDevices);
        for (int i = 0; i < nDevices; i++)
        {
            fprintf(stderr, "%s:    - Capture device #%d: '%s'\n", __func__, i, SDL_GetAudioDeviceName(i, SDL_TRUE));
        }
    }

    SDL_AudioSpec capture_spec_requested;
    SDL_AudioSpec capture_spec_obtained;

    SDL_zero(capture_spec_requested);
    SDL_zero(capture_spec_obtained);

    capture_spec_requested.freq = sample_rate;
    capture_spec_requested.format = AUDIO_F32;
    capture_spec_requested.channels = 1;
    capture_spec_requested.samples = 1024;
    capture_spec_requested.callback = [](void *userdata, uint8_t *stream, int len) {
        auto audio = static_cast<AudioAsync *>(userdata);
        audio->callback(stream, len);
    };
    capture_spec_requested.userdata = this;

    if (capture_id >= 0)
    {
        fprintf(stderr, "%s: attempt to open capture device %d : '%s' ...\n", __func__, capture_id,
                SDL_GetAudioDeviceName(capture_id, SDL_TRUE));
        m_dev_id_in = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(capture_id, SDL_TRUE), SDL_TRUE,
                                          &capture_spec_requested, &capture_spec_obtained, 0);
    }
    else
    {
        fprintf(stderr, "%s: attempt to open default capture device ...\n", __func__);
        m_dev_id_in = SDL_OpenAudioDevice(nullptr, SDL_TRUE, &capture_spec_requested, &capture_spec_obtained, 0);
    }

    if (!m_dev_id_in)
    {
        fprintf(stderr, "%s: couldn't open an audio device for capture: %s!\n", __func__, SDL_GetError());
        m_dev_id_in = 0;

        return false;
    }
    fprintf(stderr, "%s: obtained spec for input device (SDL Id = %d):\n", __func__, m_dev_id_in);
    fprintf(stderr, "%s:     - sample rate:       %d\n", __func__, capture_spec_obtained.freq);
    fprintf(stderr, "%s:     - format:            %d (required: %d)\n", __func__, capture_spec_obtained.format,
            capture_spec_requested.format);
    fprintf(stderr, "%s:     - channels:          %d (required: %d)\n", __func__, capture_spec_obtained.channels,
            capture_spec_requested.channels);
    fprintf(stderr, "%s:     - samples per frame: %d\n", __func__, capture_spec_obtained.samples);

    m_sample_rate = capture_spec_obtained.freq;

    //m_audio.resize((m_sample_rate*m_len_ms)/1000);

    m_audio_buffer.resize(m_sample_rate * m_len_ms / 1000);
    return true;
}

bool audioSystem::AudioAsync::resume()
{
    if (!m_dev_id_in)
    {
        fprintf(stderr, "%s: no audio device to resume!\n", __func__);
        return false;
    }

    if (m_running)
    {
        fprintf(stderr, "%s: already running!\n", __func__);
        return false;
    }

    SDL_PauseAudioDevice(m_dev_id_in, 0);

    m_running = true;

    return true;
}

bool audioSystem::AudioAsync::pause()
{
    if (!m_dev_id_in)
    {
        fprintf(stderr, "%s: no audio device to pause!\n", __func__);
        return false;
    }

    if (!m_running)
    {
        fprintf(stderr, "%s: already paused!\n", __func__);
        return false;
    }

    SDL_PauseAudioDevice(m_dev_id_in, 1);

    m_running = false;

    return true;
}

bool audioSystem::AudioAsync::clear()
{
    if (!m_dev_id_in)
    {
        fprintf(stderr, "%s: no audio device to clear!\n", __func__);
        return false;
    }

    if (!m_running)
    {
        fprintf(stderr, "%s: not running!\n", __func__);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
    }

    return true;
}

// callback to be called by SDL
void audioSystem::AudioAsync::callback(uint8_t *stream, int len)
{
    if (!m_running)
    {
        return;
    }

    const size_t n_samples = len / sizeof(float);

    std::lock_guard<std::mutex> lock(m_mutex);

    // Check if there is enough space in the buffer
    if (n_samples > m_audio_buffer.capacity())
    {
        m_audio_buffer.set_capacity(n_samples);
    }

    // Push new audio data to the buffer
    for (size_t i = 0; i < n_samples; i++)
    {
        m_audio_buffer.push_back(*(reinterpret_cast<float *>(&stream[i * sizeof(float)])));
    }
}

void audioSystem::AudioAsync::get(int ms, std::vector<float> &result)
{
    if (!m_dev_id_in)
    {
        fprintf(stderr, "%s: no audio device to get audio from!\n", __func__);
        return;
    }

    if (!m_running)
    {
        fprintf(stderr, "%s: not running!\n", __func__);
        return;
    }

    result.clear();

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (ms <= 0)
        {
            ms = m_len_ms;
        }

        size_t n_samples = (m_sample_rate * ms) / 1000;

        // TODO: error happens here fix needed vector too long
        result.resize(n_samples);

        if (n_samples > m_audio_buffer.size())
        {
            n_samples = m_audio_buffer.size();
            //printf("critical error the audio requested was higher then the recorded buffer size");
        }
        result.assign(m_audio_buffer.end() - n_samples, m_audio_buffer.end());
    }
}
std::vector<float> audioSystem::AudioAsync::loadAudioFile(const char *filename)
{
    SF_INFO info;
    SNDFILE *file = sf_open(filename, SFM_READ, &info);
    if (!file)
    {
        // handle error
        auto err = sf_strerror(file);
        return {};
    }

    std::vector<float> pcmF32Data(info.frames * info.channels);
    sf_readf_float(file, pcmF32Data.data(), info.frames);
    sf_close(file);

    return pcmF32Data;
}
