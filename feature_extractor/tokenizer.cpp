#include "tokenizer.hpp"
#include <boost/tokenizer.hpp>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

featureExtractor::Tokenizer::Tokenizer(std::string model_path)
    : num_special_tokens_(0)
{

    std::string tokenizer_file = model_path + "/tokenizer.json";
    try
    {
        std::ifstream ifs(tokenizer_file);
        if (ifs)
        {
            tokenizer_json_;
            ifs >> tokenizer_json_;

            #pragma omp parallel for
            for (const auto &token : tokenizer_json_["added_tokens"])
            {
                token_to_id_[token["content"]] = token["id"];
            }
        }
    }
    catch (int e)
    {
        std::cout << "Tokenizer not working\n";
    }
}

std::vector<int> featureExtractor::Tokenizer::encode(const std::string &text) const
{
    int num_tokens_ = 0;
    std::vector<int> encoded_text;
    boost::tokenizer<boost::char_separator<char>> tokenizer(text, boost::char_separator<char>(" "));
    for (const auto &token : tokenizer)
    {
        if (special_tokens_.count(token))
        {
            encoded_text.push_back(special_tokens_.at(token));
        }
        else
        {
            encoded_text.push_back(++num_tokens_);
        }
    }
    return encoded_text;
}

std::string featureExtractor::Tokenizer::decode(const std::vector<size_t> &tokens) const
{
    std::string decoded_text;
    for (const auto &token : tokens)
    {
        if (token <= num_special_tokens_)
        {
            // Special token, skip
            continue;
        }
        const auto &it =
            std::find_if(token_map_.begin(), token_map_.end(), [&](const auto &item) { return item.second == token; });
        if (it != token_map_.end())
        {
            decoded_text += it->first;
        }
    }
    return decoded_text;
}

int featureExtractor::Tokenizer::token_to_id(const std::string &token) const
{
    auto it = token_to_id_.find(token);
    if (it != token_to_id_.end())
    {
        return it->second;
    }
    else
    {
        return -1; // Token not found
    }
}
