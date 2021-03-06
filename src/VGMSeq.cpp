#include "stdafx.h"
#include "common.h"
#include "VGMSeq.h"
#include "SeqTrack.h"
#include "SeqEvent.h"
#include "Options.h"
#include "Root.h"
#include <math.h>


DECLARE_MENU(VGMSeq)

VGMSeq::VGMSeq(const string& format, RawFile* file, ULONG offset, ULONG length, wstring name)
: VGMFile(FILETYPE_SEQ, format, file, offset, length, name),
  //midi(this),
  midi(NULL),
  bMonophonicTracks(false),
  bReverb(false),
  bUseLinearAmplitudeScale(false),
  bWriteInitialTempo(false),
  bAlwaysWriteInitialVol(false),
  bAlwaysWriteInitialExpression(false),
  bAlwaysWriteInitialPitchBendRange(false),
  bAllowDiscontinuousTrackData(false),
  bLoadTrackByTrack(false),
  initialVol(100),					//GM standard (dls1 spec p16)
  initialExpression(127),			//''
  initialPitchBendRangeSemiTones(2), //GM standard.  Means +/- 2 semitones (4 total range)
  initialPitchBendRangeCents(0)
{
	AddContainer<SeqTrack>(aTracks);
}

VGMSeq::~VGMSeq(void)
{
	DeleteVect<SeqTrack>(aTracks);
}

bool VGMSeq::Load()
{
	if (!LoadMain())
		return false;

	//LoadLocalData();
	//UseLocalData();
	pRoot->AddVGMFile(this);
	return true;
}

MidiFile* VGMSeq::ConvertToMidi()
{
	int numTracks = aTracks.size();

	if (!LoadTracks(READMODE_FIND_DELTA_LENGTH))
		return NULL;

	// Find the greatest length of all tracks to use as stop point for every track
	long stopTime = -1;
	for (int i = 0; i < numTracks; i++)
		stopTime = max(stopTime, aTracks[i]->deltaLength);

	MidiFile* newmidi = new MidiFile(this);
	this->midi = newmidi;
	if (!LoadTracks(READMODE_CONVERT_TO_MIDI, stopTime))
	{
		delete midi;
		this->midi = NULL;
		return NULL;
	}
	this->midi = NULL;
	return newmidi;
}

//Load() - Function to load all the sequence data into the class
bool VGMSeq::LoadMain()
{
	if (!GetHeaderInfo())
		return false;
	if (!GetTrackPointers())
		return false;
	nNumTracks = aTracks.size();
	if (nNumTracks == 0)
		return false;

	if (!LoadTracks(READMODE_ADD_TO_UI))
		return false;

	return true;
}

bool VGMSeq::PostLoad()
{
	if (readMode == READMODE_ADD_TO_UI)
	{
		std::sort(aInstrumentsUsed.begin(), aInstrumentsUsed.end());

		for (UINT i=0; i<aTracks.size(); i++)
			sort(aTracks[i]->aEvents.begin(), aTracks[i]->aEvents.end(), ItemPtrOffsetCmp());
	}
	else if (readMode == READMODE_CONVERT_TO_MIDI)
	{
		midi->Sort();
	}
	
	return true;
}

