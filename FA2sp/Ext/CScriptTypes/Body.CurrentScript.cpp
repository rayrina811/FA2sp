#include "Body.h"
#include "Functional.h"

#include <CINI.h>
#include <CFinalSunDlg.h>

#include "../../Helpers/Translations.h"

CurrentScript* CScriptTypesExt::ExtCurrentScript;
ppmfc::CString CurrentScript::ToString()
{
	return this->ID + " (" + this->Name + ")";
}

ppmfc::CString CurrentScript::GetActionString(int index)
{
	if (!this->Actions[index].IsEmpty())
	{
		auto& action = this->Actions[index];
		ppmfc::CString ret;
		ret.Format("%d,%d", action.Type, action.Param);
		return ret;
	}
	return "";
}

int CurrentScript::AddAction(ScriptNode& node)
{
	if (this->Count < 50)
	{
		this->Actions[Count++] = node;
		return this->Count;
	}
	return -1;
}

int CurrentScript::AddAction(int type, int param)
{
	ScriptNode buffer;
	buffer.Type = type;
	buffer.Param = param;
	return this->AddAction(buffer);
}

int CurrentScript::AddAction(int type, short param, short ext)
{
	ScriptNode buffer;
	buffer.Type = type;
	buffer.ParamNormal = param;
	buffer.ParamExt = ext;
	return this->AddAction(buffer);
}

bool CurrentScript::AddActionAt(ScriptNode& node, int index)
{
	if (this->Count < 50 && index < 50)
	{
		for (int i = Count; i > index; --i)
			this->Actions[i] = this->Actions[i - 1];

		this->Actions[index] = node;
		++this->Count;
		return true;
	}
	return false;
}

bool CurrentScript::AddActionAt(int type, int param, int index)
{
	ScriptNode buffer;
	buffer.Type = type;
	buffer.Param = param;
	return this->AddActionAt(buffer, index);
}

bool CurrentScript::AddActionAt(int type, short param, short ext, int index)
{
	ScriptNode buffer;
	buffer.Type = type;
	buffer.ParamNormal = param;
	buffer.ParamExt = ext;
	return this->AddActionAt(buffer, index);
}

int CurrentScript::GetActionCount()
{
	return this->Count;
}

CurrentScript::ScriptNode& CurrentScript::RemoveActionAt(int index)
{
	ScriptNode buffer = this->Actions[index];
	for (int i = index; i < this->Count - 1; ++i)
		this->Actions[i] = this->Actions[i + 1];
	this->Actions[this->Count].MakeEmpty();
	--this->Count;
	return buffer;
}

void CurrentScript::Set(ppmfc::CString id)
{
	auto& ini = CINI::CurrentDocument();

	if (auto const pSection = ini.GetSection(id))
	{
		ppmfc::CString key;
		int count = 0;
		for (int i = 0; i < 50; ++i)
		{
			key.Format("%d", i);
			auto itr = pSection->GetEntities().find(key);
			if (itr != pSection->GetEntities().end())
			{
				key = itr->second;
				if (sscanf_s(key, "%d,%d", &this->Actions[count].Type, &this->Actions[count].Param) == 2)
					++count;
			}
		}
		this->Count = count;
		this->Name = ini.GetString(id, "Name", "No name");
		this->ID = id;
	}
}

void CurrentScript::Unset()
{
	this->Count = -1;
	this->Name = "";
	this->ID = "";
}

bool CurrentScript::IsAvailable()
{
	return this->Count != -1;
}

void CurrentScript::Write(ppmfc::CString id, ppmfc::CString name)
{
	auto& ini = CINI::CurrentDocument();
	
	ini.DeleteSection(id);
	ini.WriteString(id, "Name", name);
	ppmfc::CString buffer;
	for (int i = 0; i < this->Count; ++i)
	{
		buffer.Format("%d", i);
		ini.WriteString(id, buffer, this->GetActionString(i));
	}
}

void CurrentScript::Write()
{
	this->Write(this->ID, this->Name);
}

void CurrentScript::WriteLine(ppmfc::CString id, int line)
{
	auto& ini = CINI::CurrentDocument();

	ppmfc::CString buffer;
	buffer.Format("%d", line);
	ini.WriteString(id, buffer, this->GetActionString(line));
}

void CurrentScript::WriteLine(int line)
{
	this->WriteLine(this->ID, line);
}

bool CurrentScript::IsExtraParamEnabled(int actionIndex)
{
	if (actionIndex == 46 || actionIndex == 47 || actionIndex == 56 || actionIndex == 58)
		return true;
	auto itr = CScriptTypeAction::ExtActions.find(actionIndex);
	if (itr == CScriptTypeAction::ExtActions.end())
		return false;
	const auto& param = CScriptTypeParam::ExtParams[itr->second.ParamCode_].Param_;
	if (param < 0)
		return CScriptTypeParamCustom::ExtParamsCustom[-param].HasExtraParam_;
	return false;
}

bool CurrentScript::IsExtraParamEnabledAtLine(int line)
{
	return this->IsExtraParamEnabled(this->Actions[line].Type);
}

void CurrentScript::LoadExtraParamBox(ppmfc::CComboBox& comboBox, int actionIndex)
{
	comboBox.DeleteAllStrings();

	auto itr = CScriptTypeAction::ExtActions.find(actionIndex);
	if (itr == CScriptTypeAction::ExtActions.end())
		return;
	bool hasExtraParam = false;
	const auto& param = CScriptTypeParam::ExtParams[itr->second.ParamCode_].Param_;
	if (actionIndex == 46 || actionIndex == 47 || actionIndex == 56 || actionIndex == 58)
		hasExtraParam = true;
	if (param < 0 && CScriptTypeParamCustom::ExtParamsCustom[-param].HasExtraParam_ > 0)
		hasExtraParam = true;

	if (hasExtraParam)
	{
		if (param < 0 && CScriptTypeParamCustom::ExtParamsCustom[-param].HasExtraParam_ > 1)
		{

			MultimapHelper mmh;
			switch (CScriptTypeParamCustom::ExtParamsCustom[-param].LoadFrom2_)
			{
			default:
			case 0:
				mmh.AddINI(&CINI::FAData());
				break;
			case 1:
				mmh.AddINI(&CINI::Rules());
				break;
			case 2:
				mmh.AddINI(&CINI::Rules());
				mmh.AddINI(&CINI::CurrentDocument());
				break;
			case 3:
				mmh.AddINI(&CINI::CurrentDocument());
				break;
			}

			auto&& entries = mmh.ParseIndicies(CScriptTypeParamCustom::ExtParamsCustom[-param].XtraSection_, true);
			if (entries.size() > 0)
			{
				CString buffer;
				for (size_t i = 0, sz = entries.size(); i < sz; ++i)
				{

					buffer.Format("%u - ", i);
					buffer += entries[i];
					comboBox.SetItemData(comboBox.AddString(buffer), i);
				}
			}

		}
		else
		{
			comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptExtraParam.Preference.0", "0 - 最小威胁")), 0);
			comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptExtraParam.Preference.1", "1 - 最大威胁")), 1);
			comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptExtraParam.Preference.2", "2 - 最近")), 2);
			comboBox.SetItemData(comboBox.AddString(Translations::TranslateOrDefault("ScriptExtraParam.Preference.3", "3 - 最远")), 3);
		}
	}
}