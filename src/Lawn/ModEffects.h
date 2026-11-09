// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#ifdef PVZ_MOD
#include <vector>
#include <cstdint>
namespace Sexy { class Graphics; }
namespace Mod
{
class Effects
{
public:
	void Lightning(int x, int y, int endX, int row, int duration);
	void Update();
	void Draw(Sexy::Graphics* graphics, int row) const;
	size_t Count() const { return mBolts.size(); }
private:
	struct Bolt { int x, y, endX, row, ticks, duration; uint32_t seed; };
	std::vector<Bolt> mBolts;
	uint32_t mSequence = 0;
};
}
#endif
