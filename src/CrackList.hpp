//
//  CrackList.hpp
//  CrackList
//
//  Created by Kryc on 25/10/2024.
//  Copyright © 2024 Kryc. All rights reserved.
//

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "Common.hpp"

class CrackList
{
public:
    CrackList() = default;
    void SetHashFile(const std::filesystem::path HashFile) { m_HashFile = HashFile; }
    void SetOutFile(const std::filesystem::path OutFile) { m_OutFile = OutFile; }
    void SetWordlist(const std::string Wordlist) { m_Wordlist = Wordlist; }
    void SetAlgorithm(const HashAlgorithm Algorithm) { m_Algorithm = Algorithm; }
    const std::filesystem::path GetHashFile(void) const { return m_HashFile; }
    const std::filesystem::path GetOutFile(void) const { return m_OutFile; }
    const std::string GetWordlist(void) const { return m_Wordlist; }
    const HashAlgorithm GetAlgorithm(void) const { return m_Algorithm; }
    const bool Crack(void);
private:
    const bool Lookup(const uint8_t* Hash);
    std::filesystem::path m_HashFile;
    std::filesystem::path m_OutFile;
    std::string m_Wordlist;
    HashAlgorithm m_Algorithm = HashAlgorithmUndefined;
    size_t m_DigestLength;
    std::ifstream m_WordlistFileStream;
    std::ofstream m_OutFileStream;
    FILE* m_BinaryHashFileHandle;
    uint8_t* m_MappedHashesBase;
    size_t m_MappedHashesSize;
    size_t m_HashCount;
    bool m_BinaryHashFile = true;
};