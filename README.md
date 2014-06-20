makeUTF8
========

converts UTF-16 (BE/LE), UTF-32 (BE/LE), ISO-8859-N to UTF-8.    Removes BOM and surrogate pairs from UTF-8, converting a    codepoint between U-D800 and U-DBFF followed by a codepoint    between U-DC00 and U-DFFF to one valid codepoint > U-FFFF.
