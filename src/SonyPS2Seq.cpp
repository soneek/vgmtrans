#include "stdafx.h"
#include "SonyPS2Seq.h"
#include "SonyPS2Format.h"
#include "ScaleConversion.h"

DECLARE_FORMAT(SonyPS2);

// ******
// BGMSeq
// ******

SonyPS2Seq::SonyPS2Seq(RawFile* file, ULONG offset)
: VGMSeqNoTrks(SonyPS2Format::name, file, offset),
  compOption(0),
  bSkipDeltaTime(0)
{
	UseLinearAmplitudeScale();		// Onimusha: Kaede Theme track 2 for example of linear vol scale.
	UseReverb();
	//AlwaysWriteInitialPitchBendRange(12, 0);
	//AlwaysWriteInitialVol(127);
}

SonyPS2Seq::~SonyPS2Seq(void)
{
}

bool SonyPS2Seq::GetHeaderInfo(void)
{
	name() = L"Sony PS2 Seq";
	U32 curOffset = offset();
	//read the version chunk
	GetBytes(curOffset, 0x10, &versCk);		
	VGMHeader* versCkHdr = VGMSeq::AddHeader(curOffset, versCk.chunkSize, L"Version Chunk");
	versCkHdr->AddSimpleItem(curOffset, 4, L"Creator");
	versCkHdr->AddSimpleItem(curOffset+4, 4, L"Type");
	curOffset += versCk.chunkSize;

	//read the header chunk
	GetBytes(curOffset, 0x20, &hdrCk);		
	VGMHeader* hdrCkHdr = VGMSeq::AddHeader(curOffset, hdrCk.chunkSize, L"Header Chunk");
	hdrCkHdr->AddSimpleItem(curOffset, 4, L"Creator");
	hdrCkHdr->AddSimpleItem(curOffset+4, 4, L"Type");
	curOffset += hdrCk.chunkSize;
	//Now we're at the Midi chunk, which starts with the sig "SCEIMidi" (in 32bit little endian)
	midiChunkSize = GetWord(curOffset+8);
	maxMidiNumber = GetWord(curOffset+12);
	//Get the first midi data block addr, which is provided relative to beginning of Midi chunk
	midiOffsetAddr = GetWord(curOffset+16) + curOffset;	
	curOffset = midiOffsetAddr;
	//Now we're at the Midi Data Block
	ULONG sequenceOffset = GetWord(curOffset);		//read sequence offset
	SetEventsOffset(curOffset + sequenceOffset);	
	SetPPQN(GetShort(curOffset+4));					//read ppqn value
	if (sequenceOffset != 6)						//if a compression mode is being applied
	{
		compOption = GetShort(curOffset+6);			//read compression mode
	}

	//SSEQHdr->AddSimpleItem(dwOffset+8, 4, L"Size");
	//SSEQHdr->AddSimpleItem(dwOffset+12, 2, L"Header Size");
	//SSEQHdr->AddUnknownItem(dwOffset+14, 2);

	//TryExpandMidiTracks(16);
	nNumTracks = 16;
	channel = 0;
	SetCurTrack(channel);
	return true;
}


