//
// Created by bmmah on 7/30/2025.
//

#ifndef ALGORITHMS_B_H
#define ALGORITHMS_B_H

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>
#include <map>
#include <queue>
#include "Game.h"
#include "jaccardsSimilarity.h"

using namespace std;

enum BucketLevel {
    HIGH_RELEVANCE,
    MEDIUM_RELEVANCE,
    TAG_SIMILAR,
    WEAK_SIMILAR,
    LOW_RELEVANCE
};

class algorithms_b
{
private:
    priority_queue<pair<double, string>> savedHeap;
    unordered_map<string, double> scores;
    map<BucketLevel, vector<string>> buckets;
    unordered_set<string> processed;
    BucketLevel setBucket(string& selected, string& candidate, unordered_map<string, Game>& gameData, unordered_map<string, double> scores);
public:
    algorithms_b();
    bool isHeapEmpty();
    vector<string> decisionTree(string& selected, unordered_map<string, Game>& gameData, int num_games);
    vector<string> decisionTreeNext(string& selected, unordered_map<string, Game>& gameData, int num_games);
};





#endif //ALGORITHMS_B_H
