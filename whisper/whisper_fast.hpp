#pragma once
#include "ctranslate2/models/whisper.h"
#include "feature_extractor.hpp"

namespace whisper
{
    class WhisperFast
    {
    private:
        std::vector<std::vector<size_t>> prompts_;
        ctranslate2::models::WhisperOptions options_;

    public:
        featureExtractor::Tokenizer tokenizer;
        featureExtractor::FeatureExtractor feature;
        WhisperFast() = default;
        WhisperFast(std::string model);
        ctranslate2::models::Whisper whisper_model;
        int transcribe();
        std::string generate(std::vector<float> pcmf32);
        ctranslate2::StorageView get_ctranslate2_storage(std::vector<std::vector<float>>& segment);
        std::vector<std::vector<float>> storage_to_vectors(const ctranslate2::StorageView& storage);
        std::vector<std::vector<float>> read_csv_matrix(const char* file);
    };

}

