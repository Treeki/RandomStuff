Treeki's Random Wii Stuff
=========================

This repo is just for Wii hacking stuff which I wanted to post, but which
doesn't really need one repo. Currently, not that much, but it could change..


LH Decompressor
---------------

Used in Mario Sports Mix. Not sure what the format is actually called -- it
has the header byte _0x40_, and specifies the uncompressed size in the same
way as all the other CX formats.

NSMB Wii has (disabled) support for it, using the file extension "LH" which is
why I've provisionally named it "LH Decompressor". There's another mystery
compression format in there too (LRC).

This is pretty messy code, since it's mostly just a direct translation from
the assembly -- but it works! (Not too much testing done yet, though.)

    g++ -o LH LHDecompressor.cpp
    ./LH sourceFile.bin destFile.bin
