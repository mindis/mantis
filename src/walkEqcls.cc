
#include "compressedSetBit.h"
#include "bitvector.h"
#include <time.h>
#include <random>


void print_time_elapsed(string desc, struct timeval* start, struct timeval* end) 
{
	struct timeval elapsed;
	if (start->tv_usec > end->tv_usec) {
		end->tv_usec += 1000000;
		end->tv_sec--;
	}
	elapsed.tv_usec = end->tv_usec - start->tv_usec;
	elapsed.tv_sec = end->tv_sec - start->tv_sec;
	float time_elapsed = (elapsed.tv_sec * 1000000 + elapsed.tv_usec)/1000000.f;
	std::cout << desc << "Total Time Elapsed: " << to_string(time_elapsed) << " seconds" << std::endl;
}

// @input
// cnt: setbit cnt 
// num_samples: total number of bits
// @return true if the output of delta_compression/decompression is the same as bitvector_rrr
//template<typename IndexSizeT>
bool validate(uint16_t cnt, uint16_t num_samples=2586) {

  std::cout << "validate for " << cnt << " set bits out of " << num_samples <<"\n";
  std::set<uint32_t> randIdx;

  BitVector bv(num_samples);
  std::vector<uint32_t> idxList(cnt);
  size_t i = 0;
  while (i < cnt) {
    //std::cout << "i:"<<i<<"\n";
    uint32_t currIdx = rand() % num_samples;
	//std::cout << "b" << currIdx << "\n";
    if (randIdx.find(currIdx) != randIdx.end()) continue;
    randIdx.insert(currIdx);
    bv.set(currIdx);
    idxList[i] = currIdx;
    //std::cout << currIdx << "\n";
    i++;
  }
  BitVectorRRR bvr(bv);
  std::vector<uint32_t> bvr_idxList;
  uint16_t wrdCnt = 64;
  for (uint16_t i = 0; i < num_samples; i+=wrdCnt) {
	wrdCnt = std::min((uint16_t)64, (uint16_t)(num_samples - i));
    uint64_t wrd = bvr.get_int(i, wrdCnt);
    for (uint16_t j = 0, idx=i; j < wrdCnt; j++, idx++) {
        if (wrd >> j & 0x01) {
			//std::cout << i << " " << j << "\n";
				bvr_idxList.push_back(idx);
		}
    }  
  }
  //std::cout <<"\nidx size: " << idxList.size() << "\n";
  CompressedSetBit<uint32_t> setBitList(idxList);
  
  vector<uint32_t> output;
  setBitList.uncompress(output);
  //std::cout << "\nAfter compress&decompress size is " << output.size() << "\n";
  if (output.size() != bvr_idxList.size()) {
		  std::cout << "rrr idx list size: " << bvr_idxList.size() << " deltac size: " << output.size() << "\n";
		  return false;
  }
  for (size_t i = 0; i < output.size(); i++)
    if (output[i] != bvr_idxList[i]) {
		  std::cout << i << " rrr idx: " << bvr_idxList[i] << " deltac: " << output[i] << "\n";
            return false;
    }       
  return true;
}

