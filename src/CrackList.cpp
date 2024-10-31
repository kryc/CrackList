//
//  CrackList.cpp
//  CrackList
//
//  Created by Kryc on 25/10/2024.
//  Copyright © 2024 Kryc. All rights reserved.
//

#include <assert.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <string.h>
#include <sys/mman.h>
#include <tuple>
#include <vector>

#include "CrackList.hpp"

const bool
CrackList::Lookup(
    const uint8_t* Base,
    const size_t Size,
    const uint8_t* Hash
) const
{
    // Perform the search
    const uint8_t* base = Base;
    const uint8_t* top = Base + Size;

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
CrackList::CrackLinear(
    void
)
{
    std::istream& input = m_WordlistFileStream.is_open() ? m_WordlistFileStream : std::cin;
    std::ostream& output = m_OutputFileStream.is_open() ? m_OutputFileStream : std::cout;
    std::vector<uint8_t> hash(m_DigestLength);
    std::string last_cracked;

    auto start = std::chrono::system_clock::now();

    for( std::string line; getline(input, line); )
    {
        // Strip cr and nl
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());

        if (line == m_LastLine)
        {
            continue;
        }

        m_LastLine = line;
        m_Processed++;

        DoHash(m_Algorithm, (uint8_t*)&line[0], line.size(), &hash[0]);

        if (Lookup(m_MappedHashesBase, m_MappedHashesSize, &hash[0]))
        {
            if (!m_Deduplicate || !CheckAndAddDuplicate(hash))
            {
                auto hex = Util::ToHex(&hash[0], hash.size());
                hex = Util::ToLower(hex);
                m_Cracked++;
                output << hex << m_Separator << line << std::endl;
                last_cracked = line;
            }
        }

        if (m_Processed % m_BlockSize == 0)
        {
            auto end = std::chrono::system_clock::now();
            auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            ThreadPulse(0, elapsed_ms.count(), line);
            start = std::chrono::system_clock::now();
        }
    }

    return true;
}

const bool
CrackList::CheckDuplicate(
    const std::vector<uint8_t>& Hash
) const
{
    if (m_Found.size() > 0 && Lookup(&m_Found[0], m_Found.size(), (const uint8_t*)&Hash[0]))
    {
        return true;
    }
    return false;
}

void
CrackList::AddDuplicate(
    const std::vector<uint8_t>& Hash
)
{
    m_Found.insert(m_Found.end(), Hash.begin(), Hash.end());
    qsort_r(&m_Found[0], m_Found.size()/m_DigestLength, m_DigestLength, (__compar_d_fn_t)memcmp, (void*)m_DigestLength);
}

const bool
CrackList::CheckAndAddDuplicate(
    const std::vector<uint8_t>& Hash
)
{
    std::unique_lock lock(m_DedupeMutex);
    const bool exists = CheckDuplicate(Hash);
    if (!exists)
    {
        AddDuplicate(Hash);
    }
    return exists;
}

void
CrackList::OutputResults(
    std::vector<std::tuple<std::vector<uint8_t>,std::string,std::string>> Results
)
{
    std::ostream& output = m_OutputFileStream.is_open() ? m_OutputFileStream : std::cout;

    for (auto& [h,x,v] : Results)
    {
        m_Cracked++;
        output << x << m_Separator << v << std::endl;
        m_LastCracked = v;
    }
}

void
CrackList::ThreadPulse(
    const size_t ThreadId,
    const uint64_t BlockTime,
    const std::string Last
)
{
    m_LastBlockMs[ThreadId] = BlockTime;

    if (!Last.empty())
    {
        m_LastCracked = Last;
    }

    std::string printable = m_LastCracked;
    std::transform(printable.begin(), printable.end(), printable.begin(),
        [](unsigned char c){ return c > ' ' && c < '~' ? c : ' ' ; });

    // Output the status if we are not printing to stdout
    if (!m_OutFile.string().empty())
    {
        double averageMs = 0;
        for (auto const& [thread, val] : m_LastBlockMs)
        {
            averageMs += val;
        }
        averageMs /= m_Threads;

        double hashesPerSec = 1000.f * (m_BlockSize / averageMs);
        char multiplechar = ' ';
        if (hashesPerSec > 1000000000.f)
        {
            hashesPerSec /= 1000000000.f;
            multiplechar = 'b';
        }
        else if (hashesPerSec > 1000000.f)
        {
            hashesPerSec /= 1000000.f;
            multiplechar = 'm';
        }
        else if (hashesPerSec > 1000.f)
        {
            hashesPerSec /= 1000.f;
            multiplechar = 'k';
        }

        double percent = ((double)m_Cracked / m_HashCount) * 100.f;

        char statusbuf[80];
        statusbuf[sizeof(statusbuf) - 1] = '\0';
        fprintf(stderr, "%s", "\r");
        fflush(stderr);
        memset(statusbuf, ' ', sizeof(statusbuf) - 1);
        int count = snprintf(
            statusbuf, sizeof(statusbuf),
            "H/s:%.1lf%c C:%zu/%zu (%.1lf%%) T:%zu L:\"%s\"",
                hashesPerSec,
                multiplechar,
                m_Cracked,
                m_HashCount,
                percent,
                m_Processed,
                printable.c_str()
        );
        if (count < sizeof(statusbuf) - 1)
        {
            statusbuf[count] = ' ';
        }
        fprintf(stderr, "%s", statusbuf);
    }
}

