#include <fstream>
#include <iostream>

#include <unordered_map>
#include <queue>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <thread>
#include <limits>
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

    cout << "\nPrepping dataset, and preprocessing data for algorithms. Please wait...\n" << endl;

    cout << "[.] Loading JSON file..." << flush;
    ifstream f("../games.json");
    if (!f.is_open()) {
        cout << "\n[X] Error: Could not open given JSON." << endl;
        return 1;
    }

    json dataJSON = json::parse(f);
    cout << "[1] JSON file loaded" << " (" << dataJSON.size()  << " Games" << ")" << endl;

    string source;

    cout << "[.] Populating game data..." << endl;

    unordered_map<string, Game> metaData;
    readJson(dataJSON, metaData); // Populate game data map
    cout << "[2] Finished populating game data." << endl;

    cout << "[.] Indexing tags..." << endl;

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
                cout << "\nThank you for using Steam Search!\nCredits:\nBayan Mahmoodi\nKushagra Katiyar\nAgnivesh Kaundinya\n\nexiting..." << endl;
                return 0;
            }
        }

        //selecct the algo to use and error handle the input
        cout << "\nWhat algorithm would you like for us to use: \n0 - Jaccard's Tag Similarity\n1 - Weighted Jaccard's Tag Similarity\n2 - Rule Based Decision Tree\n3 - Min Hashing\n4 - Cosine Similarity\n5 - Multi-Feature Similarity\n6 - Compare Two Algorithms" << endl;
        cout << "-----------------------------"<< endl;

        int num_games;
        int choice;
        while (true) {
            cout << "Your choice [0-6]: ";
            cin >> choice;
            if (cin.fail() || choice < 0 && choice > 6) {
                cout << "Invalid choice (" << choice << "). Please try again." << endl;
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                continue;
            } else {
                break;
            }
        }

        //select how many games to be displayed and error handle input IF NOT CHOSEN OPTION 6
        if (choice >= 0 && choice <= 5) {
            cout << "\nHow many games would you like displayed at a time: " << endl;
            cout << "--------------------------------------------------"<< endl;
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
        //for comparision
        string algorithm_1;
        string algorithm_2;
        int alg1;
        int alg2;
        chrono::milliseconds setup_duration1(0), run_duration1(0), total_duration1(0);
        chrono::milliseconds setup_duration2(0), run_duration2(0), total_duration2(0);
        string alg1_name, alg2_name;
        priority_queue<pair<double, string>> temp_results_heap;
        vector<string> temp_results_vector;
        priority_queue<pair<double, string>> results_heap2;
        vector<string> results_vector2;

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
                    for (i = 0; i < num_games && !similarGames.empty() ; i++) {
                        cout << similarGames.top().second << endl;
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
                    for (i = 0; i < num_games && !cosineHeap.empty() ; i++) {
                        cout << cosineHeap.top().second  << endl;
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
                    for (i = 0; i < num_games && !topSimilarGames.empty() ; i++) {
                        cout << topSimilarGames.top().second << endl;
                        topSimilarGames.pop();
                    }
                    cout << "\nq - quit; m - print " << num_games << " more games; r - return to main menu" << endl;
                    cin >> response;
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                }
                break;
            }
            case 6: {
                cout << "\nSelect two different algorithms to compare:" << endl;
                cout << "0 - Jaccard's Tag Similarity\n1 - Weighted Jaccard's Tag Similarity\n2 - Rule Based Decision Tree\n3 - Min Hashing\n4 - Cosine Similarity\n5 - Multi-Feature Similarity" << endl;
                cout << "---------------------------"<< endl;
                // Get first algorithm choice
                while (true) {
                    cout << "First algorithm choice [0-5]: ";
                    cin >> alg1;
                    if (cin.fail() || alg1 < 0 || alg1 > 5) {
                        cout << "Invalid input. Please enter a number between 0 and 5." << endl;
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        continue;
                    }
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    break;
                }

                // Get second algorithm choice
                while (true) {
                    cout << "Second algorithm choice [0-5]: ";
                    cin >> alg2;
                    if (cin.fail() || alg2 < 0 || alg2 > 5) {
                        cout << "Invalid input. Please enter a number between 0 and 5." << endl;
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        continue;
                    }
                    if (alg1 == alg2) {
                        cout << "Invalid choice. Please select two different algorithms." << endl;
                        continue;
                    }
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    break;
                }

                num_games = 5; // Default to 5 games for comparison
                cout << "Please wait...\n" << endl;

                switch (alg1) {
                    case 0: { // Jaccards (unweighted)
                        alg1_name = "Jaccard's";
                        auto start_run = chrono::high_resolution_clock::now();
                        while(!maxHeap.empty()) { maxHeap.pop(); }
                        for (const auto& [key, value] : decoder) {
                            if (value != source && metaData.contains(value)) {
                                compare = value;
                                maxHeap.emplace(jaccardsSimilarity(source, compare, metaData), value);
                            }
                        }
                        auto stop_run = chrono::high_resolution_clock::now();
                        run_duration1 = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);
                        temp_results_heap = maxHeap;
                        break;
                    }
                    case 1: { // Jaccards (weighted)
                        alg1_name = "Weighted Jaccard's";
                        auto start_run = chrono::high_resolution_clock::now();
                        while(!maxHeap.empty()) { maxHeap.pop(); }
                        for (const auto& [key, value] : decoder) {
                            if (value != source && metaData.contains(value)) {
                                compare = value;
                                maxHeap.emplace(jaccardsSimilarityWeighted(source, compare, metaData), value);
                            }
                        }
                        auto stop_run = chrono::high_resolution_clock::now();
                        run_duration1 = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);
                        temp_results_heap = maxHeap;
                        break;
                    }
                    case 2: { // Decision Tree
                        alg1_name = "Decision Tree";
                        auto start_run = chrono::high_resolution_clock::now();
                        temp_results_vector = DecisionTree.decisionTree(source, metaData, num_games);
                        auto stop_run = chrono::high_resolution_clock::now();
                        run_duration1 = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);
                        break;
                    }
                    case 3: { // Min Hash
                        alg1_name = "Min-Hash";
                        auto start_setup = chrono::high_resolution_clock::now();
                        allSignatures.clear();
                        for (const auto& pair : metaData) {
                            allSignatures[pair.first] = minHash.createSignature(pair.second);
                        }
                        auto stop_setup = chrono::high_resolution_clock::now();
                        setup_duration1 = chrono::duration_cast<chrono::milliseconds>(stop_setup - start_setup);

                        auto start_run = chrono::high_resolution_clock::now();
                        while(!similarGames.empty()) { similarGames.pop(); }
                        sourceSignature = &allSignatures[source];
                        for (const auto& pair : allSignatures) {
                            if (pair.first != source) {
                                similarGames.emplace(minHash.miniJaccards(*sourceSignature, pair.second), pair.first);
                            }
                        }
                        auto stop_run = chrono::high_resolution_clock::now();
                        run_duration1 = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);
                        temp_results_heap = similarGames;
                        break;
                    }
                    case 4: { // Cosine Similarity
                        alg1_name = "Cosine Similarity";
                        auto start_setup = chrono::high_resolution_clock::now();
                        cosineSim.createGameSignatures(metaData);
                        auto stop_setup = chrono::high_resolution_clock::now();
                        setup_duration1 = chrono::duration_cast<chrono::milliseconds>(stop_setup - start_setup);

                        auto start_run = chrono::high_resolution_clock::now();
                        while(!cosineHeap.empty()) { cosineHeap.pop(); }
                        for (const auto& pair : metaData) {
                            if (pair.first != source) {
                                cosineHeap.emplace(cosineSim.similarity(source, pair.first), pair.first);
                            }
                        }
                        auto stop_run = chrono::high_resolution_clock::now();
                        run_duration1 = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);
                        temp_results_heap = cosineHeap;
                        break;
                    }
                    case 5: { // Multi-Feature Similarity
                        alg1_name = "Multi-Feature";
                        auto start_run = chrono::high_resolution_clock::now();
                        while(!topSimilarGames.empty()) { topSimilarGames.pop(); }
                        sourceGame = &metaData[source];
                        for (const auto& pair : metaData) {
                            if (pair.first == source) { continue; }
                            const Game& compareGame = pair.second;
                            double similarity = calculateOverallWeightedSimilarity(*sourceGame, compareGame, weightTags, weightPublishers, weightDevelopers, weightReviewScore);
                            topSimilarGames.emplace(similarity, pair.first);
                        }
                        auto stop_run = chrono::high_resolution_clock::now();
                        run_duration1 = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);
                        temp_results_heap = topSimilarGames;
                        break;
                    }
                }
                total_duration1 = setup_duration1 + run_duration1;

                switch (alg2) {
                    case 0: { // Jaccards (unweighted)
                        alg2_name = "Jaccard's";
                        auto start_run = chrono::high_resolution_clock::now();
                        while(!maxHeap.empty()) { maxHeap.pop(); }
                        for (const auto& [key, value] : decoder) {
                            if (value != source && metaData.contains(value)) {
                                compare = value;
                                maxHeap.emplace(jaccardsSimilarity(source, compare, metaData), value);
                            }
                        }
                        auto stop_run = chrono::high_resolution_clock::now();
                        run_duration2 = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);
                        results_heap2 = maxHeap;
                        break;
                    }
                    case 1: { // Jaccards (weighted)
                        alg2_name = "Weighted Jaccard's";
                        auto start_run = chrono::high_resolution_clock::now();
                        while(!maxHeap.empty()) { maxHeap.pop(); }
                        for (const auto& [key, value] : decoder) {
                            if (value != source && metaData.contains(value)) {
                                compare = value;
                                maxHeap.emplace(jaccardsSimilarityWeighted(source, compare, metaData), value);
                            }
                        }
                        auto stop_run = chrono::high_resolution_clock::now();
                        run_duration2 = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);
                        results_heap2 = maxHeap;
                        break;
                    }
                    case 2: { // Decision Tree
                        alg2_name = "Decision Tree";
                        auto start_run = chrono::high_resolution_clock::now();
                        results_vector2 = DecisionTree.decisionTree(source, metaData, num_games);
                        auto stop_run = chrono::high_resolution_clock::now();
                        run_duration2 = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);
                        break;
                    }
                    case 3: { // Min Hash
                        alg2_name = "Min-Hash";
                        auto start_setup = chrono::high_resolution_clock::now();
                        allSignatures.clear();
                        for (const auto& pair : metaData) {

                            allSignatures[pair.first] = minHash.createSignature(pair.second);
                        }
                        auto stop_setup = chrono::high_resolution_clock::now();
                        setup_duration2 = chrono::duration_cast<chrono::milliseconds>(stop_setup - start_setup);

                        auto start_run = chrono::high_resolution_clock::now();
                        while(!similarGames.empty()) { similarGames.pop(); }

                        sourceSignature = &allSignatures[source];
                        for (const auto& pair : allSignatures) {
                            if (pair.first != source) {
                                similarGames.emplace(minHash.miniJaccards(*sourceSignature, pair.second), pair.first);
                            }
                        }
                        auto stop_run = chrono::high_resolution_clock::now();

                        run_duration2 = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);
                        results_heap2 = similarGames;
                        break;
                    }
                    case 4: { // Cosine Similarity
                        alg2_name = "Cosine Similarity";
                        auto start_setup = chrono::high_resolution_clock::now();
                        cosineSim.createGameSignatures(metaData);

                        auto stop_setup = chrono::high_resolution_clock::now();
                        setup_duration2 = chrono::duration_cast<chrono::milliseconds>(stop_setup - start_setup);

                        auto start_run = chrono::high_resolution_clock::now();
                        while(!cosineHeap.empty()) { cosineHeap.pop(); }
                        for (const auto& pair : metaData) {
                            if (pair.first != source) {
                                cosineHeap.emplace(cosineSim.similarity(source, pair.first), pair.first);
                            }
                        }
                        auto stop_run = chrono::high_resolution_clock::now();
                        run_duration2 = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);
                        results_heap2 = cosineHeap;
                        break;
                    }
                    case 5: { // Multi-Feature Similarity
                        alg2_name = "Multi-Feature";
                        auto start_run = chrono::high_resolution_clock::now();
                        while(!topSimilarGames.empty()) { topSimilarGames.pop(); }
                        sourceGame = &metaData[source];
                        for (const auto& pair : metaData) {
                            if (pair.first == source) { continue; }
                            const Game& compareGame = pair.second;
                            double similarity = calculateOverallWeightedSimilarity(*sourceGame, compareGame, weightTags, weightPublishers, weightDevelopers, weightReviewScore);
                            topSimilarGames.emplace(similarity, pair.first);
                        }
                        auto stop_run = chrono::high_resolution_clock::now();
                        run_duration2 = chrono::duration_cast<chrono::milliseconds>(stop_run - start_run);
                        results_heap2 = topSimilarGames;
                        break;
                    }
                }
                total_duration2 = setup_duration2 + run_duration2;

                // Print top 5 games from Algorithm 1
                cout << "\n-----------------------------------"<< endl;
                cout << "Top " << num_games << " games from " << alg1_name << ":" << endl;
                if (alg1 == 2) {
                    for (i = 0; i < num_games && i < temp_results_vector.size(); i++) {
                        cout << temp_results_vector[i] << endl;
                    }
                } else {
                    for (i = 0; i < num_games && !temp_results_heap.empty(); i++) {
                        cout << temp_results_heap.top().second << endl;
                        temp_results_heap.pop();
                    }
                }

                // Print top 5 games from Algorithm 2
                cout << "\nTop " << num_games << " games from " << alg2_name << ":" << endl;
                if (alg2 == 2) {
                    for (i = 0; i < num_games && i < results_vector2.size(); i++) {
                        cout << results_vector2[i] << endl;
                    }
                } else {
                    for (i = 0; i < num_games && !results_heap2.empty(); i++) {
                        cout << results_heap2.top().second << endl;
                        results_heap2.pop();
                    }
                }
                cout << "-----------------------------------\n"<< endl;

                cout << "Comparison Results:" << endl;
                cout << "-------------------" << endl;

                // Algorithm 1 Time
                cout << alg1_name << ":" << endl;
                cout << "Setup Time:    " << setup_duration1.count() << " ms" << endl;
                cout << "Running Time:  " << run_duration1.count() << " ms" << endl;
                cout << "Total Time:    " << total_duration1.count() << " ms\n" << endl;

                // Algorithm 2 Times
                cout << alg2_name << ":" << endl;
                cout << "Setup Time:    " << setup_duration2.count() << " ms" << endl;
                cout << "Running Time:  " << run_duration2.count() << " ms" << endl;
                cout << "Total Time:    " << total_duration2.count() << " ms\n" << endl;

                // Winner fstets alg
                if (total_duration1 < total_duration2) {
                    cout << alg1_name << " was faster by " << (total_duration2 - total_duration1).count() << " ms." << endl;
                } else if (total_duration2 < total_duration1) {
                    cout << alg2_name << " was faster by " << (total_duration1 - total_duration2).count() << " ms." << endl;
                } else {
                    cout << "The algorithms took the same amount of time to run." << endl;
                }

                response = "r";
            }


            if (response == "r" || response == "R") {
                continue;
            } else if (response == "q" || response == "Q") {
                cout << "\nThank you for using Steam Search!!\nCredits:\nBayan Mahmoodi\nKushagra Katiyar\nAgnivesh Kaundinya\n\nexiting..." << endl;
                break;
            } else if (response != "m" && response != "M") {
                // invalid command
                cout << "Invalid choice, returning to main menu..." << endl;
                continue;
            }
        }
    }
    return 0;
}