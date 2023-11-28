#include "cache.h"

cache::cache()
{
	for (int i=0; i<L1_CACHE_SETS; i++)
		L1[i].valid = false; 
	

	for (int i=0; i<L2_CACHE_SETS; i++)
	{
		int LRUpos = 0;
		for (int j=0; j<L2_CACHE_WAYS; j++){
			L2[i][j].valid = false;
			L2[i][j].lru_position = LRUpos;
			LRUpos++;
		}
	}
	
	int LRUpos = 0;
	for (int i=0; i < VICTIM_SIZE; i++){
		victimCache[i].valid = false; 
		victimCache[i].lru_position = LRUpos;
		LRUpos++;
	}
	// cout << endl;

	// Do the same for Victim Cache ...

	this->myStat.missL1 =0;
	this->myStat.missL2 =0;
	this->myStat.accL1 =0;
	this->myStat.accL2 =0;

	// Add stat for Victim cache ... 	
	this->myStat.missVic = 0;
	this->myStat.accVic = 0;

	
}
void cache::controller(bool MemR, bool MemW, int* data, int adr, int* myMem)
{
	// add your code here

	bitset<32> addrBits(adr);
	// cout << addrBits << endl;
	int index = ((addrBits & (bitset<32>(0x3C))) >> 2).to_ulong(); //get index which is 4 bits after two-bit offset
	int tag = (addrBits >> 6).to_ulong(); // get value of upper 26 bits for tag
	// cout << "address: " << adr << endl;
	// cout << "tag: " << tag << endl;
	// cout << "index: " << index << endl;
	
	bool L1Hit = false;
	bool VCHit = false;
	bool L2Hit = false;

	if (MemW){
		// check if address is in cache
		if (L1[index].valid && L1[index].tag == tag){
			L1Hit = true;
		}
			

		if (!L1Hit){
			// search victim cache first
			bitset<4> indexBits(index);
			bitset<26> tagBits(tag);
			bitset<30> indexTagBits(indexBits.to_string() + tagBits.to_string());
			int indexTag = indexTagBits.to_ulong();

			// cout << "indexBits: " << indexBits << endl;
			// cout << "tagBits: " << tagBits << endl;
			// cout << "indexTag: " << indexTagBits<<endl;

			for (int i = 0; i < VICTIM_SIZE; i++){
				if (victimCache[i].valid && victimCache[i].tag == indexTag){
					updateVictimLRU(i);
					VCHit = true;
					victimCache[i].data = *data;
					break;
				}
			}
		}

		// search L2 if not in victim

		if (!VCHit){
			for (int i = 0; i < L2_CACHE_WAYS; i++){
				if (L2[index][i].valid && L2[index][i].tag == tag){
					updateL2LRU(index, i);
					L2[index][i].data = *data;
					break;
				}
			}
		}

		myMem[adr] = *data;
		// cout << "myMem at adr: " << adr << " contains data: " << *data << endl << endl;
	}
	
	else if (MemR){
		// cout << "READ" << endl;
		if (L1[index].valid && L1[index].tag == tag){
			// cout << "memR found in L1" << endl << endl;
			myStat.accL1++;
			L1Hit = true;
			return;
		}

		if (!L1Hit){
			myStat.missL1++;
			bitset<4> indexBits(index);
			bitset<26> tagBits(tag);
			bitset<30> indexTagBits(indexBits.to_string() + tagBits.to_string());
			int indexTag = indexTagBits.to_ulong();

			for (int i = 0; i < VICTIM_SIZE; i++){
				if (victimCache[i].valid && victimCache[i].tag == indexTag){
					myStat.accVic++;
					VCHit = true;
					// cout << "memR found in VC" << endl << endl;;
					// swap L1 and VC lines

					bitset<32> L1tag(L1[index].tag); // get tag of what is currently in L1
					int L1data = L1[index].data;
					
					// swap L1 tag and data for VC tag and data
					L1[index].tag = tag;
					L1[index].data = victimCache[i].data;

					int L1indexTag = (bitset<30>(indexBits.to_string() + L1tag.to_string())).to_ulong(); // get indexTag of what we swapped in L1
					victimCache[i].tag = L1indexTag;
					victimCache[i].data = L1data;

					updateVictimLRU(i); // update LRU of victim cache
					return;
				}
			}
		}

		if (!VCHit){
			myStat.missVic++;
			for (int i = 0; i < L2_CACHE_WAYS; i++){
				if (L2[index][i].valid && L2[index][i].tag == tag){
					myStat.accL2++;
					L2Hit = true;
					// cout << "memR found in L2" << endl << endl;

					bitset<4> indexBits(index);
					bitset<26> L1tag(L1[index].tag);
					int L1data = L1[index].data;

					// update L1 to have contents of L2
					L1[index].tag = tag;
					L1[index].data = L2[index][i].data;

					L2[index][i].valid = false; // evict L2 line

					// find line in victim cache to put old L1
					int victimLRU = findVictimLRU();
					// get tag and index of current victim cache to later put into L2
					int victimIndex = (bitset<30>(victimCache[victimLRU].tag) >> 26).to_ulong();
					int victimTag = ((bitset<30>(victimCache[victimLRU].tag) << 4) >> 4).to_ulong();
					int victimData = victimCache[victimLRU].data;

					int L1indexTag = (bitset<30>(indexBits.to_string() + L1tag.to_string())).to_ulong();
					victimCache[victimLRU].tag = L1indexTag;
					victimCache[victimLRU].data = L1data;
					updateVictimLRU(victimLRU);

					// place evicted victim cache line into L2
					int L2LRU = findL2LRU(victimIndex);
					L2[victimIndex][L2LRU].valid = true;
					L2[victimIndex][L2LRU].tag = victimTag;
					L2[victimIndex][L2LRU].data = victimData;
					updateL2LRU(victimIndex, L2LRU);

				}
			}
		}

		if (!L2Hit){
			// cout << "memR had to fetch from mem" << endl << endl;
			myStat.missL2++;
			*data = myMem[adr];

			if (!L1[index].valid){
				L1[index].valid = true;
				L1[index].tag = tag;
				L1[index].data = *data;
			}else{
				// get tag and data of what is currently in L1
				bitset<4> indexBits(index);
				bitset<26> L1tag(L1[index].tag);
				int L1data = L1[index].data;
				
				// update L1 w/ contents from memory
				L1[index].tag = tag;
				L1[index].data = *data;

				// put evicted line into VC
				int victimLRU = findVictimLRU();
				int victimIndex = (bitset<30>(victimCache[victimLRU].tag) >> 26).to_ulong();
				int victimTag = ((bitset<30>(victimCache[victimLRU].tag) << 4) >> 4).to_ulong();
				int victimData = victimCache[victimLRU].data;
				
				// cout << "victim LRU: " << victimLRU << endl;

				int L1indexTag = (bitset<30>(indexBits.to_string() + L1tag.to_string())).to_ulong();
				victimCache[victimLRU].tag = L1indexTag;
				victimCache[victimLRU].data = L1data;
				updateVictimLRU(victimLRU);

				if (!victimCache[victimLRU].valid){
					// cout << "**" << endl;
					victimCache[victimLRU].valid = true;
				}else{
					// put evicted line into L2
					int L2LRU = findL2LRU(victimIndex);
					L2[victimIndex][L2LRU].valid = true;
					L2[victimIndex][L2LRU].tag = victimTag;
					L2[victimIndex][L2LRU].data = victimData;

					// cout << "placed index: " << victimIndex << " and tag: " << victimTag << " into L2" << endl;
					updateL2LRU(victimIndex, L2LRU);
				}

				// cout << "current victim cache: " << endl;
				// for (int i = 0; i < VICTIM_SIZE; i++){
				// 	cout << bitset<30>(victimCache[i].tag) << endl;
				// }
				// cout << "---" << endl << endl;
			}
		}
	}
}