void compareCompressions(std::string& filename, size_t num_samples) {
  size_t gtBV = 0;
  size_t gtBVR = 0;
  size_t bvrGTbv = 0;
  size_t bvEQbvr = 0;
  size_t compressedSum = 0;
  size_t bvSum = 0;
  size_t bvrSum = 0;

  size_t roundIdxCnt = 0;
  size_t totalIdxCnt = 0;
  BitVectorRRR eqcls(filename);
  size_t totalEqClsCnt = eqcls.bit_size()/num_samples; //222584822;
  std::cout << "Total bit size: " << eqcls.bit_size() << "\ntotal # of equivalence classes: " << totalEqClsCnt << "\n";
  for (size_t eqclsCntr = 0; eqclsCntr < totalEqClsCnt; eqclsCntr++) {
    BitVector bv(num_samples);
    std::vector<uint32_t> idxList;
    idxList.reserve(num_samples);
	size_t i = 0;
    while (i < num_samples) {
      size_t bitCnt = std::min(num_samples-i, (size_t)64);
      size_t wrd = eqcls.get_int(eqclsCntr*num_samples+i, bitCnt);
      for (size_t j = 0, curIdx = i; j < bitCnt; j++, curIdx++) {
        if ((wrd >> j) & 0x01) {
          bv.set(curIdx);
          idxList.push_back(curIdx);
        }
      }
      i+=bitCnt;
    }
    //if (idxList.size() == 0) std::cerr << "Error!! " << eqclsCntr << " Shouldn't ever happen\n";
    totalIdxCnt += idxList.size();
    roundIdxCnt += idxList.size();
    if (eqclsCntr != 0 && (eqclsCntr) % 1000000 == 0) {
      std::cout << "\n\nTotal number of experiments are : " << eqclsCntr <<
        "\nTotal average set bits: " << totalIdxCnt /eqclsCntr <<
        "\nThis round average set bits: " << roundIdxCnt/1000000 <<
                                                        "\nbv average size : " << bvSum/eqclsCntr <<
                                                        "\nbvr average size : " << bvrSum/eqclsCntr <<
                                                        "\ncompressed average size : " << compressedSum/eqclsCntr <<
                                                        "\ncompressed > bv : " << gtBV << " or " << (gtBV*100)/eqclsCntr << "% of the time" <<
                                                        "\ncompressed > bvr : " << gtBVR << " or " << (gtBVR*100)/eqclsCntr << "% of the time" <<
                                                        "\nbvr > bv : " << bvrGTbv << " or " << (bvrGTbv*100)/eqclsCntr << "% of the time" <<
                                                        "\n";
      roundIdxCnt = 0;
    }

    BitVectorRRR bvr(bv);
    CompressedSetBit<uint64_t> setBitList(idxList);

    size_t bvSize = bv.size_in_bytes();
    size_t bvrSize = bvr.size_in_bytes();
    size_t compressedSize = setBitList.size_in_bytes();
    bvSum += bvSize;
    bvrSum += bvrSize;
    compressedSum += compressedSize;
    if (compressedSize > bvSize) gtBV++;
    if (compressedSize > bvrSize) gtBVR++;
    if (bvrSize > bvSize) bvrGTbv++;
    if (bvrSize == bvSize) bvEQbvr++;
    //vector<uint32_t> output;
    //setBitList.uncompress(output);
  }
  std::cout << "\n\n\nFinalResults:\n" <<
    "Total number of experiments are : " << totalEqClsCnt << "\n" <<
    "\nbv average size : " << bvSum/totalEqClsCnt <<
    "\nbvr average size : " << bvrSum/totalEqClsCnt <<
    "\ncompressed average size : " << compressedSum/totalEqClsCnt <<
    "\nHow many times compressed > bv : " << gtBV << " or " << (gtBV*100)/totalEqClsCnt << "% of the time" <<
    "\nHow many times compressed > bvr : " << gtBVR << " or " << (gtBVR*100)/totalEqClsCnt << "% of the time" <<
    "\nHow many times bvr > bv : " << bvrGTbv << " or " << (bvrGTbv*100)/totalEqClsCnt << "% of the time" <<
    "\nHow many times bvr == bv : " << bvEQbvr <<  " or " << (bvEQbvr*100)/totalEqClsCnt << "% of the time" <<
    "\n";
}

void compareCopies(size_t num_samples) {
  struct timeval start, end;
	struct timezone tzp;

  size_t numOfCopies = 10;
  
  size_t bit_cnt = 1000000000;
  size_t i;
  sdsl::bit_vector bits(bit_cnt);
  srand (time(NULL));

  gettimeofday(&start, &tzp);  
  for (auto i = 0; i < 1000000; i++) {
    bits[rand()% bit_cnt] = 1;
  } 
  gettimeofday(&end, &tzp);
	print_time_elapsed("Set random bits done: ", &start, &end);

  sdsl::bit_vector bits2(bit_cnt);
  sdsl::bit_vector bits3(bit_cnt);
  sdsl::bit_vector bits4(bit_cnt+1);

  gettimeofday(&start, &tzp);
  for (auto c = 0; c < numOfCopies; c++) {
    bits2 = bits;
  }
  gettimeofday(&end, &tzp);
	print_time_elapsed("MemCpy: ", &start, &end);

  gettimeofday(&start, &tzp);
  for (auto c = 0; c < numOfCopies; c++) {
    i = 0;
    while (i < bit_cnt) {
      if (bits[i])
        bits3[i] = 1;
      i++;
    }
  }
  gettimeofday(&end, &tzp);
	print_time_elapsed("Index by index!: ", &start, &end);

  gettimeofday(&start, &tzp);
  for (auto c = 0; c < numOfCopies; c++) {
    i = 0;
	size_t j = 1;
    while (i < bit_cnt) {
      size_t bitCnt = std::min(bit_cnt-i, (size_t)64);
      size_t wrd = bits.get_int(i, bitCnt);
      bits4.set_int(j, wrd, bitCnt);
      i+=bitCnt;
	  j+=bitCnt;
    }
  }
  gettimeofday(&end, &tzp);
	print_time_elapsed("Read int and write int: ", &start, &end);

}

