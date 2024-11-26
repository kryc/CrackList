//
//  HashList.cpp
//  HashList
//
//  Created by Kryc on 12/11/2024.
//  Copyright © 2024 Kryc. All rights reserved.
//

#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "HashList.hpp"

// The threshold at which we perform an index
// of all of the hashes and perform fast lookup
#define FAST_LOOKUP_THRESHOLD (65536 * 4)
// The threshold below which we just perform linear
// lookups and not bother with binary search
#define LINEAR_LOOKUP_THRESHOLD (512)

const bool
HashList::Lookup(
    const uint8_t* Base,
    const size_t Size,
    const uint8_t* Hash,
    const size_t HashSize
)
{
    // Perform the search
    const uint8_t* base = Base;
    const uint8_t* top = Base + Size;

    uint8_t* low = (uint8_t*)base;
    uint8_t* high = (uint8_t*)top - HashSize;
    uint8_t* mid;

    while (low <= high)
    {
        mid = low + ((high - low) / (2 * HashSize)) * HashSize;
        int cmp = memcmp(mid, Hash, HashSize);
        if (cmp == 0)
        {
            return true;
        }
        else if (cmp < 0)
        {
            low = mid + HashSize;
        }
        else
        {
            high = mid - HashSize;
        }
    }

    return false;
}

const bool
HashList::Initialize(
    const std::filesystem::path Path,
    const size_t DigestLength,
    const bool Sort
)
{
    m_Path = Path;
    m_DigestLength = DigestLength;

    // Get the file size
    m_Size = std::filesystem::file_size(m_Path);
    m_Count = m_Size / m_DigestLength;

    m_BinaryHashFileHandle = fopen(m_Path.c_str(), "r");
    if (m_BinaryHashFileHandle == nullptr)
    {
        std::cerr << "Error: unable to open binary hash file" << std::endl;
        return false;
    }

    if (m_Size % m_DigestLength != 0)
    {
        std::cerr << "Error: length of hash file does not match digest" << std::endl;
        return false;
    }

    // Mmap the file
    m_Base = (uint8_t*)mmap(nullptr, m_Size, PROT_READ, MAP_SHARED, fileno(m_BinaryHashFileHandle), 0);
    if (m_Base == nullptr)
    {
        std::cerr << "Error: Unable to map hashes file" << std::endl;
        return false;
    }

    if (Sort)
    {
        this->Sort();
    }

    return InitializeInternal();
}

const bool
HashList::Initialize(
    uint8_t* Base,
    const size_t Size,
    const size_t DigestLength,
    const bool Sort
)
{
    m_Base = Base;
    m_Size = Size;
    m_DigestLength = DigestLength;
    m_Count = m_Size / m_DigestLength;
    
    if (Sort)
    {
        this->Sort();
    }

    return InitializeInternal();
}

