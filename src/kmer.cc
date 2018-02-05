#include <fstream>
#include "kmer.h"
#include <iostream>

/*return the integer representation of the base */
inline char Kmer::map_int(uint8_t base)
{
	switch(base) {
		case DNA_MAP::A: { return 'A'; }
		case DNA_MAP::T: { return 'T'; }
		case DNA_MAP::C: { return 'C'; }
		case DNA_MAP::G: { return 'G'; }
		default:  { return DNA_MAP::G+1; }
	}
}

/*return the integer representation of the base */
inline uint8_t Kmer::map_base(char base)
{
	switch(base) {
		case 'A': { return DNA_MAP::A; }
		case 'T': { return DNA_MAP::T; }
		case 'C': { return DNA_MAP::C; }
		case 'G': { return DNA_MAP::G; }
		default:  { return DNA_MAP::G+1; }
	}
}

/**
 * Converts a string of "ATCG" to a uint64_t
 * where each character is represented by using only two bits
 */
uint64_t Kmer::str_to_int(string str)
{
	uint64_t strint = 0;
	for (auto it = str.begin(); it != str.end(); it++) {
		uint8_t curr = 0;
		switch (*it) {
			case 'A': { curr = DNA_MAP::A; break; }
			case 'T': { curr = DNA_MAP::T; break; }
			case 'C': { curr = DNA_MAP::C; break; }
			case 'G': { curr = DNA_MAP::G; break; }
		}
		strint = strint | curr;
		strint = strint << 2;
	}
	return strint >> 2;
}

/**
 * Converts a uint64_t to a string of "ACTG"
 * where each character is represented by using only two bits
 */
string Kmer::int_to_str(uint64_t kmer)
{
	uint8_t base;
	string str;
	string rstr = "";
	for (int i=K; i>0; i--) {
		base = (kmer >> (i*2-2)) & 3ULL;
		char chr = Kmer::map_int(base);
		char rchr = 'T';
		switch(chr) {
			case 'A': {rchr = 'T'; break;}
			case 'C': {rchr = 'G'; break;}
			case 'G': {rchr = 'C'; break;}
			case 'T': {rchr = 'A'; break;}
		}
		str.push_back(chr);
		rstr = rchr + rstr;
	}
	//std::cout << "\n" << rstr << "\n" << str << "\n";
	if (rstr < str) return rstr;
	return str;
}

/* Return the reverse complement of a base */
inline int Kmer::reverse_complement_base(int x) { return 3 - x; }

/* Calculate the revsese complement of a kmer */
uint64_t Kmer::reverse_complement(uint64_t kmer)
{
	uint64_t rc = 0;
	uint8_t base = 0;
	for (int i=0; i<K; i++) {
		base = kmer & 3ULL;
		base = reverse_complement_base(base);
		kmer >>= 2;
		rc |= base;
		rc <<= 2;
	}
	rc >>=2;
	return rc;
}

/* Compare the kmer and its reverse complement and return the result 
 * Return true if the kmer is greater than or equal to its
 * reverse complement. 
 * */
inline bool Kmer::compare_kmers(uint64_t kmer, uint64_t kmer_rev)
{
	return kmer >= kmer_rev;
}

/* This code is taken from Jellyfish 2.0
 * git@github.com:gmarcais/Jellyfish.git 
 * */

// Checkered mask. cmask<uint16_t, 1> is every other bit on
// (0x55). cmask<uint16_t,2> is two bits one, two bits off (0x33). Etc.
template<typename U, int len, int l = sizeof(U) * 8 / (2 * len)>
struct cmask {
	static const U v =
		(cmask<U, len, l - 1>::v << (2 * len)) | (((U)1 << len) - 1);
};
template<typename U, int len>
struct cmask<U, len, 0> {
	static const U v = 0;
};

// Fast reverse complement of one word through bit tweedling.
inline uint32_t Kmer::word_reverse_complement(uint32_t w) {
	typedef uint64_t U;
	w = ((w >> 2)  & cmask<U, 2 >::v) | ((w & cmask<U, 2 >::v) << 2);
	w = ((w >> 4)  & cmask<U, 4 >::v) | ((w & cmask<U, 4 >::v) << 4);
	w = ((w >> 8)  & cmask<U, 8 >::v) | ((w & cmask<U, 8 >::v) << 8);
	w = ( w >> 16                   ) | ( w                    << 16);
	return ((U)-1) - w;
}

