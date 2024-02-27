#include <fstream>
#include <iostream>
#include <streambuf>
#include "teebuf.h"

teebuf::teebuf(std::streambuf *sb1, std::streambuf *sb2)
    : sb1(sb1), sb2(sb2) {}

int teebuf::overflow(int c)
{
    if (c == EOF)
    {
        return !EOF;
    }
    else
    {
        int const r1 = sb1->sputc(c);
        int const r2 = sb2->sputc(c);
        return r1 == EOF || r2 == EOF ? EOF : c;
    }
}

int teebuf::sync()
{
    int const r1 = sb1->pubsync();
    int const r2 = sb2->pubsync();
    return r1 == 0 && r2 == 0 ? 0 : -1;
}
