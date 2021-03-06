#pragma once
#include "VGMItem.h"
#include "Menu.h"

#define DESCRIPTION(_str_)									\
	virtual wstring GetDescription()						\
	{														\
		wostringstream	desc;								\
		desc << name << L" -  " << _str_;					\
		return desc.str();									\
	}


class SeqTrack;

enum EventType 
{ 
	EVENTTYPE_UNDEFINED, 
	EVENTTYPE_NOTEON,
	EVENTTYPE_NOTEOFF,
	EVENTTYPE_DURNOTE, 
	EVENTTYPE_REST, 
	EVENTTYPE_EXPRESSION,
	EVENTTYPE_EXPRESSIONSLIDE, 
	EVENTTYPE_VOLUME,
	EVENTTYPE_VOLUMESLIDE,
	EVENTTYPE_PAN,
	EVENTTYPE_REVERB,
	EVENTTYPE_PROGCHANGE, 
	EVENTTYPE_PITCHBEND,
	EVENTTYPE_PITCHBENDRANGE,
	EVENTTYPE_TRANSPOSE,
	EVENTTYPE_TEMPO, 
	EVENTTYPE_TIMESIG,
	EVENTTYPE_MODULATION, 
	EVENTTYPE_BREATH,
	EVENTTYPE_SUSTAIN,
	EVENTTYPE_PORTAMENTO,
	EVENTTYPE_PORTAMENTOTIME,
	EVENTTYPE_MARKER,
	EVENTTYPE_TRACKEND,
	EVENTTYPE_LOOPFOREVER
};

class SeqEvent :
	public VGMItem
{
public:
	SeqEvent(SeqTrack* pTrack, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"", BYTE color = 0, Icon icon = ICON_BINARY, const wchar_t* desc = L"");
	virtual ~SeqEvent(void) {}						//note: virtual destructor
	virtual wstring GetDescription() { return desc.empty() ? wstring(name) : (wstring(name) + L" - " + desc); }
	virtual ItemType GetType() const { return ITEMTYPE_SEQEVENT; }
	virtual EventType GetEventType() { return EVENTTYPE_UNDEFINED; }
	virtual Icon GetIcon() { return icon; }

	//BEGIN_MENU(SeqEvent)
	//	MENU_ITEM(SeqEvent, DummyFunction, L"Dummy Function")
	//END_MENU()


	//bool DummyFunction(void)
	//{
	//	return false;
	//}

public:
	BYTE channel;
	SeqTrack* parentTrack;
private:
	Icon icon;
	wstring desc;
};


//  ***************
//  DurNoteSeqEvent
//  ***************

class DurNoteSeqEvent :
	public SeqEvent
{
public:
	DurNoteSeqEvent(SeqTrack* pTrack, BYTE absoluteKey, BYTE velocity, ULONG duration, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~DurNoteSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_DURNOTE; }
	virtual Icon GetIcon() { return ICON_NOTE; }
	DESCRIPTION(L"Abs Key: " << (int)absKey << L"  Velocity: " << (int)vel << L"  Duration: " << dur)
public:
	BYTE absKey;
	BYTE vel;
	ULONG dur;
};

//  **************
//  NoteOnSeqEvent
//  ***************

class NoteOnSeqEvent :
	public SeqEvent
{
public:
	NoteOnSeqEvent(SeqTrack* pTrack, BYTE absoluteKey, BYTE velocity, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~NoteOnSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_NOTEON; }
	virtual Icon GetIcon() { return ICON_NOTE; }
	DESCRIPTION(L"Abs Key: " << (int)absKey << L"  Velocity: " << (int)vel)
public:
	BYTE absKey;
	BYTE vel;
};

//  ***************
//  NoteOffSeqEvent
//  ***************