const bool
HashList::InitializeInternal(
    void
)
{
    auto ret = madvise(m_Base, m_Size, MADV_RANDOM|MADV_WILLNEED);
    if (ret != 0)
    {
        std::cerr << "Madvise not happy" << std::endl;
        return false;
    }

    // Build lookup table
    std::cerr << "Indexing hash table" << std::endl;

    // Zero the lengths
    memset(m_MappedTableLookupSize, 0, sizeof(m_MappedTableLookupSize));
    // Zero the pointers
    memset(m_MappedTableLookup, 0, sizeof(m_MappedTableLookup));

    const uint8_t* base = m_Base;
    
    // Save the first hash
    uint8_t* next = (uint8_t*)base;
    uint16_t last = *(uint16_t*)next;
    m_MappedTableLookup[last] = base;

    constexpr size_t READAHEAD = 256;

    // For lists larger than FAST_LOOKUP_THRESHOLD
    // We need to index the offset list
    if (m_Count >= FAST_LOOKUP_THRESHOLD)
    {
        // First pass
        std::cerr << "Pass 1" << std::endl;
        for (size_t i = 0; i < m_Count; i+= READAHEAD)
        {
            const uint16_t index = *(uint16_t*)next;
            if (index != last)
            {
                m_MappedTableLookup[index] = next;
            }
            last = index;
            next += (m_DigestLength * READAHEAD);
        }

        // Second pass
        std::cerr << "Pass 2" << std::endl;
        next = (uint8_t*)base + (m_DigestLength * (READAHEAD/2));
        for (size_t i = READAHEAD/2; i < m_Count; i+= READAHEAD)
        {
            const uint16_t index = *(uint16_t*)next;
            if (m_MappedTableLookup[index] == nullptr ||
                m_MappedTableLookup[index] > next)
            {
                m_MappedTableLookup[index] = next;
            }
            next += (m_DigestLength * READAHEAD);
        }

        // Third pass -> N'th pass
        // Loop over each known endpoint and check the previous entry
        size_t pass = 3;
        bool foundNewEntry;
        do
        {
            foundNewEntry = false;
            std::cerr << "Pass " << pass++ << std::endl;
            for (size_t i = 0; i < LOOKUP_SIZE; i++)
            {
                const uint8_t* offset = m_MappedTableLookup[i];
                if (offset == nullptr)
                {
                    continue;
                }

                // Check if it is the base
                if (offset == base)
                {
                    continue;
                }

                // Walk backwards until we find the previous
                const uint16_t index = *(uint16_t*)offset;
                for (;;)
                {
                    offset -= m_DigestLength;
                    const uint16_t next = *(uint16_t*)offset;
                    if (next != index)
                    {
                        if (m_MappedTableLookup[next] == nullptr)
                        {
                            m_MappedTableLookup[next] = offset;
                            foundNewEntry = true;
                        }
                        break;
                    }
                    m_MappedTableLookup[i] -= m_DigestLength;
                }
            }
        } while(foundNewEntry);

        // Calculate the counts
        // We walk through each item, look for the next offset
        // and calculate the distance between them
        const uint8_t* max = 0;
        uint16_t maxIndex = 0;
        for (size_t i = 0; i < LOOKUP_SIZE; i++)
        {
            const uint8_t* offset = m_MappedTableLookup[i];
            // Find the next closest offset
            const uint8_t* next = (const uint8_t*)-1;
            for (size_t j = i + 1; j < LOOKUP_SIZE; j++)
            {
                assert(j != i);

                const uint8_t* test = m_MappedTableLookup[j];
                if (test == nullptr)
                {
                    continue;
                }

                if (test > offset && test < next)
                {
                    next = test;
                }
            }
            // Calculate the distance
            assert(next > offset);
            
            if(next != (const uint8_t*)-1)
            {
                m_MappedTableLookupSize[i] = (next - offset);

                // Update the max so we can calculate the last entry
                if (offset > max)
                {
                    max = offset;
                    maxIndex = i;
                }
            }
        }

        // Calculate final size
        const uint8_t* end = m_Base + m_Size - m_DigestLength;
        m_MappedTableLookupSize[maxIndex] = (end - max);
    }

    return true;
}

const bool
HashList::LookupLinear(
    const uint8_t* Hash
) const
{
    for (uint8_t* offset = m_Base; offset < m_Base + m_Size; offset += m_DigestLength)
    {
        if (memcmp(offset, Hash, m_DigestLength) == 0)
        {
            return true;
        }
    }
    return false;
}

const bool
HashList::LookupFast(
    const uint8_t* Hash
) const
{
    const uint16_t index = *(uint16_t*)Hash;
    const uint8_t* base = m_MappedTableLookup[index];
    const size_t size = m_MappedTableLookupSize[index];

    if (base == nullptr)
    {
        return false;
    }

    return Lookup(
        base,
        size,
        Hash,
        m_DigestLength
    );
}

const bool
HashList::LookupBinary(
    const uint8_t* Hash
) const
{
    return Lookup(
        m_Base,
        m_Size,
        Hash,
        m_DigestLength
    );
}

const bool
HashList::Lookup(
    const uint8_t* Hash
) const
{
    if (m_Count >= FAST_LOOKUP_THRESHOLD)
    {
        return LookupFast(Hash);
    }
    else if (m_Count <= LINEAR_LOOKUP_THRESHOLD)
    {
        return LookupLinear(Hash);
    }
    else
    {
        return LookupBinary(Hash);
    }
}

void
HashList::Sort(
    void
)
{
    qsort_r(m_Base, m_Count, m_DigestLength, (__compar_d_fn_t)memcmp, (void*)m_DigestLength);
}