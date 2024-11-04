#include <iostream>
#include <algorithm>
#include <filesystem>
#include <Windows.h>

#include "common/types.h"
#include "common/platform.h"
#include "common/file.h"
#include "common/logfile.h"
#include "stringfile/StringFile.h"
#include "modfile/espfile.h"
#include "modfile/modutils.h"
#include "common/utils.h"
#include "modfile/records/alchrecord.h"
#include "modfile/records/ammorecord.h"
#include "modfile/records/armorecord.h"
#include "modfile/records/avifrecord.h"
#include "modfile/records/biomrecord.h"
#include "modfile/records/bookrecord.h"
#include "modfile/records/cellrecord.h"
#include "modfile/records/clasrecord.h"
#include "modfile/records/cobjrecord.h"
#include "modfile/records/dialrecord.h"
#include "modfile/records/enchrecord.h"
#include "modfile/records/equprecord.h"
#include "modfile/records/factrecord.h"
#include "modfile/records/florrecord.h"
#include "modfile/records/flstrecord.h"
#include "modfile/records/furnrecord.h"
#include "modfile/records/gbfmrecord.h"
#include "modfile/records/gmstrecord.h"
#include "modfile/records/inforecord.h"
#include "modfile/records/iresrecord.h"
#include "modfile/records/lctnrecord.h"
#include "modfile/records/lscrrecord.h"
#include "modfile/records/lvlirecord.h"
#include "modfile/records/mgefrecord.h"
#include "modfile/records/miscrecord.h"
#include "modfile/records/npcrecord.h"
#include "modfile/records/omodrecord.h"
#include "modfile/records/perkrecord.h"
#include "modfile/records/pndtrecord.h"
#include "modfile/records/qustrecord.h"
#include "modfile/records/racerecord.h"
#include "modfile/records/refrrecord.h"
#include "modfile/records/spelrecord.h"
#include "modfile/records/stdtrecord.h"
#include "modfile/records/sunprecord.h"
#include "modfile/records/termrecord.h"
#include "modfile/records/tmlmrecord.h"
#include "modfile/records/weaprecord.h"
#include "modfile/records/wrldrecord.h"

using namespace sfwiki;
using std::string;
using std::vector;

/*
BOOK
WEAP
AMMO
ARMO
FURN
MISC
NPC_
RACE
FACT
CLAS
MGEF
GMST
EQUP
BIOM
SPEL
ENCH
ALCH
ACIF
WRLD
LCTN
STDT / SUNP
PNDT

 / PCCN
PERK

STAT
QUST
DIAL ?
INFO ?
HAZD?

weapon mods/perks
starships
research
planets
skills/powers
*/

std::unordered_map<dword, string> StarNameMap;
std::unordered_map<dword, std::unordered_map<dword, string>> PlanetIndexMap;


string EscapeCsv(const string Orig)
{
	string Buffer = Orig;
	size_t pos = 0;
	string old = "\"";
	string rep = "\"\"";

	while ((pos = Buffer.find(old, pos)) != std::string::npos) {
		Buffer.replace(pos, old.length(), rep);
		pos += rep.length();
	}

	old = "\r\n";
	rep = "\\n";
	pos = 0;

	while ((pos = Buffer.find(old, pos)) != std::string::npos) {
		Buffer.replace(pos, old.length(), rep);
		pos += rep.length();
	}

	old = "\n";
	rep = "\\n";
	pos = 0;

	while ((pos = Buffer.find(old, pos)) != std::string::npos) {
		Buffer.replace(pos, old.length(), rep);
		pos += rep.length();
	}

	old = "\r";
	rep = "\\r";
	pos = 0;

	while ((pos = Buffer.find(old, pos)) != std::string::npos) {
		Buffer.replace(pos, old.length(), rep);
		pos += rep.length();
	}

	std::replace(Buffer.begin(), Buffer.end(), '\x92', '\'');
	return Buffer;
}


string FindRecordEditorId(CEspFile& espFile, formid_t formId, rectype_t NameRecord)
{
	if (formId == 0) return "";

	string Buffer = FormatString("0x%08X", formId);

	auto pRecord = espFile.FindFormId<CRecord>(formId);
	if (pRecord == nullptr) return Buffer;

	auto pName = pRecord->FindSubrecord<CLStringSubrecord>(NameRecord);
	if (pName != nullptr) return pName->GetString();

	auto pEditorId = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
	if (pEditorId == nullptr) return Buffer;

	return pEditorId->GetString();
}



string FindRecordEditorId(CEspFile& espFile, formid_t formId)
{
	if (formId == 0) return "";

	string Buffer = FormatString("0x%08X", formId);

	auto pRecord = espFile.FindFormId<CRecord>(formId);
	if (pRecord == nullptr) return Buffer;

	auto pEditorId = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
	if (pEditorId == nullptr) return Buffer;

	return pEditorId->GetString();
}


