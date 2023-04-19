#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

namespace featureExtractor
{
    class Tokenizer
    {
    public:
        Tokenizer(std::string model_path);
        std::vector<int> encode(const std::string& text) const;
        std::string decode(const std::vector<size_t>& tokens) const;
        int token_to_id(const std::string& token) const;

    private:
        nlohmann::json tokenizer_json_;
        std::string task_;
        std::string language_;
        int num_special_tokens_;
        std::unordered_map<std::string, int> special_tokens_;
        std::unordered_map<std::string, int> token_map_;
        std::unordered_map<std::string, int> token_to_id_;
    };

}

