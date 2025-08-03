#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <queue>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>
#include <limits>
#include <iomanip>
#include <chrono>

#include "Game.h"
#include "readJson.h"
#include "minHash.h"
#include "jaccardsSimilarity.h"
#include "cosineSimilarity.h"
#include "multiFeatureSimilarity.h"
#include "algorithms_B.h"
#include "RapidFuzzie.h"

using json = nlohmann::json;
using namespace std;

// NOTICE: the CLI does not print foreign language characters correctly. Keep that in mind when there is input that
// looks unintelligible. That is always an issue with the CLI. We already removed ™ ® © during preprocessing. But it
// doesn't feel proper to replace accented characters or other foregin language characters from games, it's too
// transformative. To account for this we implemented a fuzzy matching system so it shouldn't be a problems
void showLogo() {
    string steam[] = {
        "       _____ _____ ______          __   __ ",
        "      / ____|_   _|  ____|   /\\   |  \\/  |",
        "     | (___   | | | |__     /  \\  | \\  / |",
        "      \\___ \\  | | |  __|   / /\\ \\ | |\\/| |",
        "      ____) | | | | |____ / ____ \\| |  | |",
        "     |_____/  |_| |______/_/    \\_\\_|  |_|"
    };

    string search[] = {
        "  _____  ______           _____   _____  __   __   ",
        " / ____||  ____|   /\\    |    \\\\ (  ___)| |  | | ",
        "| (___  | |__     /  \\   | |___)|| |    | |__| |  ",
        " \\___ \\ |  __|   / /\\    |  ___/ | |    | |__| |  ",
        " ____) || |____ / ____ \\ | |  \\\\ | |___ | |  | | ",
        "|_____/ |______/_/    \\_\\|_|   \\\\(_____)|_|  |_| "
    };


    cout << endl;

    for (auto& line : steam) {
        cout << line << endl;
        this_thread::sleep_for(chrono::milliseconds(40));
    }

    cout << endl;
    for (auto& line : search) {
        cout << line << endl;
        this_thread::sleep_for(chrono::milliseconds(40));
    }

    cout << "\n\nLoading..." << endl;
}

