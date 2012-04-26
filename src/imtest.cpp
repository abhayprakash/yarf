/**
 *
 */

#include "ImagePGM.hpp"
#include <iostream>


void testRead(const char file[])
{
    Image2D<int>::Ptr im = ImagePGM::read<int>(file);
    ImagePGM::write(*im, std::cout, false);
    ImagePGM::write(*im, "foo.out", true);
}

int main(int argc, char* argv[])
{
    testRead("foo.pgm");
    return 0;
}

