// Many thanks to Neill Corlett for his work in analyzing the playstation SPU.
// Most of the code below is based on his work.
// Also, thanks to Antires for his ADPCM decompression routine.

#include "stdafx.h"
#include "PSXSPU.h"
#include "DLSFile.h"
#include "RawFile.h"






// ***********
// PSXSampColl
// ***********

PSXSampColl::PSXSampColl(const string& format, RawFile* rawfile, U32 offset, U32 length)
: VGMSampColl(format, rawfile, offset, length)
{
}

PSXSampColl::PSXSampColl(const string& format, VGMInstrSet* instrset, U32 offset, U32 length)
: VGMSampColl(format, instrset->rawfile, instrset, offset, length)
{
}

bool PSXSampColl::GetSampleInfo()
{
	//We scan through the sample section, and determine the offsets and size of each sample
	//We do this by searchiing for series of 16 0x00 value bytes.  These indicate the beginning of a sample,
	//and they will never be found at any other point within the adpcm sample data.

	UINT nFileLength = dwOffset+unLength;// = rawfile->size();
	if (unLength == 0)
		nFileLength = rawfile->size();  

	ULONG i = dwOffset;
	while (i < nFileLength-32)
	{
		if (GetWord(i) == 0 && GetWord(i+4) == 0 && GetWord(i+8) == 0 && GetWord(i+12) == 0  )
		{
			ULONG extraGunkLength = 0;
			BYTE filterRangeByte = GetByte(i+16);
			BYTE keyFlagByte = GetByte(i+16+1);
			if ((keyFlagByte & 0xF8) != 0)
				break;
			
			//if (filterRangeByte == 0 && keyFlagByte == 0)	// Breaking on FFXII 309 - Eruyt Village at 61D50 of the WD
			if (GetWord(i+16) == 0 && GetWord(i+20) == 0 && GetWord(i+24) == 0 && GetWord(i+28) == 0  )
				break;

			ULONG beginOffset = i;
			i += 16;
			
			//skip through until we reach the chunk with the end flag set
			for  ( ; (i < nFileLength) && ((GetByte(i+1) & 1) != 1); i += 16)
				;
			
			if (i+16 < nFileLength)
			{
				i += 16;
				BYTE theByte = GetByte(i+1); 
				//deal with exceptional cases where we see 00 07 77 77 77 77 77 etc.
				while (((theByte & 1) == 1) &&  (i < nFileLength))
				{
					extraGunkLength += 16;
					i += 16;
					theByte = GetByte(i+1);
				}
			}
			//for (BYTE theByte = GetByte(i+1); ((theByte & 1) == 1) && (i < nFileLength); i += 16)						
			//	theByte = GetByte(i+1);

			wostringstream name;
			name << L"Sample " << samples.size();
			PSXSamp* samp = new PSXSamp(this, beginOffset, i-beginOffset, beginOffset, i-beginOffset-extraGunkLength, 1, 16, 44100, name.str());
			samples.push_back(samp);
		}
		else
			break;
	}
	unLength = i - dwOffset;
	return true;
}






#define NUM_CHUNKS_READAHEAD 10
#define MAX_ALLOWED_RANGE_DIFF 10
#define MAX_ALLOWED_FILTER_DIFF 5
#define MIN_ALLOWED_RANGE_DIFF 0
#define MIN_ALLOWED_FILTER_DIFF 0

