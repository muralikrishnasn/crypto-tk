//
// libsse_crypto - An abstraction layer for high level cryptographic features.
// Copyright (C) 2015-2106 Raphael Bost
//
// This file is part of libsse_crypto.
//
// libsse_crypto is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// libsse_crypto is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with libsse_crypto.  If not, see <http://www.gnu.org/licenses/>.
//

/*******
 *  prf_mac.cpp
 *
 *  Implementation of the HMAC's test vector verification.
 *  Reference vectors are taken from RFC 4231 [https://tools.ietf.org/html/rfc4231]
 *  Only the first four test cases are implemented:
 *  the HMAC-based PRF implementation does not support keys larger than 64 bytes
 ********/

//#include "../tests/test_hmac.hpp"

#include "../src/hash/sha512.hpp"
#include "../src/hmac.hpp"
#include "../src/key.hpp"


#include <iostream>
#include <iomanip>
#include <string>

#include "gtest/gtest.h"

using namespace std;

template <uint16_t N> using HMAC_SHA512 = sse::crypto::HMac<sse::crypto::hash::sha512,N>;

//bool hmac_tests()
//{
//	return hmac_test_case_1() && hmac_test_case_2() && hmac_test_case_3() && hmac_test_case_4();
//}

TEST(hmac_sha_512, test_vector_1)
{

	array<uint8_t,HMAC_SHA512<20>::kKeySize> k;
	k.fill(0x0b);
	
	
    HMAC_SHA512<20> hmac(k.data());
    
	string in = "Hi There";
	
    array<uint8_t,64> result_64 = hmac.hmac(in);
	
	array<uint8_t,64> reference = {{
							0x87, 0xaa, 0x7c, 0xde, 0xa5, 0xef, 0x61, 0x9d, 0x4f, 0xf0, 0xb4, 0x24, 0x1a, 0x1d, 0x6c, 0xb0,
							0x23, 0x79, 0xf4, 0xe2, 0xce, 0x4e, 0xc2, 0x78, 0x7a, 0xd0, 0xb3, 0x05, 0x45, 0xe1, 0x7c, 0xde,
							0xda, 0xa8, 0x33, 0xb7, 0xd6, 0xb8, 0xa7, 0x02, 0x03, 0x8b, 0x27, 0x4e, 0xae, 0xa3, 0xf4, 0xe4,
							0xbe, 0x9d, 0x91, 0x4e, 0xeb, 0x61, 0xf1, 0x70, 0x2e, 0x69, 0x6c, 0x20, 0x3a, 0x12, 0x68, 0x54
								}};
	
	
    ASSERT_EQ(result_64, reference);
}

// Comment the next test as HMac explicitely takes keys larger than 16 bytes
/*
TEST(hmac_sha_512, test_vector_2)
{
	array<uint8_t,4> k = {{ 0x4a, 0x65, 0x66, 0x65}};
	
	
	HMAC_SHA512<4> hmac(k.data());

	unsigned char in [28] = 	{
							0x77, 0x68, 0x61, 0x74, 0x20, 0x64, 0x6f, 0x20, 0x79, 0x61, 0x20, 0x77, 0x61, 0x6e, 0x74, 0x20,
		                   	0x66, 0x6f, 0x72, 0x20, 0x6e, 0x6f, 0x74, 0x68, 0x69, 0x6e, 0x67, 0x3f
							};
		
							array<uint8_t,64> result_64 = hmac.hmac(in, 28);
	
	array<uint8_t,64> reference = 	{{
							0x16, 0x4b, 0x7a, 0x7b, 0xfc, 0xf8, 0x19, 0xe2, 0xe3, 0x95, 0xfb, 0xe7, 0x3b, 0x56, 0xe0, 0xa3,
							0x87, 0xbd, 0x64, 0x22, 0x2e, 0x83, 0x1f, 0xd6, 0x10, 0x27, 0x0c, 0xd7, 0xea, 0x25, 0x05, 0x54,
							0x97, 0x58, 0xbf, 0x75, 0xc0, 0x5a, 0x99, 0x4a, 0x6d, 0x03, 0x4f, 0x65, 0xf8, 0xf0, 0xe6, 0xfd,
							0xca, 0xea, 0xb1, 0xa3, 0x4d, 0x4a, 0x6b, 0x4b, 0x63, 0x6e, 0x07, 0x0a, 0x38, 0xbc, 0xe7, 0x37
									}};
	
	
    ASSERT_EQ(result_64, reference);
}
*/

