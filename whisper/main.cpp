#include <iostream>

#include "Instrumentor.hpp"
#include "ctranslate2/models/whisper.h"
#include "audio_decoder.hpp"
#include "whisper_fast.hpp"
#include "whisper_stream.hpp"
#include "audio_async.hpp"


int test_stream()
{
	whisper::WhisperStream wis;
    wis.init();
    std::string str;

    bool isEnded = true;

    while (isEnded)
    {
        wis.process_audio();
        wis.get_last_transcribed(str);
        if (!str.empty())
        {
            std::cout << str << "\n";
        }
    }
    std::cin.get();
    return 0;
}


int test_fast()
{
    audioSystem::AudioAsync audio;
    std::vector<float> pcmf32;

    if (!audio.init(-1, 16000, 10000))
    {
        fprintf(stderr, "%s: audio.init() failed!\n", __func__);
        return 0;
    }
    audio.resume();

    std::string model_path = "C:/dev/Resources/Models/whisper-tiny.en-ct2";
    whisper::WhisperFast whisper_fast(model_path);

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    std::cout << "now\n";
    std::cin.get();
    audio.get(5000, pcmf32);
    Timer timer("generate");
    whisper_fast.generate(pcmf32);
    timer.Stop();
    return 0;
}

int test_file()
{
    audioSystem::AudioAsync audio;
    const char* path = "C:/Users/novel/Documents/Sound Recordings/Recording.wav";
    std::vector<float> pcmf32 = audioSystem::AudioDecoder::DecodeAudio(path);

    std::cout << "audio loaded\n";

    std::string model_path = "C:/dev/Resources/Models/whisper-tiny.en-ct2";
    whisper::WhisperFast whisper_fast(model_path);

    std::cout << "model loaded\n";

    //whisper_fast.generate(pcmf32);

    return 0;
}

int main()
{
    return test_stream();
}