// GENERIC FUNCTION USED FOR SCANNERS
PSXSampColl* PSXSampColl::SearchForPSXADPCM (RawFile* file, const string& format)
{
	UINT nFileLength = file->size();
	for (UINT i=0; i+16+NUM_CHUNKS_READAHEAD*16<nFileLength; i++)
	{
		// if we have 16 0s in a row.
		if (file->GetWord(i) == 0 && file->GetWord(i+4) == 0 && file->GetWord(i+8) == 0 && file->GetWord(i+12) == 0  )
		{
			bool bBad = false;
			ULONG firstChunk = i+16;
			BYTE filterRangeByte = file->GetByte(firstChunk);
			BYTE keyFlagByte = file->GetByte(firstChunk+1);

			if (filterRangeByte == 0 && keyFlagByte == 0)
				continue;
			//if ((keyFlagByte & 0x04) == 0)
			//	continue;
			if ((keyFlagByte & 0xF8) != 0)
				continue;

			BYTE maxRangeChange = 0;
			BYTE maxFilterChange = 0;
			int prevRange = file->GetByte(firstChunk+16) & 0xF;					//+16 because we're skipping the first chunk for uncertain reasons
			int prevFilter = (file->GetByte(firstChunk+16) & 0xF0) >> 4;
			for (UINT j=0; j < NUM_CHUNKS_READAHEAD; j++)
			{
				ULONG curChunk = firstChunk+16+j*16;
				BYTE keyFlagByte = file->GetByte(curChunk + 1);
				if ((keyFlagByte & 0xFC) != 0)
				{
					bBad = true;
					break;
				}
				if ((file->GetWord(curChunk) == 0 && ((file->GetWord(curChunk+4) == 0) || file->GetWord(curChunk+8) == 0)) )
				{
					bBad = true;
					break;
				}

				//do range and filter value comparison
				int range = ((int)file->GetByte(firstChunk+16+j*16)) & 0xF;
				int diff = abs( range - prevRange);
				if (diff > maxRangeChange)
					maxRangeChange = diff; 
				prevRange = range;
				int filter = (((int)file->GetByte(firstChunk+16+j*16)) & 0xF0) >> 4;
				diff = abs(filter - prevFilter);
				if (diff > maxFilterChange)
					maxFilterChange = diff;
				prevFilter = filter;
			}
			if ((maxRangeChange > MAX_ALLOWED_RANGE_DIFF || maxFilterChange > MAX_ALLOWED_FILTER_DIFF) ||
				(maxRangeChange < MIN_ALLOWED_RANGE_DIFF || maxFilterChange < MIN_ALLOWED_FILTER_DIFF))
				continue;
			else if (bBad)
				continue;

			//VabSampColl* newSampColl = new VabSampColl(file, i);
			PSXSampColl* newSampColl = new PSXSampColl(format, file, i);
			if (!newSampColl->LoadVGMFile())
			{
				delete newSampColl;
				return NULL;
			}
			return newSampColl;
			//i += newSampColl->unLength-1;
		}
	}
	return NULL;
}





//  *******
//  PSXSamp
//  *******

PSXSamp::PSXSamp(VGMSampColl* sampColl, ULONG offset, ULONG length, ULONG dataOffset,
				 ULONG dataLen, BYTE nChannels, USHORT theBPS,
				 ULONG theRate, wstring name, bool bSetloopOnConversion)
: VGMSamp(sampColl, offset, length, dataOffset, dataLen, nChannels, theBPS, theRate, name),
  bSetLoopOnConversion(bSetloopOnConversion)
{
	bPSXLoopInfoPrioritizing = true;
}

PSXSamp::~PSXSamp()
{
}

double PSXSamp::GetCompressionRatio()
{
	return ((28.0/16.0)*2); //aka 3.5;
}


/*PSXSamp::PSXSamp(VGMInstrSet* parentSet, ULONG offset, DWORD compressedSize)
: VGMItem(parentSet, offset), parentInstrSet(parentSet), bLoops(false)
{
	SetCompressedSize(compressedSize);
}

PSXSamp::~PSXSamp(void)
{
}*/

/*void PSXSamp::SetCompressedSize(DWORD compSize)
{
	dwCompSize = compSize;
	dwUncompSize = (compSize/16)*56;
}*/

