// Common TLM functionality abstracted out so that it'll be the same in all modules which need it
#ifndef TLMCOMMON_H
#define TLMCOMMON_H
namespace TLM
{
	float generateDrivingValue(int iteration, float frequency, int scale );
}
#endif