//
// Created by bmmah on 7/30/2025.
//

#include "algorithms_B.h"
#include <iostream>
#include <ostream>

// checks if two containers share any items
bool sharesAny(const vector<string>& a, const vector<string>& b) {
    // for the vast majority of checks the containers are 1 sized, this acts as a short circuit in that case
    if (a.size() == 1 && b.size() == 1)
    {
        return a[0] == b[0];
    }

    // creates a set of 'a' for quick look up and then iterates down 'b' checking if its contents exist in 'a'
    unordered_set<string> aset(a.begin(), a.end());
    for (const auto& item : b) {
        if (aset.count(item))
        {
            return true;
        }
    }
    return false;
}

// this function takes in two games (selected and candidate) and then decides what bucket to place candidate in wrt to selected
BucketLevel algorithms_b::setBucket(string& selected, string& candidate, unordered_map<string, Game>& gameData, unordered_map<string, double> scores)
{
    const auto& selDevs = gameData[selected].getDevelopers();
    const auto& canDevs = gameData[candidate].getDevelopers();
    const auto& selPubs = gameData[selected].getPublisher();
    const auto& canPubs = gameData[candidate].getPublisher();

    // checks if the selected game has the same publisher and/or developer as the candidate game
    bool sameDev = sharesAny(selDevs, canDevs);
    bool samePub = sharesAny(selPubs, canPubs);

    double review = gameData[candidate].getReviewScore();

    // used an enum to simplify the output process, don't need some abstract values or other method
    if (sameDev) return HIGH_RELEVANCE;
    if (samePub) return MEDIUM_RELEVANCE;
    if (scores[candidate] > 0.74) return TAG_SIMILAR;
    if (scores[candidate] > 0.24 && review > 85) return WEAK_SIMILAR;
    return LOW_RELEVANCE;
}

algorithms_b::algorithms_b()
{
    processed.clear();
    buckets.clear();
    scores.clear();
}

// allows us to see if the savedHeap is exhausted, useful to decisionTreeNext logic in main
bool algorithms_b::isHeapEmpty()
{
    return savedHeap.empty();
}

// this function embodies the entire rule-based decision tree logic
vector<string> algorithms_b::decisionTree(string& selected, unordered_map<string, Game>& gameData, int num_games)
{
    unordered_map<string, int> tags = gameData[selected].getTags();
    unordered_set<string> candidates;

    // find all games that share a tag with the selected game, these will be our candidates for the decision tree
    for (auto& [game, data] : gameData)
    {
        for (auto& [tag, votes] : data.getTags())
        {
            if (tags.contains(tag) && game != selected)
            {
                candidates.insert(game);
                break;
            }
        }
    }

    // this section creates a maxHeap of games (uses weighted jaccards) which will be the similarity score that was used
    // in setBucket (scores[candidate] >= 0.75) return TAG_SIMILAR
    string source = selected;
    string compare;
    priority_queue<pair<double, string>> maxHeap;
    for (const auto& name : candidates) {
        if (name != source) {
            compare = name;
            maxHeap.emplace(jaccardsSimilarityWeighted(source, compare, gameData), name);
        }
    }
    savedHeap = maxHeap;

    // here we're finding the maximum of the jaccards scores so we can normalize maxHeap before calling setBucket
    if (maxHeap.top().second == selected)
    {
        maxHeap.pop();
    }
    double maxScore = maxHeap.empty() ? 0.0 : maxHeap.top().first;
    int count = 0;
    while (!maxHeap.empty()) {
        auto [sim, candidate] = maxHeap.top();
        maxHeap.pop();
        if (candidate == selected)
        {
            continue;
        }
        // now normalize the maxHeap (which has been transferred to scores container), also has some error handling
        scores[candidate] = (maxScore > 0.0) ? (sim / maxScore) : 0.0;
        count++;
    }

    // iterate through the candidates container and for each candidate decide which bucket it belongs in
    int chunk = 1000;
    for (int i = 0; i < chunk; i++) // chunks the candidates evaluated to avoid prolonged computation
    {
        if (processed.contains(savedHeap.top().second))
        {
            savedHeap.pop();
            continue;
        }
        string name = savedHeap.top().second;
        savedHeap.pop();
        BucketLevel level = setBucket(selected, name, gameData, scores);
        processed.insert(savedHeap.top().second);
        buckets[level].push_back(name);
    }

    // buckets is a 2D container, one dimension is the enum from above (buckets[enum]) and the other dimension
    // is all the games that belong in that enum's bucket category (high-relevance, etc). This loop sorts each
    // bucket by review score
    for (auto& [bucket, games] : buckets)
    {
        sort(games.begin(), games.end(), [&gameData](const string& a, const string& b)
        {
            return gameData[a].getReviewScore() > gameData[b].getReviewScore();
        });
    }

    // here we're accessing the buckets container and returning its contents, we use the enums to simplify traversal
    vector<string> finalResults;
    for (int level = HIGH_RELEVANCE; level <= LOW_RELEVANCE; level++)
    {
        const auto& bucket = buckets[static_cast<BucketLevel>(level)];
        finalResults.insert(finalResults.end(), bucket.begin(), bucket.end());
    }
    return finalResults;
}

// this function allows for us to compute more games if the user exhausts the previous chunk of games
vector<string> algorithms_b::decisionTreeNext(string& selected, unordered_map<string, Game>& gameData, int num_games)
{
    buckets.clear();
    // iterate through the candidates container and for each candidate decide which bucket it belongs in
    int chunk = 1000;
    for (int i = 0; i < chunk; i++) // chunks the candidates evaluated to avoid prolonged computation
    {
        if (processed.contains(savedHeap.top().second))
        {
            savedHeap.pop();
            continue;
        }
        string name = savedHeap.top().second;
        savedHeap.pop();
        BucketLevel level = setBucket(selected, name, gameData, scores);
        processed.insert(savedHeap.top().second);
        buckets[level].push_back(name);
    }

    // buckets is a 2D container, one dimension is the enum from above (buckets[enum]) and the other dimension
    // is all the games that belong in that enum's bucket category (high-relevance, etc). This loop sorts each
    // bucket by review score
    for (auto& [bucket, games] : buckets)
    {
        sort(games.begin(), games.end(), [&gameData](const string& a, const string& b)
        {
            return gameData[a].getReviewScore() > gameData[b].getReviewScore();
        });
    }

    // here we're accessing the buckets container and returning its contents, we use the enums to simplify traversal
    vector<string> finalResults;
    for (int level = HIGH_RELEVANCE; level <= LOW_RELEVANCE; level++)
    {
        const auto& bucket = buckets[static_cast<BucketLevel>(level)];
        finalResults.insert(finalResults.end(), bucket.begin(), bucket.end());
    }
    return finalResults;
}
