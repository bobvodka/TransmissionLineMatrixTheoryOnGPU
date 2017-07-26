// Common TLM Functions
#define _USE_MATH_DEFINES
#include <math.h>

namespace TLM
{
	float generateDrivingValue(int iteration, float frequency, int scale )
	{
		return static_cast<float>(scale * sin((2 * M_PI) * (iteration / 19.798989) * frequency));
	}
}