int main() {
    showLogo();

    cout << "\nPrepping dataset, and preprocessing data for algorithms. Please wait up to 2 minutes...\n" << endl;

    cout << "[.] Loading JSON file..." << flush;
    ifstream f("../games.json");
    if (!f.is_open()) {
        cout << "\n[X] Error: Could not open given JSON." << endl;
        return 1;
    }

    json dataJSON = json::parse(f);
    cout << "\r[1] JSON file loaded" << " (" << dataJSON.size()  << " Games" << ")" << endl;

    string source;

    cout << "\r[.] Populating game data..." << endl;

    unordered_map<string, Game> metaData;
    readJson(dataJSON, metaData); // Populate game data map
    cout << "[2] Finished populating game data." << endl;

    cout << "\r[.] Indexing tags..." << endl;

    string tagFile = "../tags.txt";
    unordered_map<string, int> indexedTags = readTags(tagFile);

    // update decoder from file, this allows us to map gameIDs to gameNames for quick lookup
    unordered_map<string, string> decoder;
    ifstream inFile("../decoder.txt");
    string line;
    while (getline(inFile, line)) {
        istringstream iss(line);
        if (string key, value; getline(iss, key, '\t') && getline(iss, value)) {
            decoder[key] = value;
        }
    }

    cout << "[3] Finished indexing tags." << endl;

    string response;
    while (response != "q" && response != "Q") {
        bool invalid = true;
        while (invalid) {
            // RapidFuzzy implementation
            RapidFuzzie fuzzie(metaData, 75.0);
            // call function to get correct game name
            source = fuzzie.getMatchedName();
            if (!source.empty()) {
                invalid = false;
            }

            if (source == "q") {
                cout << "\nThank you for using our program!\nCredits:\nBayan Mahmoodi\nKushagra Katiyar\nAgnivesh Kaundinya\nexiting..." << endl;
                return 0;
            }
        }

        cout << "\nWhat algorithm would you like for us to use: \n0 - Jaccard's Tag Similarity\n1 - Weighted Jaccard's Tag Similarity\n2 - Rule Based Decision Tree\n3 - Min Hashing\n4 - Cosine Similarity\n5 - Multi-Feature Similarity" << endl;
        cout << "-----------------------------"<< endl;

        int choice;
        while (true) {
            cout << "Your choice [0-5]: ";
            cin >> choice;
            if (cin.fail()) {
                cout << "Invalid input. Please enter a number." << endl;
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                continue;
            }
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            if (choice >= 0 && choice <= 5) {
                break;
            } else {
                cout << "Invalid choice (" << choice << "). Please try again." << endl;
            }
        }

        cout << "\nHow many games would you like displayed at a time: " << endl;
        cout << "--------------------------------------------------"<< endl;
        int num_games;
        while (true) {
            cout << "Number of games: ";
            cin >> num_games;
            if (cin.fail()) {
                cout << "Invalid input. Please enter a positive number." << endl;
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                continue;
            }
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            if (num_games > 0) {
                 if (choice == 2 && num_games > 500) {
                    cout << num_games << " is an invalid display amount... Printing 500 games\n" << endl;
                    num_games = 500;
                }
                cout << "Please wait..." << endl;
                break;
            } else {
                cout << "Invalid number of games... Please enter a positive number.\n";
            }
        }

        chrono::milliseconds setup_duration(0), run_duration(0);

        // declaring all varaibles used within the switch-case, needed as switches don't allow object declaration
        string compare;
        int i;
        priority_queue<pair<double, string>> maxHeap;
        vector<string> rankings;
        algorithms_b DecisionTree;
        priority_queue<pair<double,string>> similarGames;
        unordered_map<string, vector<int>> allSignatures;
        vector<int>* sourceSignature = nullptr;
        minHash minHash(150, indexedTags);
        priority_queue<pair<double, string>> cosineHeap;
        cosineSimilarity cosineSim(indexedTags);
        Game* sourceGame = nullptr;
        // Max-heap to store (similarity, gameName) pairs, ordered by similarity (highest first)
        priority_queue<pair<double, string>> topSimilarGames;
        // Define importance weights for the multi-feature algorithm
        double weightTags = 0.5;
        double weightPublishers = 0.1;
        double weightDevelopers = 0.1;
        double weightReviewScore = 0.3;

        switch (choice) {
            case 0: { // Jaccards (unweighted)
                auto start_run = chrono::high_resolution_clock::now();

                // clears maxHeap if necessary
                while(!maxHeap.empty())
                {
                    maxHeap.pop();
                }

                for (const auto& [key, value] : decoder) {
                    if (value != source && metaData.contains(value)) {
                        compare = value; // This line creates a non-const copy
                        maxHeap.emplace(jaccardsSimilarity(source, compare, metaData), value);
                    }
                }
                auto stop_run = chrono::high_resolution_clock::now();
                run_duration = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);

                cout << "\nPrinting top " << num_games << " most similar games:" << endl;
                cout << "------------------------------------" << endl;
                for (i = 0; i < num_games && !maxHeap.empty(); i++) {
                    cout << maxHeap.top().second << endl;
                    maxHeap.pop();
                }

                cout << "-------------------"<< endl;
                cout << "Jaccards:" << endl;
                cout << "Running Time:  " << run_duration.count() << " ms" << endl;
                cout << "Total Time:    " << (setup_duration + run_duration).count() << " ms" << endl;

                cout << "\nq - quit; m - print " << num_games << " more games; r - return to main menu" << endl;
                cin >> response;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                while (response == "m" || response == "M") {
                    if (maxHeap.empty()) {
                        cout << "No more games to display." << endl;
                        break;
                    }
                    cout << "\n";
                    for (i = 0; i < num_games && !maxHeap.empty(); i++) {
                        cout << maxHeap.top().second << endl;
                        maxHeap.pop();
                    }
                    cout << "\nq - quit; m - print " << num_games << " more games; r - return to main menu" << endl;
                    cin >> response;
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                }
                break;
            }
            case 1: { // Jaccard (weighted)
                auto start_run = chrono::high_resolution_clock::now();

                // clears maxHeap if necessary
                while(!maxHeap.empty())
                {
                    maxHeap.pop();
                }

                for (const auto& [key, value] : decoder) {
                    if (value != source && metaData.contains(value)) {
                        compare = value; // This line creates a non-const copy
                        maxHeap.emplace(jaccardsSimilarityWeighted(source, compare, metaData), value);
                    }
                }
                auto stop_run = chrono::high_resolution_clock::now();
                run_duration = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);

                cout << "\nPrinting top " << num_games << " most similar games:" << endl;
                cout << "------------------------------------" << endl;
                for (i = 0; i < num_games && !maxHeap.empty(); i++) {
                    cout << maxHeap.top().second << endl;
                    maxHeap.pop();
                }

                cout << "-------------------"<< endl;
                cout << "Weighted Jaccards:" << endl;
                cout << "Running Time:  " << run_duration.count() << " ms" << endl;
                cout << "Total Time:    " << (setup_duration + run_duration).count() << " ms" << endl;

                cout << "\nq - quit; m - print " << num_games << " more games; r - return to main menu" << endl;
                cin >> response;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                while (response == "m" || response == "M") {
                    if (maxHeap.empty()){
                        cout << "No more games to display." << endl;
                        break;
                    }
                    cout << "\n";
                    for (i = 0; i < num_games && !maxHeap.empty(); i++) {
                        cout << maxHeap.top().second << endl;
                        maxHeap.pop();
                    }
                    cout << "\nq - quit; m - print " << num_games << " more games; r - return to main menu" << endl;
                    cin >> response;
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                }
                break;
            }
            case 2: { // Decision Tree
                auto start_run = chrono::high_resolution_clock::now();
                rankings = DecisionTree.decisionTree(source, metaData, num_games);
                auto stop_run = chrono::high_resolution_clock::now();
                run_duration = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);

                cout << "\nPrinting top " << num_games << " most similar games:" << endl;
                cout << "------------------------------------" << endl;
                for (i = 0; i < num_games && i < rankings.size(); i++) {
                    cout << rankings[i] << endl;
                }

                cout << "-------------------"<< endl;
                cout << "Decision Tree:" << endl;
                cout << "Running Time:  " << run_duration.count() << " ms" << endl;
                cout << "Total Time:    " << (setup_duration + run_duration).count() << " ms" << endl;

                cout << "\nq - quit; m - print " << num_games << " more games; r - return to main menu" << endl;
                cin >> response;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                while (response == "m" || response == "M") {
                    if (i >= rankings.size() && !DecisionTree.isHeapEmpty()) {
                        cout << "\nNo more games to display in this ranking. Would you like to calculate more? [y/n]" << endl;
                        cin >> response;
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        if (response == "y") {
                            cout << "\nPlease Wait..." << endl;
                            i = 0;
                            auto start_run_new = chrono::high_resolution_clock::now();
                            rankings = DecisionTree.decisionTreeNext(source, metaData, num_games);
                            auto stop_run_new = chrono::high_resolution_clock::now();
                            run_duration = chrono::duration_cast<chrono::milliseconds>(stop_run_new - start_run_new);

                            cout << "\nPrinting new batch of games:" << endl;
                            cout << "------------------------------------" << endl;
                            for (int j = 0; j < num_games && i < rankings.size(); j++, i++) {
                                cout << rankings[j] << endl;
                            }

                            cout << "-------------------"<< endl;
                            cout << "New Batch Decision Tree Speed:" << endl;
                            cout << "Running Time:  " << run_duration.count() << " ms" << endl;

                            cout << "\nq - quit; m - print " << num_games << " more games; r - return to main menu" << endl;
                            cin >> response;
                            cin.ignore(numeric_limits<streamsize>::max(), '\n');
                            continue;
                        } else {
                           // THE FIX IS HERE: Treat 'n' or anything else as 'r'
                           response = "r";
                           break;
                        }
                    }
                    if (i >= rankings.size() || DecisionTree.isHeapEmpty()) {
                        cout << "\nNo more games to display." << endl;
                        break;
                    }
                    cout << "\n";
                    for (int j = 0; j < num_games && i < rankings.size(); j++, i++) {
                        cout << rankings[i] << endl;
                    }
                    cout << "\nq - quit; m - print " << num_games << " more games; r - return to main menu" << endl;
                    cin >> response;
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                }
                break;
            }
            case 3: { // Min Hash
                auto start_setup = chrono::high_resolution_clock::now();
                //minhashing preprep
                allSignatures.clear();
                for (const auto& pair : metaData) {
                    allSignatures[pair.first] = minHash.createSignature(pair.second);
                }
                auto stop_setup = chrono::high_resolution_clock::now();

                auto start_run = chrono::high_resolution_clock::now();
                sourceSignature = &allSignatures[source];
                // clears similarGames if necessary
                while(!similarGames.empty()) { similarGames.pop(); }
                for (const auto& pair : allSignatures) {
                    if (pair.first != source) {
                        similarGames.emplace(minHash.miniJaccards(*sourceSignature, pair.second), pair.first);
                    }
                }
                auto stop_run = chrono::high_resolution_clock::now();

                setup_duration = chrono::duration_cast<chrono::milliseconds>(stop_setup - start_setup);
                run_duration = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);

                cout << "\nPrinting top " << num_games << " most similar games:" << endl;
                cout << "------------------------------------" << endl;
                for (i = 0; i < num_games && !similarGames.empty() ; i++) {
                    cout << similarGames.top().second << endl;
                    similarGames.pop();
                }

                cout << "-------------------"<< endl;
                cout << "Min-Hash:" << endl;
                cout << "Setup Time:    " << setup_duration.count() << " ms" << endl;
                cout << "Running Time:  " << run_duration.count() << " ms" << endl;
                cout << "Total Time:    " << (setup_duration + run_duration).count() << " ms" << endl;

                cout << "\nq - quit; m - print " << num_games << " more games; r - return to main menu" << endl;
                cin >> response;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                while (response == "m" || response == "M") {
                    if (similarGames.empty()) {
                        cout << "No more games to display." << endl;
                        break;
                    }
                    cout << "\n";
                    for (i = 0; i < num_games && !similarGames.empty() ; i++) {
                        cout << "Similarity: " << similarGames.top().first << "  |  Game: " << similarGames.top().second << endl;
                        similarGames.pop();
                    }
                    cout << "\nq - quit; m - print " << num_games << " more games; r - return to main menu" << endl;
                    cin >> response;
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                }
                break;
            }
            case 4: { // Cosine Similarity
                auto start_setup = chrono::high_resolution_clock::now();
                cosineSim.createGameSignatures(metaData);
                auto stop_setup = chrono::high_resolution_clock::now();

                auto start_run = chrono::high_resolution_clock::now();
                for (const auto& pair : metaData) {
                    if (pair.first != source) {
                        cosineHeap.emplace(cosineSim.similarity(source, pair.first), pair.first);
                    }
                }
                auto stop_run = chrono::high_resolution_clock::now();

                setup_duration = chrono::duration_cast<chrono::milliseconds>(stop_setup - start_setup);
                run_duration = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);

                cout << "\nPrinting top " << num_games << " most similar games:" << endl;
                cout << "------------------------------------" << endl;
                for (i = 0; i < num_games && !cosineHeap.empty() ; i++) {
                    cout << cosineHeap.top().second  << endl;
                    cosineHeap.pop();
                }

                cout << "-------------------"<< endl;
                cout << "Cosine Similarity:" << endl;
                cout << "Setup Time:    " << setup_duration.count() << " ms" << endl;
                cout << "Running Time:  " << run_duration.count() << " ms" << endl;
                cout << "Total Time:    " << (setup_duration + run_duration).count() << " ms" << endl;

                cout << "\nq - quit; m - print " << num_games << " more games; r - return to main menu" << endl;
                cin >> response;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                while (response == "m" || response == "M") {
                    if (cosineHeap.empty()) {
                        cout << "No more games to display." << endl;
                        break;
                    }
                    cout << "\n";
                    for (i = 0; i < num_games && !cosineHeap.empty() ; i++) {
                        cout << "Similarity: " << cosineHeap.top().first << "  |  Game: " << cosineHeap.top().second  << endl;
                        cosineHeap.pop();
                    }
                    cout << "\nq - quit; m - print " << num_games << " more games; r - return to main menu" << endl;
                    cin >> response;
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                }
                break;
            }
            case 5: { // Multi-Feature Similarity
                auto start_run = chrono::high_resolution_clock::now();

                // TODO: check for any bugs
                sourceGame = &metaData[source];
                // Iterate through all other games in metaData
                for (const auto& pair : metaData) {
                    // Skip comparing the game with itself
                    if (pair.first == source) { continue; }
                    const Game& compareGame = pair.second;
                    // Calculate the overall weighted similarity
                    double similarity = calculateOverallWeightedSimilarity(*sourceGame, compareGame, weightTags, weightPublishers, weightDevelopers, weightReviewScore);
                    // Add to the priority queue
                    topSimilarGames.emplace(similarity, pair.first);
                }
                auto stop_run = chrono::high_resolution_clock::now();
                run_duration = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);

                // prints similar games
                cout << "\nPrinting top " << num_games << " most similar games:" << endl;
                cout << "------------------------------------" << endl;
                for (i = 0; i < num_games && !topSimilarGames.empty() ; i++) {
                    cout << topSimilarGames.top().second << endl;
                    topSimilarGames.pop();
                }

                cout << "-------------------"<< endl;
                cout << "Multi-Feature Similarity:" << endl;
                cout << "Running Time:  " << run_duration.count() << " ms" << endl;
                cout << "Total Time:    " << (setup_duration + run_duration).count() << " ms" << endl;

                cout << "\nq - quit; m - print " << num_games << " more games; r - return to main menu" << endl;
                cin >> response;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                while (response == "m" || response == "M") {
                    if (topSimilarGames.empty()) {
                        cout << "No more games to display." << endl;
                        break;
                    }
                    cout << "\n";
                    for (i = 0; i < num_games && !topSimilarGames.empty() ; i++) {
                        cout << "Similarity: " << fixed << setprecision(4) << topSimilarGames.top().first << " | Game:  " << topSimilarGames.top().second << endl;
                        topSimilarGames.pop();
                    }
                    cout << "\nq - quit; m - print " << num_games << " more games; r - return to main menu" << endl;
                    cin >> response;
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                }
                break;
            }
        }


        if (response == "r" || response == "R") {
            continue;
        } else if (response == "q" || response == "Q") {
            cout << "\nThank you for using our program!\nCredits:\nBayan Mahmoodi\nKushagra Katiyar\nAgnivesh Kaundinya\nexiting..." << endl;
            break;
        } else if (response != "m" && response != "M") {
            // invalid command
            cout << "Invalid choice, returning to main menu..." << endl;
            continue;
        }
    }
    return 0;
}