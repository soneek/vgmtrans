#include "stdafx.h"
#include "HOSAScanner.h"
#include "HOSA.h"
#include "HOSAInstr.h"
#include "PSXSPU.h"

#define SRCH_BUF_SIZE 0x20000

HOSAScanner::HOSAScanner(void)
{
}

HOSAScanner::~HOSAScanner(void)
{
}

void HOSAScanner::Scan(RawFile* file, void* info)
{
	HOSASeq* seq = SearchForHOSASeq(file);
	if (!seq)
		return;
	PSXSampColl* sampcoll = PSXSampColl::SearchForPSXADPCM(file, HOSAFormat::name);
	if (!sampcoll)
		return;
	HOSAInstrSet* instrset = SearchForHOSAInstrSet(file, sampcoll);
	if (!instrset)
	{
		pRoot->RemoveVGMFile(sampcoll);
		return;
	}
	sampcoll->UseInstrSet(instrset);

	VGMColl* coll = new VGMColl(_T("HOSA Song"));
	coll->UseSeq(seq);
	coll->AddInstrSet(instrset);
	if (!coll->Load())
	{
		delete coll;
	}

	return;
}

HOSASeq* HOSAScanner::SearchForHOSASeq (RawFile* file)
{
	UINT nFileLength = file->size();
	for (UINT i=0; i+4<nFileLength; i++)
	{
		// Signature must match
		if (file->GetWordBE(i) != 0x484F5341 || file->GetByte(i+4) != 'V')		//"HOSAV"
			continue;
		// Number of tracks must not exceed 24 (I'm pretty sure)
		if (file->GetByte(i+6) > 24)
			continue;
		// First track pointer must != 0
		U16 firstTrkPtr = file->GetShort(i+0x50);
		if (firstTrkPtr == 0)
			continue;
		// First track pointer must be > second track pointer (if more than one track)
		if (firstTrkPtr >= file->GetShort(i+0x54) && firstTrkPtr != 0x54)
			continue;

		HOSASeq* seq = new HOSASeq(file, i);
		if (!seq->LoadVGMFile())
		{
			delete seq;
			return NULL;
		}
		return seq;
	}
	return NULL;
}

// This Scanner is quite imperfect.  It compares the offsets of the sample collection against the sample
// offsets in the region data, assuming that samples will be referenced consecutively.
#define MIN_NUM_SAMPLES_COMPARE 5
#define MIN_SAMPLES_MATCH 4
HOSAInstrSet* HOSAScanner::SearchForHOSAInstrSet (RawFile* file, PSXSampColl* sampcoll)
{
	U32 numSamples = sampcoll->samples.size();
	if (numSamples < MIN_NUM_SAMPLES_COMPARE)
	{
		pRoot->RemoveVGMFile(sampcoll);
		return NULL;
	}

	U32* sampOffsets = new U32[numSamples];
	for (unsigned int i=0; i<numSamples; i++)
		sampOffsets[i] = sampcoll->samples[i]->dwOffset - sampcoll->dwOffset;

	UINT nFileLength = file->size();
	for (UINT i=0x20; i+0x14<nFileLength; i++)
	{
		if (RecursiveRgnCompare(file, i, 0, numSamples, 0, sampOffsets))
		{
			for (; i>=0x20; i-=4)
			{
				if ((file->GetWord(i+4) != 0) || (file->GetWord(i) != 0))
					continue;

				HOSAInstrSet* instrset = new HOSAInstrSet(file, i);
				if (!instrset->LoadVGMFile())
				{
					delete instrset;
					delete[] sampOffsets;
					return NULL;
				}
				delete[] sampOffsets;
				return instrset;
			}
		}
	}
	delete[] sampOffsets;
	return NULL;
}

bool HOSAScanner::RecursiveRgnCompare(RawFile* file, int i, int sampNum, int numSamples, int numFinds, U32* sampOffsets)
{
	if (i < 0 || (ULONG)(i+0x14) >= file->size())
		return false;
	if (sampNum >= numSamples-1)
		return (numFinds >= MIN_SAMPLES_MATCH);
	// i+0 would be sample pointer of next region of same instr
	// i+4 would be sample pointer of first region in new instr
	U32 word1 = file->GetWord(i);
	U32 word2 = file->GetWord(i+4);
	U32 sampOffset = sampOffsets[sampNum];
	if (word1 == sampOffset)
		return RecursiveRgnCompare(file, i+0x10, sampNum+1, numSamples, numFinds+1, sampOffsets);	
	else if (word1 == 0)
		return RecursiveRgnCompare(file, i+0x10, sampNum+1, numSamples, numFinds, sampOffsets);
	else if (word2 == sampOffset)	
		return RecursiveRgnCompare(file, i+4+0x10, sampNum+1, numSamples, numFinds+1, sampOffsets);
	else if (word2 == 0)
		return RecursiveRgnCompare(file, i+4+0x10, sampNum+1, numSamples, numFinds, sampOffsets);
	else
		return (numFinds >= MIN_SAMPLES_MATCH);
}