//
//  main.cpp
//  CrackList
//
//  Created by Kryc on 11/08/2024.
//  Copyright © 2024 Kryc. All rights reserved.
//

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "CrackList.hpp"
#include "Util.hpp"

#define ARGCHECK() \
    if (argc <= i) \
    { \
        std::cerr << "No value specified for " << arg << std::endl; \
        return 1; \
    }

int main(
	int argc,
	const char * argv[]
)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " hashfile wordlist" << std::endl;
    }

    CrackList cracklist;

    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "--out" || arg == "-o")
        {
            ARGCHECK();
            cracklist.SetOutFile(argv[++i]);
        }
        else if (arg == "--sha1")
        {
            cracklist.SetAlgorithm(HashAlgorithmSHA1);
        }
        else if (cracklist.GetHashFile() == "")
        {
            cracklist.SetHashFile(arg);
        }
        else if (cracklist.GetWordlist() == "")
        {
            cracklist.SetWordlist(arg);
        }
        else
        {
            std::cerr << "Unrecognised positional argument: " << argv[i] << std::endl;
        }
    }

    cracklist.Crack();

    return 0;
}