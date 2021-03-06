#pragma once
#include "common.h"
#include "HOSAFormat.h"
#include "VGMInstrSet.h"
#include "VGMSampColl.h"



// *****************
// HOSAInstrSet
// *****************

class HOSAInstrSet
	: public VGMInstrSet
{

public:
	HOSAInstrSet(RawFile* file, ULONG offset);
	virtual ~HOSAInstrSet(void);

	virtual bool GetHeaderInfo();
	virtual bool GetInstrPointers();

public:

	typedef struct _InstrHeader
	{
		char	strHeader[8];
		U32		numInstr;
	} InstrHeader;

public:
	InstrHeader instrheader;
};


// **************
// HOSAInstr
// **************

class HOSAInstr
	: public VGMInstr
{
public:
	
	typedef struct _InstrInfo
	{
		U32 numRgns;
	} InstrInfo;

	typedef struct _RgnInfo
	{
		U32	sampOffset;
		U8	volume;					//percent volume 0-0xFF
		U8	note_range_high;
		U8	iSemiToneTune;			//unity key
		U8	iFineTune;				//unknown - definitely not finetune
		U8	ADSR_unk;				//the nibbles get read individually.  Conditional code related to this gets 0'd out in PSF file
									//I disassembled (removed during optimization), so I can't see what it does. probably determines
									//Sm and Sd, so not terribly important.
		U8	ADSR_Am;				// Determines ADSR Attack Mode value.
		U8	unk_A;
		U8	iPan;					//pan 0x80 - hard left    0xFF - hard right.  anything below results in center (but may be undefined)
		U32	ADSR_vals;				//The ordering is all messed up.  The code which loads these values is at 8007D8EC
	} RgnInfo;


public:
	HOSAInstr(VGMInstrSet* instrSet, ULONG offset, ULONG length, ULONG theBank, ULONG theInstrNum);
	~HOSAInstr() { if (rgns) delete[] rgns; }
	virtual bool LoadInstr();

public:
	InstrInfo instrinfo;
	RgnInfo* rgns;
};