void DumpBooks(CEspFile& espFile, const string Filename)
{
	auto pBooks = espFile.GetTypeGroup(NAME_BOOK);
	CFile File;

	if (pBooks == nullptr || pBooks->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("Found %d Books!\n", (int) pBooks->GetNumRecords());
	File.Printf("======================\n");

	for (auto i : pBooks->GetRecords())
	{
		auto pBook = dynamic_cast<CBookRecord *>(i);
		if (pBook == nullptr) continue;

		File.Printf("FormID: 0x%08X (%d)\n", pBook->GetFormID(), pBook->GetFormID());

		auto pEditorID = pBook->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pTitle = pBook->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pText = pBook->FindSubrecord<CLStringSubrecord>(NAME_DESC);

		if (pEditorID) File.Printf("EditorID: %s\n", pEditorID->GetString().c_str());
		if (pTitle) File.Printf("   Title: %s\n", pTitle->GetString().c_str());
		if (pText) File.Printf("%s\n", pText->GetString().c_str());

		File.Printf("======================\n");
	}
}


void DumpBooksCsv(CEspFile& espFile, const string Filename)
{
	auto pBooks = espFile.GetTypeGroup(NAME_BOOK);
	CFile File;

	if (pBooks == nullptr || pBooks->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Title, Value, Weight, Model\n");

	for (auto i : pBooks->GetRecords())
	{
		auto pBook = dynamic_cast<CBookRecord *>(i);
		if (pBook == nullptr) continue;

		File.Printf("0x%08X", pBook->GetFormID());

		auto pEditorID = pBook->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pTitle = pBook->FindSubrecord<CLStringSubrecord>(NAME_FULL);

		if (pEditorID)
			File.Printf(",\"%s\"", pEditorID->GetString().c_str());
		else
			File.Printf(",\"\"");

		if (pTitle) 
			File.Printf(",\"%s\"", pTitle->GetString().c_str());
		else
			File.Printf(",\"\"");

		auto pData = pBook->FindSubrecord<CBookDataSubrecord>(NAME_DATA);

		if (pData)
		{
			File.Printf(",\"%d\",\"%f\"", pData->GetBookData().Value, pData->GetBookData().Weight);
		}
		else
		{
			File.Printf(",\"\",\"\"");
		}

		auto pModel = pBook->FindSubrecord<CStringSubrecord>(NAME_MODL);
		File.Printf(",\"%s\"", pModel ? pModel->GetString().c_str() : "");


		File.Printf("\n");
	}
}


void DumpBookFiles(CEspFile& espFile, const string Path)
{
	auto pBooks = espFile.GetTypeGroup(NAME_BOOK);
	CFile File;
	string Filename;

	if (pBooks == nullptr || pBooks->GetNumRecords() == 0) return;

	printf("Exporting Book Texts to '%s'...\n", Path.c_str());
	CreateDirectory(Path.c_str(), nullptr);

	for (auto i : pBooks->GetRecords())
	{
		auto pBook = dynamic_cast<CBookRecord *>(i);
		if (pBook == nullptr) continue;

		auto pText = pBook->FindSubrecord<CLStringSubrecord>(NAME_DESC);
		if (pText == nullptr) continue;

		Filename = FormatString("%s0x%08X.txt", Path.c_str(), pBook->GetFormID());
		if (!File.Open(Filename, "wt")) return;

		File.Printf("%s", pText->GetString().c_str());

		File.Close();
	}

}

void DumpWeapons(CEspFile& espFile, const string Filename)
{
	auto pWeapons = espFile.GetTypeGroup(NAME_WEAP);
	CFile File;

	if (pWeapons == nullptr || pWeapons->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, EquipType, WGEN1, Weight, Value, WGEN2, WGEN3, WGEN4, Ammo, MagSize, Ammo2, WAM4, WAM5, WAM6, WAM7, Model\n");

	for (auto i : pWeapons->GetRecords())
	{
		auto pWeapon = dynamic_cast<CWeapRecord *>(i);
		if (pWeapon == nullptr) continue;

		File.Printf("0x%08X", pWeapon->GetFormID());

		auto pEditorID = pWeapon->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pWeapon->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pDesc = pWeapon->FindSubrecord<CLStringSubrecord>(NAME_DESC);
		auto pFullName = pFull ? pFull->GetString() : "";

		trim(pFullName);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", pFullName.c_str());

		auto pEquipType = pWeapon->FindSubrecord<CFormidSubrecord>(NAME_ETYP);

		if (pEquipType)
		{
			CStringSubrecord* pTypeEditorId = nullptr;
			auto pType = espFile.FindFormId<CEqupRecord>(pEquipType->GetValue());
			if (pType) pTypeEditorId = pType->FindSubrecord<CStringSubrecord>(NAME_EDID);

			if (pTypeEditorId)
				File.Printf(",\"%s\"", pTypeEditorId->GetString().c_str());
			else
				File.Printf(",\"0x%08X\"", pEquipType->GetValue());
		}
		else
		{
			File.Printf(",\"\"");
		}

		auto pWgen = pWeapon->FindSubrecord<CWgenSubrecord>(NAME_WGEN);

		if (pWgen)
		{
			auto data = pWgen->GetWeaponData();
			File.Printf(",\"%d\",\"%f\",\"%d\",\"%f\",\"%f\",\"0x%08X\"", data.Unknown1, data.Weight, data.Value, data.Unknown2, data.Unknown3, data.Unknown4);
		}
		else
		{
			File.Printf(",\"\",\"\",\"\",\"\",\"\",\"\"");
		}


		auto pWam2 = pWeapon->FindSubrecord<CWam2Subrecord>(NAME_WAM2);

		if (pWam2)
		{
			auto data = pWam2->GetWeaponData();
			auto Ammo1 = FindRecordEditorId(espFile, data.AmmoFormid, NAME_ONAM);
			auto Ammo2 = FindRecordEditorId(espFile, data.UnknownFormid);

			File.Printf(",\"%s\",\"%d\",\"%s\",\"%d\",\"%d\",\"%d\",\"%d\"", Ammo1.c_str(), data.MagSize, Ammo2.c_str(), data.Unknown1, data.Unknown2, (int) data.Unknown3, (int)data.Unknown4);
		}
		else
		{
			File.Printf(",\"\",\"\",\"\",\"\",\"\",\"\",\"\"");
		}

		auto pModel = pWeapon->FindSubrecord<CStringSubrecord>(NAME_MODL);
		File.Printf(",\"%s\"", pModel ? pModel->GetString().c_str() : "");

		File.Printf("\n");
	}

}


void DumpAmmo(CEspFile& espFile, const string Filename)
{
	auto pAmmos = espFile.GetTypeGroup(NAME_AMMO);
	CFile File;

	if (pAmmos == nullptr || pAmmos->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, Desc, ONAM, Value, Weight, Model\n");

	for (auto i : pAmmos->GetRecords())
	{
		auto pAmmo = dynamic_cast<CAmmoRecord *>(i);
		if (pAmmo == nullptr) continue;

		File.Printf("0x%08X", pAmmo->GetFormID());

		auto pEditorID = pAmmo->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pAmmo->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pDesc = pAmmo->FindSubrecord<CLStringSubrecord>(NAME_DESC);
		auto pFullName = pFull ? pFull->GetString() : "";

		trim(pFullName);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", pFullName.c_str());

		File.Printf(",\"%s\"", pDesc ? pDesc->GetString().c_str() : "");

		auto pOnam = pAmmo->FindSubrecord<CLStringSubrecord>(NAME_ONAM);
		File.Printf(",\"%s\"", pOnam ? pOnam->GetString().c_str() : "");

		auto pData = pAmmo->FindSubrecord<CAmmoDataSubrecord>(NAME_DATA);

		if (pData)
		{
			auto data = pData->GetAmmoData();
			File.Printf(",\"%d\",\"%f\"", data.Value, data.Weight);
		}
		else
		{
			File.Printf(",\"\",\"\"");
		}

		auto pModel = pAmmo->FindSubrecord<CStringSubrecord>(NAME_MODL);
		File.Printf(",\"%s\"", pModel ? pModel->GetString().c_str() :"");

		File.Printf("\n");
	}
}


void DumpArmors(CEspFile& espFile, const string Filename)
{
	auto pArmors = espFile.GetTypeGroup(NAME_ARMO);
	CFile File;

	if (pArmors == nullptr || pArmors->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, Value, Weight, Unknown3, Model\n");

	for (auto i : pArmors->GetRecords())
	{
		auto pArmor = dynamic_cast<CArmoRecord *>(i);
		if (pArmor == nullptr) continue;

		File.Printf("0x%08X", pArmor->GetFormID());

		auto pEditorID = pArmor->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pArmor->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pDesc = pArmor->FindSubrecord<CLStringSubrecord>(NAME_DESC);
		auto pFullName = pFull ? pFull->GetString() : "";

		trim(pFullName);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", pFullName.c_str());

		auto pData = pArmor->FindSubrecord<CArmoDataSubrecord>(NAME_DATA);

		if (pData)
		{
			auto data = pData->GetAmmoData();
			File.Printf(",\"%d\",\"%f\",\"%d\"", data.Value, data.Weight, data.Unknown3);
		}
		else
		{
			File.Printf(",\"\",\"\",\"\"");
		}

		auto pModel = pArmor->FindSubrecord<CStringSubrecord>(NAME_MOD2);
		File.Printf(",\"%s\"", pModel ? pModel->GetString().c_str() : "");

		File.Printf("\n");
	}

}


void DumpFurnitures(CEspFile& espFile, const string Filename)
{
	auto pFurnitures = espFile.GetTypeGroup(NAME_FURN);
	CFile File;

	if (pFurnitures == nullptr || pFurnitures->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, Model\n");

	for (auto i : pFurnitures->GetRecords())
	{
		auto pFurn = dynamic_cast<CFurnRecord *>(i);
		if (pFurn == nullptr) continue;

		File.Printf("0x%08X", pFurn->GetFormID());

		auto pEditorID = pFurn->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pFurn->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pFullName = pFull ? pFull->GetString() : "";

		trim(pFullName);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", pFullName.c_str());

		auto pModel = pFurn->FindSubrecord<CStringSubrecord>(NAME_MODL);
		File.Printf(",\"%s\"", pModel ? pModel->GetString().c_str() : "");

		File.Printf("\n");
	}

}


void DumpMisc(CEspFile& espFile, const string Filename)
{
	auto pMiscs = espFile.GetTypeGroup(NAME_MISC);
	CFile File;

	if (pMiscs == nullptr || pMiscs->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, Model, Value, Weight\n");

	for (auto i : pMiscs->GetRecords())
	{
		auto pMisc = dynamic_cast<CMiscRecord *>(i);
		if (pMisc == nullptr) continue;

		File.Printf("0x%08X", pMisc->GetFormID());

		auto pEditorID = pMisc->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pMisc->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pFullName = pFull ? pFull->GetString() : "";

		trim(pFullName);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", pFullName.c_str());

		auto pModel = pMisc->FindSubrecord<CStringSubrecord>(NAME_MODL);
		File.Printf(",\"%s\"", pModel ? pModel->GetString().c_str() : "");

		auto pData = pMisc->FindSubrecord<CMiscDataSubrecord>(NAME_DATA);

		if (pData)
		{
			File.Printf(",\"%d\",\"%f\"", pData->GetMiscData().Value, pData->GetMiscData().Weight);
		}
		else
		{
			File.Printf(",\"\",\"\"");
		}

		File.Printf("\n");
	}

}


void DumpNpcs(CEspFile& espFile, const string Filename)
{
	auto pNpcs = espFile.GetTypeGroup(NAME_NPC_);
	CFile File;

	if (pNpcs == nullptr || pNpcs->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, Race, Acbs1, Acbs2, Acbs3, Acbs4, Factions\n");

	for (auto i : pNpcs->GetRecords())
	{
		auto pNpc = dynamic_cast<CNpcRecord *>(i);
		if (pNpc == nullptr) continue;

		File.Printf("0x%08X", pNpc->GetFormID());

		auto pEditorID = pNpc->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pNpc->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pFullName = pFull ? pFull->GetString() : "";

		trim(pFullName);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", pFullName.c_str());

		auto pRnam = pNpc->FindSubrecord<CFormidSubrecord>(NAME_RNAM);
		string Race = "";
		if (pRnam) Race = FindRecordEditorId(espFile, pRnam->GetValue(), NAME_FULL);

		File.Printf(",\"%s\"", Race.c_str());

		auto pAcbs = pNpc->FindSubrecord<CAcbsNpcSubrecord>(NAME_ACBS);

		if (pAcbs)
		{
			auto data = pAcbs->GetAcbsData();
			File.Printf(",\"0x%08X\",\"0x%08X\",\"0x%08X\",\"0x%08X\"", data.Unknown1, data.Unknown2, data.Unknown3, data.Unknown4);
		}
		else
		{
			File.Printf(",\"\",\"\",\"\",\"\"");
		}

			//TODO: Can have many
		auto pSnam = pNpc->FindSubrecord<CSnamNpcSubrecord>(NAME_SNAM);
		auto factions = pNpc->FindAllSubrecords<CSnamNpcSubrecord>(NAME_SNAM);

		if (factions.size() > 0)
		{
			string output;

			for (auto j : factions)
			{
				auto factionId = j->GetSnamData().Unknown1;
				auto rank = j->GetSnamData().Unknown2;

				if (!output.empty()) output += ", ";

				if (rank > 0)
					output += FormatString("%s(%d)", FindRecordEditorId(espFile, factionId).c_str(), (int)rank);
				else
					output += FormatString("%s", FindRecordEditorId(espFile, factionId).c_str());
			}

			File.Printf(",\"%s\"", output.c_str());
		}
		else
		{
			File.Printf(",\"\"");
		}

		File.Printf("\n");
	}

}


void DumpRaces(CEspFile& espFile, const string Filename)
{
	auto pRaces = espFile.GetTypeGroup(NAME_RACE);
	CFile File;

	if (pRaces == nullptr || pRaces->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, Desc\n");

	for (auto i : pRaces->GetRecords())
	{
		auto pRace = dynamic_cast<CRaceRecord *>(i);
		if (pRace == nullptr) continue;

		File.Printf("0x%08X", pRace->GetFormID());

		auto pEditorID = pRace->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRace->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pDesc = pRace->FindSubrecord<CLStringSubrecord>(NAME_DESC);
		auto pFullName = pFull ? pFull->GetString() : "";
		auto pDescText = pDesc ? pDesc->GetString() : "";

		trim(pFullName);
		trim(pDescText);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", pFullName.c_str());
		File.Printf(",\"%s\"", pDescText.c_str());

		File.Printf("\n");
	}

}


void DumpFactions(CEspFile& espFile, const string Filename)
{
	auto pFactions = espFile.GetTypeGroup(NAME_FACT);
	CFile File;

	if (pFactions == nullptr || pFactions->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name\n");

	for (auto i : pFactions->GetRecords())
	{
		auto pFaction = dynamic_cast<CFactRecord *>(i);
		if (pFaction == nullptr) continue;

		File.Printf("0x%08X", pFaction->GetFormID());

		auto pEditorID = pFaction->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pFaction->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pFullName = pFull ? pFull->GetString() : "";

		trim(pFullName);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", pFullName.c_str());

		File.Printf("\n");
	}

}


void DumpClasses(CEspFile& espFile, const string Filename)
{
	auto pClasses = espFile.GetTypeGroup(NAME_CLAS);
	CFile File;

	if (pClasses == nullptr || pClasses->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, Desc\n");

	for (auto i : pClasses->GetRecords())
	{
		auto pClass = dynamic_cast<CClasRecord *>(i);
		if (pClass == nullptr) continue;

		File.Printf("0x%08X", pClass->GetFormID());

		auto pEditorID = pClass->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pClass->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pDesc = pClass->FindSubrecord<CLStringSubrecord>(NAME_DESC);
		auto pFullName = pFull ? pFull->GetString() : "";
		auto pDescText = pDesc ? pDesc->GetString() : "";

		trim(pFullName);
		trim(pDescText);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", pFullName.c_str());
		File.Printf(",\"%s\"", pDescText.c_str());

		File.Printf("\n");
	}

}


void DumpMagicEffects(CEspFile& espFile, const string Filename)
{
	auto pEffects = espFile.GetTypeGroup(NAME_MGEF);
	CFile File;

	if (pEffects == nullptr || pEffects->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, Description");
	for (int i = 0; i < MGEFDATA_SUBRECORD_SIZE / 4; ++i) File.Printf(", Unknown%d", i+1);
	File.Printf("\n");
	
	for (auto i : pEffects->GetRecords())
	{
		auto pEffect = dynamic_cast<CMgefRecord *>(i);
		if (pEffect == nullptr) continue;

		File.Printf("0x%08X", pEffect->GetFormID());

		auto pEditorID = pEffect->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pEffect->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pDesc = pEffect->FindSubrecord<CLStringSubrecord>(NAME_DNAM);
		auto FullName = pFull ? pFull->GetString() : "";
		auto DescText = pDesc ? pDesc->GetString() : "";

		trim(FullName);
		trim(DescText);
		DescText = EscapeCsv(DescText);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", FullName.c_str());
		File.Printf(",\"%s\"", DescText.c_str());

		auto pData = pEffect->FindSubrecord<CMgefDataSubrecord>(NAME_DATA);

		if (pData)
		{
			for (int j = 0; j < MGEFDATA_SUBRECORD_SIZE / 4; ++j)
			{
				File.Printf(",\"%d\"", pData->GetMgefData().Unknown[j]);
			}
		}
		else
		{
			for (int j = 0; j < MGEFDATA_SUBRECORD_SIZE / 4; ++j)
			{
				File.Printf(",\"\"");
			}
		}

		File.Printf("\n");
	}

}


void DumpSettings(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_GMST);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Data\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CGmstRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pData = pRecord->FindSubrecord<CGmstDataSubrecord>(NAME_DATA);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");

		if (pData)
		{
			if (pData->IsGlobalBool())
				File.Printf(",\"%d\"", pData->GetBool());
			else if (pData->IsGlobalInt())
				File.Printf(",\"%d\"", pData->GetInteger());
			else if (pData->IsGlobalFloat())
				File.Printf(",\"%f\"", pData->GetFloat());
			else if (pData->IsGlobalString())
				File.Printf(",\"%s\"", EscapeCsv(pData->GetString()).c_str());
			else
				File.Printf(",\"%u\"", pData->GetInteger());
		}
		else
		{
			File.Printf(",\"\"");
		}

		File.Printf("\n");
	}

}


void DumpEquipSlots(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_EQUP);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Data\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CEqupRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pData = pRecord->FindSubrecord<CDwordSubrecord>(NAME_DATA);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");

		if (pData)
			File.Printf(",\"%d\"", pData->GetValue());
		else
			File.Printf(",\"\"");

		File.Printf("\n");
	}

}


void DumpBioms(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_BIOM);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Full, SNAM\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CBiomRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pSnam = pRecord->FindSubrecord<CStringSubrecord>(NAME_SNAM);
		auto FullName = pFull ? pFull->GetString() : "";
		auto SnamText = pSnam ? pSnam->GetString() : "";

		trim(FullName);
		trim(SnamText);
		//DescText = EscapeCsv(DescText);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", FullName.c_str());
		File.Printf(",\"%s\"", SnamText.c_str());

		File.Printf("\n");
	}

}


void DumpSpells(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_SPEL);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, Data1, Data2, Data3, Data4, Data5, Data6, Data7\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CSpelRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		
		auto FullName = pFull ? pFull->GetString() : "";

		trim(FullName);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", FullName.c_str());

		auto pData = pRecord->FindSubrecord<CSpitSubrecord>(NAME_SPIT);

		if (pData)
		{
			auto data = pData->GetSpitData();
			File.Printf(",\"%d\",\"%d\",\"%d\",\"%d\",\"%d\",\"%d\",\"%d\"", data.Unknown1, data.Unknown2, data.Unknown3, data.Unknown4, data.Unknown5, (dword) data.Unknown6, (dword)data.Unknown7);
		}
		else
		{
			File.Printf(",\"\",\"\",\"\",\"\",\"\",\"\",\"\"");
		}

		File.Printf("\n");
	}

}


void DumpEnchants(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_ENCH);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, Data1, Data2, Data3, Data4, Data5, Data6, Data7\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CEnchRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);

		auto FullName = pFull ? pFull->GetString() : "";

		trim(FullName);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", FullName.c_str());

		auto pData = pRecord->FindSubrecord<CEnitSubrecord>(NAME_ENIT);

		if (pData)
		{
			auto data = pData->GetEnitData();
			File.Printf(",\"%d\",\"%d\",\"%d\",\"%d\",\"%d\",\"%d\",\"%d\"", data.Unknown1, data.Unknown2, data.Unknown3, data.Unknown4, data.Unknown5, (dword)data.Unknown6, (dword)data.Unknown7);
		}
		else
		{
			File.Printf(",\"\",\"\",\"\",\"\",\"\",\"\",\"\"");
		}

		File.Printf("\n");
	}

}


void DumpAlchemy(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_ALCH);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, Description, Weight");
	for (int i = 0; i < ENITALCH_SUBRECORD_SIZE / 4; ++i) File.Printf(", Unknown%d", i + 1);
	File.Printf(", Model\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CAlchRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pDesc = pRecord->FindSubrecord<CLStringSubrecord>(NAME_DESC);
		auto pModel = pRecord->FindSubrecord<CStringSubrecord>(NAME_MODL);
		auto FullName = pFull ? pFull->GetString() : "";
		auto DescText = pDesc ? pDesc->GetString() : "";

		trim(FullName);
		trim(DescText);
		DescText = EscapeCsv(DescText);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", FullName.c_str());
		File.Printf(",\"%s\"", DescText.c_str());

		auto pData1 = pRecord->FindSubrecord<CFloatSubrecord>(NAME_DATA);

		if (pData1)
			File.Printf(",\"%f\"", pData1->GetValue());
		else
			File.Printf(",\"\"");

		auto pData = pRecord->FindSubrecord<CEnitAlchSubrecord>(NAME_ENIT);

		if (pData)
		{
			for (int j = 0; j < ENITALCH_SUBRECORD_SIZE / 4; ++j)
			{
				File.Printf(",\"%d\"", pData->GetEnitData().Unknown[j]);
			}
		}
		else
		{
			for (int j = 0; j < ENITALCH_SUBRECORD_SIZE / 4; ++j)
			{
				File.Printf(",\"\"");
			}
		}

		File.Printf(",\"%s\"", pModel ? pModel->GetString().c_str() : "");
		File.Printf("\n");
	}

}


