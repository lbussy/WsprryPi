#include "rp1_gpclk_provider_core.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

static int failures;
#define EXPECT(c, m) do { if (!(c)) { fprintf(stderr, "FAIL: %s\n", m); failures++; } } while (0)

static struct rp1_gpclk_acquire acquire_request(unsigned drive)
{
	struct rp1_gpclk_acquire r = { .version = 1, .size = sizeof(r), .drive_ma = drive };
	return r;
}
static struct rp1_gpclk_program program(unsigned tone, unsigned long long generation)
{
	static const unsigned lower[] = {232445,232444,232444,232444};
	static const unsigned upper[] = {232446,232445,232445,232445};
	static const unsigned counts[] = {66312,1134,2747,4360};
	struct rp1_gpclk_program r = { .version=1,.size=sizeof(r),.fractional_bits=16,
		.lower_divider_word=lower[tone],.upper_divider_word=upper[tone],
		.lower_count=counts[tone],.upper_count=66792-counts[tone],
		.writes_per_symbol=66792,.tick_divider=511,.generation=generation };
	return r;
}
int main(void)
{
	unsigned drive, tone; struct rp1_gpclk_provider_core core = {0};
	for (drive=2; drive<=12; drive += drive==2?2:4) { struct rp1_gpclk_acquire a=acquire_request(drive); EXPECT(!rp1_gpclk_core_acquire(&core,&a),"drive acquire"); EXPECT(!rp1_gpclk_core_release(&core),"idle release"); }
	for (drive=0; drive<=16; drive+=2) if (drive!=2&&drive!=4&&drive!=8&&drive!=12) { struct rp1_gpclk_acquire a=acquire_request(drive); EXPECT(rp1_gpclk_core_acquire(&core,&a)==-EINVAL,"invalid drive rejection"); }
	{ struct rp1_gpclk_acquire a=acquire_request(2); a.version=2; EXPECT(rp1_gpclk_core_acquire(&core,&a)==-EPROTO,"version rejection"); a.version=1;a.size--;EXPECT(rp1_gpclk_core_acquire(&core,&a)==-EPROTO,"size rejection"); }
	{ struct rp1_gpclk_acquire a=acquire_request(2); EXPECT(!rp1_gpclk_core_acquire(&core,&a),"acquire"); EXPECT(rp1_gpclk_core_acquire(&core,&a)==-EBUSY,"exclusive acquire"); }
	for (tone=0;tone<4;tone++) { struct rp1_gpclk_program p=program(tone,tone+1); EXPECT(!rp1_gpclk_core_submit(&core,&p),"tone submit"); EXPECT(core.lower_div_frac==((p.lower_divider_word&0xffff)<<16),"provider lower packing"); EXPECT(core.upper_div_frac==((p.upper_divider_word&0xffff)<<16),"provider upper packing"); EXPECT(rp1_gpclk_core_stop(&core,tone)==-ESTALE,"stale stop"); EXPECT(!rp1_gpclk_core_stop(&core,tone+1),"finite stop"); EXPECT(rp1_gpclk_core_release(&core)==-EBUSY,"no release while draining"); EXPECT(!rp1_gpclk_core_complete(&core,tone+1,0),"completion"); }
	EXPECT(!rp1_gpclk_core_release(&core),"terminal release");
	if (failures)
		return 1;
	puts("RP1 GPCLK provider core tests passed");
	return 0;
}
