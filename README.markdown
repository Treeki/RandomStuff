Treeki's Random Stuff
=====================

This repo is just for general hacking stuff which I wanted to post, but which
doesn't really need one repo. Currently, not that much, but it could change..
And it's not Wii-specific any more. :p


Wii U GTX Extractor
-------------------

Extracts textures in RGBA8 and DXT5 formats from the 'Gfx2' (.gtx file
extension) format used in Wii U games. A bit of work could get it to extract
.bflim files, too.

Somewhat unfinished, pretty buggy. Use at your own risk. It's not great, but I
figured I'd throw it out there to save other people the work I already did.

More details on compilation and usage in the comments inside the file.


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


Sega LZS2 Decompressor
----------------------

I reverse-engineered this compression format when andlabs was messing around
with some Sega Tetris game. It's present in PAK files(?) and has the magic
"LZS2".

Some of the variable names don't really make sense, but the decompression
works fine as far as we've tested so far.

I'm not totally sure if it works with decompressed files over 50kb in size.
I hope it does.

    gcc -o SegaLZS2 SegaLZS2Decomp.c
    ./SegaLZS2 sourceFile.bin destFile.bin


Hammer Decompressor
-------------------

Unpacks a format found in the Acorn Archimedes game 'OddBall'. It appears to
be called 'hammer' and files compressed with it are identified by the magic
string `Hmr\0` or `48 6D 72 00`. Is it used anywhere else? No idea!

    python hammer_decomp.py Sounds/MUSIC music_decomp


