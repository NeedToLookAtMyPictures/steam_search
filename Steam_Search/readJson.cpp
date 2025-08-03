//
// Created by kkati on 7/27/2025.
//

#include <fstream>
#include <iostream>
#include "readJson.h"

using json = nlohmann::json;
using namespace std;

// remove ™ ® © from a given string and return cleaned version
string cleanName(string name) {
    static const vector<string> unwanted = {"©", "®", "™"};
    for (const auto& symbol : unwanted) {
        size_t pos;
        while ((pos = name.find(symbol)) != string::npos) {
            name.erase(pos, symbol.size());
        }
    }
    return name;
}

// used each run for converting the data in steam_games.json into a manipulable object
void readJson(json& dataJSON, unordered_map<string, Game>& allGames) {

    for (const auto& [game_id, game_info] : dataJSON.items()) {
        // iterates 111452 times (amount of games)
        string name = game_info.value("name", "");
        name = cleanName(name);

        allGames[name].setID(stoi(game_id));
        allGames[name].setPrice(static_cast<double>(game_info.value("price", 0.0)));
        allGames[name].setRequiredAge(game_info.value("requiredAge", 0));

        vector<string> platforms;
        if (game_info.value("windows", false) == true) {
            platforms.emplace_back("windows");
        }
        if (game_info.value("mac",false) == true) {
            platforms.emplace_back("mac");
        }
        if (game_info.value("linux",false) == true) {
            platforms.emplace_back("linux");
        }
        allGames[name].setPlatform(platforms);

        allGames[name].setMetacriticScore(game_info.value("metacritic_score", -1));

        vector<string> languages;
        for (auto& [trash1, trash] : game_info["supported_languages"].items()) {
            string language = to_string(trash).substr(1, to_string(trash).size()-2);
            languages.push_back(language);
        }
        allGames[name].setSupportedLanguages(languages);

        vector<string> audioLanguages;
        for (auto& [trash1, trash] : game_info["full_audio_languages"].items()) {
            string audio = to_string(trash).substr(1, to_string(trash).size()-2);
            audioLanguages.push_back(audio);
        }
        allGames[name].setFullAudioLanguages(audioLanguages);

        vector<string> developers;
        for (auto& [trash1, trash] : game_info["developers"].items()) {
            string developer = to_string(trash).substr(1, to_string(trash).size()-2);
            developers.push_back(developer);
        }
        allGames[name].setDevelopers(developers);

        vector<string> publishers;
        for (auto& [trash1, trash] : game_info["publishers"].items()) {
            string publisher = to_string(trash).substr(1, to_string(trash).size()-2);
            publishers.push_back(publisher);
        }
        allGames[name].setPublisher(publishers);

        vector<string> categories;
        for (auto& [trash1, trash] : game_info["categories"].items()) {
            string category = to_string(trash).substr(1, to_string(trash).size()-2);
            categories.push_back(category);
        }
        allGames[name].setCategories(categories);

        vector<string> genres;
        for (auto& [trash1, trash] : game_info["genres"].items()) {
            string genre = to_string(trash).substr(1, to_string(trash).size()-2);
            genres.push_back(genre);
        }
        allGames[name].setGenres(genres);

        unordered_map<string, int> tags;
        for (const auto& [tag_name, vote_count] : game_info["tags"].items()) {
            for (const auto& [trash2, trash] : vote_count.items()) {
                string votes = to_string(trash);
                if (votes == "0") {
                    continue;
                }
                tags[tag_name] = stoi(votes);
            }
        }
        allGames[name].setTags(tags);

        int positive = game_info.value("positive", 0);
        int negative = game_info.value("negative", 0);
        allGames[name].setReviewScore(positive, negative);

        allGames[name].setImageURL(game_info.value("header_image", ""));
    }
}

unordered_map<string, int> readTags(const string& file) {
    unordered_map<string, int> indexedTags;

    //put tags in map
    ifstream tagFile(file);
    if (!tagFile.is_open()) {
        cerr << "error in opening tag file" << endl;
    }

    string currentTag;
    int index = 0;

    while (getline(tagFile, currentTag)) {
        indexedTags[currentTag] = index;
        index++;
    }

    if (indexedTags.empty()) {
        return indexedTags;
    }

    return indexedTags;
}

// used for initally populating our decoder.txt, dataSet.txt, and tags.txt, LEGACY CODE
void collectMetrics(json& dataJSON, unordered_map<string, string>& decoder)
{
    for (const auto& [game_id, game_info] : dataJSON.items()) { // iterates 111452 times (amount of games)
        for (const auto& [game_name, trash] : game_info["name"].items()) {
            string name = to_string(trash).substr(1, to_string(trash).size()-2);
            decoder[game_id] = cleanName(name);
        }
    }
}

// used for initally populating our decoder.txt, dataSet.txt, and tags.txt, LEGACY CODE
void saveToFile(unordered_map<string,string>& decoder)
{
    ofstream outFile("../decoder.txt");
    for (const auto& [key, value] : decoder) {
        outFile << key << '\t' << value << '\n';
    }
}