void DumpAvif(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_AVIF);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, Flags\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CAvifRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", pFull ? pFull->GetString().c_str() : "");

		auto pData = pRecord->FindSubrecord<CDwordSubrecord>(NAME_AVFL);

		if (pData)
			File.Printf(",\"%s\"", CreateBitString(pData->GetValue()).c_str());
		else
			File.Printf(",\"\"");

		File.Printf("\n");
	}

}


void DumpWorlds(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_WRLD);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, Flags\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CWrldRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", pFull ? pFull->GetString().c_str() : "");

		auto pData = pRecord->FindSubrecord<CByteSubrecord>(NAME_DATA);

		if (pData)
			File.Printf(",\"%s\"", CreateBitString(pData->GetValue()).c_str());
		else
			File.Printf(",\"\"");

		File.Printf("\n");
	}

}


void DumpLocations(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_LCTN);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, Faction, Flags, Parent, ANAM, RNAM, TNAM\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CLctnRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", pFull ? pFull->GetString().c_str() : "");

		auto pData = pRecord->FindSubrecord<CLctnDataSubrecord>(NAME_DATA);

		if (pData)
		{
			string Faction = FindRecordEditorId(espFile, pData->GetLctnData().FactionFormId, NAME_FULL);
			File.Printf(",\"%s\",\"%s\"", Faction.c_str(), CreateBitString(pData->GetLctnData().Flags).c_str());
		}
		else
		{
			File.Printf(",\"\",\"\"");
		}

		auto pParent = pRecord->FindSubrecord<CFormidSubrecord>(NAME_PNAM);
		File.Printf(",\"%s\"", FindRecordEditorId(espFile, pParent ? pParent->GetValue(): 0, NAME_FULL).c_str());

		auto pAnam = pRecord->FindSubrecord<CFloatSubrecord>(NAME_ANAM);
		auto pRnam = pRecord->FindSubrecord<CDwordSubrecord>(NAME_RNAM);
		auto pTnam = pRecord->FindSubrecord<CFloatSubrecord>(NAME_TNAM);

		File.Printf(",\"%f\"", pAnam ? pAnam->GetValue() : 0);
		File.Printf(",\"%d\"", pRnam ? pRnam->GetValue() : 0);
		File.Printf(",\"%f\"", pTnam ? pTnam->GetValue() : 0);

		File.Printf("\n");
	}

}


void DumpStars(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_STDT);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, ANAM, SystemId, SunPreset, ENAM, X, Y, Z, Data1U1, Data1U2, Data1U3, Data1U4, Data1U5, Gliese, Spectral, AbsMag, Data2U2, Inner OHZ, Outer OHZ, HIP, Data2Unknown6, Temperature, Data3U1, Data3U2, Data3U3, Data3U4, Data3U5, Data3U6, Data3U7, Data3U8, Data3U, Data3U10, Data3U11, Data3U12, Data3U13, Data3U14, Data3U15\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CStdtRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", pFull ? pFull->GetString().c_str() : "");

		auto pAnam = pRecord->FindSubrecord<CStringSubrecord>(NAME_ANAM);
		auto pPnam = pRecord->FindSubrecord<CFormidSubrecord>(NAME_PNAM);
		auto pEnam = pRecord->FindSubrecord<CDwordSubrecord>(NAME_ENAM);
		auto pDnam = pRecord->FindSubrecord<CDwordSubrecord>(NAME_DNAM);

		File.Printf(",\"%s\"", pAnam ? pAnam->GetCString() : "");
		File.Printf(",\"%d\"", pDnam ? pDnam->GetValue() : 0);
		File.Printf(",\"%s\"", FindRecordEditorId(espFile, pPnam ? pPnam->GetValue() : 0, NAME_FULL).c_str());
		File.Printf(",\"0x%08X\"", pEnam ? pEnam->GetValue() : 0);

		StarNameMap[pDnam->GetValue()] = pFull ? pFull->GetString() : pEditorID->GetString();

		auto pBnam = pRecord->FindSubrecord<CBnamStdtSubrecord>(NAME_BNAM);

		if (pBnam)
		{
			auto data = pBnam->GetBnamData();
			File.Printf(",\"%f\",\"%f\",\"%f\"", data.Unknown1, data.Unknown2, data.Unknown3);
		}
		else
		{
			File.Printf(",\"\",\"\",\"\"");
		}

		auto pData1 = pRecord->FindSubrecord<CData1StdtSubrecord>(NAME_DATA);

		if (pData1)
		{
			auto data = pData1->GetStdtData();
			File.Printf(",\"%d\",\"%d\",\"%d\",\"%f\",\"%f\"", data.Unknown1, data.Unknown2, data.Unknown3, data.Unknown4, data.Unknown5);
		}
		else
		{
			File.Printf(",\"\",\"\",\"\",\"\",\"\"");
		}

		auto pData2 = pRecord->FindSubrecord<CData2StdtSubrecord>(NAME_DATA);

		if (pData2)
		{
			auto data = pData2->GetStdtData();
			File.Printf(",\"%s\",\"%s\",\"%f\",\"%d\",\"%f\",\"%f\",\"%d\",\"%d\",\"%d\"", data.Name.c_str(), data.Type.c_str(), data.Unknown1, data.Unknown2, data.Unknown3, data.Unknown4, data.Unknown5, data.Unknown6, data.Unknown7);
		}
		else
		{
			File.Printf(",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\"");
		}

		auto pData3 = pRecord->FindSubrecord<CData3StdtSubrecord>(NAME_DATA);

		if (pData3)
		{
			auto data = pData3->GetStdtData();
			File.Printf(",\"%f\",\"%f\",\"%f\",\"%f\",\"%f\",\"%f\",\"%f\",\"%f\",\"%f\",\"%f\",\"%f\",\"%f\",\"%f\",\"%f\",\"%d\"", data.Unknown1[0], data.Unknown1[1], data.Unknown1[2], data.Unknown1[3], data.Unknown1[4], data.Unknown1[5], data.Unknown1[6], data.Unknown1[7], data.Unknown1[8], 
						data.Unknown1[9], data.Unknown1[10], data.Unknown1[11], data.Unknown1[12], data.Unknown1[13], (int)data.Unknown2);
		}
		else
		{
			File.Printf(",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\"");
		}

		File.Printf("\n");
	}

}


void DumpSunPresets(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_SUNP);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Parent\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CIdRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		
		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");

		auto pParent = pRecord->FindSubrecord<CFormidSubrecord>(NAME_RFDP);
		File.Printf(",\"%s\"", FindRecordEditorId(espFile, pParent ? pParent->GetValue() : 0, NAME_EDID).c_str());
		
		File.Printf("\n");
	}

}


void DumpPlanets(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_PNDT);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID,EditorID,Name,ANAM,Density,Temperature,Model,Fnam, Fnam1, Mass, Radius, Gravity, Fnam5, Dnam1, Dnam2, Enam1, Enam2, Aphelion, Eccentricity, Enam6, MeanOrbit, Enam7, Enam8, Enam9, Enam10, Enam11, Enam12, Gravity1, StarId, StarName, Primary, PlanetId, Orbits, ");
	File.Printf("Hnam1,Class,Gliese,Life,MagField,Mass1,Type,System,Special, Perihelion, StarDistance, Density1, Heat, HydroPct, Hnam6, Hnam7, PeriAngle, Hnam8, StartAngle, Year, Asteroids, Hnam10, Seed, Hnam11\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CPndtRecord *>(i);
		if (pRecord == nullptr) continue;

		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pGnam2 = pRecord->FindSubrecord<CGnamPndtSubrecord>(NAME_GNAM);

		if (pFull && pGnam2)
		{
			auto data = pGnam2->GetGnamData();
			//File.Printf(",\"%d\",\"%s\",\"%d\",\"%d\"", data.StarId, StarName.c_str(), data.Primary, data.PlanetId);
			PlanetIndexMap[data.StarId][data.PlanetId] = pFull->GetString();
		}
	}

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CPndtRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pAnam = pRecord->FindSubrecord<CStringSubrecord>(NAME_ANAM);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetString().c_str() : "");
		File.Printf(",\"%s\"", pFull ? pFull->GetCString() : "");
		File.Printf(",\"%s\"", pAnam ? pAnam->GetCString() : "");

		auto pDens = pRecord->FindSubrecord<CFloatSubrecord>(NAME_DENS);
		auto pTemp = pRecord->FindSubrecord<CFloatSubrecord>(NAME_TEMP);

		File.Printf(",\"%f\"", pDens ? pDens->GetValue() : 0);
		File.Printf(",\"%f\"", pTemp ? pTemp->GetValue() : 0);
		
		auto pModel = pRecord->FindSubrecord<CStringSubrecord>(NAME_MODL);
		File.Printf(",\"%s\"", pModel ? pModel->GetCString() : "");

		auto pFnam1 = pRecord->FindSubrecord<CDwordSubrecord>(NAME_FNAM);
		auto pFnam2 = pRecord->FindSubrecord<CFnamPndtSubrecord>(NAME_FNAM);

		File.Printf(",\"%d\"", pFnam1 ? pFnam1->GetValue() : 0);

		if (pFnam2)
		{
			auto data = pFnam2->GetFnamData();
			File.Printf(",\"%f\",\"%e\",\"%f\",\"%f\",\"%d\"", data.Unknown1, data.Mass, data.Radius, data.Gravity, data.Unknown5);
		}
		else
		{
			File.Printf(",\"\",\"\",\"\",\"\",\"\"");
		}

		auto pDnam = pRecord->FindSubrecord<CDnamPndtSubrecord>(NAME_DNAM);

		if (pDnam)
		{
			auto data = pDnam->GetDnamData();
			File.Printf(",\"%s\",\"%d\"", data.Unknown1.c_str(),(dword)data.Unknown2);
		}
		else
		{
			File.Printf(",\"\",\"\"");
		}

		auto pEnam = pRecord->FindSubrecord<CEnamPndtSubrecord>(NAME_ENAM);

		if (pEnam)
		{
			auto data = pEnam->GetEnamData();
			File.Printf(",\"%f\",\"%f\",\"%f\",\"%f\",\"%f\",\"%f\",\"%f\",\"%d\",\"%d\",\"%d\",\"%d\",\"%d\"", data.Unknown1, data.Unknown2, data.Aphelion, data.Eccentricity, data.Unknown3, data.MeanOrbit, data.Unknown4, data.Unknown5, data.Unknown6, data.Unknown7, data.Unknown8, data.Unknown9);
		}
		else
		{
			File.Printf(",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\"");
		}

		auto pGnam1 = pRecord->FindSubrecord<CFloatSubrecord>(NAME_GNAM);
		File.Printf(",\"%f\"", pGnam1 ? pGnam1->GetValue() : 0);

		auto pGnam2 = pRecord->FindSubrecord<CGnamPndtSubrecord>(NAME_GNAM);

		if (pGnam2)
		{
			auto data = pGnam2->GetGnamData();
			string StarName = StarNameMap[data.StarId];
			File.Printf(",\"%d\",\"%s\",\"%d\",\"%d\"", data.StarId, StarName.c_str(), data.Primary, data.PlanetId);

			string OrbitsName = data.Primary == 0 ? StarName : PlanetIndexMap[data.StarId][data.Primary];
			File.Printf(",\"%s\"", OrbitsName.c_str());
		}
		else
		{
			File.Printf(",\"\",\"\",\"\"");
		}

		auto pHnam = pRecord->FindSubrecord<CHnamPndtSubrecord>(NAME_HNAM);

		if (pHnam)
		{
			auto data = pHnam->GetHnamData();
			File.Printf(",\"%d\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%f\",\"%f\",\"%f\"", data.Unknown1, data.Class.c_str(), data.Gliese.c_str(), data.Life.c_str(), data.MagField.c_str(), data.Mass.c_str(), data.Type.c_str(), data.ParentStar.c_str(), data.Special.c_str(), data.Perihelion, data.StarDistance, data.Density);
			File.Printf(",\"%f\",\"%f\",\"%d\",\"%d\",\"%f\",\"%d\",\"%f\",\"%f\",\"%d\",\"%d\",\"%ud\",\"%d\"", data.Heat, data.HydroPct, data.Unknown6, data.Unknown7, data.PeriAngle, data.Unknown8, data.StartAngle, data.Year, data.Asteroids, data.Unknown10, data.Seed, data.Unknown11);
		}
		else
		{
			File.Printf(",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\"");
			File.Printf(",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\"");
		}

		File.Printf("\n");
	}

}


