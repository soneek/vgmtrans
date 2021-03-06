#pragma once

#include "common.h"
//#include "RawFile.h"
//#include "VGMInstrSet.h"
#include "Loader.h"
#include "Scanner.h"
//#include "Matcher.h"
#include "LogItem.h"

class VGMColl;
class VGMFile;
class RawFile;
class VGMSeq;
class VGMInstrSet;
class VGMSampColl;
class VGMMiscFile;
class LogItem;
FORWARD_DECLARE_TYPEDEF_STRUCT( ItemSet );



class VGMRoot
{
public:
	VGMRoot(void);
public:
	virtual ~VGMRoot(void);

	bool Init(void);
	void Reset(void);
	void Exit(void);
	bool OpenRawFile(const wstring& filename);
	bool CreateVirtFile(BYTE *databuf, ULONG fileSize, const wstring& filename, const wstring& parRawFileFullPath=L"");
	bool SetupNewRawFile(RawFile* newRawFile);
	bool CloseRawFile(RawFile* targFile);
	void AddVGMFile(VGMFile* theFile);
	void RemoveVGMFile(VGMFile* theFile, bool bRemoveFromRaw = true);
	void AddVGMColl(VGMColl* theColl);
	void RemoveVGMColl(VGMColl* theFile);
	void AddLogItem(LogItem* theLog);
		
	//template <class T> void AddFormat()
	//{
	//	T* theFmt = new T();
	//	if (!theFmt->Init())
	//		return;
	//	AddScanner(theFmt->NewScanner());
	//	//fmt_map[theFmt->GetFormatID()] = theFmt;
	//}

	//template <class T> void AddFormat()
	//{
	//	static T fmt;
	//	AddScanner(fmt.GetScanner());
	//	//fmt_map[theFmt->GetFormatID()] = theFmt;
	//}

	//template <class T> void AddScanner()
	//{
	//	vScanner.push_back(new T());
	//}

	//void AddScanner(VGMScanner& scanner)
	void AddScanner(const string& formatname);

	template <class T> void AddLoader()
	{
		vLoader.push_back(new T());
	}

	//Format* GetFormat(ULONG fmt_id)
	//{
	//	return fmt_map[fmt_id];
	//}
	
	virtual void UI_SetRootPtr(VGMRoot** theRoot) = 0;
	virtual void UI_PreExit() {}
	virtual void UI_Exit() = 0;
	virtual void UI_AddRawFile(RawFile* newFile) {}
	virtual void UI_CloseRawFile(RawFile* targFile) {}

	virtual void UI_OnBeginScan() {}
	virtual void UI_SetScanInfo() {}
	virtual void UI_OnEndScan() {}
	virtual void UI_AddVGMFile(VGMFile* theFile);
	virtual void UI_AddVGMSeq(VGMSeq* theSeq) {}
	virtual void UI_AddVGMInstrSet(VGMInstrSet* theInstrSet) {}
	virtual void UI_AddVGMSampColl(VGMSampColl* theSampColl) {}
	virtual void UI_AddVGMMisc(VGMMiscFile* theMiscFile) {}
	virtual void UI_AddVGMColl(VGMColl* theColl) {}
	virtual void UI_AddLogItem(LogItem* theLog) {}
	virtual void UI_RemoveVGMFile(VGMFile* theFile) {}
	virtual void UI_BeginRemoveVGMFiles() {}
	virtual void UI_EndRemoveVGMFiles() {}
	virtual void UI_RemoveVGMColl(VGMColl* theColl) {}
	//virtual void UI_RemoveVGMFileRange(VGMFile* first, VGMFile* last) {}
	virtual void UI_AddItem(VGMItem* item, VGMItem* parent, const wstring& itemName, VOID* UI_specific) {}
	virtual void UI_AddItemSet(VOID* UI_specific, vector<ItemSet>* itemset) {}
	virtual wstring UI_GetSaveFilePath(const wstring& suggestedFilename, const wstring& extension = L"") = 0;
	virtual wstring UI_GetSaveDirPath(const wstring& suggestedDir = L"") = 0;
	virtual bool UI_WriteBufferToFile(const wstring& filepath, BYTE* buf, ULONG size);


	bool SaveAllAsRaw();

protected:
	//MatchMaker matchmaker;
	vector<RawFile*> vRawFile;
	vector<VGMFile*> vVGMFile;
	vector<VGMColl*> vVGMColl;
	vector<LogItem*> vLogItem;

	vector<VGMLoader*> vLoader;
	vector<VGMScanner*> vScanner;
	//map<ULONG, Format*> fmt_map;
	//vector<Format*> vFormats;
};

extern VGMRoot* pRoot;