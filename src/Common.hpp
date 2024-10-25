//
//  Common.hpp
//  CrackDB++
//
//  Created by Kryc on 11/08/2024.
//  Copyright © 2024 Kryc. All rights reserved.
//

#ifndef Common_hpp
#define Common_hpp

#include <openssl/md5.h>
#include <openssl/sha.h>
#include <string>
#include <vector>

#include "Util.hpp"

#define MAX_DIGEST_LENGTH SHA512_DIGEST_LENGTH

typedef enum _HashAlgorithm
{
    HashAlgorithmUndefined,
    HashAlgorithmMD5,
    HashAlgorithmSHA1,
    HashAlgorithmSHA256,
    HashAlgorithmSHA384,
    HashAlgorithmSHA512
} HashAlgorithm;

const std::string
HashAlgorithmToString(
    const HashAlgorithm Algorithm
);

const size_t
GetDigestLength(
    const HashAlgorithm Algorithm
);

const HashAlgorithm
DetectHashAlgorithm(
    const std::vector<uint8_t>& Hash
);

inline const HashAlgorithm
DetectHashAlgorithm(
    const std::string& Hash
)
{
    return DetectHashAlgorithm(
        Util::ParseHex(Hash)
    );
}

inline void
DoHash(
    const HashAlgorithm Algorithm,
    const uint8_t* Data,
    const size_t Length,
    uint8_t* Digest
)
{
    switch (Algorithm)
    {
        case HashAlgorithmMD5:
            MD5(Data, Length, Digest);
            break;
        case HashAlgorithmSHA1:
            SHA1(Data, Length, Digest);
            break;
        case HashAlgorithmSHA256:
            SHA256(Data, Length, Digest);
            break;
        case HashAlgorithmSHA384:
            SHA384(Data, Length, Digest);
            break;
        case HashAlgorithmSHA512:
            SHA512(Data, Length, Digest);
            break;
        default:
            break;
    }
}

std::string
AsciiOrHex(
    std::string& Value
);

#endif /* Common_hpp */