void DumpQuests(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_QUST);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;
	if (pRecords == nullptr) return;

	File.Printf("FormID, EditorID, Full, Summary\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CQustRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
		File.Printf(",\"%s\"", pFull ? pFull->GetCString() : "");

		auto pSummary = pRecord->FindSubrecord<CStringSubrecord>(NAME_NAM3);

		File.Printf(",\"%s\"", EscapeCsv(pSummary ? pSummary->GetString() : "").c_str());

		File.Printf("\n");
	}

}


void DumpQuestStages(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_QUST);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;
	if (pRecords == nullptr) return;

	File.Printf("QuestFormID, QuestEditorID, QuestFull, INDX1, INDX2, NAM2, CNAM\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CQustRecord *>(i);
		if (pRecord == nullptr) continue;

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);

		CDwordSubrecord* pLastIndx = nullptr;
		CStringSubrecord* pLastNam2 = nullptr;
		CLStringSubrecord* pLastNam1 = nullptr;
		CLStringSubrecord* pLastCnam = nullptr;

		for (auto pSubrecord : pRecord->GetSubrecordArray())
		{
			if (pSubrecord->GetRecordType() == NAME_INDX)
			{
				if (pLastIndx)
				{
					File.Printf("0x%08X", pRecord->GetFormID());
					File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
					File.Printf(",\"%s\"", pFull ? pFull->GetCString() : "");
					File.Printf(",\"%d\"", pLastIndx->GetValue() & 0xFFFF);
					File.Printf(",\"%d\"", pLastIndx->GetValue() >> 16);
					//File.Printf(",\"%s\"", pLastNam1 ? EscapeCsv(pLastNam1->GetString()).c_str() : "");
					File.Printf(",\"%s\"", pLastNam2 ? EscapeCsv(pLastNam2->GetString()).c_str() : "");
					File.Printf(",\"%s\"", pLastCnam ? EscapeCsv(pLastCnam->GetString()).c_str() : "");
					File.Printf("\n");
				}

				pLastIndx = dynamic_cast<CDwordSubrecord *>(pSubrecord);
				pLastNam1 = nullptr;
				pLastNam2 = nullptr;
				pLastCnam = nullptr;
			}

			if (pSubrecord->GetRecordType() == NAME_CNAM) pLastCnam = dynamic_cast<CLStringSubrecord *>(pSubrecord);
			if (pSubrecord->GetRecordType() == NAME_NAM2) pLastNam2 = dynamic_cast<CStringSubrecord *>(pSubrecord);
			if (pSubrecord->GetRecordType() == NAME_NAM1) pLastNam1 = dynamic_cast<CLStringSubrecord *>(pSubrecord);
		}
	}

}


void DumpQuestObjectives(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_QUST);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;
	if (pRecords == nullptr) return;

	File.Printf("QuestFormID, QuestEditorID, QuestFull, Index, Flags, NNAM\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CQustRecord *>(i);
		if (pRecord == nullptr) continue;

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);

		CWordSubrecord* pLastQobj = nullptr;
		CDwordSubrecord* pLastFnam = nullptr;
		CLStringSubrecord* pLastNnam = nullptr;

		for (auto pSubrecord : pRecord->GetSubrecordArray())
		{
			if (pSubrecord->GetRecordType() == NAME_QOBJ)
			{
				if (pLastQobj)
				{
					File.Printf("0x%08X", pRecord->GetFormID());
					File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
					File.Printf(",\"%s\"", pFull ? pFull->GetCString() : "");
					File.Printf(",\"%u\"", (dword) pLastQobj->GetValue());
					File.Printf(",\"%u\"", pLastFnam ? pLastFnam->GetValue() : 0);
					File.Printf(",\"%s\"", pLastNnam ? EscapeCsv(pLastNnam->GetString()).c_str() : "");
					File.Printf("\n");
				}

				pLastQobj = dynamic_cast<CWordSubrecord *>(pSubrecord);
				pLastFnam = nullptr;
				pLastNnam = nullptr;
			}

			if (pSubrecord->GetRecordType() == NAME_FNAM) pLastFnam = dynamic_cast<CDwordSubrecord *>(pSubrecord);
			if (pSubrecord->GetRecordType() == NAME_NNAM) pLastNnam = dynamic_cast<CLStringSubrecord *>(pSubrecord);
		}
	}

}


void DumpQuestScripts(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_QUST);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;
	if (pRecords == nullptr) return;

	File.Printf("QuestFormID, QuestEditorID, QuestName, Script, Fragment\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CQustRecord *>(i);
		if (pRecord == nullptr) continue;

		auto pVmad = pRecord->FindSubrecord<CVmadSubrecord>(NAME_VMAD);
		if (pVmad == nullptr) continue;

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);

		for (auto script : pVmad->GetScriptData().Scripts)
		{
			File.Printf("0x%08X", pRecord->GetFormID());
			File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
			File.Printf(",\"%s\"", pFull ? pFull->GetCString() : "");
			File.Printf(",\"%s\"", script.Name.c_str() );
			File.Printf(",\n");
		}
	}

}


void DumpFlora(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_FLOR);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, Model\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CFlorRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pModel = pRecord->FindSubrecord<CStringSubrecord>(NAME_MODL);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
		File.Printf(",\"%s\"", pFull ? pFull->GetCString() : "");
		File.Printf(",\"%s\"", pModel ? pModel->GetCString() : "");

		File.Printf("\n");
	}

}



void DumpPerks(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_PERK);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Name, Description\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CPerkRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pDesc = pRecord->FindSubrecord<CLStringSubrecord>(NAME_DESC);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
		File.Printf(",\"%s\"", pFull ? pFull->GetCString() : "");
		File.Printf(",\"%s\"", pDesc ? EscapeCsv(pDesc->GetString()).c_str() : "");
		/*
		auto pFulls = pRecord->FindAllSubrecords<CLStringSubrecord>(NAME_FULL);
		auto pDescs = pRecord->FindAllSubrecords<CLStringSubrecord>(NAME_DESC);

		string AllFulls;
		string AllDescs;

		for (auto j : pFulls)
		{
			AllFulls += j->GetString() + ":";
		}

		for (auto j : pDescs)
		{
			AllDescs += j->GetString() + ":";
		}

		File.Printf(",\"%s\"", AllFulls.c_str());
		File.Printf(",\"%s\"", AllDescs.c_str());
		*/
		File.Printf("\n");
	}

}


void DumpLoadingScreens(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_LSCR);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Description, Icon\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CLscrRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pDesc = pRecord->FindSubrecord<CLStringSubrecord>(NAME_DESC);
		auto pIcon = pRecord->FindSubrecord<CStringSubrecord>(NAME_ICON);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
		File.Printf(",\"%s\"", pDesc ? EscapeCsv(pDesc->GetString()).c_str() : "");
		File.Printf(",\"%s\"", pIcon ? pIcon->GetCString() : "");

		File.Printf("\n");
	}

}


void DumpCells(CEspFile& espFile, const string Filename)
{
	auto Records = espFile.FindAllRecords(NAME_CELL);
	CFile File;

	if (Records.size() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Full, Data\n");

	for (auto i : Records)
	{
		auto pRecord = dynamic_cast<CCellRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pData = pRecord->FindSubrecord<CDwordSubrecord>(NAME_DATA);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
		File.Printf(",\"%s\"", pFull ? EscapeCsv(pFull->GetString()).c_str() : "");
		File.Printf(",\"%d\"", pData ? pData->GetValue() : 0);

		File.Printf("\n");
	}

}


void DumpGbfm(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_GBFM);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Full, DNAM, Model\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CGbfmRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pDnam = pRecord->FindSubrecord<CStringSubrecord>(NAME_DNAM);
		auto pModel = pRecord->FindSubrecord<CStringSubrecord>(NAME_MODL);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
		File.Printf(",\"%s\"", pFull ? EscapeCsv(pFull->GetString()).c_str() : "");
		File.Printf(",\"%s\"", pDnam ? pDnam->GetCString() : "");
		File.Printf(",\"%s\"", pModel ? pModel->GetCString() : "");

		File.Printf("\n");
	}

}


void DumpFlst(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_FLST);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Full\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CFlstRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		
		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
		File.Printf(",\"%s\"", pFull ? EscapeCsv(pFull->GetString()).c_str() : "");

		File.Printf("\n");
	}

}


