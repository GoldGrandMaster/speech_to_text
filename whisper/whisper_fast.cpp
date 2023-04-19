#include "whisper_fast.hpp"
#include "Instrumentor.hpp"
#include "ctranslate2/storage_view.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

whisper::WhisperFast::WhisperFast(std::string model) : whisper_model(model, ctranslate2::Device::CPU), tokenizer(model)
{
    feature = featureExtractor::FeatureExtractor();
    prompts_ = feature.get_prompt(tokenizer);

    // init model options
    options_.beam_size = 5;
    options_.patience = 1;
    options_.length_penalty = 1;
    options_.max_length = 448;
    options_.return_scores = true;
    options_.return_no_speech_prob = true;
    options_.suppress_tokens = std::vector<int>{-1};
    options_.suppress_blank = true;
    options_.max_initial_timestamp_index = 50;
}

int whisper::WhisperFast::transcribe()
{
    return 0;
}

std::string whisper::WhisperFast::generate(std::vector<float> pcmf32)
{
    auto features = feature.extract(pcmf32, true);
    Timer timer_other("other");
    auto content_frames = features[0].size() - feature.nb_max_frames;
    auto seek = 0;
    std::string text = "";
    while (seek < content_frames)
    {

        std::vector<std::vector<float>> segment(features.size(),
                                                std::vector<float>(feature.nb_max_frames));
        int end = seek + feature.nb_max_frames;
        for (int i = 0; i < features.size(); i++)
        {
            for (int j = seek; j < end; j++)
            {
                segment[i][j - seek] = features[i][j];
            }
        }
        timer_other.Stop();
        Timer timer_storage("storage");
        auto features = get_ctranslate2_storage(segment);
        timer_storage.Stop();
        Timer timer_whis_generate("whis generate");
        auto result = whisper_model.generate(features, prompts_, options_);
        timer_whis_generate.Stop();
        Timer timer("inference");
        auto res = result[0].get();
        timer.Stop();
        auto tokens = res.sequences_ids[0];
        text = tokenizer.decode(tokens);
        std::cout << text << "\n";
        seek = content_frames;
    }

	// TODO: return text here
    return text;
}

ctranslate2::StorageView whisper::WhisperFast::get_ctranslate2_storage(std::vector<std::vector<float>> &segment)
{
    auto device = ctranslate2::Device::CPU;
    auto dtype = ctranslate2::DataType::FLOAT32;

    int size_x = segment.size();
    int size_y = segment[0].size();
    std::vector<float> new_data;
    new_data.reserve(size_x * size_y);

    for (auto i = segment.begin(); i != segment.end(); ++i)
    {
        for (auto j = i->begin(); j != i->end(); ++j)
        {
            new_data.push_back(std::move(*j));
        }
    }

    ctranslate2::Shape new_shape({1, size_x, size_y});
    ctranslate2::StorageView view(new_shape, std::move(new_data), device);

    return view;
}

std::vector<std::vector<float>> whisper::WhisperFast::storage_to_vectors(const ctranslate2::StorageView &storage)
{
    auto shape = storage.shape();
    const float *data = storage.data<float>();
    std::vector<std::vector<float>> vectors;
    for (int i = 0; i < storage.dim(1); ++i)
    {
        std::vector<float> row;
        for (int j = 0; j < storage.dim(2); ++j)
        {
            row.push_back(data[i * storage.dim(2) + j]);
        }
        vectors.push_back(row);
    }
    return vectors;
}

std::vector<std::vector<float>> whisper::WhisperFast::read_csv_matrix(const char *file_name)
{
    // Open the CSV file
    std::ifstream file(file_name);

    // Initialize a vector to hold the rows
    std::vector<std::vector<float>> data;

    // Read the file line by line
    std::string line;
    while (std::getline(file, line))
    {
        std::vector<float> row;

        // Use a stringstream to parse the comma-separated values
        std::stringstream ss(line);
        std::string value;
        while (std::getline(ss, value, ','))
        {
            row.push_back(std::stof(value));
        }

        // Add the row to the data vector
        data.push_back(row);
    }
    return data;
}
