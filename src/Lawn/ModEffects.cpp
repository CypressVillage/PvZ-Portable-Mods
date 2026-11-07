// SPDX-License-Identifier: LGPL-3.0-or-later
#ifdef PVZ_MOD
#include "ModEffects.h"
#include "graphics/Graphics.h"
#include <algorithm>
namespace Mod
{
void Effects::Lightning(int x, int y, int endX, int row)
{
	mBolts.push_back({x, y, endX, row, 12, ++mSequence});
}
void Effects::Update()
{
	for (auto& bolt : mBolts) --bolt.ticks;
	std::erase_if(mBolts, [](const Bolt& bolt) { return bolt.ticks <= 0; });
}
void Effects::Draw(Sexy::Graphics* graphics, int row) const
{
	Sexy::Graphics g(*graphics);
	for (const auto& bolt : mBolts)
	{
		if (bolt.row != row) continue;
		uint32_t noise = bolt.seed * 747796405U + (bolt.ticks / 2) * 2891336453U;
		int x = bolt.x, y = bolt.y;
		for (int nextX = x + 22; x < bolt.endX; nextX += 22)
		{
			noise = noise * 1664525U + 1013904223U;
			int endX = std::min(nextX, bolt.endX);
			int endY = bolt.y + static_cast<int>((noise >> 16) % 23) - 11;
			int alpha = bolt.ticks * 255 / 12;
			g.SetColor(Sexy::Color(80, 110, 255, alpha / 2));
			for (int offset : {-2, -1, 1, 2}) g.DrawLine(x, y + offset, endX, endY + offset);
			g.SetColor(Sexy::Color(210, 245, 255, alpha));
			g.DrawLineAA(x, y, endX, endY);
			if ((noise & 3) == 0) g.DrawLineAA(endX, endY, endX + 12, endY - 17);
			x = endX;
			y = endY;
		}
	}
}
}
#endif