void DumpDialogueInfos(CEspFile& espFile, CQustRecord* pQuest, CDialRecord* pDialog, CGroup* pGroup, CFile& File)
{
	if (pQuest == nullptr || pDialog == nullptr || pGroup == nullptr) return;

	for (auto pRecord : pGroup->GetRecords())
	{
		auto pInfo = dynamic_cast<CInfoRecord*>(pRecord);
		if (pInfo == nullptr) continue;

		auto editorID = pInfo->GetEditorId();

		auto pRec = dynamic_cast<CRecord *>(pRecord);
		if (pRec == nullptr) continue;

		CDataSubrecord* pTrda = nullptr;
		CDataSubrecord* pTrot = nullptr;
		CLStringSubrecord* pNam1 = nullptr;
		CStringSubrecord* pNam2 = nullptr;
		CStringSubrecord* pNam3 = nullptr;
		CDwordSubrecord* pPerk = nullptr;
		CDwordSubrecord* pGnam = nullptr;
		CCtdaSubrecord* pCtdaPerk = nullptr;
		CCtdaSubrecord* pCtdaFaction = nullptr;
		CCtdaSubrecord* pCtdaRank = nullptr;
		CCtdaSubrecord* pGlobCtdaPerk = nullptr;
		CCtdaSubrecord* pGlobCtdaFaction = nullptr;
		CCtdaSubrecord* pGlobCtdaRank = nullptr;
		CDwordSubrecord* pGlobGnam = nullptr;
		
		//INAM, ENAM, CTDA, NAM4, NAM9
		int infoCount = 0;

		for (auto pSubrecord : pRec->GetSubrecordArray())
		{
			if (pSubrecord->GetRecordType() == NAME_TRDA)
			{
				if (pTrda != nullptr)
				{
					File.Printf("\"INFO\",,,,,,,,,,");
					File.Printf(",\"%d\"", infoCount);
					File.Printf(",\"0x%08X\"", pInfo->GetFormID());
					File.Printf(",\"%s\"", editorID.c_str());

					auto pPerkRec = espFile.FindFormId<CPerkRecord>(pPerk ? pPerk->GetValue() : 0);
					auto pPerkName = pPerkRec ? pPerkRec->FindSubrecord<CLStringSubrecord>(NAME_FULL) : nullptr;
					
					if (pPerk == nullptr && pCtdaPerk != nullptr)
					{
						pPerkRec = espFile.FindFormId<CPerkRecord>(pCtdaPerk->GetCtdaData().param1);
						pPerkName = pPerkRec ? pPerkRec->FindSubrecord<CLStringSubrecord>(NAME_FULL) : nullptr;
					}

					File.Printf(",\"%s\"", pPerkName ? pPerkName->GetCString() : "");

					auto pFactRec = pCtdaFaction ? espFile.FindFormId<CFactRecord>(pCtdaFaction->GetCtdaData().param1) : nullptr;
					auto pFactName = pFactRec ? pFactRec->FindSubrecord<CLStringSubrecord>(NAME_FULL) : nullptr;
					auto pFactEdid = pFactRec ? pFactRec->FindSubrecord<CStringSubrecord>(NAME_EDID) : nullptr;
					auto FactString = pFactName ? pFactName->GetString() : (pFactEdid ? pFactEdid->GetString() : "");

					if (pCtdaRank)
					{
						pFactRec = pCtdaRank ? espFile.FindFormId<CFactRecord>(pCtdaRank->GetCtdaData().param1) : nullptr;
						auto pFactName = pFactRec ? pFactRec->FindSubrecord<CStringSubrecord>(NAME_EDID) : nullptr;
						FactString += " / " + (pFactName ? pFactName->GetString() : "");
						FactString += " (" + std::to_string(pCtdaRank->GetCtdaData().value) + ")";
					}

					File.Printf(",\"%s\"", FactString.c_str());

					File.Printf(",\"%s\"", pNam1 ? EscapeCsv(pNam1->GetString()).c_str() : "");
					File.Printf(",\"%s\"", pNam2 ? EscapeCsv(pNam2->GetString()).c_str() : "");
					File.Printf(",\"%s\"", pNam3 ? EscapeCsv(pNam3->GetString()).c_str() : "");

					if (pGnam)
					{
						auto pGnamInfo = espFile.FindFormId<CInfoRecord>(pGnam->GetValue());
						File.Printf(",\"0x%08X\"", pGnam->GetValue());
						File.Printf(",\"%s\"", pGnamInfo ? pGnamInfo->GetEditorId().c_str() : "");
					}
					else
					{
						File.Printf(",,");
					}

					File.Printf("\n");
				}

				pTrda = dynamic_cast<CDataSubrecord *>(pSubrecord);
				pTrot = nullptr;
				pNam1 = nullptr;
				pNam2 = nullptr;
				pNam3 = nullptr;
				pPerk = nullptr;
				pGnam = pGlobGnam;
				pCtdaPerk = pGlobCtdaPerk;
				pCtdaFaction = pGlobCtdaFaction;
				pCtdaRank = pGlobCtdaRank;

				infoCount++;
			}
			else if (pSubrecord->GetRecordType() == NAME_TROT)
			{
				pTrot = dynamic_cast<CDataSubrecord *>(pSubrecord);
			}
			else if (pSubrecord->GetRecordType() == NAME_NAM1)
			{
				pNam1 = dynamic_cast<CLStringSubrecord *>(pSubrecord);
			}
			else if (pSubrecord->GetRecordType() == NAME_NAM2)
			{
				pNam2 = dynamic_cast<CStringSubrecord *>(pSubrecord);
			}
			else if (pSubrecord->GetRecordType() == NAME_NAM3)
			{
				pNam3 = dynamic_cast<CStringSubrecord *>(pSubrecord);
			}
			else if (pSubrecord->GetRecordType() == NAME_PERK)
			{
				pPerk = dynamic_cast<CDwordSubrecord *>(pSubrecord);
			}
			else if (pSubrecord->GetRecordType() == NAME_GNAM)
			{
				pGnam = dynamic_cast<CDwordSubrecord *>(pSubrecord);
				pGlobGnam = pGnam;
			}
			else if (pSubrecord->GetRecordType() == NAME_CTDA && pTrda == nullptr)
			{
				auto pCtda = dynamic_cast<CCtdaSubrecord *>(pSubrecord);

				if (pCtda && pCtda->GetCtdaData().function == 448)
					pGlobCtdaPerk = pCtda;
				else if (pCtda && pCtda->GetCtdaData().function == 507)	//Not used?
					pGlobCtdaRank = pCtda;
				else if (pCtda && pCtda->GetCtdaData().function == 71)
					pGlobCtdaFaction = pCtda;
				else if (pCtda && pCtda->GetCtdaData().function == 73)	//Only used once?
					pGlobCtdaRank = pCtda;

				if (pCtda) SystemLog.Printf("INFO GLOB CTDA: %d %d %f %X", pCtda->GetCtdaData().type, pCtda->GetCtdaData().function, pCtda->GetCtdaData().value, pCtda->GetCtdaData().param1);
			}
			else if (pSubrecord->GetRecordType() == NAME_CTDA && pTrda)
			{
				auto pCtda = dynamic_cast<CCtdaSubrecord *>(pSubrecord);

				if (pCtda && pCtda->GetCtdaData().function == 448)
					pCtdaPerk = pCtda;
				else if (pCtda && pCtda->GetCtdaData().function == 507)	//Not used?
					pCtdaRank = pCtda;
				else if (pCtda && pCtda->GetCtdaData().function == 71)
					pCtdaFaction = pCtda;
				else if (pCtda && pCtda->GetCtdaData().function == 73)	//Only used once?
					pCtdaRank = pCtda;

				if (pCtda) SystemLog.Printf("INFO TRDA CTDA: %d %d %f %X", pCtda->GetCtdaData().type, pCtda->GetCtdaData().function, pCtda->GetCtdaData().value, pCtda->GetCtdaData().param1);
			}
		}

		if (pTrda != nullptr)
		{
			File.Printf("\"INFO\",,,,,,,,,,");
			File.Printf(",\"%d\"", infoCount);
			File.Printf(",\"0x%08X\"", pInfo->GetFormID());
			File.Printf(",\"%s\"", editorID.c_str());

			auto pPerkRec = espFile.FindFormId<CPerkRecord>(pPerk ? pPerk->GetValue() : 0);
			auto pPerkName = pPerkRec ? pPerkRec->FindSubrecord<CLStringSubrecord>(NAME_FULL) : nullptr;

			if (pPerk == nullptr && pCtdaPerk != nullptr)
			{
				pPerkRec = espFile.FindFormId<CPerkRecord>(pCtdaPerk->GetCtdaData().param1);
				pPerkName = pPerkRec ? pPerkRec->FindSubrecord<CLStringSubrecord>(NAME_FULL) : nullptr;
			}

			File.Printf(",\"%s\"", pPerkName ? pPerkName->GetCString() : "");

			auto pFactRec = pCtdaFaction ? espFile.FindFormId<CFactRecord>(pCtdaFaction->GetCtdaData().param1) : nullptr;
			auto pFactName = pFactRec ? pFactRec->FindSubrecord<CLStringSubrecord>(NAME_FULL) : nullptr;
			auto pFactEdid = pFactRec ? pFactRec->FindSubrecord<CStringSubrecord>(NAME_EDID) : nullptr;
			auto FactString = pFactName ? pFactName->GetString() : (pFactEdid ? pFactEdid ->GetString() : "");

			if (pCtdaRank)
			{
				pFactRec = pCtdaRank ? espFile.FindFormId<CFactRecord>(pCtdaRank->GetCtdaData().param1) : nullptr;
				auto pFactName = pFactRec ? pFactRec->FindSubrecord<CStringSubrecord>(NAME_EDID) : nullptr;
				FactString += " / " + (pFactName ? pFactName->GetString() : "");
				FactString += " (" + std::to_string(pCtdaRank->GetCtdaData().value) + ")";
			}

			File.Printf(",\"%s\"", FactString.c_str());

			File.Printf(",\"%s\"", pNam1 ? EscapeCsv(pNam1->GetString()).c_str() : "");
			File.Printf(",\"%s\"", pNam2 ? EscapeCsv(pNam2->GetString()).c_str() : "");
			File.Printf(",\"%s\"", pNam3 ? EscapeCsv(pNam3->GetString()).c_str() : "");

			if (pGnam)
			{
				auto pGnamInfo = espFile.FindFormId<CInfoRecord>(pGnam->GetValue());
				File.Printf(",\"0x%08X\"", pGnam->GetValue());
				File.Printf(",\"%s\"", pGnamInfo ? pGnamInfo->GetEditorId().c_str() : "");
			}
			else
			{
				File.Printf(",,");
			}

			File.Printf("\n");
		}
	}

}


void DumpDialogueGroup(CEspFile& espFile, CQustRecord* pQuest, CGroup* pGroup, CFile& File)
{
	CDialRecord* pLastDial = nullptr;

	if (pQuest == nullptr || pGroup == nullptr) return;

	auto pQuestEDID = pQuest->FindSubrecord<CStringSubrecord>(NAME_EDID);
	auto questEditorId = pQuestEDID ? pQuestEDID->GetString() : "";
	auto pQuestFULL = pQuest->FindSubrecord<CLStringSubrecord>(NAME_FULL);
	auto questName = pQuestFULL ? pQuestFULL->GetString() : "";

	for (auto pRecord : pGroup->GetRecords())
	{
		if (pRecord->GetRecordType() == NAME_DIAL)
		{
			pLastDial = dynamic_cast<CDialRecord* >(pRecord);
			if (pLastDial == nullptr) continue;

			File.Printf("\"DIAL\"", questEditorId.c_str());
			File.Printf(", \"%s\"", questEditorId.c_str());
			File.Printf(",\"%s\"", EscapeCsv(questName).c_str());

			auto pEditorID = pLastDial->FindSubrecord<CStringSubrecord>(NAME_EDID);
			auto pFull = pLastDial->FindSubrecord<CLStringSubrecord>(NAME_FULL);

			File.Printf(",\"0x%08X\"", pLastDial->GetFormID());
			File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");

			auto pData = pLastDial->FindSubrecord<CDwordSubrecord>(NAME_DATA);
			auto pBnam = pLastDial->FindSubrecord<CDwordSubrecord>(NAME_BNAM);
			auto pQnam = pLastDial->FindSubrecord<CDwordSubrecord>(NAME_QNAM);
			auto pSnam = pLastDial->FindSubrecord<CStringSubrecord>(NAME_SNAM);
			auto pPnam = pLastDial->FindSubrecord<CFloatSubrecord>(NAME_PNAM);

			//rectype_t Subtype = pSnam ? pSnam->GetValue() : 0;

			File.Printf(",\"0x%08X\"", pData ? pData->GetValue() : 0);
			File.Printf(",\"%f\"", pPnam ? pPnam->GetValue() : 0);
			File.Printf(",\"0x%08X\"", pQnam ? pQnam->GetValue() : 0);
			File.Printf(",\"0x%08X\"", pBnam ? pBnam->GetValue() : 0);
			File.Printf(",\"%s\"", pSnam ? pSnam->GetCString() : "");
			//File.Printf(",\"%4.4s\"", Subtype.Name);

			File.Printf(",\"%s\"", pFull ? EscapeCsv(pFull->GetString()).c_str() : "");
			File.Printf("\n");
		}
		else if (pRecord->IsGroup())
		{
			DumpDialogueInfos(espFile, pQuest, pLastDial, dynamic_cast<CGroup*>(pRecord), File);
		}
	}

}


