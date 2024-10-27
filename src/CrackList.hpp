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
#include <map>
#include <mutex>
#include <string>

#include "DispatchQueue.hpp"

#include "Common.hpp"

class CrackList
{
public:
    CrackList() = default;
    void SetHashFile(const std::filesystem::path HashFile) { m_HashFile = HashFile; }
    void SetOutFile(const std::filesystem::path OutFile) { m_OutFile = OutFile; }
    void SetWordlist(const std::string Wordlist) { m_Wordlist = Wordlist; }
    void SetAlgorithm(const HashAlgorithm Algorithm) { m_Algorithm = Algorithm; }
    void SetSeparator(const std::string Separator) { m_Separator = Separator; }
    void SetThreads(const size_t Threads) { m_Threads = Threads; }
    void SetBlockSize(const size_t BlockSize) { m_BlockSize = BlockSize; }
    const std::filesystem::path GetHashFile(void) const { return m_HashFile; }
    const std::filesystem::path GetOutFile(void) const { return m_OutFile; }
    const std::string GetWordlist(void) const { return m_Wordlist; }
    const HashAlgorithm GetAlgorithm(void) const { return m_Algorithm; }
    const std::string GetSeparator(void) const { return m_Separator; }
    const size_t GetThreads(void) const { return m_Threads; }
    const size_t GetBlockSize(void) const { return m_BlockSize; }
    const bool Crack(void);
    const bool CrackLinear(void);
    void CrackWorker(const size_t Id);
    void OutputResults(std::map<std::string,std::string> Results);
    void ThreadPulse(const size_t ThreadId, const uint64_t BlockTime, const std::string Last);
    void WorkerFinished(void);
private:
    const bool Lookup(const uint8_t* Hash);
    std::filesystem::path m_HashFile;
    std::filesystem::path m_OutFile;
    std::string m_Wordlist;
    HashAlgorithm m_Algorithm = HashAlgorithmUndefined;
    size_t m_DigestLength;
    std::ifstream m_WordlistFileStream;
    std::ofstream m_OutputFileStream;
    FILE* m_BinaryHashFileHandle;
    uint8_t* m_MappedHashesBase;
    size_t m_MappedHashesSize;
    size_t m_HashCount;
    bool m_BinaryHashFile = true;
    std::string m_Separator = ":";
    std::string m_LastLine;
    std::string m_LastCracked;
    size_t m_Processed = 0;
    size_t m_Cracked = 0;
    // Threading
    std::mutex m_InputMutex;
    size_t m_Threads = 1;
    dispatch::DispatcherBasePtr m_MainThread;
    dispatch::DispatcherPoolPtr m_DispatchPool;
    size_t m_ActiveWorkers;
    size_t m_BlockSize = 4096;
    std::map<size_t, uint64_t> m_LastBlockMs;
};