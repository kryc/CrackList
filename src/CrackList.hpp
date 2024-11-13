//
//  CrackList.hpp
//  CrackList
//
//  Created by Kryc on 25/10/2024.
//  Copyright © 2024 Kryc. All rights reserved.
//

#ifndef CrackList_hpp
#define CrackList_hpp

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <string>
#include <tuple>

#include "DispatchQueue.hpp"
#include "simdhash.h"

#include "HashList.hpp"

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
    void SetDeduplicate(const bool Deduplicate) { m_Deduplicate = Deduplicate; }
    void DisableAutohex(void) { m_Hexlify = false; }
    const std::filesystem::path GetHashFile(void) const { return m_HashFile; }
    const std::filesystem::path GetOutFile(void) const { return m_OutFile; }
    const std::string GetWordlist(void) const { return m_Wordlist; }
    const HashAlgorithm GetAlgorithm(void) const { return m_Algorithm; }
    const std::string GetSeparator(void) const { return m_Separator; }
    const size_t GetThreads(void) const { return m_Threads; }
    const size_t GetBlockSize(void) const { return m_BlockSize; }
    const bool GetDeduplicate(void) const { return m_Deduplicate; }
    const bool Crack(void);
    const bool CrackLinear(void);
    void CrackWorker(const size_t Id);
    void OutputResults(std::vector<std::tuple<std::vector<uint8_t>,std::string,std::string>> Results);
    void ThreadPulse(const size_t ThreadId, const uint64_t BlockTime, const std::string LastCracked, const std::string LastTry);
    void WorkerFinished(void);
private:
    const bool CheckDuplicate(const uint8_t* Hash) const;
    void AddDuplicate(const uint8_t* Hash);
    const bool CheckAndAddDuplicate(const uint8_t* Hash);
    void ReadInput(void);
    void ReadBlock(std::vector<std::string>& Block);
    const std::string Hexlify(const std::string& Value) const;
    bool m_Hexlify = true;
    std::filesystem::path m_HashFile;
    std::filesystem::path m_OutFile;
    std::string m_Wordlist;
    HashAlgorithm m_Algorithm = HashAlgorithmUndefined;
    size_t m_DigestLength;
    HashList m_HashList;
    std::ifstream m_WordlistFileStream;
    std::ofstream m_OutputFileStream;
    bool m_BinaryHashFile = true;
    std::string m_Separator = ":";
    std::string m_LastLine;
    std::string m_LastCracked;
    size_t m_Processed = 0;
    size_t m_Cracked = 0;
    bool m_Deduplicate = false;
    std::vector<uint8_t> m_Found;
    std::shared_mutex m_DedupeMutex;
    // Threading
    std::mutex m_InputMutex;
    std::queue<std::vector<std::string>> m_InputCache;
    std::queue<std::vector<std::string>> m_Freelist;
    size_t m_CacheSizeBlocks = 65535;
    bool m_Exhausted = false;
    size_t m_Threads = 1;
    dispatch::DispatcherBasePtr m_MainThread;
    dispatch::DispatcherBasePtr m_IoThread;
    dispatch::DispatcherPoolPtr m_DispatchPool;
    size_t m_ActiveWorkers;
    size_t m_BlockSize = 4096;
    std::map<size_t, uint64_t> m_LastBlockMs;
};

#endif //CrackList_hpp