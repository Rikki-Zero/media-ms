#pragma once
#include <string>

class Media {
public:
    Media(const std::string& title, const std::string& creator);
    virtual ~Media() = default;

    std::string getTitle() const;
    std::string getCreator() const;

    virtual std::string getType() const = 0;
    virtual void printDetails() const;

protected:
    std::string title;
    std::string creator;
};

class Movie : public Media {
public:
    Movie(const std::string& title, const std::string& director, int year);
    std::string getType() const override;
    void printDetails() const override;
private:
    int year;
};

class Song : public Media {
public:
    Song(const std::string& title, const std::string& artist, const std::string& album);
     std::string getType() const override;
     void printDetails() const override;
private:
    std::string album;
};