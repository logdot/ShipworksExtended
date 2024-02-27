#pragma once

#include <streambuf>

class teebuf : public std::streambuf
{
public:
    teebuf(std::streambuf *sb1, std::streambuf *sb2);

protected:
    virtual int overflow(int c);
    virtual int sync();

private:
    std::streambuf *sb1;
    std::streambuf *sb2;
};
