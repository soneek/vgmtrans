#include "stdafx.h"
#include "SF2File.h"
#include "VGMInstrSet.h"
#include "VGMSamp.h"
#include "SynthFile.h"
#include "ScaleConversion.h"
#include "common.h"
#include "Root.h"



SF2InfoListChunk::SF2InfoListChunk(string name)
	: LISTChunk("INFO")
{
	// Create a date string
	SYSTEMTIME time;
	GetLocalTime(&time);
	stringstream datestr;
	datestr << time.wMonth << "/" << time.wDay << "/" << time.wYear;

	// Add the child info chunks
	Chunk* ifilCk = new Chunk("ifil");
	sfVersionTag versionTag;		//soundfont version 2.01
	versionTag.wMajor = 2;
	versionTag.wMinor = 1;
	ifilCk->SetData(&versionTag, sizeof(versionTag));
	AddChildChunk(ifilCk);
	AddChildChunk(new SF2StringChunk("isng", "EMU8000"));
	AddChildChunk(new SF2StringChunk("INAM", name));
	AddChildChunk(new SF2StringChunk("ICRD", datestr.str()));
	AddChildChunk(new SF2StringChunk("ISFT", string("VGMTrans " + string(VERSION)) ));
}


//  *******
//  SF2File
//  *******