bool VGMSeq::LoadTracks(ReadMode readMode, long stopTime)
{
	// set read mode
	this->readMode = readMode;
	for (UINT trackNum = 0; trackNum < nNumTracks; trackNum++)
	{
		aTracks[trackNum]->readMode = readMode;
	}

	// reset variables
	ResetVars();
	for (UINT trackNum = 0; trackNum < nNumTracks; trackNum++)
	{
		if (!aTracks[trackNum]->LoadTrackInit(trackNum))
			return false;
	}

	// determine the stop offsets
	ULONG* aStopOffset = new ULONG[nNumTracks];
	for (UINT trackNum = 0; trackNum < nNumTracks; trackNum++)
	{
		if (readMode == READMODE_ADD_TO_UI)
		{
			aStopOffset[trackNum] = GetEndOffset();
			if (unLength != 0)
			{
				aStopOffset[trackNum] = dwOffset + unLength;
			}
			else
			{
				if (!bAllowDiscontinuousTrackData)
				{
					// set length from the next track by offset
					for (UINT j = 0; j < nNumTracks; j++)
					{
						if (aTracks[j]->dwOffset > aTracks[trackNum]->dwOffset &&
							aTracks[j]->dwOffset < aStopOffset[trackNum])
						{
							aStopOffset[trackNum] = aTracks[j]->dwOffset;
						}
					}
				}
			}
		}
		else
		{
			aStopOffset[trackNum] = aTracks[trackNum]->dwOffset + aTracks[trackNum]->unLength;
		}
	}

	// load all tracks
	bool succeeded = true;
	if (!bLoadTrackByTrack)
	{
		long time = 0;
		while (HasActiveTracks())
		{
			// check time limit
			if (time >= stopTime)
			{
				if (readMode == READMODE_ADD_TO_UI)
				{
					pRoot->AddLogItem(new LogItem(wstring(*this->GetName()) + L" - Abort loading tracks by time limit.", LOG_LEVEL_WARN, wstring(L"VGMSeq")));
				}

				InactiveAllTracks();
				break;
			}

			// process tracks
			for (UINT trackNum = 0; trackNum < nNumTracks; trackNum++)
			{
				if (!aTracks[trackNum]->active)
					continue;

				// tick
				if (!aTracks[trackNum]->LoadTrackMainLoop(aStopOffset[trackNum]))
				{
					succeeded = false;
					break;
				}
			}
			time++;

			// check loop count
			int requiredLoops = (readMode == READMODE_ADD_TO_UI) ? 1 : ConversionOptions::GetNumSequenceLoops();
			if (GetForeverLoops() >= requiredLoops)
			{
				InactiveAllTracks();
				break;
			}
		}
	}
	else
	{
		// load track by track
		for (UINT trackNum = 0; trackNum < nNumTracks; trackNum++)
		{
			if (!aTracks[trackNum]->LoadTrackMainLoop(aStopOffset[trackNum]))
			{
				succeeded = false;
				break;
			}
			aTracks[trackNum]->active = false;
		}
	}
	delete[] aStopOffset;

	if (succeeded)
	{
		// adjust container length
		if (readMode == READMODE_ADD_TO_UI)
		{
			// track length
			for (UINT trackNum = 0; trackNum < nNumTracks; trackNum++)
			{
				if (aTracks[trackNum]->unLength == 0)			//if unLength has not been changed from default value of 0
				{
					if (!aTracks[trackNum]->aEvents.empty())
					{
						SeqEvent* lastTrkEvent = aTracks[trackNum]->aEvents.back();
						aTracks[trackNum]->unLength = lastTrkEvent->dwOffset + lastTrkEvent->unLength - aTracks[trackNum]->dwOffset;
					}
				}
			}

			// sequence length
			if (unLength == 0)
			{
				// a track can sometimes cover other ones (i.e. a track has a "hole" between the head and tail)
				// it means that the tail of the last track is not always the tail of a sequence
				// therefore, check the length of each tracks

				for (vector<SeqTrack*>::iterator itr = aTracks.begin(); itr != aTracks.end(); ++itr)
				{
					assert(dwOffset <= (*itr)->dwOffset);

					ULONG expectedLength = (*itr)->dwOffset + (*itr)->unLength - dwOffset;
					if (unLength < expectedLength)
					{
						unLength = expectedLength;
					}
				}
			}
		}

		if (!PostLoad())
		{
			succeeded = false;
		}
	}

	return succeeded;
}

bool VGMSeq::HasActiveTracks()
{
	for (UINT trackNum = 0; trackNum < nNumTracks; trackNum++)
	{
		if (aTracks[trackNum]->active)
			return true;
	}
	return false;
}

void VGMSeq::InactiveAllTracks()
{
	for (UINT trackNum = 0; trackNum < nNumTracks; trackNum++)
	{
		aTracks[trackNum]->active = false;
	}
}

int VGMSeq::GetForeverLoops()
{
	if (nNumTracks == 0)
		return 0;

	int foreverLoops = INT_MAX;
	for (UINT trackNum = 0; trackNum < nNumTracks; trackNum++)
	{
		if (!aTracks[trackNum]->active)
			continue;

		if (foreverLoops > aTracks[trackNum]->foreverLoops)
			foreverLoops = aTracks[trackNum]->foreverLoops;
	}
	return (foreverLoops != INT_MAX) ? foreverLoops : 0;
}

bool VGMSeq::GetHeaderInfo(void)
{
	return true;
}


//GetTrackPointers() should contain logic for parsing track pointers 
// and instantiating/adding each track in the sequence
bool VGMSeq::GetTrackPointers(void)
{
	return true;
}

void VGMSeq::ResetVars(void)
{
	if (readMode == READMODE_ADD_TO_UI)
	{
		aInstrumentsUsed.clear();
	}
}

void VGMSeq::SetPPQN(WORD ppqn)
{
	this->ppqn = ppqn;
	if (readMode == READMODE_CONVERT_TO_MIDI)
		midi->SetPPQN(ppqn);
}

WORD VGMSeq::GetPPQN(void)
{
	return this->ppqn;
	//return midi->GetPPQN();
}

void VGMSeq::AddInstrumentRef(ULONG progNum)
{
	if (std::find(aInstrumentsUsed.begin(), aInstrumentsUsed.end(), progNum) == aInstrumentsUsed.end())
	{
		aInstrumentsUsed.push_back(progNum);
	}
}

bool VGMSeq::OnSaveAsMidi(void)
{
	wstring filepath = pRoot->UI_GetSaveFilePath(name.c_str(), L"mid");
	if (filepath.length() != 0)
		return SaveAsMidi(filepath.c_str());
	return false;
}


bool VGMSeq::SaveAsMidi(const wchar_t* filepath)
{
	MidiFile* midi = this->ConvertToMidi();
	if (!midi)
		return false;
	bool result = midi->SaveMidiFile(filepath);
	delete midi;
	return result;
}
