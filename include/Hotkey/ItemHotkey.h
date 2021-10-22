#pragma once
#include "Hotkey/Hotkey.h"

namespace EHKS
{
	class ItemHotkey : public Hotkey
	{
	public:
		RE::TESForm*	   GetBaseForm();
		RE::ExtraDataList* GetExtraData();

		std::uint8_t extraDataId;
	};
};