SF2File::SF2File(SynthFile* synthfile)
	: RiffFile(synthfile->name, "sfbk")
{
	// convert wstring to stringstream
	//char* c_name = new char[name.length()+1];
	//wcstombs(c_name, name.c_str(), name.length()+1);
	//stringstream mbs_name;
	//mbs_name << c_name;
	//delete c_name;

	//***********
	// INFO chunk
	//***********
	AddChildChunk(new SF2InfoListChunk(name));

	// sdta chunk and its child smpl chunk containing all samples
	LISTChunk* sdtaCk = new LISTChunk("sdta");
	Chunk* smplCk = new Chunk("smpl");

	// Concatanate all of the samples together and add the result to the smpl chunk data
	int numWaves = synthfile->vWaves.size();
	smplCk->size = 0;
	for (int i = 0; i < numWaves; i++)
	{
		SynthWave* wave = synthfile->vWaves[i];
		wave->ConvertTo16bitSigned();
		smplCk->size += wave->dataSize + (46*2);	// plus the 46 padding samples required by sf2 spec
	}
	smplCk->data = new BYTE[smplCk->size];
	ULONG bufPtr = 0;
	for (int i = 0; i < numWaves; i++)
	{
		SynthWave* wave = synthfile->vWaves[i];

		memcpy(smplCk->data + bufPtr, wave->data, wave->dataSize);
		memset(smplCk->data + bufPtr + wave->dataSize, 0, 46*2);
		bufPtr += wave->dataSize + (46*2);		// plus the 46 padding samples required by sf2 spec
	}

	sdtaCk->AddChildChunk(smplCk);
	this->AddChildChunk(sdtaCk);

	//***********
	// pdta chunk
	//***********

	LISTChunk* pdtaCk = new LISTChunk("pdta");

	//***********
	// phdr chunk
	//***********
	Chunk* phdrCk = new Chunk("phdr");
	int numInstrs = synthfile->vInstrs.size();
	phdrCk->size = (numInstrs+1) * sizeof(sfPresetHeader);
	phdrCk->data = new BYTE[phdrCk->size];
	
	for (int i = 0; i < numInstrs; i++)
	{
		SynthInstr* instr = synthfile->vInstrs[i];
		
		sfPresetHeader presetHdr;
		memset(&presetHdr, 0, sizeof(sfPresetHeader));
		memcpy(presetHdr.achPresetName, instr->name.c_str(), min(instr->name.length(), 20));
		presetHdr.wPreset =			(WORD)instr->ulInstrument;
		presetHdr.wBank =			(WORD)instr->ulBank;
		presetHdr.wPresetBagNdx =	i;
		presetHdr.dwLibrary =		0;
		presetHdr.dwGenre =			0;
		presetHdr.dwMorphology =	0;

		memcpy(phdrCk->data + (i*sizeof(sfPresetHeader)), &presetHdr, sizeof(sfPresetHeader));
	}
	//  add terminal sfPresetBag
	sfPresetHeader presetHdr;
	memset(&presetHdr, 0, sizeof(sfPresetHeader));
	presetHdr.wPresetBagNdx =	numInstrs;
	memcpy(phdrCk->data + (numInstrs*sizeof(sfPresetHeader)), &presetHdr, sizeof(sfPresetHeader));
	pdtaCk->AddChildChunk(phdrCk);

	//***********
	// pbag chunk
	//***********
	Chunk* pbagCk = new Chunk("pbag");
	pbagCk->size = (numInstrs+1) * sizeof(sfPresetBag);
	pbagCk->data = new BYTE[pbagCk->size];
	for (int i = 0; i < numInstrs; i++)
	{
		SynthInstr* instr = synthfile->vInstrs[i];
		
		sfPresetBag presetBag;
		memset(&presetBag, 0, sizeof(sfPresetBag));
		presetBag.wGenNdx = i*2;
		presetBag.wModNdx = 0;

		memcpy(pbagCk->data + (i*sizeof(sfPresetBag)), &presetBag, sizeof(sfPresetBag));
	}
	//  add terminal sfPresetBag
	sfPresetBag presetBag;
	memset(&presetBag, 0, sizeof(sfPresetBag));
	presetBag.wGenNdx = numInstrs*2;
	memcpy(pbagCk->data + (numInstrs*sizeof(sfPresetBag)), &presetBag, sizeof(sfPresetBag));
	pdtaCk->AddChildChunk(pbagCk);

	//***********
	// pmod chunk
	//***********
	Chunk* pmodCk = new Chunk("pmod");
	//  create the terminal field
	sfModList modList;
	memset(&modList, 0, sizeof(sfModList));
	pmodCk->SetData(&modList, sizeof(sfModList));
	//modList.sfModSrcOper = cc1_Mod;
	//modList.sfModDestOper = startAddrsOffset;
	//modList.modAmount = 0;
	//modList.sfModAmtSrcOper = cc1_Mod;
	//modList.sfModTransOper = linear;
	pdtaCk->AddChildChunk(pmodCk);

	//***********
	// pgen chunk
	//***********
	Chunk* pgenCk = new Chunk("pgen");
	//pgenCk->size = (synthfile->vInstrs.size()+1) * sizeof(sfGenList);
	pgenCk->size = (synthfile->vInstrs.size() * sizeof(sfGenList) * 2) + sizeof(sfGenList);
	pgenCk->data = new BYTE[pgenCk->size];
	ULONG dataPtr = 0;
	for (int i = 0; i < numInstrs; i++)
	{
		SynthInstr* instr = synthfile->vInstrs[i];

		sfGenList genList;
		memset(&genList, 0, sizeof(sfGenList));
		
		// reverbEffectsSend
		genList.sfGenOper = reverbEffectsSend;
		genList.genAmount.shAmount= 700;
		memcpy(pgenCk->data + dataPtr, &genList, sizeof(sfGenList));
		dataPtr += sizeof(sfGenList);

		genList.sfGenOper = instrument;
		genList.genAmount.wAmount = i;
		memcpy(pgenCk->data + dataPtr, &genList, sizeof(sfGenList));
		dataPtr += sizeof(sfGenList);
	}
	//  add terminal sfGenList
	sfGenList genList;
	memset(&genList, 0, sizeof(sfGenList));
	memcpy(pgenCk->data + dataPtr, &genList, sizeof(sfGenList));
	
	pdtaCk->AddChildChunk(pgenCk);

	//***********
	// inst chunk
	//***********
	Chunk* instCk = new Chunk("inst");
	instCk->size = (synthfile->vInstrs.size()+1) * sizeof(sfInst);
	instCk->data = new BYTE[instCk->size];
	int rgnCounter = 0;
	for (int i = 0; i < numInstrs; i++)
	{
		SynthInstr* instr = synthfile->vInstrs[i];

		sfInst inst;
		memset(&inst, 0, sizeof(sfInst));
		memcpy(inst.achInstName, instr->name.c_str(), min(instr->name.length(), 20));
		inst.wInstBagNdx = rgnCounter;
		rgnCounter += instr->vRgns.size();

		memcpy(instCk->data + (i*sizeof(sfInst)), &inst, sizeof(sfInst));
	}
	//  add terminal sfInst
	sfInst inst;
	memset(&inst, 0, sizeof(sfInst));
	inst.wInstBagNdx = rgnCounter;
	memcpy(instCk->data + (numInstrs*sizeof(sfInst)), &inst, sizeof(sfInst));
	pdtaCk->AddChildChunk(instCk);

	//***********
	// ibag chunk - stores all zones (regions) for instruments 
	//***********
	Chunk* ibagCk = new Chunk("ibag");
	
	int numRgns = 0;
	for (int i = 0; i < numInstrs; i++)
		numRgns += synthfile->vInstrs[i]->vRgns.size();

	ibagCk->size = (numRgns+1) * sizeof(sfInstBag);
	ibagCk->data = new BYTE[ibagCk->size];

	rgnCounter = 0;
	int instGenCounter = 0;
	for (int i = 0; i < numInstrs; i++)
	{
		SynthInstr* instr = synthfile->vInstrs[i];

		int numRgns = instr->vRgns.size();
		for (int j = 0; j < numRgns; j++)
		{
			SynthRgn* rgn = instr->vRgns[j];
			sfInstBag instBag;
			memset(&instBag, 0, sizeof(sfInstBag));
			instBag.wInstGenNdx = instGenCounter;
			instGenCounter += 11;
			instBag.wInstModNdx = 0;

			memcpy(ibagCk->data + (rgnCounter++*sizeof(sfInstBag)), &instBag, sizeof(sfInstBag));			
		}	
	}
	//  add terminal sfInstBag
	sfInstBag instBag;
	memset(&instBag, 0, sizeof(sfInstBag));
	instBag.wInstGenNdx = instGenCounter;
	instBag.wInstModNdx = 0;
	memcpy(ibagCk->data + (rgnCounter*sizeof(sfInstBag)), &instBag, sizeof(sfInstBag));
	pdtaCk->AddChildChunk(ibagCk);

	//***********
	// imod chunk
	//***********
	Chunk* imodCk = new Chunk("imod");
	//  create the terminal field
	memset(&modList, 0, sizeof(sfModList));
	imodCk->SetData(&modList, sizeof(sfModList));
	pdtaCk->AddChildChunk(imodCk);

	//***********
	// igen chunk
	//***********
	Chunk* igenCk = new Chunk("igen");
	igenCk->size = (numRgns * sizeof(sfInstGenList) * 11) + sizeof(sfInstGenList);
	igenCk->data = new BYTE[igenCk->size];
	dataPtr = 0;
	for (int i = 0; i < numInstrs; i++)
	{
		SynthInstr* instr = synthfile->vInstrs[i];

		int numRgns = instr->vRgns.size();
		for (int j = 0; j < numRgns; j++)
		{
			SynthRgn* rgn = instr->vRgns[j];

			sfInstGenList instGenList;
			// Key range - (if exists) this must be the first chunk
			instGenList.sfGenOper = keyRange;
			instGenList.genAmount.ranges.byLo = (BYTE)rgn->usKeyLow;
			instGenList.genAmount.ranges.byHi = (BYTE)rgn->usKeyHigh;
			memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
			dataPtr += sizeof(sfInstGenList);

			// Velocity range (if exists) this must be the next chunk
			instGenList.sfGenOper = velRange;
			instGenList.genAmount.ranges.byLo = (BYTE)rgn->usVelLow;
			instGenList.genAmount.ranges.byHi = (BYTE)rgn->usVelHigh;
			memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
			dataPtr += sizeof(sfInstGenList);

			// initialAttenuation
			instGenList.sfGenOper = sampleModes;
			instGenList.genAmount.shAmount= (SHORT)(rgn->sampinfo->attenuation * 10);
			memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
			dataPtr += sizeof(sfInstGenList);

			// pan
			instGenList.sfGenOper = pan;
			instGenList.genAmount.shAmount= (SHORT)ConvertPercentPanTo10thPercentUnits(rgn->art->pan);
			memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
			dataPtr += sizeof(sfInstGenList);

			// sampleModes
			instGenList.sfGenOper = sampleModes;
			instGenList.genAmount.wAmount = rgn->sampinfo->cSampleLoops;
			memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
			dataPtr += sizeof(sfInstGenList);

			// overridingRootKey
			instGenList.sfGenOper = overridingRootKey;
			instGenList.genAmount.wAmount = rgn->sampinfo->usUnityNote;
			memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
			dataPtr += sizeof(sfInstGenList);

			// attackVolEnv
			instGenList.sfGenOper = attackVolEnv;
			instGenList.genAmount.shAmount = (rgn->art->attack_time == 0) ?  -32768 : round(SecondsToTimecents(rgn->art->attack_time));
			memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
			dataPtr += sizeof(sfInstGenList);

			// decayVolEnv
			instGenList.sfGenOper = decayVolEnv;
			instGenList.genAmount.shAmount = (rgn->art->decay_time == 0) ?  -32768 :  round(SecondsToTimecents(rgn->art->decay_time));
			memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
			dataPtr += sizeof(sfInstGenList);

			// sustainVolEnv
			instGenList.sfGenOper = sustainVolEnv;
			if (rgn->art->sustain_lev > 100.0)
				rgn->art->sustain_lev = 100.0;
			instGenList.genAmount.shAmount = (SHORT)(rgn->art->sustain_lev * 10);
			memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
			dataPtr += sizeof(sfInstGenList);

			// releaseVolEnv
			instGenList.sfGenOper = releaseVolEnv;
			instGenList.genAmount.shAmount = (rgn->art->release_time == 0) ?  -32768 : round(SecondsToTimecents(rgn->art->release_time));
			memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
			dataPtr += sizeof(sfInstGenList);

			// reverbEffectsSend
			//instGenList.sfGenOper = reverbEffectsSend;
			//instGenList.genAmount.shAmount = 800;
			//memcpy(pgenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
			//dataPtr += sizeof(sfInstGenList);

			// sampleID - this is the terminal chunk
			instGenList.sfGenOper = sampleID;
			instGenList.genAmount.wAmount = (WORD)(rgn->tableIndex);
			memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
			dataPtr += sizeof(sfInstGenList);

			//int numConnBlocks = rgn->art->vConnBlocks.size();
			//for (int k = 0; k < numConnBlocks; k++)
			//{
			//	SynthConnectionBlock* connBlock = rgn->art->vConnBlocks[k];
			//	connBlock->
			//}
		}	
	}
	//  add terminal sfInstBag
	sfInstGenList instGenList;
	memset(&instGenList, 0, sizeof(sfInstGenList));
	memcpy(igenCk->data + dataPtr, &instGenList, sizeof(sfInstGenList));
	//memset(ibagCk->data + (numRgns*sizeof(sfInstBag)), 0, sizeof(sfInstBag));
	//igenCk->SetData(&genList, sizeof(sfGenList));
	pdtaCk->AddChildChunk(igenCk);

	//***********
	// shdr chunk
	//***********
	Chunk* shdrCk = new Chunk("shdr");

	int numSamps = synthfile->vWaves.size();
	shdrCk->size = (numSamps+1) * sizeof(sfSample);
	shdrCk->data = new BYTE[shdrCk->size];

	
	ULONG sampOffset = 0;
	for (int i = 0; i < numSamps; i++)
	{
		SynthWave* wave = synthfile->vWaves[i];

		sfSample samp;
		memset(&samp, 0, sizeof(sfSample));
		memcpy(samp.achSampleName, wave->name.c_str(), min(wave->name.length(), 20));
		samp.dwStart = sampOffset;
		samp.dwEnd = samp.dwStart + (wave->dataSize / sizeof(WORD));
		sampOffset = samp.dwEnd + 46;		// plus the 46 padding samples required by sf2 spec

		// Search through all regions for an associated sampInfo structure with this sample
		SynthSampInfo* sampInfo = NULL;
		for (int j = 0; j < numInstrs; j++)
		{
			SynthInstr* instr = synthfile->vInstrs[j];

			int numRgns = instr->vRgns.size();
			for (int k = 0; k < numRgns; k++)
			{
				SynthRgn* rgn = instr->vRgns[k];
				if (rgn->tableIndex == i && rgn->sampinfo != NULL)
				{
					sampInfo = rgn->sampinfo;
					break;
				}
			}
			if (sampInfo != NULL)
				break;
		}
		//  If we didn't find a rgn association, then it should be in the SynthWave structure.
		if (sampInfo == NULL)
			sampInfo = wave->sampinfo;
		assert (sampInfo != NULL);

		samp.dwStartloop = samp.dwStart + sampInfo->ulLoopStart;
		samp.dwEndloop = samp.dwStartloop + sampInfo->ulLoopLength;
		samp.dwSampleRate = wave->dwSamplesPerSec;
		samp.byOriginalKey = (BYTE)(sampInfo->usUnityNote);
		samp.chCorrection = (CHAR)(sampInfo->sFineTune);
		samp.wSampleLink = 0;
		samp.sfSampleType = monoSample;

		memcpy(shdrCk->data + (i*sizeof(sfSample)), &samp, sizeof(sfSample));
	}

	//  add terminal sfSample
	memset(shdrCk->data + (numSamps*sizeof(sfSample)), 0, sizeof(sfSample));
	pdtaCk->AddChildChunk(shdrCk);

	this->AddChildChunk(pdtaCk);

}

SF2File::~SF2File(void)
{
	//DeleteVect(aInstrs);
	//DeleteVect(aWaves);
}


bool SF2File::SaveSF2File(const wchar_t* filepath)
{
	UINT size = this->GetSize();
	BYTE* buf = new BYTE[size];
	this->Write(buf);
	bool result = pRoot->UI_WriteBufferToFile(filepath, buf, size);
	delete[] buf;
	return result;
}
