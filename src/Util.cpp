#include "Util.h"

namespace MHK
{
	UInt32 GetGamepadIconIndex(UInt32 a_scanCode)
	{
		switch (a_scanCode)
		{
		case 0x1:
			//up
			return 265 + 1;
		case 0x2:
			//down
			return 265 + 2;
		case 0x4:
			//left
			return 265 + 3;
		case 0x8:
			//right
			return 265 + 4;
		case 0x10:
			//start
			return 265 + 5;
		case 0x20:
			//back
			return 265 + 6;
		case 0x40:
			//L3
			return 265 + 7;
		case 0x80:
			//R3
			return 265 + 8;
		case 0x100:
			//LB
			return 265 + 9;
		case 0x200:
			//RB
			return 265 + 10;
		case 0x1000:
			//A
			return 265 + 11;
		case 0x2000:
			//B
			return 265 + 12;
		case 0x4000:
			//X
			return 265 + 13;
		case 0x8000:
			//Y
			return 265 + 14;
		case 0x9:
			//LT
			return 265 + 15;
		case 0xA:
			//RT
			return 265 + 16;
		default:
			//???
			return 265 + 17;
		}
	}

	bool IsPointInPlane(RE::NiPoint2 a_point, RE::NiPoint2 a_planePos, RE::NiPoint2 a_planeEnd)
	{
		_ASSERT(a_planePos.x > a_planeEnd.x);
		_ASSERT(a_planePos.y > a_planeEnd.y);

		if (!(a_planePos.x < a_point.x && a_point.x < a_planeEnd.x))
		{
			return false;
		}
		if (!(a_planePos.y < a_point.y && a_point.y < a_planeEnd.y))
		{
			return false;
		}
		return true;
	}
}