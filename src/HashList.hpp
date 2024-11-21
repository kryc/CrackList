//
//  HashList.hpp
//  HashList
//
//  Created by Kryc on 12/11/2024.
//  Copyright © 2024 Kryc. All rights reserved.
//

#ifndef HashList_hpp
#define HashList_hpp

#include <filesystem>

#include <stdio.h>

class HashList
{
public:
    HashList(void) = default;
    const bool Initialize(const std::filesystem::path Path, const size_t DigestLength, const bool Sort = false);
    const bool Initialize(uint8_t* Base, const size_t Size, const size_t DigestLength, const bool Sort = true);
    const bool Lookup(const uint8_t* Hash) const;
    const bool LookupLinear(const uint8_t* Hash) const;
    const bool LookupFast(const uint8_t* Hash) const;
    const bool LookupBinary(const uint8_t* Hash) const;
    void Sort(void);
    const size_t GetCount(void) const { return m_Count; };
    // Static
    static const bool Lookup(const uint8_t* Base, const size_t Size, const uint8_t* Hash, const size_t HashSize);
private:
    const bool InitializeInternal(void);
    std::filesystem::path m_Path;
    size_t m_DigestLength;
    FILE* m_BinaryHashFileHandle;
    uint8_t* m_Base;
    size_t m_Size;
    size_t m_Count;
    static constexpr size_t LOOKUP_SIZE = std::numeric_limits<uint16_t>::max() + 1;
    const uint8_t* m_MappedTableLookup[LOOKUP_SIZE];
    size_t m_MappedTableLookupSize[LOOKUP_SIZE];
};

#endif //HashList_hpp