inline int64_t Kmer::word_reverse_complement(uint64_t w) {
	typedef uint64_t U;
	w = ((w >> 2)  & cmask<U, 2 >::v) | ((w & cmask<U, 2 >::v) << 2);
	w = ((w >> 4)  & cmask<U, 4 >::v) | ((w & cmask<U, 4 >::v) << 4);
	w = ((w >> 8)  & cmask<U, 8 >::v) | ((w & cmask<U, 8 >::v) << 8);
	w = ((w >> 16) & cmask<U, 16>::v) | ((w & cmask<U, 16>::v) << 16);
	w = ( w >> 32                   ) | ( w                    << 32);
	return ((U)-1) - w;
}

#ifdef HAVE_INT128
inline static unsigned __int128 Kmer::word_reverse_complement(unsigned __int128 w) {
	typedef unsigned __int128 U;
	w = ((w >> 2)  & cmask<U, 2 >::v) | ((w & cmask<U, 2 >::v) << 2);
	w = ((w >> 4)  & cmask<U, 4 >::v) | ((w & cmask<U, 4 >::v) << 4);
	w = ((w >> 8)  & cmask<U, 8 >::v) | ((w & cmask<U, 8 >::v) << 8);
	w = ((w >> 16) & cmask<U, 16>::v) | ((w & cmask<U, 16>::v) << 16);
	w = ((w >> 32) & cmask<U, 32>::v) | ((w & cmask<U, 32>::v) << 32);
	w = ( w >> 64                   ) | ( w                    << 64);
	return ((U)-1) - w;
}
#endif

mantis::QuerySets Kmer::parse_kmers(const char *filename, uint32_t seed,
                                    uint64_t range, uint64_t& total_kmers) {
  mantis::QuerySets multi_kmers;
	total_kmers = 0;
	std::ifstream ipfile(filename);
	std::string read;
	while (ipfile >> read) {
    mantis::QuerySet kmers_set;

		if (read.length() < K)
			continue;

start_read:
		uint64_t first = 0;
		uint64_t first_rev = 0;
		uint64_t item = 0;
		for(uint32_t i = 0; i < K; i++) { //First kmer
			uint8_t curr = Kmer::map_base(read[i]);
			if (curr > DNA_MAP::G) { // 'N' is encountered
				read = read.substr(i+1, read.length());
				goto start_read;
			}
			first = first | curr;
			first = first << 2;
		}
		first = first >> 2;
		first_rev = Kmer::reverse_complement(first);

		//cout << "kmer: "; cout << int_to_str(first);
		//cout << " reverse-comp: "; cout << int_to_str(first_rev) << endl;

		if (Kmer::compare_kmers(first, first_rev))
			item = first;
		else
			item = first_rev;

		// hash the kmer using murmurhash/xxHash before adding to the list
		//item = HashUtil::MurmurHash64A(((void*)&item), sizeof(item), seed);
		item = HashUtil::hash_64(item, BITMASK(2*K));
		kmers_set.insert(item % range);

		uint64_t next = (first << 2) & BITMASK(2*K);
		uint64_t next_rev = first_rev >> 2;

		for(uint32_t i=K; i<read.length(); i++) { //next kmers
			//cout << "K: " << read.substr(i-K+1,K) << endl;
			uint8_t curr = Kmer::map_base(read[i]);
			if (curr > DNA_MAP::G) { // 'N' is encountered
				read = read.substr(i+1, read.length());
				goto start_read;
			}
			next |= curr;
			uint64_t tmp = Kmer::reverse_complement_base(curr);
			tmp <<= (K*2-2);
			next_rev = next_rev | tmp;
			if (Kmer::compare_kmers(next, next_rev))
				item = next;
			else
				item = next_rev;

			// hash the kmer using murmurhash/xxHash before adding to the list
			//item = HashUtil::MurmurHash64A(((void*)&item), sizeof(item), seed);
			item = HashUtil::hash_64(item, BITMASK(2*K));
			kmers_set.insert(item % range);

			next = (next << 2) & BITMASK(2*K);
			next_rev = next_rev >> 2;
		}
		total_kmers += kmers_set.size();
		//if (kmers_set.size() != kmers.size())
			//std::cout << "set size: " << kmers_set.size() << " vector size: " << kmers.size() << endl;
		multi_kmers.push_back(kmers_set);
	}
	return multi_kmers;
}

// A=1, C=0, T=2, G=3
std::string Kmer::generate_random_string(uint64_t len) {
	std::string transcript;
	for (uint32_t i = 0; i < len; i++) {
		uint8_t c = rand() % 4;
		transcript += Kmer::map_int(c);
	}
	return transcript;
}
