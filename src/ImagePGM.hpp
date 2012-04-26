/**
 * Read and write a PGM image
 */
#ifndef YARF_IMAGEPGM_HPP
#define YARF_IMAGEPGM_HPP

#include <fstream>
#include "Image2D.hpp"


/**
 * Read and write PGM image files.
 * Also supports reading PBM files.
 * http://www.fileformat.info/format/pbm/egff.htm
 */
class ImagePGM
{
public:
    /**
     * Read a PGM file
     * file: File path
     */
    template <typename T>
    static Image2D<T>* read(const char file[]) {
        std::ifstream is(file, std::ios::in | std::ios::binary);
        if (!is) {
            // TODO: throw error
            return NULL;
        }

        return read<T>(is);
    }

    /**
     * Read a PGM file (can also read a PBM file)
     * is: Input stream
     */
    template <typename T>
    static Image2D<T>* read(std::istream& is) {
        // TODO: check status of all reads
        char magic[2];
        is.read(magic, 2);
        if (magic[0] != 'P') {
            // TODO: throw error
            return NULL;
        }

        bool packbits, binary;

        switch (magic[1]) {
        case '1':
            binary = true;
            packbits = false;
            break;
        case '2':
            binary = false;
            packbits = false;
            break;
        case '4':
            binary = true;
            packbits = true;
            break;
        case '5':
            binary = false;
            packbits = true;
            break;
        default:
            // TODO: throw error
            return NULL;
        }

        int w, h, maxgrey;
        is >> w;
        is >> h;

        if (binary) {
            maxgrey = 1;
        }
        else {
            is >> maxgrey;
        }
        assert(w > 0);
        assert(h > 0);
        // Packbits mode only supported for single bytes
        assert(!packbits || maxgrey < 256);

        Image2D<T>* pim = new Image2D<T>(w, h);
        Image2D<T>& im = *pim;

        if (packbits) {
            // Remove single whitespace between header and data
            char buf;
            is.read(&buf, 1);
            unsigned char* n;

            if (binary) {
                // 8 pixels packed into one byte
                for (uint y = 0; y < uint(h); ++y) {
                    for (uint x = 0; x < uint(w); ++x) { 
                        uint offset = (y * w + x) % 8;
                        if (offset == 0) {
                            // char might be signed, but we should be dealing
                            // with unsigned values only
                            is.read(&buf, 1);
                            n = reinterpret_cast<unsigned char*>(&buf);
                        }
                        // PBM: 1 = black, 0 = white
                        im(x, y) = (*n & (0x80 >> offset)) == 0;
                    }
                }
            }
            else {
                for (uint y = 0; y < uint(h); ++y) {
                    for (uint x = 0; x < uint(w); ++x) { 
                        // char might be signed, but we should be dealing
                        // with unsigned values only
                        is.read(&buf, 1);
                        n = reinterpret_cast<unsigned char*>(&buf);
                        im(x, y) = *n;
                    }
                }
            }
        }
        else {
            for (uint y = 0; y < uint(h); ++y) {
                for (uint x = 0; x < uint(w); ++x) {
                    is >> im(x, y);
                }
            }
        }

        return pim;
    }


    /**
     * Write a PGM file
     * im: The image
     * file: The file path to be written to
     * packbits: If true write out the file in packbits mode, otherwise ascii
     */
    template <typename T>
    static void write(const Image2D<T>& im, const char file[], bool packbits) {
        std::ofstream os(file, std::ios::binary);
        if (!os) {
            // TODO: throw error
        }
        write(im, os, packbits);
    }

    /**
     * Write a PGM file
     * im: The image
     * os: The output stream
     * packbits: If true write out the file in packbits mode, otherwise ascii
     */
    template <typename T>
    static void write(const Image2D<T>& im, std::ostream& os, bool packbits) {
        // TODO: check status of writes?
        if (packbits) {
            os << "P5 ";
        }
        else {
            os << "P2 ";
        }

        os << im.xsize() << " " << im.ysize() << " ";
        // Max grey value
        // TODO: For non-packbits mode this can be greater than 255
        os << 255 << "\n";

        if (packbits) {
            for (uint y = 0; y < im.ysize(); ++y) {
                for (uint x = 0; x < im.xsize(); ++x) {
                    assert(im(x, y) <= 255);
                    // TODO: Does char signed/unsigned matter?
                    // I don't think it does, since even if char is unsigned
                    // the bit pattern should be the same
                    char buf = im(x, y);
                    os.write(&buf, 1);
                }
            }
        }
        else {
            for (uint y = 0; y < im.ysize(); ++y) {
                for (uint x = 0; x < im.xsize(); ++x) {
                    if (x > 0) {
                        os << " ";
                    }
                    os << im(x, y);
                }
                os << "\n";
            }
        }
    }

};


#endif // YARF_IMAGEPGM_HPP
