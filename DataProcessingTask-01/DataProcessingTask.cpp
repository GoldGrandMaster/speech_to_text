// data_processing_task.cpp : Defines the entry point for the application.
//

#include "DataProcessingTask.h"
#include "ctranslate2/models/whisper.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>
#include <filesystem>


std::vector<std::vector<float>> read_csv_matrix(const char* file_name)
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

/*
 * TODO: the task for you is to complete the following function
 *
 * input: a (80, 3000) metrix
 *
 * output: StorageView type
 *
 * hint: go to definition of StorageView
 */
ctranslate2::StorageView get_ctranslate2_storage(std::vector<std::vector<float>>& segment)
{
    auto device = ctranslate2::Device::CPU;
    auto dtype = ctranslate2::DataType::FLOAT32;

    // put your code here

    ctranslate2::StorageView view(dtype, device);

    // put your code here

    return view;
}


int main()
{
    // init and define options for whisper
    ctranslate2::models::WhisperOptions options;
    options.beam_size = 1;
    options.patience = 1;
    options.length_penalty = 1;
    options.max_length = 448;
    options.return_scores = true;
    options.return_no_speech_prob = true;
    options.suppress_blank = true;
    options.max_initial_timestamp_index = 50;

    // define prompts
    std::vector<std::vector<size_t>> prompts{ std::vector<size_t>{ 50257 } };

    // load the whisper model
    ctranslate2::models::Whisper whisper_model("C:/dev/Resources/Models/whisper-tiny.en-ct2", ctranslate2::Device::CPU);

    // load a 30 second data sample 
    auto segment = read_csv_matrix("D:/Git/DataProcessingTask-01/audio_test.csv");

    // load the vector<vector<float>> type into a ctranslate2::StorageView type
    auto features = get_ctranslate2_storage(segment);

    // perform inference
    auto result = whisper_model.generate(features, prompts, options);
    auto res = result[0].get();

    // print the result of the test
	std::cout << res.sequences[0][1] << std::endl;
    
    if (res.sequences[0][1] == "ĠHello") std::cout << "Good job, go collect your prize!";
    else std::cout << "Failed, sheesh gotta try again!";

	return 0;
}