void run_reorder(std::string filename, 
				std::string out_filename, 
        uint64_t num_samples) {
  
  std::vector<std::vector<double>> editDistMat(num_samples);
  for (auto& v : editDistMat) {
    v.resize(num_samples);
    for (auto& d : v) d = 0;
  }
  BitVectorRRR eqcls(filename);
  size_t totalEqClsCnt = eqcls.bit_size()/num_samples; //222584822;
  //totalEqClsCnt = 10;
  std::cout << "Total bit size: " << eqcls.bit_size() 
            << "\ntotal # of equivalence classes: " << totalEqClsCnt << "\n";
  for (size_t eqclsCntr = 0; eqclsCntr < totalEqClsCnt; eqclsCntr++) {
    std::vector<bool> row(num_samples);
    for (size_t i = 0; i < row.size(); i++) row[i] = false;

    // set the set bits
    size_t i = 0;
    while (i < num_samples) {
      size_t bitCnt = std::min(num_samples-i, (size_t)64);
      size_t wrd = eqcls.get_int(eqclsCntr*num_samples+i, bitCnt);
      for (size_t j = 0, curIdx = i; j < bitCnt; j++, curIdx++) {
        if ((wrd >> j) & 0x01) {
          row[i] = true;
        }
      }
      i+=bitCnt;
    }
    for (i = 0; i < row.size(); i++) {
      for (size_t j = i+1; j < row.size(); j++) {
        if (row[i] != row[j]) editDistMat[i][j]++;
      }
    }
    //if (eqclsCntr != 0 && (eqclsCntr) % 1000000 == 0) {
      std::cout << eqclsCntr << " eqclses processed\n";
    //}
  }

  std::ofstream out(out_filename);
  out << editDistMat.size() << "\n";
  for (size_t i = 0; i < editDistMat.size(); i++) {
    //bool firstTime = true;
    for (size_t j = 0; j < editDistMat[i].size(); j++) {
      if (editDistMat[i][j] != 0) {
        /* if (firstTime) {
          std::cout << i << ":\t";
          firstTime = false;
        } */
        std::cout << j << " ";
        out << i << "\t" << j << "\t" << editDistMat[i][j] << "\n";
      }
    }
    /* if (!firstTime) {
      std::cout << "\n";
    } */
  }

}

int main(int argc, char *argv[]) {


  uint64_t num_samples = 2586;
  std::string command = argv[1];

  if (command == "validate") {
    std::cout << "validate for different number of set bits\n";
    for (uint16_t i = 1; i <= num_samples; i++){
        if (!validate(i, num_samples)) {
          std::cerr << "ERROR: NOT VALIDATED\n";
          std::exit(1);
      }
    }
    std::cout << "SUCCESSFULLY VALIDATED THE COMPRESSION/DECOMPRESSION PROCESS.\n\n#####NEXT STEP#####\nCompare Sizes:\n";
  }
  else if (command == "compareCompressions") {
    std::string filename = argv[2];
    compareCompressions(filename, num_samples);
  }
  else if (command == "compareCopies") 
    compareCopies(num_samples);
  else if (command == "reorder") {
    if (argc < 4) {
      std::cerr << "ERROR: MISSING LAST ARGUMENT\n";
      std::exit(1);
    }
    std::string filename = argv[2];
    std::string output_filename = argv[3];
    run_reorder(filename, output_filename, num_samples);
  } else {
    std::cerr << "ERROR: NO COMMANDS PROVIDED\n"
              << "OPTIONS ARE: validate, compareCompressions, compareCopies, reorder\n";
    std::exit(1);
  }

}