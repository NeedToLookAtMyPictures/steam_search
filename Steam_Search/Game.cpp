//
// Created by kkati on 7/27/2025.
//

#include "Game.h"


//Constructors
Game::Game(){}

//Getter
int Game:: getID() const {
    return ID;
}

double Game:: getPrice() const {
    return price;
}

int Game:: getRequiredAge() const {
    return requiredAge;
}

vector<string> Game:: getPlatforms() const {
    return platforms;
}

int Game:: getMetacriticScore() const {
    return metacriticScore;
}

vector<string> Game :: getSupportedLanguages() const {
    return supportedLanguages;
}

vector<string> Game:: getDevelopers() const {
    return developers;
}

vector<string> Game:: getPublisher() const {
    return publisher;
}

vector<string> Game :: getCategories() const {
    return categories;
}

vector<string> Game:: getGenres() const {
    return genres;
}


double Game:: getReviewScore() const {
    return reviewScore;
}

unordered_map<string, int>Game :: getTags() const {
    return tags;
}

string Game::getImageURL() const {
    return imageURL;
}

//Setters
void Game :: setID(int id){
    this->ID = id;
}

void Game:: setPrice(double price) {
    this->price = price;
}

void Game:: setRequiredAge(int age) {
    this->requiredAge = age;
}

void Game:: setPlatform(vector<string> platform) {
    this->platforms = platform;
}

void Game :: setMetacriticScore(int score) {
    if (score == 0) {
        this->metacriticScore = -1;
    }
    else {
        this->metacriticScore = score;
    }
}

void Game :: setSupportedLanguages(vector<string> supportedLanguages) {
    this->supportedLanguages = supportedLanguages;
}

void Game :: setFullAudioLanguages(vector<string> fullAudioLanguages) {
    this->fullAudioLanguages = fullAudioLanguages;
}

void Game :: setDevelopers(vector<string> developers) {
    this->developers = developers;
}

void Game :: setPublisher(vector<string> publishers) {
    this->publisher = publishers;
}


void Game :: setCategories (vector<string> categories) {
    this->categories = categories;
}

void Game:: setGenres(vector<string> genres) {
    this->genres = genres;
}


void Game :: setReviewScore(int positive, int negative) {
    if (positive + negative == 0) {
        this->reviewScore = -1.0;
    }
    else {
        this->reviewScore = static_cast<double>(positive) / (positive + negative);
    }
}

void Game :: setTags(unordered_map<string, int> tags) {
    this->tags = tags;
}

void Game:: setImageURL(string url) {
    this->imageURL = url;
}

//methods
int Game :: getTagCount() {
    return tags.size();
}