class NoteOffSeqEvent :
	public SeqEvent
{
public:
	NoteOffSeqEvent(SeqTrack* pTrack, BYTE absoluteKey, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~NoteOffSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_NOTEOFF; }
	virtual Icon GetIcon() { return ICON_NOTE; }
	DESCRIPTION(L"Abs Key: " << (int)absKey)
public:
	BYTE absKey;
};

//  ************
//  RestSeqEvent
//  ************

class RestSeqEvent :
	public SeqEvent
{
public:
	RestSeqEvent(SeqTrack* pTrack, ULONG duration, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~RestSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_REST; }
	virtual Icon GetIcon() { return ICON_REST; }
	DESCRIPTION(L"Duration: " << dur)

public:
	ULONG dur;
};

//  *****************
//  SetOctaveSeqEvent
//  *****************

class SetOctaveSeqEvent :
	public SeqEvent
{
public:
	SetOctaveSeqEvent(SeqTrack* pTrack, BYTE octave, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~SetOctaveSeqEvent(void) {}						//note: virtual destructor
	DESCRIPTION(L"Octave: " << (int)octave)
public:
	BYTE octave;
};

//  ***********
//  VolSeqEvent
//  ***********

class VolSeqEvent :
	public SeqEvent
{
public:
	VolSeqEvent(SeqTrack* pTrack, BYTE volume, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~VolSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_VOLUME; }
	DESCRIPTION(L"Volume: " << (int)vol)

public:
	BYTE vol;
};

//  ****************
//  VolSlideSeqEvent
//  ****************

class VolSlideSeqEvent :
	public SeqEvent
{
public:
	VolSlideSeqEvent(SeqTrack* pTrack, BYTE targetVolume, ULONG duration, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~VolSlideSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_VOLUMESLIDE; }
	DESCRIPTION(L"Target Volume: " << (int)targVol << L"  Duration: " << dur)

public:
	BYTE targVol;
	ULONG dur;
};


//  ***********
//  MastVolSeqEvent
//  ***********

class MastVolSeqEvent :
	public SeqEvent
{
public:
	MastVolSeqEvent(SeqTrack* pTrack, BYTE volume, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~MastVolSeqEvent(void) {}
	DESCRIPTION(L"Master Volume: " << (int)vol)

public:
	BYTE vol;
};

//  ****************
//  MastVolSlideSeqEvent
//  ****************

class MastVolSlideSeqEvent :
	public SeqEvent
{
public:
	MastVolSlideSeqEvent(SeqTrack* pTrack, BYTE targetVolume, ULONG duration, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~MastVolSlideSeqEvent(void) {}
	DESCRIPTION(L"Target Volume: " << (int)targVol << L"  Duration: " << dur)

public:
	BYTE targVol;
	ULONG dur;
};

//  ******************
//  ExpressionSeqEvent
//  ******************

class ExpressionSeqEvent :
	public SeqEvent
{
public:
	ExpressionSeqEvent(SeqTrack* pTrack, BYTE level, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~ExpressionSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_EXPRESSION; }
	DESCRIPTION(L"Expression: " << (int)level)

public:
	BYTE level;
};

//  ***********************
//  ExpressionSlideSeqEvent
//  ***********************

class ExpressionSlideSeqEvent :
	public SeqEvent
{
public:
	ExpressionSlideSeqEvent(SeqTrack* pTrack, BYTE targetExpression, ULONG duration, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~ExpressionSlideSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_EXPRESSIONSLIDE; }
	DESCRIPTION(L"Target Expression: " << (int)targExpr << L"  Duration: " << dur)

public:
	BYTE targExpr;
	ULONG dur;
};


//  ***********
//  PanSeqEvent
//  ***********

class PanSeqEvent :
	public SeqEvent
{
public:
	PanSeqEvent(SeqTrack* pTrack, BYTE pan, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~PanSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_PAN; }
	DESCRIPTION(L"Pan: " << (int)pan)

public:
	BYTE pan;
};

//  ****************
//  PanSlideSeqEvent
//  ****************

class PanSlideSeqEvent :
	public SeqEvent
{
public:
	PanSlideSeqEvent(SeqTrack* pTrack, BYTE targetPan, ULONG duration, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~PanSlideSeqEvent(void) {}
	DESCRIPTION(L"Target Pan: " << (int)targPan << L"  Duration: " << dur)

public:
	BYTE targPan;
	ULONG dur;
};


//  **************
//  ReverbSeqEvent
//  **************

class ReverbSeqEvent :
	public SeqEvent
{
public:
	ReverbSeqEvent(SeqTrack* pTrack, BYTE reverb, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~ReverbSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_REVERB; }
	DESCRIPTION(L"Reverb: " << (int)reverb)

public:
	BYTE reverb;
};


//  *****************
//  PitchBendSeqEvent
//  *****************

class PitchBendSeqEvent :
	public SeqEvent
{
public:
	PitchBendSeqEvent(SeqTrack* pTrack, short thePitchBend, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual EventType GetEventType() { return EVENTTYPE_PITCHBEND; }
	DESCRIPTION(L"Pitch Bend: " << pitchbend)

public:
	short pitchbend;
};


//  **********************
//  PitchBendRangeSeqEvent
//  **********************

class PitchBendRangeSeqEvent :
	public SeqEvent
{
public:
	PitchBendRangeSeqEvent(SeqTrack* pTrack, BYTE semiTones, BYTE cents, 
		ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual EventType GetEventType() { return EVENTTYPE_PITCHBENDRANGE; }
	DESCRIPTION(L"Pitch Bend Range: " << semitones << L" semitones, " << cents << L" cents")

public:
	BYTE semitones;
	BYTE cents;
};

//  *****************
//  TransposeSeqEvent
//  *****************

class TransposeSeqEvent :
	public SeqEvent
{
public:
	TransposeSeqEvent(SeqTrack* pTrack, int theTranspose, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual EventType GetEventType() { return EVENTTYPE_TRANSPOSE; }
	DESCRIPTION(L"Transpose: " << transpose)

public:
	int transpose;
};


//  ******************
//  ModulationSeqEvent
//  ******************

class ModulationSeqEvent :
	public SeqEvent
{
public:
	ModulationSeqEvent(SeqTrack* pTrack, BYTE theDepth, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~ModulationSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_MODULATION; }
	DESCRIPTION(L"Depth: " << (int)depth)

public:
	BYTE depth;
};

//  **************
//  BreathSeqEvent
//  **************

class BreathSeqEvent :
	public SeqEvent
{
public:
	BreathSeqEvent(SeqTrack* pTrack, BYTE theDepth, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~BreathSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_BREATH; }
	DESCRIPTION(L"Depth: " << (int)depth)

public:
	BYTE depth;
};

//  ****************
//  SustainSeqEvent
//  ****************

class SustainSeqEvent :
	public SeqEvent
{
public:
	SustainSeqEvent(SeqTrack* pTrack, bool bSustain, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~SustainSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_SUSTAIN; }
	DESCRIPTION(L"Sustain Pedal: " << (bOn)?L"On":L"Off")

public:
	bool bOn;
};

//  ******************
//  PortamentoSeqEvent
//  ******************

class PortamentoSeqEvent :
	public SeqEvent
{
public:
	PortamentoSeqEvent(SeqTrack* pTrack, bool bPortamento, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~PortamentoSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_PORTAMENTO; }
	DESCRIPTION(L"Portamento: " << (bOn)?L"On":L"Off")

public:
	bool bOn;
};

//  **********************
//  PortamentoTimeSeqEvent
//  **********************

class PortamentoTimeSeqEvent :
	public SeqEvent
{
public:
	PortamentoTimeSeqEvent(SeqTrack* pTrack, BYTE time, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~PortamentoTimeSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_PORTAMENTOTIME; }
	DESCRIPTION(L"Portamento Time: " << (int)time)

public:
	BYTE time;
};


//  ******************
//  ProgChangeSeqEvent
//  ******************

class ProgChangeSeqEvent :
	public SeqEvent
{
public:
	ProgChangeSeqEvent(SeqTrack* pTrack, ULONG programNumber, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~ProgChangeSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_PROGCHANGE; }
	DESCRIPTION(L"Program Number: " << (int)progNum)

public:
	ULONG progNum;
};

//  *************
//  TempoSeqEvent
//  *************

class TempoSeqEvent :
	public SeqEvent
{
public:
	TempoSeqEvent(SeqTrack* pTrack, double beatsperminute, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~TempoSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_TEMPO; }
	virtual Icon GetIcon() { return ICON_TEMPO; }
	DESCRIPTION(L"BPM: " << bpm)

public:
	double bpm;
};

//  ******************
//  TempoSlideSeqEvent
//  ******************

class TempoSlideSeqEvent :
	public SeqEvent
{
public:
	TempoSlideSeqEvent(SeqTrack* pTrack, double targBPM, ULONG duration, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~TempoSlideSeqEvent(void) {}
	virtual Icon GetIcon() { return ICON_TEMPO; }
	DESCRIPTION(L"BPM: " << targbpm << L"  Duration: " << dur)

public:
	double targbpm;
	ULONG dur;
};

//  ***************
//  TimeSigSeqEvent
//  ***************

class TimeSigSeqEvent :
	public SeqEvent
{
public:
	TimeSigSeqEvent(SeqTrack* pTrack, BYTE numerator, BYTE denominator, BYTE theTicksPerQuarter, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"");
	virtual ~TimeSigSeqEvent(void) {}
	virtual EventType GetEventType() { return EVENTTYPE_TIMESIG; }
	DESCRIPTION(L"Signature: " << (int)numer << L"/" << (int)denom << L"  Ticks Per Quarter: " << (int)ticksPerQuarter)

public:
	BYTE numer;
	BYTE denom;
	BYTE ticksPerQuarter;
};

//  **************
//  MarkerSeqEvent
//  **************

class MarkerSeqEvent :
	public SeqEvent
{
public:
	MarkerSeqEvent(SeqTrack* pTrack, string& markername, BYTE databyte1, BYTE databyte2, ULONG offset = 0, ULONG length = 0,
				   const wchar_t* name = L"", BYTE color = CLR_MARKER) 
	: SeqEvent(pTrack, offset, length, name, color), databyte1(databyte1), databyte2(databyte2)
	{}
	virtual EventType GetEventType() { return EVENTTYPE_MARKER; }

public:
	BYTE databyte1;
	BYTE databyte2;
};

//  ***************
//  VibratoSeqEvent
//  ***************

//class VibratoSeqEvent :
//	public SeqEvent
//{
//public:
//	VibratoSeqEvent(SeqTrack* pTrack, BYTE detph, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"") 
//	: SeqEvent(pTrack, offset, length, name), depth(depth)
//	{}
//	virtual EventType GetEventType() { return EVENTTYPE_VIBRATO; }
//
//public:
//	BYTE depth;
//};

//  ****************
//  TrackEndSeqEvent
//  ****************

class TrackEndSeqEvent :
	public SeqEvent
{
public:
	TrackEndSeqEvent(SeqTrack* pTrack, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"")
	: SeqEvent(pTrack, offset, length, name, CLR_TRACKEND) {}
	virtual EventType GetEventType() { return EVENTTYPE_TRACKEND; }
};

//  *******************
//  LoopForeverSeqEvent
//  *******************

class LoopForeverSeqEvent :
	public SeqEvent
{
public:
	LoopForeverSeqEvent(SeqTrack* pTrack, ULONG offset = 0, ULONG length = 0, const wchar_t* name = L"")
	: SeqEvent(pTrack, offset, length, name, CLR_LOOPFOREVER) {}
	virtual EventType GetEventType() { return EVENTTYPE_LOOPFOREVER; }
};