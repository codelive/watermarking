#pragma once
#include "WatermarkData.h"

/**
 * @brief The SimpleWatermarkData class
 *
 * Ecrit les données une unique fois.
 */
class SimpleWatermarkData : public WatermarkData
{
	public:
		virtual bool nextBit() final override
		{
			return bits[_position++];
		}

		virtual bool isComplete() final override
		{
			return _position >= _size;
		}
};