void cache::updateVictimLRU(int index){
	if (victimCache[index].lru_position != 3){
		for (int i = 0; i < VICTIM_SIZE; i++){
			if (i != index){
				if (victimCache[i].lru_position > victimCache[index].lru_position)
					victimCache[i].lru_position--;
			}
		}
		victimCache[index].lru_position = 3;
	}
}

int cache::findVictimLRU(){
	for (int i = 0; i < VICTIM_SIZE; i++){
		if (!victimCache[i].valid)
			return i;
		else if (victimCache[i].lru_position == 0)
			return i;
	}
	return 0;
}

void cache::updateL2LRU(int set, int way){
	if (L2[set][way].lru_position != 7){
		for (int i = 0; i < L2_CACHE_WAYS; i++){
			if (i != way){
				if (L2[set][i].lru_position > L2[set][way].lru_position)
					L2[set][i].lru_position--;
			}
		}
		L2[set][way].lru_position = 7;
	}
}

int cache::findL2LRU(int index){
	for (int i = 0; i < L2_CACHE_WAYS; i++){
		if (!L2[index][i].valid)
			return i;
		else if (L2[index][i].lru_position == 0)
			return i;
	}
	return 0;
}

double cache::L1missRate(){
	// cout << (double(myStat.missL1)/double(myStat.missL1 + myStat.accL1)) << endl;
	return (double(myStat.missL1)/double(myStat.missL1 + myStat.accL1));
}

double cache::L2missRate(){
	// cout << (double(myStat.missL2)/double(myStat.missL2 + myStat.accL2)) << endl;
	return (double(myStat.missL2)/double(myStat.missL2 + myStat.accL2));
}

double cache::VCmissRate(){
	// cout << (double(myStat.missVic)/double(myStat.missVic + myStat.accVic)) << endl;
	return (double(myStat.missVic)/double(myStat.missVic + myStat.accVic));
}