bool SonyPS2Seq::ReadEvent(void)
{
	ULONG beginOffset = curOffset;
	ULONG deltaTime;
	if (bSkipDeltaTime)
		deltaTime = 0;
	else
		deltaTime = ReadVarLen(curOffset);
	AddTime(deltaTime);
	if (curOffset >= rawfile->size())
		return false;

	bSkipDeltaTime = false;

	BYTE status_byte = GetByte(curOffset++);

	if (status_byte <= 0x7F)			// Running Status
	{
		if (status_byte == 0)			// some games were ripped to PSF with the EndTrack event missing, so
		{
			if (GetWord(curOffset) == 0)	//if we read a sequence of four 0 bytes, then just treat that
				return false;				//as the end of the track
		}
		status_byte = runningStatus;
		curOffset--;
	}
	else if (status_byte != 0xFF)
		runningStatus = status_byte;


	channel = status_byte&0x0F;
	SetCurTrack(channel);

	switch (status_byte & 0xF0)
	{
	case 0x80 :						//note off event. Unlike SMF, there is no velocity data byte, just the note val.
		key = GetDataByte(curOffset++);				
		AddNoteOff(beginOffset, curOffset-beginOffset, key);
		break;
	case 0x90 :						//note on event (note off if velocity is zero)
		key = GetByte(curOffset++);
		vel = GetDataByte(curOffset++);
		if (vel > 0)													//if the velocity is > 0, it's a note on
			AddNoteOn(beginOffset, curOffset-beginOffset, key, vel);
		else															//otherwise it's a note off
			AddNoteOff(beginOffset, curOffset-beginOffset, key);
		break;

	case 0xB0 :
		{
			BYTE controlNum = GetByte(curOffset++);
			BYTE value = GetDataByte(curOffset++);
			switch (controlNum)		//control number
			{
			case 1 :
				AddModulation(beginOffset, curOffset-beginOffset, value);
				break;
			case 2 :
				AddBreath(beginOffset, curOffset-beginOffset, value);
				break;
			case 6 :
				//AddGenericEvent(beginOffset, curOffset-beginOffset, L"NRPN Data Entry", NULL, BG_CLR_PINK);
				AddGenericEvent(beginOffset, curOffset-beginOffset, L"Loop start number", NULL, CLR_LOOP);
				break;

			case 7 :							//volume
				AddVol(beginOffset, curOffset-beginOffset, value);
				break;

			case 10 :							//pan
				AddPan(beginOffset, curOffset-beginOffset, value);
				break;

			case 11 :							//expression
				AddExpression(beginOffset, curOffset-beginOffset, value);
				break;

			case 38 :							//0 == endless loop
				AddGenericEvent(beginOffset, curOffset-beginOffset, L"Loop count", NULL, CLR_LOOP);
				break;

			case 99 :							//(0x63) nrpn msb
				switch (value)
				{
				case 0 :
					AddGenericEvent(beginOffset, curOffset-beginOffset, L"Loop Start", NULL, CLR_LOOP);
					break;

				case 1 :
					AddGenericEvent(beginOffset, curOffset-beginOffset, L"Loop End", NULL, CLR_LOOP);
					break;
				}
				break;
			}
		}
		break;

	case 0xC0 :
		{
			BYTE progNum = GetDataByte(curOffset++);
			AddProgramChange(beginOffset, curOffset-beginOffset, progNum);
		}
		break;

	case 0xE0 :
		{
			BYTE hi = GetByte(curOffset++);
			BYTE lo = GetDataByte(curOffset++);
			AddPitchBendMidiFormat(beginOffset, curOffset-beginOffset, hi, lo);
		}
		break;

	case 0xF0 : 
		{
			if (status_byte == 0xFF)
			{
				switch (GetByte(curOffset++))
				{
				case 0x51 :			//tempo. identical to SMF
					{
						U32 microsPerQuarter = GetWordBE(curOffset) & 0x00FFFFFF;	//mask out the hi byte 0x03
						AddTempo(beginOffset, curOffset+4-beginOffset, microsPerQuarter);
						curOffset += 4;
					}
					break;
				
				case 0x2F :
					AddEndOfTrack(beginOffset, curOffset-beginOffset);
					return false;

				default :
					AddEndOfTrack(beginOffset, curOffset-beginOffset-1);
					return false;
				}
			}
		}
		break;
	default: 
		AddGenericEvent(beginOffset, curOffset-beginOffset, L"UNKNOWN", NULL, CLR_UNRECOGNIZED);
		return false;
	}
	return true;
}

BYTE SonyPS2Seq::GetDataByte(U32 offset)
{
	BYTE dataByte = GetByte(offset);
	if (dataByte & 0x80)
	{
		bSkipDeltaTime = true;
		dataByte &= 0x7F;
	}
	else
		bSkipDeltaTime = false;
	return dataByte;
}