void DumpDialogue(CEspFile& espFile, const string Filename)
{
	auto pQuests = espFile.GetTypeGroup(NAME_QUST);
	CQustRecord* pLastQuest = nullptr;
	CFile File;

	if (pQuests == nullptr || pQuests->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("Type, QuestEditorID, QuestName, FormId, EditorId, Flags, Priority, QNAM, BNAM, Subtype, FULL, InfoNum, InfoFormId, InfoEditorId, InfoPerk, InfoFaction, InfoNam1, InfoNam2, InfoNam3, InfoGNamForm, InfoGNamEditorId\n");

	for (auto pRecord : pQuests->GetRecords())
	{
		if (pRecord->GetRecordType() == NAME_QUST) pLastQuest = dynamic_cast<CQustRecord*>(pRecord);

		if (pRecord->IsGroup()) DumpDialogueGroup(espFile, pLastQuest, dynamic_cast<CGroup *>(pRecord), File);

	}
}



struct espstructinfo_t {
	rectype_t Name;
	int Count;
};



void DumpStructure(CGroup* pGroup, CFile& File, const int tabLevel)
{
	std::unordered_map<rectype_t, espstructinfo_t> recordStats;
	rectype_t lastRecordType = NAME_NULL;
	int lastRecordCount = 0;
	string Tabs(tabLevel, '\t');

	if (pGroup == nullptr) return;

	for (auto i : pGroup->GetRecords())
	{
		if (i->IsRecord() && lastRecordType == i->GetRecordType())
		{
			++lastRecordCount;
			continue;
		}
		else if (lastRecordType != NAME_NULL)
		{
			File.Printf("%s%4.4s - Record (x%d)\n", Tabs.c_str(), lastRecordType.Name, lastRecordCount);
			lastRecordType = NAME_NULL;
			lastRecordCount = 0;
		}

		if (i->IsRecord())
		{
			lastRecordType = i->GetRecordType();
			lastRecordCount = 1;
			//auto stat = recordStats[i->GetRecordType()];
			//++stat.Count;
			//File.Printf("%s%4.4s - Record\n", Tabs.c_str(), i->GetRecordType().Name);
		}
		else if (i->IsGroup())
		{
			auto pGroup = dynamic_cast<CGroup*>(i);
			//if (pGroup == nullptr) continue;

			File.Printf("%s%4.4s - GROUP (%d records)\n", Tabs.c_str(), i->GetRecordType().Name, pGroup ? pGroup->GetNumRecords() : -1);
			DumpStructure(pGroup, File, tabLevel + 1);
		}
		else
		{
			File.Printf("%s%4.4s - Unknown\n", Tabs.c_str(), i->GetRecordType().Name);
		}
	}

	if (lastRecordType != NAME_NULL)
	{
		File.Printf("%s%4.4s - Record (x%d)\n", Tabs.c_str(), lastRecordType.Name, lastRecordCount);
		lastRecordType = NAME_NULL;
		lastRecordCount = 0;
	}

}


void DumpStructure(CEspFile& espFile, const string Filename)
{
	
	CFile File;
	if (!File.Open(Filename, "wt")) return;

	for (auto i : espFile.GetRecords())
	{
		if (i->IsRecord())
		{
			File.Printf("%4.4s - Record\n", i->GetRecordType().Name);
		}
		else if (i->IsGroup())
		{
			auto pGroup = dynamic_cast<CGroup*>(i);
			File.Printf("%4.4s - GROUP (%d records)\n", pGroup ? pGroup->GetHeader().Data.ContainsType : i->GetRecordType().Name, pGroup ? pGroup->GetNumRecords() : -1);
			DumpStructure(pGroup, File, 1);
		}
		else
		{
			File.Printf("%4.4s - Unknown\n", i->GetRecordType().Name);
		}
	}
}


void DumpCobj(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_COBJ);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Description, Data, NNam, ResultFormId, ResultEditorId, ResultType, ResultName, BenchFormId, BenchEditorId, BenchType, FNAM, LRNM, NumIngr, Ingrediants, RawIngrediants\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CCobjRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pDesc = pRecord->FindSubrecord<CLStringSubrecord>(NAME_DESC);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
		File.Printf(",\"%s\"", pDesc ? EscapeCsv(pDesc->GetString()).c_str() : "");

		auto pData = pRecord->FindSubrecord<CDwordSubrecord>(NAME_DATA);
		File.Printf(",\"%d\"", pData ? pData->GetValue() : 0);

		auto pNnam = pRecord->FindSubrecord<CWordSubrecord>(NAME_NNAM);
		File.Printf(",\"%d\"", pNnam ? pNnam->GetValue() : 0);

		auto pCnam = pRecord->FindSubrecord<CDwordSubrecord>(NAME_CNAM);
		auto pResult = pCnam ? espFile.FindFormId<CRecord>(pCnam->GetValue()) : nullptr;

		if (pResult && pCnam && pCnam->GetValue() != 0)
		{
			File.Printf(",0x%08X", pCnam->GetValue());
			auto pEditorID = pResult->FindSubrecord<CStringSubrecord>(NAME_EDID);
			auto pFull = pResult->FindSubrecord<CLStringSubrecord>(NAME_FULL);

			File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
			File.Printf(",\"%4.4s\"", pResult->GetRecordType().Name);
			File.Printf(",\"%s\"", pFull ? EscapeCsv(pFull->GetString()).c_str() : "");
		}
		else
		{
			File.Printf(",\"\",\"\",\"\",\"\"");
		}

		auto pBnam = pRecord->FindSubrecord<CDwordSubrecord>(NAME_BNAM);
		auto pBench = espFile.FindFormId<CRecord>(pBnam ? pBnam->GetValue() : 0);

		if (pBench && pBnam && pBnam->GetValue())
		{
			File.Printf(",0x%08X", pBench->GetFormID());
			auto pEditorID = pBench->FindSubrecord<CStringSubrecord>(NAME_EDID);

			File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
			File.Printf(",\"%4.4s\"", pBench->GetRecordType().Name);
		}
		else
		{
			File.Printf(",\"\",\"\",\"\"");
		}

		auto pFnam = pRecord->FindSubrecord<CDwordSubrecord>(NAME_FNAM);
		auto pFkywd = espFile.FindFormId<CRecord>(pFnam ? pFnam->GetValue() : 0);

		if (pFkywd && pFnam && pFnam->GetValue())
		{
			auto pEditorID = pFkywd->FindSubrecord<CStringSubrecord>(NAME_EDID);
			File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
		}
		else
		{
			File.Printf(",\"\"");
		}

		auto pLrnm = pRecord->FindSubrecord<CByteSubrecord>(NAME_LRNM);
		File.Printf(",\"%d\"", pLrnm ? pLrnm->GetValue() : 0);
		
		auto pFvpa = pRecord->FindSubrecord<CDataSubrecord>(NAME_FVPA);
		int numIngr = pFvpa ? pFvpa->GetRecordSize()/12: 0;
		File.Printf(",\"%d\"", numIngr);

		if (pFvpa && numIngr > 0)
		{
			string buffer;
			string rawBuffer;

			for (auto j = 0; j < numIngr; ++j)
			{
				size_t offset = j * 12;
				dword resId = *(dword*) (pFvpa->GetData() + offset + 0);
				dword qnt = *(dword*)(pFvpa->GetData() + offset + 4);
				dword unk = *(dword*)(pFvpa->GetData() + offset + 8);

				auto pIres = espFile.FindFormId<CIresRecord>(resId);
				string ingrRawBuffer = FormatString("x%d ", qnt);
				string ingrBuffer = ingrRawBuffer;

				if (pIres && resId)
				{
					auto pEditorID = pIres->FindSubrecord<CStringSubrecord>(NAME_EDID);
					auto pFull = pIres->FindSubrecord<CLStringSubrecord>(NAME_FULL);
					ingrRawBuffer += pEditorID ? pEditorID->GetString() : FormatString("0x%X", resId);
					ingrBuffer += pFull ? pFull->GetCString() : FormatString("0x%X", resId);
				}
				else
				{
					ingrBuffer += FormatString("0x%X", resId);
					ingrRawBuffer += FormatString("0x%X", resId);
				}
				
				//ingrBuffer += FormatString(" (%d)", unk); //Always 0?

				if (!buffer.empty()) buffer += ", ";
				if (!rawBuffer.empty()) rawBuffer += ", ";
				buffer += ingrBuffer;
				rawBuffer += ingrRawBuffer;
			}

			File.Printf(",\"%s\"", buffer.c_str());
			File.Printf(",\"%s\"", rawBuffer.c_str());
		}
		else
		{
			File.Printf(",\"\"");
		}


		File.Printf("\n");
	}
}



void DumpTerm(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_TERM);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Full, Model, TMLM\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CTermRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pModel = pRecord->FindSubrecord<CStringSubrecord>(NAME_MODL);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
		File.Printf(",\"%s\"", pFull ? EscapeCsv(pFull->GetString()).c_str() : "");
		File.Printf(",\"%s\"", pModel ? EscapeCsv(pModel->GetString()).c_str() : "");

		auto pDnam = pRecord->FindSubrecord<CDwordSubrecord>(NAME_DNAM);
		auto pTmlm = espFile.FindFormId<CTmlmRecord>(pDnam ? pDnam->GetValue() : 0);

		if (pDnam && pTmlm && pDnam->GetValue() > 0)
		{
			auto pEditorID = pTmlm->FindSubrecord<CStringSubrecord>(NAME_EDID);
			File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
		}
		else
		{
			File.Printf(",\"\"");
		}

		File.Printf("\n");
	}

}


void DumpTmlm(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_TMLM);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Full, BTXT, Index, ITXT, ISTX, ITID, TNAM, UNAM\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CTmlmRecord *>(i);
		if (pRecord == nullptr) continue;

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pBtxt = pRecord->FindSubrecord<CLStringSubrecord>(NAME_BTXT);
		auto pIsiz = pRecord->FindSubrecord<CDwordSubrecord>(NAME_ISIZ);

		CLStringSubrecord* pLastItxt = nullptr;
		CLStringSubrecord* pIstx = nullptr;
		CDataSubrecord* pIset = nullptr;
		CWordSubrecord* pItid = nullptr;
		CDwordSubrecord* pTnam = nullptr;
		CLStringSubrecord* pUnam = nullptr;
		int index = 0;

		for (auto j : pRecord->GetSubrecordArray())
		{
			if (j->GetRecordType() == NAME_ITXT)
			{
				if (pLastItxt)
				{
					File.Printf("0x%08X", pRecord->GetFormID());
					File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
					File.Printf(",\"%s\"", pFull ? EscapeCsv(pFull->GetString()).c_str() : "");
					File.Printf(",\"%s\"", pBtxt ? EscapeCsv(pBtxt->GetString()).c_str() : "");

					auto pTmlm = espFile.FindFormId<CTmlmRecord>(pTnam ? pTnam->GetValue() : 0);
					auto pEditorID = pTmlm ? pTmlm->FindSubrecord<CStringSubrecord>(NAME_EDID) : nullptr;

					File.Printf(",\"%d\"", index);
					File.Printf(",\"%s\"", pLastItxt ? pLastItxt->GetCString() : "");
					File.Printf(",\"%s\"", pIstx ? EscapeCsv(pIstx->GetString()).c_str() : 0);
					File.Printf(",\"%d\"", (int) (pItid ? pItid->GetValue() : 0));
					File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
					File.Printf(",\"%s\"", pUnam ? EscapeCsv(pUnam->GetString()).c_str() : "");
					File.Printf("\n");
				}

				pLastItxt = dynamic_cast<CLStringSubrecord *>(j);
				pIstx = nullptr;
				pIset = nullptr;
				pItid = nullptr;
				pTnam = nullptr;
				pUnam = nullptr;
				++index;
			}
			else if (j->GetRecordType() == NAME_ISTX)
			{
				pIstx = dynamic_cast<CLStringSubrecord*>(j);
			}
			else if (j->GetRecordType() == NAME_ISET)
			{
				pIset = dynamic_cast<CDataSubrecord*>(j);
			}
			else if (j->GetRecordType() == NAME_ITID)
			{
				pItid = dynamic_cast<CWordSubrecord*>(j);
			}
			else if (j->GetRecordType() == NAME_TNAM)
			{
				pTnam = dynamic_cast<CDwordSubrecord*>(j);
			}
			else if (j->GetRecordType() == NAME_UNAM)
			{
				pUnam = dynamic_cast<CLStringSubrecord*>(j);
			}
		}

		if (pLastItxt)
		{
			File.Printf("0x%08X", pRecord->GetFormID());
			File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
			File.Printf(",\"%s\"", pFull ? EscapeCsv(pFull->GetString()).c_str() : "");
			File.Printf(",\"%s\"", pBtxt ? EscapeCsv(pBtxt->GetString()).c_str() : "");

			auto pTmlm = espFile.FindFormId<CTmlmRecord>(pTnam ? pTnam->GetValue() : 0);
			auto pEditorID = pTmlm ? pTmlm->FindSubrecord<CStringSubrecord>(NAME_EDID) : nullptr;

			File.Printf(",\"%d\"", index);
			File.Printf(",\"%s\"", pLastItxt ? pLastItxt->GetCString() : "");
			File.Printf(",\"%s\"", pIstx ? EscapeCsv(pIstx->GetString()).c_str() : 0);
			File.Printf(",\"%d\"", (int)(pItid ? pItid->GetValue() : 0));
			File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
			File.Printf(",\"%s\"", pUnam ? EscapeCsv(pUnam->GetString()).c_str() : "");
			File.Printf("\n");
		}
	}

}


void DumpIres(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_IRES);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Full, Short, FNAM, Nam1, Nam2, Nam3, SNAM, TINC, CNAM\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CIresRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pFnam = pRecord->FindSubrecord<CDwordSubrecord>(NAME_FNAM);
		auto pNam1 = pRecord->FindSubrecord<CDwordSubrecord>(NAME_NAM1);
		auto pNam2 = pRecord->FindSubrecord<CDwordSubrecord>(NAME_NAM2);
		auto pNam3 = pRecord->FindSubrecord<CDwordSubrecord>(NAME_NAM3);
		auto pNnam = pRecord->FindSubrecord<CLStringSubrecord>(NAME_NNAM);
		auto pSnam = pRecord->FindSubrecord<CDwordSubrecord>(NAME_SNAM);
		auto pCnam = pRecord->FindSubrecord<CDwordSubrecord>(NAME_CNAM);
		auto pTinc = pRecord->FindSubrecord<CDwordSubrecord>(NAME_TINC);

		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
		File.Printf(",\"%s\"", pFull ? EscapeCsv(pFull->GetString()).c_str() : "");
		File.Printf(",\"%s\"", pNnam ? EscapeCsv(pNnam->GetString()).c_str() : "");
		File.Printf(",\"0x%08X\"", pFnam ? pFnam->GetValue() : 0);
		File.Printf(",\"0x%08X\"", pNam1 ? pNam1->GetValue() : 0);
		File.Printf(",\"0x%08X\"", pNam2 ? pNam2->GetValue() : 0);
		File.Printf(",\"0x%08X\"", pNam3 ? pNam3->GetValue() : 0);
		File.Printf(",\"0x%08X\"", pSnam ? pSnam->GetValue() : 0);
		File.Printf(",\"0x%08X\"", pTinc ? pTinc->GetValue() : 0);
		File.Printf(",\"0x%08X\"", pCnam ? pCnam->GetValue() : 0);

		File.Printf("\n");
	}

}


void DumpOmod(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_OMOD);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Full, Description, Model, FLLD, NAM1, XFLG, Includes, Properties, AttachParentSlots, DataName, AttachPointFormId, AttachPoint, Unknown1, Unknown2, Unknown3\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<COmodRecord *>(i);
		if (pRecord == nullptr) continue;

		File.Printf("0x%08X", pRecord->GetFormID());

		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);
		auto pDesc = pRecord->FindSubrecord<CLStringSubrecord>(NAME_DESC);
		auto pModl = pRecord->FindSubrecord<CStringSubrecord>(NAME_MODL);
		
		File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
		File.Printf(",\"%s\"", pFull ? EscapeCsv(pFull->GetString()).c_str() : "");
		File.Printf(",\"%s\"", pDesc ? EscapeCsv(pDesc->GetString()).c_str() : "");
		File.Printf(",\"%s\"", pModl ? EscapeCsv(pModl->GetString()).c_str() : "");
				
		auto pFlld = pRecord->FindSubrecord<CDwordSubrecord>(NAME_FLLD);
		auto pNam1 = pRecord->FindSubrecord<CByteSubrecord>(NAME_NAM1);
		auto pXflg = pRecord->FindSubrecord<CByteSubrecord>(NAME_XFLG);

		File.Printf(",%d", pFlld ? pFlld->GetValue() : 0);
		File.Printf(",%d", pNam1 ? pNam1->GetValue() : 0);
		File.Printf(",%d", pXflg ? pXflg->GetValue() : 0);

		auto pData = pRecord->FindSubrecord<COmodDataSubrecord>(NAME_DATA);

		if (pData)
		{
			auto data = pData->GetOmodData();
			File.Printf(",%d,%d,%d", data.includeCount, data.propertyCount, data.attachParentSlotCount);
			File.Printf(",\"%s\"", data.name.c_str());

			auto pAttach = espFile.FindFormId<CIdRecord>(data.attachPointFormId);
			auto pAttachEditorId = pAttach ? pAttach->FindSubrecord<CStringSubrecord>(NAME_EDID) : nullptr;
			File.Printf(",0x%08X", data.attachPointFormId);
			File.Printf(",\"%s\"", pAttachEditorId ? pAttachEditorId->GetCString() : "");

			File.Printf(",0x%04X,0x%04X,0x%08X", data.unknown1, data.unknown2, data.unknown3);
		}
		else
		{
			File.Printf(",0,0,0,\"\",0,\"\",0,0,0");
		}

		File.Printf("\n");
	}

}


