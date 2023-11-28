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
	cout << endl;

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
	if (MemW){
		bitset<32> addrBits(adr);
		cout << addrBits << endl;
		int index = ((addrBits & (bitset<32>(0x3C))) >> 2).to_ulong(); //get index which is 4 bits after two-bit offset
		int tag = (addrBits >> 6).to_ulong(); // get value of upper 26 bits for tag
		cout << "index: " << index << endl;
		cout << "tag: " << tag << endl;

		bool L1Hit = false;
		bool VCHit = false;
 		
		// check if address is in cache
		if (L1[index].valid && L1[index].tag == tag){
			cout << "memW found in L1" << endl;
			L1Hit = true;
		}
			

		if (!L1Hit){
			// search victim cache first

			for (int i = 0; i < VICTIM_SIZE; i++){
				if (victimCache[i].valid && victimCache[i].tag == tag){
					updateVictimLRU(i);
					VCHit = true;
					cout << "memW found in VC" << endl;
					victimCache[i].data = *data;
					break;
				}
			}

			// search L2 if not in victim

			if (!VCHit){
				for (int i = 0; i < L2_CACHE_WAYS; i++){
					if (L2[index][i].valid && L2[index][i].tag == tag){
						updateL2LRU(index, i);
						L2[index][i].data = *data;
						cout << "memW found in L2" << endl;
						break;
					}
				}
			}
		}

		myMem[adr] = *data;
		cout << "myMem at adr: " << adr << " contains data: " << *data << endl << endl;
	}
}

bool cache::updateL1(int* data, int addr, int* myMem){
	// get the data from memory if needed
	// unless you get it from controlelr idk

	// calcualte tag index and what not from address
	// if unable to insert into L1, success = false -> if false, insert into L2
	bool success = false;
	bitset<32> addrBits(addr);
	int tag = (addrBits & (bitset<32>(0xFC000000))).to_ulong();
	cout << "tag: " << tag << endl;

	return success;
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

void cache::updateL2LRU(int set, int way){
	if (L2[set][way].lru_position != 7){
		for (int i = 0; i < L2_CACHE_WAYS; i++){
			if (i != way){
				if (L2[set][i].lru_position > L2[set][way].lru_position)
					L2[set][i].lru_position --;
			}
		}
		L2[set][way].lru_position = 7;
	}
}