void
CrackList::WorkerFinished(
    void
)
{
    if (--m_ActiveWorkers == 0)
    {
        // Stop the pool
        m_DispatchPool->Stop();
        m_DispatchPool->Wait();
        // Stop the current main thread
        dispatch::CurrentDispatcher()->Stop();
    }
}

void
CrackList::CrackWorker(
    const size_t Id
)
{
    std::vector<std::string> block;
    std::istream& input = m_WordlistFileStream.is_open() ? m_WordlistFileStream : std::cin;
    std::vector<uint8_t> hash(m_DigestLength);
    std::string last_cracked;

    // Read a block of input words
    {
        std::lock_guard<std::mutex> lock(m_InputMutex);

        if (input.eof())
        {
            // Track the completion of this worker
            dispatch::PostTaskToDispatcher(
                "main",
                std::bind(
                    &CrackList::WorkerFinished,
                    this
                )
            );
            // Terminate our current queue
            dispatch::CurrentQueue()->Stop();
            return;
        }

        std::string line;
        do
        {
            getline(input, line);

            // Strip cr and nl
            line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
            line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());

            if (line != m_LastLine)
            {
                block.push_back(line);
                m_LastLine = line;
                m_Processed++;
            }
        } while(block.size() < m_BlockSize && !input.eof());
    }

    std::vector<std::tuple<std::vector<uint8_t>,std::string,std::string>> cracked;

    auto start = std::chrono::system_clock::now();

    for (auto& line : block)
    {
        DoHash(m_Algorithm, (uint8_t*)&line[0], line.size(), &hash[0]);

        if (Lookup(m_MappedHashesBase, m_MappedHashesSize, &hash[0]))
        {
            // Check first in a shared lock
            if (m_Deduplicate)
            {
                std::shared_lock lock(m_DedupeMutex);
                if (CheckDuplicate(hash))
                {
                    continue;
                }
            }

            // Check and add in a unique lock if not found
            if (!m_Deduplicate || !CheckAndAddDuplicate(hash))
            {
                auto hex = Util::ToHex(&hash[0], hash.size());
                hex = Util::ToLower(hex);
                cracked.push_back({hash, hex, line});
            }
        }
    }

    auto end = std::chrono::system_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    if (cracked.size() > 0)
    {
        last_cracked = std::get<2>(cracked.back());
        dispatch::PostTaskToDispatcher(
            "main",
            std::bind(
                &CrackList::OutputResults,
                this,
                std::move(cracked)
            )
        );
    }

    dispatch::PostTaskToDispatcher(
        "main",
        std::bind(
            &CrackList::ThreadPulse,
            this,
            Id,
            elapsed_ms.count(),
            last_cracked
        )
    );

    dispatch::PostTaskFast(
        std::bind(
            &CrackList::CrackWorker,
            this,
            Id
        )
    );
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
        m_OutputFileStream.open(m_OutFile, std::ios::out);
    }

    // Open the hash file
    if (m_BinaryHashFile)
    {
        if (m_Algorithm == HashAlgorithmUndefined)
        {
            std::cerr << "Error: binary hash list with no algorithm" << std::endl;
            return false;
        }

        // Get the file size
        m_MappedHashesSize = std::filesystem::file_size(m_HashFile);
        m_DigestLength = GetDigestLength(m_Algorithm);
        m_HashCount = m_MappedHashesSize / m_DigestLength;

        m_BinaryHashFileHandle = fopen(m_HashFile.c_str(), "r");
        if (m_BinaryHashFileHandle == nullptr)
        {
            std::cerr << "Error: unable to open binary hash file" << std::endl;
            return false;
        }

        if (m_MappedHashesSize % m_DigestLength != 0)
        {
            std::cerr << "Error: length of hash file does not match digest" << std::endl;
            return false;
        }

        // Mmap the file
        m_MappedHashesBase = (uint8_t*)mmap(nullptr, m_MappedHashesSize, PROT_READ, MAP_SHARED, fileno(m_BinaryHashFileHandle), 0);
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

        // Sort the hashes
        // std::cerr << "Sorting hashes" << std::endl;
        // qsort_r(m_MappedHashesBase, m_HashCount, m_DigestLength, (__compar_d_fn_t)memcmp, (void*)m_DigestLength);
    }

    std::cerr << "Beginning cracking" << std::endl;
    
    if (m_Threads == 1)
    {
        return CrackLinear();
    }
    else if (m_Threads == 0)
    {
        m_Threads = std::thread::hardware_concurrency();
    }

    m_DispatchPool = dispatch::CreateDispatchPool("worker", m_Threads);
    m_ActiveWorkers = m_Threads;

    for (size_t i = 0; i < m_Threads; i++)
    {
        m_DispatchPool->PostTask(
            dispatch::bind(
                &CrackList::CrackWorker,
                this,
                i
            )
        );
    }

    dispatch::CreateAndEnterDispatcher(
        "main",
        std::bind(
            &dispatch::DoNothing
        )
    );

    std::cerr << "Cracked " << m_Cracked << " hashes" << std::endl;

    return true;
}