void DumpOmodProperties(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_OMOD);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("OModFormID, OModEditorID, OModFull, Index, ValueType, FunctionType, PropertyName, Value1, Value2, Step\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<COmodRecord *>(i);
		if (pRecord == nullptr) continue;
		auto pEditorID = pRecord->FindSubrecord<CStringSubrecord>(NAME_EDID);
		auto pFull = pRecord->FindSubrecord<CLStringSubrecord>(NAME_FULL);

		auto pData = pRecord->FindSubrecord<COmodDataSubrecord>(NAME_DATA);
		if (pData == nullptr) continue;

		auto data = pData->GetOmodData();
		dword index = 0;

		bool isFloat = false;
		bool isFormId = false;
		float testFloat = 0;

		for (auto p : data.properties)
		{ 
			++index;

			File.Printf("0x%08X", pRecord->GetFormID());
			File.Printf(",\"%s\"", pEditorID ? pEditorID->GetCString() : "");
			File.Printf(",\"%s\"", pFull ? EscapeCsv(pFull->GetString()).c_str() : "");
			File.Printf(",%d", index);
			File.Printf(",%d", p.valueType);
			File.Printf(",%d", p.valueFunction);
			File.Printf(",%4.4s", p.propertyName.Name);

			isFloat = false;
			isFormId = false;
			testFloat = *(float *)&p.value1;
			isFloat = (testFloat > -10000 && testFloat < 10000);
			isFormId = (espFile.FindFormId(p.value1) != nullptr);

			if (p.valueType == 4 || p.valueType == 6)
			{
				auto pRec = espFile.FindFormId<CRecord>(p.value1);
				auto pEdid = pRec ? pRec->FindSubrecord<CStringSubrecord>(NAME_EDID) : nullptr;
				
				if (pEdid)
					File.Printf(",\"%4.4s:%08X %s\"", pRec->GetRecordType().Name, p.value1, pEdid->GetCString());
				else if (pRec)
					File.Printf(",\"%4.4s:%08X\"", pRec->GetRecordType().Name, p.value1);
				else
					File.Printf(",\"%08X\"", p.value1);

				//File.Printf(",\"formid\"");
			}
			else if (p.valueType == 1)
			{
				File.Printf(",%f", testFloat);
				//File.Printf(",\"float\"");
			}
			else
			{
				File.Printf(",%d", p.value1);
				//File.Printf(",\"dword\"");
			}

			if (p.valueType == 1 || p.valueType == 6)
				File.Printf(",%f", p.value2);
			else
				File.Printf(",%d", *(dword *)&p.value2);

			File.Printf(",%f", p.step);

			File.Printf("\n");
		}
	}

}


void DumpCreatures(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_NPC_);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Full, Keywords\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CNpcRecord *>(i);
		if (pRecord == nullptr) continue;

		if (!pRecord->HasKeyword(0x1e667f)) continue;

		File.Printf("0x%08X", pRecord->GetFormID());
		File.Printf(",\"%s\"", pRecord->GetEditorId().c_str());
		File.Printf(",\"%s\"", EscapeCsv(GetRecordItemName(pRecord)).c_str());

		File.Printf("\n");
	}
}


void DumpLvli(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_LVLI);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("FormID, EditorID, Model, ChanceNone, Flags, FlagText, Count, MaxCount, UseGlobal, LVLL, Override Name\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CLvliRecord *>(i);
		if (pRecord == nullptr) continue;

		auto pModl = pRecord->FindSubrecord<CStringSubrecord>(NAME_MODL);

		File.Printf("0x%08X", pRecord->GetFormID());
		File.Printf(",\"%s\"", pRecord->GetEditorId().c_str());
		File.Printf(",\"%s\"", pModl ? EscapeCsv(pModl->GetString()).c_str() : "");

		float ChanceNone = pRecord->GetSubrecordValue(NAME_LVLD, 0.0f);
		int Flags = pRecord->GetSubrecordValue(NAME_LVLF, (word)0);
		int Count = pRecord->GetSubrecordValue(NAME_LLCT, (byte)0);
		int MaxCount = pRecord->GetSubrecordValue(NAME_LVLM, (byte)0);
		int Lvll = pRecord->GetSubrecordValue(NAME_LVLL, (byte)0);
		dword UseGlobal = pRecord->GetSubrecordValue(NAME_LVLG, (dword)0);
		string Onam = pRecord->GetSubrecordValue(NAME_ONAM, "");

		File.Printf(",%f", ChanceNone);

		File.Printf(",%d,\"", Flags);
		if (Flags & 1) File.Printf("AllLevels ");
		if (Flags & 2) File.Printf("EachItem ");
		if (Flags & 4) File.Printf("UseAll ");
		if (Flags & 8) File.Printf("U8 ");
		if (Flags & 16) File.Printf("U16 ");
		if (Flags & 32) File.Printf("U32 ");
		if (Flags & 64) File.Printf("U64 ");
		if (Flags & 128) File.Printf("U128 ");
		if (Flags & 256) File.Printf("U256 ");

		File.Printf("\",%d", Count);
		File.Printf(",%d", MaxCount);
		File.Printf(",%d", UseGlobal);
		File.Printf(",%d", Lvll);
		File.Printf(",\"%s\"", EscapeCsv(Onam).c_str());

		File.Printf("\n");
	}
}

void DumpLvliEntries(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_LVLI);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("LvliFormID, LvliEditorID, Index, Level,  Count, ChanceNone, FormID, EditorID, Type\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CLvliRecord *>(i);
		if (pRecord == nullptr) continue;

		auto lvliEditorId = pRecord->GetEditorId();
		auto lvliFormId = pRecord->GetFormID();

		auto items = pRecord->FindAllSubrecords<CLvloSubrecord>(NAME_LVLO);
		auto index = 0;

		for (auto j : items)
		{
			auto data = j->GetLvloData();
			++index;

			File.Printf("0x%08X", lvliFormId);
			File.Printf(",\"%s\"", lvliEditorId.c_str());
			File.Printf(",%d", index);

			File.Printf(",%d", (int)data.Level);
			File.Printf(",%d", (int)data.Count);
			File.Printf(",%d", (int)data.ChanceNone);
			File.Printf(",0x%08X", data.ItemFormId);

			auto pItemRecord = espFile.FindFormId<CRecord>(data.ItemFormId);
			auto pItemEdid = pItemRecord ? pItemRecord->FindSubrecord<CStringSubrecord>(NAME_EDID) : nullptr;

			File.Printf(",\"%s\"", pItemEdid ? pItemEdid->GetCString() : "");
			File.Printf(",\"%4.4s\"", pItemRecord ? pItemRecord->GetRecordType().Name : "");

			File.Printf("\n");
		}
		
	}
}


void DumpRefUsesWorldCells(CFile& CsvFile, CEspFile& espFile, CWrldRecord* pLastWorld, CGroup* pLastGroup)
{
	if (pLastWorld == nullptr || pLastGroup == nullptr) return;

	auto refs = pLastGroup->FindAllRecords<CRefrRecord>(NAME_REFR);

	auto worldEditorId = pLastWorld->GetEditorId();
	auto worldName = pLastWorld->FindSubrecord<CLStringSubrecord>(NAME_FULL);
	printf("\t\t%s has %zd REFR records\n", worldEditorId.c_str(), refs.size());

	std::unordered_map<string, int> refCounts;

	for (auto i : refs)
	{
		auto refId = i->GetRefId();
		if (refId == 0) continue;
		
		auto editorId = FindRecordEditorId(espFile, refId);
		if (editorId.empty()) continue;
		refCounts[editorId]++;

		//printf("\t\t\t%s\n", );
	}

	for (auto i : refCounts)
	{
		CsvFile.Printf("\"%s\",WRLD,\"%s\",\"%s\",%d\n", i.first.c_str(), worldName ? worldName->GetCString() : "", worldEditorId.c_str(), i.second);
	}
}


void DumpRefUsesCells(CFile& CsvFile, CEspFile& espFile, CCellRecord* pCell, CGroup* pLastGroup)
{
	if (pCell == nullptr || pLastGroup == nullptr) return;

	auto refs = pLastGroup->FindAllRecords<CRefrRecord>(NAME_REFR);

	auto cellEditorId = pCell->GetEditorId();
	auto cellName = pCell->FindSubrecord<CLStringSubrecord>(NAME_FULL);
	printf("\t\t%s has %zd REFR records\n", cellEditorId.c_str(), refs.size());

	std::unordered_map<string, int> refCounts;

	for (auto i : refs)
	{
		auto refId = i->GetRefId();
		if (refId == 0) continue;

		auto editorId = FindRecordEditorId(espFile, refId);
		if (editorId.empty()) continue;
		refCounts[editorId]++;

		//printf("\t\t\t%s\n", );
	}

	for (auto i : refCounts)
	{
		CsvFile.Printf("\"%s\",CELL,\"%s\",\"%s\",%d\n", i.first.c_str(), cellName ? cellName->GetCString() : "", cellEditorId.c_str(), i.second);
	}
}


