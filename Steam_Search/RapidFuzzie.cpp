//
// Created by Agnivesh Kaundinya on 7/31/2025.
//

#include "RapidFuzzie.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <cctype>

#include <rapidfuzz/fuzz.hpp>

// Constructor Initializes values
RapidFuzzie::RapidFuzzie(const std::unordered_map<std::string, Game> &metaData, double threshold)
    : allGameMetaData(metaData), similarityThreshold(threshold) {
    success = false;
}

bool RapidFuzzie::getSuccess() {
    return success;
}

std::string RapidFuzzie::getMatchedName() {
    std::string inputGN;
    std::string selectedGameName = "";

    // Keep looping until a valid game is chosen or the user decides to quit.
    while (true) {
        std::cout << "\nPlease input the game you want recommendations based on (type 'q' to quit): " << std::endl;
        std::cout << "---------------------------------------------------------------------------" << endl;
        std::getline(std::cin, inputGN); // read line of full game name
        inputGN.erase(inputGN.find_last_not_of(" \t\r\n") + 1); // remove trailing or leading whitespace

        if (inputGN == "q" || inputGN == "Q") {
            success = false; // User chose to quit
            return "q";
        }

        // First, check for an exact match (case-sensitive, as per your original logic)
        if (allGameMetaData.count(inputGN)) { // Using .count() for direct key lookup
            success = true;
            return inputGN;
        }

        // Normalize user input to lowercase for case-insensitive fuzzy comparison
        std::string normalizedInputGN = inputGN;
        std::transform(normalizedInputGN.begin(), normalizedInputGN.end(), normalizedInputGN.begin(),
                       [](unsigned char c){ return std::tolower(c); });


        std::vector<std::string> allGameNames; // prepare a list of all the game names in the JSON
        for (const auto& pair : allGameMetaData) {
            allGameNames.push_back(pair.first);
        }

        if (allGameNames.empty()) { // if it's empty then return
            std::cout << "Game is not in the dataset." << std::endl;
            success = false;
            return ""; // Exit the loop and function
        }

        // Store all matches that meet the similarity threshold.
        // This will hold pairs of {similarity_score, original_game_name}.
        std::vector<std::pair<double, std::string>> potentialMatches;

        // Iterate through all available names to find matches
        for (const std::string& currChoice : allGameNames) {
            // Normalize database game name to lowercase for case-insensitive comparison
            std::string normalizedCurrChoice = currChoice;
            std::transform(normalizedCurrChoice.begin(), normalizedCurrChoice.end(), normalizedCurrChoice.begin(),
                           [](unsigned char c){ return std::tolower(c); });

            // Calculate similarity score between normalized strings
            double currScore = rapidfuzz::fuzz::ratio(normalizedInputGN, normalizedCurrChoice);

            // If the score meets the threshold, add it to potential matches
            if (currScore >= similarityThreshold) {
                potentialMatches.push_back({currScore, currChoice});
            }
        }

        // Sort potential matches by score in descending order (highest score first)
        std::sort(potentialMatches.begin(), potentialMatches.end(),
                  [](const auto& a, const auto& b) {
                      return a.first > b.first; // Sort by score, descending
                  });

        // Define how many top suggestions to print
        int maxSuggestionsToPrint = 3; // You can change this to 2, 5, etc.

        if (!potentialMatches.empty()) {
            std::cout << "Did you mean one of these?" << std::endl;
            std::cout << "--------------------------" << std::endl;

            for (int i = 0; i < potentialMatches.size() && i < maxSuggestionsToPrint; ++i) {
                std::cout << (i + 1) << ". '" << potentialMatches[i].second << "'" << endl;
            }

            // Prompt user to select one of the suggestions or re-enter
            std::cout << "--------------------------" << std::endl;
            std::cout << "Enter the number of your choice, or 0 to re-enter a new game: ";
            int choiceNum;

            // Use a loop for robust numerical input, clearing error flags if non-numeric input is given.
            while (!(std::cin >> choiceNum)) {
                std::cout << "Invalid input. Please enter a number: ";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard invalid input
            }
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear leftover newline

            // Process user's choice.
            if (choiceNum > 0 && choiceNum <= potentialMatches.size() && choiceNum <= maxSuggestionsToPrint) {
                selectedGameName = potentialMatches[choiceNum - 1].second;
                success = true; // A valid game was chosen
                std::cout << "You selected: '" << selectedGameName << "'." << std::endl;
                return selectedGameName; // Exit the loop and function with the selected name
            } else if (choiceNum == 0) {

                // Loop continues to re-prompt for inputGN
            } else {
                std::cout << "Invalid choice. Please enter a valid number from the list or 0 to re-enter." << std::endl;
                // Loop continues to re-prompt for inputGN
            }
        }
        else {
            // No matches found above the similarity threshold.
            std::cout << "No close matches found for your input above." << endl;
            std::cout << "Please try again with a more accurate name." << std::endl;

            // Offer to re-enter or return empty.
            std::cout << "Enter 0 to re-enter game name, or any other number to exit: ";
            int choiceNum;
            while (!(std::cin >> choiceNum)) {
                std::cout << "Invalid input. Please enter a number: ";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear newline

            if (choiceNum == 0) {
                // Loop continues to re-prompt for inputGN
            } else {
                success = false; // User chose to exitg
                return ""; // Exit the loop and function
            }
        }
    }
}