TEST(hmac_sha_512, test_vector_3)
{
	array<uint8_t,HMAC_SHA512<20>::kKeySize> k;
	k.fill(0xaa);
	
	
	HMAC_SHA512<20> hmac(k.data());
		
	unsigned char in [50];
	memset(in,0xdd,50);
		
	array<uint8_t,64> result_64 = hmac.hmac(in, 50);
	
	array<uint8_t,64> reference = 	{{
							0xfa, 0x73, 0xb0, 0x08, 0x9d, 0x56, 0xa2, 0x84, 0xef, 0xb0, 0xf0, 0x75, 0x6c, 0x89, 0x0b, 0xe9,
							0xb1, 0xb5, 0xdb, 0xdd, 0x8e, 0xe8, 0x1a, 0x36, 0x55, 0xf8, 0x3e, 0x33, 0xb2, 0x27, 0x9d, 0x39,
							0xbf, 0x3e, 0x84, 0x82, 0x79, 0xa7, 0x22, 0xc8, 0x06, 0xb4, 0x85, 0xa4, 0x7e, 0x67, 0xc8, 0x07,
							0xb9, 0x46, 0xa3, 0x37, 0xbe, 0xe8, 0x94, 0x26, 0x74, 0x27, 0x88, 0x59, 0xe1, 0x32, 0x92, 0xfb
									}};
	
	
    ASSERT_EQ(result_64, reference);
}

TEST(hmac_sha_512, test_vector_4)
{
	array<uint8_t,25> k = {{ 	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
								0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19}};
	
	
	HMAC_SHA512<25> hmac(k.data());
	
	unsigned char in [50];
	memset(in,0xcd,50);
		
	array<uint8_t,64> result_64 = hmac.hmac(in, 50);
	
	array<uint8_t,64> reference = 	{{
								0xb0, 0xba, 0x46, 0x56, 0x37, 0x45, 0x8c, 0x69, 0x90, 0xe5, 0xa8, 0xc5, 0xf6, 0x1d, 0x4a, 0xf7,
								0xe5, 0x76, 0xd9, 0x7f, 0xf9, 0x4b, 0x87, 0x2d, 0xe7, 0x6f, 0x80, 0x50, 0x36, 0x1e, 0xe3, 0xdb,
								0xa9, 0x1c, 0xa5, 0xc1, 0x1a, 0xa2, 0x5e, 0xb4, 0xd6, 0x79, 0x27, 0x5c, 0xc5, 0x78, 0x80, 0x63,
								0xa5, 0xf1, 0x97, 0x41, 0x12, 0x0c, 0x4f, 0x2d, 0xe2, 0xad, 0xeb, 0xeb, 0x10, 0xa2, 0x98, 0xdd
									}};
	
	
    ASSERT_EQ(result_64, reference);
}

TEST(hmac, consistency)
{
    constexpr uint16_t HMAC_max_key_size = HMAC_SHA512<25>::kHMACKeySize;
    array<uint8_t,HMAC_max_key_size> key_arr, key_copy;

    sse::crypto::random_bytes(key_arr);
    key_copy = key_arr;
    
    HMAC_SHA512<HMAC_max_key_size> hmac1(key_arr.data());
    HMAC_SHA512<HMAC_max_key_size> hmac2(sse::crypto::Key<HMAC_max_key_size>(key_copy.data()));

    string in = sse::crypto::random_string(1000);
    auto ref = hmac1.hmac(in);
    
    ASSERT_EQ(ref, hmac2.hmac(in));
}

TEST(hmac, exception)
{ 
    ASSERT_THROW(HMAC_SHA512<25> hmac(NULL), std::invalid_argument);
}