void DumpReferenceUses(CEspFile& espFile, const string Filename)
{
	auto pWorlds = espFile.GetTypeGroup(NAME_WRLD);
	CFile File;

	if (pWorlds == nullptr || pWorlds->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("Reference, Parent, FullName, World/Cell, Uses\n");

	CWrldRecord* pLastWorld = nullptr;
	CGroup* pLastGroup = nullptr;

	for (auto i : pWorlds->GetRecords())
	{
		//printf("%4.4s\n", (const char *)i->GetRecordType());

		auto pWorld = dynamic_cast<CWrldRecord *>(i);
		auto pGroup = dynamic_cast<CGroup *>(i);
		if (pGroup == nullptr && pWorld == nullptr) continue;

		if (pWorld != nullptr)
		{
			auto pEditorId = pWorld->GetEditorId();
			printf("\tFound world %s\n", pEditorId.c_str());

			pLastWorld = pWorld;
			pLastGroup = nullptr;
			continue;
		}

		if (pGroup != nullptr && pLastWorld == nullptr)
		{
			//printf("\t\tFound group\n");
			pLastGroup = nullptr;
			continue;
		}

		pLastGroup = pGroup;
		DumpRefUsesWorldCells(File, espFile, pLastWorld, pLastGroup);

		pLastWorld = nullptr;
		pLastGroup = nullptr;
	}

	auto pCells = espFile.GetTypeGroup(NAME_CELL);
	CCellRecord* pLastCell = nullptr;
	pLastGroup = nullptr;

	for (auto i : pCells->GetRecords())
	{
		auto pGroup1 = dynamic_cast<CGroup *>(i);
		if (pGroup1 == nullptr) continue;

		for (auto j : pGroup1->GetRecords())
		{
			auto pGroup2 = dynamic_cast<CGroup *>(j);
			if (pGroup2 == nullptr) continue;

			for (auto k : pGroup2->GetRecords())
			{
				auto pCell = dynamic_cast<CCellRecord *>(k);
				auto pGroup = dynamic_cast<CGroup *>(k);
				if (pGroup == nullptr && pCell == nullptr) continue;

				if (pCell != nullptr)
				{
					auto pEditorId = pCell->GetEditorId();
					printf("\tFound cell %s\n", pEditorId.c_str());

					pLastCell = pCell;
					pLastGroup = nullptr;
					continue;
				}

				if (pGroup != nullptr && pLastCell == nullptr)
				{
					//printf("\t\tFound cell group\n");
					pLastGroup = nullptr;
					continue;
				}

				pLastGroup = pGroup;
				DumpRefUsesCells(File, espFile, pLastCell, pLastGroup);

				pLastCell = nullptr;
				pLastGroup = nullptr;
			}
		}

	}

}


void DumpLvliWeapons(CEspFile& espFile, const string Filename)
{
	auto pRecords = espFile.GetTypeGroup(NAME_LVLI);
	CFile File;

	if (pRecords == nullptr || pRecords->GetNumRecords() == 0) return;

	if (!File.Open(Filename, "wt")) return;

	File.Printf("LvliFormID, LvliEditorID, Index\n");

	for (auto i : pRecords->GetRecords())
	{
		auto pRecord = dynamic_cast<CLvliRecord *>(i);
		if (pRecord == nullptr) continue;

		auto lvliEditorId = pRecord->GetEditorId();
		auto lvliFormId = pRecord->GetFormID();

		auto items = pRecord->FindAllSubrecords<CLvloSubrecord>(NAME_LVLO);
		auto index = 0;
		auto hasWeapons = false;

		for (auto j : items)
		{
			if (j->GetRecordType() == NAME_WEAP)
			{
				hasWeapons = true;
				break;
			}
		}

		if (!hasWeapons) continue;

		for (auto j : items)
		{
			auto data = j->GetLvloData();
			++index;

			File.Printf("0x%08X", lvliFormId);
			File.Printf(",\"%s\"", lvliEditorId.c_str());
			File.Printf(",%d", index);

			auto pWeapon = espFile.FindFormId<CWeapRecord>(data.ItemFormId);

			if (pWeapon == nullptr)
			{
				File.Printf("\n");
				continue;
			}

			
			File.Printf("\n");
		}

	}
}


std::string stripExtension(const std::string &filePath) {
	return { filePath, 0, filePath.rfind('.') };
}


bool ExtractBatchFile(string baseDir, string filename, string outputPath)
{
	string fullFilename = baseDir + filename;
	CEspFile espFile;
	string baseFilename = stripExtension(filename) + "\\";

	bool Result = espFile.Load(fullFilename);
	if (!Result) { printf("\tERROR: Failed to load ESM file '%s'!\n", fullFilename.c_str());  return false; }

	printf("\tSuccessfully loaded ESM file '%s'!\n", fullFilename.c_str());

	CreateDirectory(outputPath.c_str(), NULL);
	CreateDirectory((outputPath + baseFilename).c_str(), NULL);

	espFile.OutputStats(outputPath + baseFilename + "Stats.txt");
	DumpStructure(espFile, outputPath + baseFilename + "Structure.txt");
	
		//DumpBooks(espFile, outputPath + baseFilename + "_Books.txt");
	DumpBookFiles(espFile, outputPath + baseFilename + "/Books/");
	/*
	DumpBooksCsv(espFile, outputPath + baseFilename + "Books.csv");

	DumpWeapons(espFile, outputPath + baseFilename + "Weapons.csv");
	DumpAmmo(espFile, outputPath + baseFilename + "Ammo.csv");
	DumpArmors(espFile, outputPath + baseFilename + "Armors.csv");
	DumpFurnitures(espFile, outputPath + baseFilename + "Furniture.csv");
	DumpMisc(espFile, outputPath + baseFilename + "Misc.csv");
	DumpNpcs(espFile, outputPath + baseFilename + "Npcs.csv");
	DumpRaces(espFile, outputPath + baseFilename + "Races.csv");
	DumpFactions(espFile, outputPath + baseFilename + "Factions.csv");
	DumpClasses(espFile, outputPath + baseFilename + "Classes.csv");
	DumpMagicEffects(espFile, outputPath + baseFilename + "Effects.csv");
	DumpSettings(espFile, outputPath + baseFilename + "Settings.csv");
	DumpEquipSlots(espFile, outputPath + baseFilename + "EquipSlots.csv");
	DumpBioms(espFile, outputPath + baseFilename + "Bioms.csv");
	DumpSpells(espFile, outputPath + baseFilename + "Spells.csv");
	DumpEnchants(espFile, outputPath + baseFilename + "Enchants.csv");
	DumpAlchemy(espFile, outputPath + baseFilename + "Alchemy.csv");
	DumpAvif(espFile, outputPath + baseFilename + "ActorValues.csv");
	DumpWorlds(espFile, outputPath + baseFilename + "Worlds.csv");
	DumpLocations(espFile, outputPath + baseFilename + "Locations.csv");
	DumpSunPresets(espFile, outputPath + baseFilename + "SunPresets.csv");
	DumpStars(espFile, outputPath + baseFilename + "Stars.csv");
	DumpPlanets(espFile, outputPath + baseFilename + "Planets.csv");

	DumpQuests(espFile, outputPath + baseFilename + "Quests.csv");
	DumpQuestStages(espFile, outputPath + baseFilename + "QuestStages.csv");
	DumpQuestObjectives(espFile, outputPath + baseFilename + "QuestObjectives.csv");
	//DumpQuestScripts(espFile, outputPath + baseFilename + "QuestScripts3.csv");

	DumpFlora(espFile, outputPath + baseFilename + "Flora.csv");
	DumpPerks(espFile, outputPath + baseFilename + "Perks.csv");
	DumpLoadingScreens(espFile, outputPath + baseFilename + "LoadingScreens.csv");
	DumpCells(espFile, outputPath + baseFilename + "Cells.csv");
	DumpReferenceUses(espFile, outputPath + baseFilename + "RefUses.csv");
	DumpGbfm(espFile, outputPath + baseFilename + "Gbfm.csv");
	DumpFlst(espFile, outputPath + baseFilename + "Flst.csv");

	DumpDialogue(espFile, outputPath + baseFilename + "Dialogue.csv");
	DumpTerm(espFile, outputPath + baseFilename + "Term.csv");
	DumpTmlm(espFile, outputPath + baseFilename + "Tmlm.csv");

	DumpIres(espFile, outputPath + baseFilename + "Ires.csv");
	DumpCobj(espFile, outputPath + baseFilename + "Cobj.csv");

	DumpOmod(espFile, outputPath + baseFilename + "Omod.csv");
	DumpOmodProperties(espFile, outputPath + baseFilename + "OmodProperties.csv");

	DumpCreatures(espFile, outputPath + baseFilename + "Fauna.csv");

	DumpLvli(espFile, outputPath + baseFilename + "Lvli.csv");
	DumpLvliEntries(espFile, outputPath + baseFilename + "LvliEntries.csv");
	DumpLvliWeapons(espFile, outputPath + baseFilename + "LvliWeapons.csv");
	*/
	return true;
}


void ExtractBatch1()
{
	string baseDir = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Starfield\\Data\\";
	string outputPath = "C:\\Temp\\Starfield\\ExportBatch1\\";
	vector<string> files = {
		"sfbgs00a_a.esm",
		"sfbgs00a_d.esm",
		"sfbgs00a_e.esm",
		"sfbgs00a_f.esm",
		"sfbgs00a_j.esm",
		"sfbgs00b.esm",
		"sfbgs00e.esm",
		"sfbgs00f_a.esm",
		"sfbgs01b.esm",
		"sfbgs01c.esm",
		"sfbgs02a_a.esm",
		"sfbgs02b_a.esm",
		"SFBGS003.esm",
		"SFBGS004.esm",
		"SFBGS006.esm",
		"SFBGS007.esm",
		"SFBGS008.esm",
		"sfbgs009.esm",
		"sfbgs021.esm",
		"sfbgs023.esm",
		"sfbgs031.esm",
		"sfta01.esm" };

	for (auto & i : files)
	{
		ExtractBatchFile(baseDir, i, outputPath);
	}
}


void ExtractBatch2()
{
	string baseDir = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Starfield\\Data\\";
	string outputPath = "C:\\Temp\\Starfield\\ExportBatch2\\";
	vector<string> files = {
		"SFBGS004.esm",
		"SFBGS003.esm",
		"SFBGS008.esm",
		"Starfield.esm",
		"ShatteredSpace.esm",
		};

	for (auto & i : files)
	{
		ExtractBatchFile(baseDir, i, outputPath);
	}
}


int main()
{
	SystemLog.Open("testesm.log");

	ExtractBatch2();
	return 0;

	CEspFile espFile;
	CStringFile strings;

	//espFile.Load("C:\\Downloads\\Starfield\\Constellation.esm");
	//espFile.OutputStats("Constellation_Stats.txt");

	//espFile.Load("C:\\Downloads\\Starfield\\OldMars.esm");
	//espFile.OutputStats("OldMars_Stats.txt");

	//espFile.Load("C:\\Downloads\\Starfield\\BlueprintShips-Starfield.esm");
	//espFile.OutputStats("BlueprintShips-Starfield_Stats.txt");

	//bool Result = espFile.Load("C:\\Downloads\\Starfield\\Starfield.esm");
	//bool Result = espFile.Load("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Starfield\\Data\\SFBGS003.esm"); 
	bool Result = espFile.Load("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Starfield\\Data\\Starfield.esm"); 

	if (!Result) { printf("\tFailed to load ESM file!\n");  return -1; }

	//espFile.OutputStats("Starfield_Stats.txt");

	DumpReferenceUses(espFile, "RefUses.csv");

	//DumpStructure(espFile, "Structure.txt");

	//DumpBooks(espFile, "Books.txt");
	//DumpBookFiles(espFile, "./Books/");
	//DumpBooksCsv(espFile, "Books.csv");

	//DumpWeapons(espFile, "Weapons.csv");
	//DumpAmmo(espFile, "Ammo.csv");
	//DumpArmors(espFile, "Armors.csv");

	//DumpFurnitures(espFile, "Furniture.csv");
	//DumpMisc(espFile, "Misc.csv");

	//DumpNpcs(espFile, "Npcs.csv");
	//espFile.SaveRaw("Npcs.esm", NAME_NPC_);

	//DumpRaces(espFile, "Races.csv");
	//DumpFactions(espFile, "Factions.csv");

	//DumpClasses(espFile, "Classes.csv");
	//DumpMagicEffects(espFile, "Effects.csv");
	//DumpSettings(espFile, "Settings.csv");
	//DumpEquipSlots(espFile, "EquipSlots.csv");

	//DumpBioms(espFile, "Bioms.csv");
	//DumpSpells(espFile, "Spells.csv");
	//DumpEnchants(espFile, "Enchants.csv");
	//DumpAlchemy(espFile, "Alchemy.csv");
	//DumpAvif(espFile, "ActorValues.csv");
	//DumpWorlds(espFile, "Worlds.csv");
	//DumpLocations(espFile, "Locations.csv");
	//DumpSunPresets(espFile, "SunPresets.csv");

	//DumpStars(espFile, "Stars.csv");
	//DumpPlanets(espFile, "Planets.csv");
	//espFile.SaveRaw("Planets.esm", NAME_PNDT);

	//DumpQuests(espFile, "Quests3.csv");
	//DumpQuestStages(espFile, "QuestStages3.csv");
	//DumpQuestObjectives(espFile, "QuestObjectives3.csv");
	//DumpQuestScripts(espFile, "QuestScripts3.csv");


	//DumpFlora(espFile, "Flora.csv");

	//DumpPerks(espFile, "Perks.csv");
	//DumpLoadingScreens(espFile, "LoadingScreens.csv");

	//DumpCells(espFile, "Cells.csv");
	//DumpGbfm(espFile, "Gbfm.csv");
	//DumpFlst(espFile, "Flst.csv");

	//DumpDialogue(espFile, "Dialogue.csv");

	//DumpTerm(espFile, "Term.csv");
	//DumpTmlm(espFile, "Tmlm.csv");

	//DumpIres(espFile, "Ires.csv");
	//DumpCobj(espFile, "Cobj.csv");

	//DumpOmod(espFile, "Omod.csv");
	//DumpOmodProperties(espFile, "OmodProperties.csv");	

	//DumpCreatures(espFile, "Fauna.csv");

	//DumpLvli(espFile, "Lvli.csv");
	//DumpLvliEntries(espFile, "LvliEntries.csv");
	//DumpLvliWeapons(espFile, "LvliWeapons.csv");

	//auto s = CreateStringFilename("C:\\Downloads\\Starfield\\Starfield.esm", "ilstrings");
	//printf(s.c_str());
	//if (FileExists(s)) printf("\nFile Exists\n");

	/*
	strings.Load("c:\\Temp\\strings\\starfield_en.strings");
	strings.DumpText("starfield_strings.txt");
	strings.Load("c:\\Temp\\strings\\starfield_en.dlstrings");
	strings.DumpText("starfield_dlstrings.txt");
	strings.Load("c:\\Temp\\strings\\starfield_en.ilstrings");
	strings.DumpText("starfield_ilstrings.txt");

	strings.Load("c:\\Temp\\strings\\blueprintships-starfield_en.strings");
	strings.DumpText("blueprintships-starfield_strings.txt");
	strings.Load("c:\\Temp\\strings\\blueprintships-starfield_en.dlstrings");
	strings.DumpText("blueprintships-starfield_dlstrings.txt");
	strings.Load("c:\\Temp\\strings\\blueprintships-starfield_en.ilstrings");
	strings.DumpText("blueprintships-starfield_ilstrings.txt");

	strings.Load("c:\\Temp\\strings\\constellation_en.strings");
	strings.DumpText("constellation_strings.txt");
	strings.Load("c:\\Temp\\strings\\constellation_en.dlstrings");
	strings.DumpText("constellation_dlstrings.txt");
	strings.Load("c:\\Temp\\strings\\constellation_en.ilstrings");
	strings.DumpText("constellation_ilstrings.txt");
	*/
  
}
