//
// Created by kkati on 7/27/2025.
//

#ifndef GAME_H
#define GAME_H

#include <vector>
#include <string>
#include <unordered_map>

using namespace std;


class Game {
private:

    //variables are in order based on order of the json
    int ID;
    double price;
    int requiredAge;

    vector<string> platforms;

    int metacriticScore;

    vector<string> supportedLanguages;
    vector<string> fullAudioLanguages;
    vector<string> developers;

    vector<string> publisher;

    vector<string> categories; //"Single-Player", "Multi-Player", etc.
    vector<string> genres; //"Casual", "Indie", etc

    double reviewScore; //positive - negative

    unordered_map<string, int> tags;
    string imageURL;

public:
    Game();

    //Getters
    int getID() const ;

    double getPrice() const;

    int getRequiredAge () const;

    vector<string> getPlatforms() const;

    int getMetacriticScore() const;

    vector<string> getSupportedLanguages() const;

    vector<string> getFullAudioLanguages() const;

    vector<string> getDevelopers() const;

    vector<string> getPublisher() const;

    vector<string> getCategories() const;

    vector<string> getGenres() const;


    double getReviewScore() const;//returns the actual review score accounting for downvotes and upvotes

    unordered_map<string, int> getTags() const;

    string getImageURL() const;

    //Setters
    void setID(int id);

    void setPrice(double price);

    void setRequiredAge(int age);

    void setPlatform(vector<string> platform);

    void setMetacriticScore(int score);

    void setSupportedLanguages(vector<string> supportedLanguages);

    void setFullAudioLanguages(vector<string> fullAudioLanguages);

    void setDevelopers(vector<string> developers);

    void setPublisher(vector<string> publisher);

    void setCategories(vector<string> categories);

    void setGenres(vector<string> genres);

    void setReviewScore(int positive, int negative);

    void setTags(unordered_map<string, int> tags);

    void setImageURL(string url);

    //methods
    int getTagCount();

};


#endif //GAME_H
