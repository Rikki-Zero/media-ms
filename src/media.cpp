#include <iostream>
#include "media.h"

Media::Media(const std::string& title, const std::string& creator) : title(title), creator(creator) {}

std::string Media::getTitle() const {
    return title;
}

std::string Media::getCreator() const{
    return creator;
}

void Media::printDetails() const{
    std::cout << "Title: " << title << ", Creator: " << creator << std::endl;
}

Movie::Movie(const std::string& title, const std::string& director, int year) : Media(title, director), year(year){}

std::string Movie::getType() const{
    return "Movie";
}

void Movie::printDetails() const{
    Media::printDetails();
     std::cout << "Type: Movie, Year: " << year << std::endl;
}

Song::Song(const std::string& title, const std::string& artist, const std::string& album) : Media(title, artist), album(album) {}

std::string Song::getType() const {
    return "Song";
}

void Song::printDetails() const{
    Media::printDetails();
    std::cout << "Type: Song, Album: " << album << std::endl;
}