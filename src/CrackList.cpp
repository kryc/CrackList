//
//  CrackList.cpp
//  CrackList
//
//  Created by Kryc on 25/10/2024.
//  Copyright © 2024 Kryc. All rights reserved.
//

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <string.h>
#include <sys/mman.h>

#include "CrackList.hpp"

// static int
// Compare(
//     const void* Value1,
//     const void* Value2,
//     const void* Size
// )
// {
//     return memcmp(Value1, Value2, (size_t)Size);
// }

const bool
CrackList::Lookup(
    const uint8_t* Hash
)
{
    // Perform the search
    const uint8_t* base = m_MappedHashesBase;
    const uint8_t* top = base + m_MappedHashesSize;

    // const size_t count = m_MappedHashesSize / m_DigestLength;

    uint8_t* low = (uint8_t*)base;
    uint8_t* high = (uint8_t*)top - m_DigestLength;
    uint8_t* mid;

    while (low <= high)
    {
        mid = low + ((high - low) / (2 * m_DigestLength)) * m_DigestLength;
        int cmp = memcmp(mid, Hash, m_DigestLength);
        if (cmp == 0)
        {
            return true;
        }
        else if (cmp < 0)
        {
            low = mid + m_DigestLength;
        }
        else
        {
            high = mid - m_DigestLength;
        }
    }

    return false;
}

const bool
CrackList::Crack(
    void
)
{
    // Check parameters
    if (m_HashFile.string() == "")
    {
        std::cerr << "Error: no hash file specified" << m_HashFile << std::endl;
        return false;
    }
    
    // Open the input file
    if (m_Wordlist != "-" && m_Wordlist != "")
    {
        if (!std::filesystem::exists(m_Wordlist))
        {
            std::cerr << "Error: Wordlist file does not exist" << std::endl;
            return false;
        }
        m_WordlistFileStream.open(m_Wordlist, std::ios::in);
    }

    if (m_OutFile != "")
    {
        m_OutFileStream.open(m_OutFile, std::ios::out);
    }

    // Open the hash file
    if (m_BinaryHashFile)
    {
        if (m_Algorithm == HashAlgorithmUndefined)
        {
            std::cerr << "Error: binary hash list with no algorithm" << std::endl;
            return false;
        }

        m_DigestLength = GetDigestLength(m_Algorithm);

        m_BinaryHashFileHandle = fopen(m_HashFile.c_str(), "r+");
        if (m_BinaryHashFileHandle == nullptr)
        {
            std::cerr << "Error: unable to open binary hash file" << std::endl;
            return false;
        }
        
        // Get the file size
        m_MappedHashesSize = std::filesystem::file_size(m_HashFile);

        if (m_MappedHashesSize % m_DigestLength != 0)
        {
            std::cerr << "Error: length of hash file does not match digest" << std::endl;
            return false;
        }

        m_HashCount = m_MappedHashesSize / m_DigestLength;

        // Mmap the file
        m_MappedHashesBase = (uint8_t*)mmap(nullptr, m_MappedHashesSize, PROT_READ|PROT_WRITE, MAP_SHARED, fileno(m_BinaryHashFileHandle), 0);
        if (m_MappedHashesBase == nullptr)
        {
            std::cerr << "Error: Unable to map hashes file" << std::endl;
            return false;
        }

        auto ret = madvise(m_MappedHashesBase, m_MappedHashesSize, MADV_RANDOM|MADV_WILLNEED);
        if (ret != 0)
        {
            std::cerr << "Madvise not happy" << std::endl;
            return false;
        }

        // std::vector<uint8_t> t = std::vector(m_MappedHashesBase, m_MappedHashesBase + m_DigestLength);
        // std::cerr << Util::ToHex(t.data(), t.size()) << std::endl;

        // Sort the hashes
        // std::cerr << "Sorting hashes" << std::endl;
        // qsort_r(m_MappedHashesBase, m_HashCount, m_DigestLength, (__compar_d_fn_t)memcmp, (void*)m_DigestLength);
    }

    // Repeatedly read word, hash and check
    std::cerr << "Beginning cracking" << std::endl;
    std::istream& input = m_WordlistFileStream.is_open() ? m_WordlistFileStream : std::cin;
    std::vector<uint8_t> hash(m_DigestLength);

    for( std::string line; getline(input, line); )
    {
        // Strip cr and nl
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());

        DoHash(m_Algorithm, (uint8_t*)&line[0], line.size(), &hash[0]);

        if (Lookup(&hash[0]))
        {
            auto hex = Util::ToHex(&hash[0], hash.size());
            std::transform(hex.begin(), hex.end(), hex.begin(),
                [](unsigned char c){ return std::tolower(c); });
            std::cout << hex << ":" << line << std::endl;
        }
    }

    return true;
}