void PSXSamp::ConvertToStdWave(BYTE* buf)
{
	SHORT* uncompBuf = (SHORT*)buf;
	VAGBlk theBlock;
	f32 prev1;
	f32 prev2;

	if (this->bSetLoopOnConversion)
		SetLoopStatus(0); //loopStatus is initiated to -1.  We should default it now to not loop

	for (UINT k=0; k<dataLength; k+=0x10)				//for every adpcm chunk
	{
		theBlock.range =		GetByte(dwOffset+k) & 0xF;
		theBlock.filter =		(GetByte(dwOffset+k) & 0xF0) >> 4;
		theBlock.flag.end =		GetByte(dwOffset+k+1) & 1; 
		theBlock.flag.looping =	 (GetByte(dwOffset+k+1) & 2) > 0;
		
		theBlock.flag.loop =	(GetByte(dwOffset+k+1) & 4) > 0; //this can be the loop point, but in wd, this info is stored in the instrset
		if (theBlock.flag.loop)
		{
			if (this->bSetLoopOnConversion)
			{
				this->SetLoopOffset( k );
				this->SetLoopLength( dataLength - k );
			}
		}
		
		if (this->bSetLoopOnConversion)
			SetLoopStatus( GetLoopStatus() | theBlock.flag.looping );
		GetRawFile()->GetBytes(dwOffset+k+2, 14, theBlock.brr);

		DecompVAGBlk(uncompBuf+((k*28)/16), &theBlock, &prev1, &prev2);	//each decompressed pcm block is 52 bytes   EDIT: (wait, isn't it 56 bytes? or is it 28?)
		//if (theBlock.flag.end)
		//{
		//	dataLength = this->dwOffset - (k+16);	//sometimes the sample with 16 bytes padding b/w next sample
		//	if (this->bSetLoopOnConversion)
		//		if (GetLoopStatus())
		//			SetLoopLength(GetLoopLength() - 16);
		//	break;									//with the pattern 0x00 07 77 77 77 77 etc
		//}
	}
}
/*
int PSXSamp::UncompSample(SHORT* uncompBuf)
{
	VAGBlk theBlock;
	f32 prev1;
	f32 prev2;
	//VGMDoc* pDoc = assocDoc;

	for (UINT k=0; k<dwCompSize; k+=0x10)				//for every adpcm chunk
	{
		theBlock.range =		GetByte(dwOffset+k) & 0xF; //stuff[sample[i].offset+k] & 0xF;
		theBlock.filter =		(GetByte(dwOffset+k) & 0xF0) >> 4; //(stuff[sample[i].offset+k] & 0xF0) >> 4;
		theBlock.flag.end =		GetByte(dwOffset+k+1) & 1;  //stuff[sample[i].offset+k+1] & 1;
		theBlock.flag.looping =	 (GetByte(dwOffset+k+1) & 2) > 0;//(stuff[sample[i].offset+k+1] & 2) > 0;
		//theBlock.flag.loop =	(((stuff[sample[i].offset+k+1]) & 4) > 0); //this would be the loop point, but loop points are stored in the wd data
		bLoops |= theBlock.flag.looping;
		//test =	stuff[sample[i].offset+k+1];
		GetRawFile()->GetBytes(dwOffset+k+2, 14, theBlock.brr);
		//memcpy(theBlock.brr,stuff+SampVGMItem.dwOffset+k+2, 14);

		//if(test > 3)
		//{
			//printf("\nLoop Start Block at %X.  Offset in WD file %X. Sample num: %d", k, sample[i].offset+k, i);
		//}
		DecompVAGBlk(uncompBuf+((k*28)/0x10), &theBlock, &prev1, &prev2);	//each decompressed pcm block is 52 bytes
	}
	return TRUE;
}*/


//This next function is taken from Antires's work
void PSXSamp::DecompVAGBlk(s16 *pSmp, VAGBlk *pVBlk, f32 *prev1, f32 *prev2)
{
	u32	i,shift;								//Shift amount for compressed samples
	f32	t;										//Temporary sample
	f32	f1,f2;
	f32	p1,p2;
	static const f32	Coeff[5][2]={
		{  0.0,      0.0},
		{ 60.0/64.0, 0.0},
		{115.0/64.0, 52.0/64.0},
		{ 98.0/64.0, 55.0/64.0},
		{122.0/64.0, 60.0/64.0}};


	//Expand samples ---------------------------
	shift = pVBlk->range + 16;

	for (i=0; i<14; i++)
	{
		pSmp[i*2] = ((s32)pVBlk->brr[i] << 28) >> shift;
		pSmp[i*2+1] = ((s32)(pVBlk->brr[i] & 0xF0) << 24) >> shift;
	}

	//Apply ADPCM decompression ----------------
	i = pVBlk->filter;

	if (i)
	{
		f1 = Coeff[i][0];
		f2 = Coeff[i][1];
		p1 = *prev1;
		p2 = *prev2;

		for (i=0;i<28;i++)
		{
			t = pSmp[i] + (p1 * f1) - (p2 * f2);
			pSmp[i] = (s16)t;
			p2 = p1;
			p1 = t;
		}

		*prev1 = p1;
		*prev2 = p2;
	}
	else
	{
		*prev2 = pSmp[26];
		*prev1 = pSmp[27];
	}
}