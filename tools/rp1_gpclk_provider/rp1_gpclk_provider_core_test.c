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
static struct rp1_gpclk_program program(unsigned long long generation)
{
	static const unsigned lower[] = {232445,232444,232444,232444};
	static const unsigned upper[] = {232446,232445,232445,232445};
	static const unsigned counts[] = {66312,1134,2747,4360};
	struct rp1_gpclk_program r = { .version=1,.size=sizeof(r),.fractional_bits=16,
		.writes_per_symbol=66792,.tick_divider=511,.symbol_count=162,.tone_count=4,
		.generation=generation };
	unsigned i;
	for (i=0;i<4;i++) r.tones[i]=(struct rp1_gpclk_symbol){
		.lower_divider_word=lower[i],.upper_divider_word=upper[i],
		.lower_count=counts[i],.upper_count=66792-counts[i]};
	for (i=0;i<162;i++) r.symbols[i]=i%4;
	return r;
}
int main(void)
{
	unsigned drive; struct rp1_gpclk_provider_core core = {0};
	EXPECT(rp1_gpclk_core_valid_frame_elapsed(110592000000ULL), "nominal cadence");
	EXPECT(rp1_gpclk_core_valid_frame_elapsed(110585250000ULL), "lower cadence bound");
	EXPECT(rp1_gpclk_core_valid_frame_elapsed(110598750000ULL), "upper cadence bound");
	EXPECT(!rp1_gpclk_core_valid_frame_elapsed(110585249999ULL), "below cadence bound");
	EXPECT(!rp1_gpclk_core_valid_frame_elapsed(110598750001ULL), "above cadence bound");
	for (drive=2; drive<=12; drive += drive==2?2:4) { struct rp1_gpclk_acquire a=acquire_request(drive); EXPECT(!rp1_gpclk_core_acquire(&core,&a),"drive acquire"); EXPECT(!rp1_gpclk_core_release(&core),"idle release"); }
	for (drive=0; drive<=16; drive+=2) if (drive!=2&&drive!=4&&drive!=8&&drive!=12) { struct rp1_gpclk_acquire a=acquire_request(drive); EXPECT(rp1_gpclk_core_acquire(&core,&a)==-EINVAL,"invalid drive rejection"); }
	{ struct rp1_gpclk_acquire a=acquire_request(2); a.version=2; EXPECT(rp1_gpclk_core_acquire(&core,&a)==-EPROTO,"version rejection"); a.version=1;a.size--;EXPECT(rp1_gpclk_core_acquire(&core,&a)==-EPROTO,"size rejection"); }
	{ struct rp1_gpclk_acquire a=acquire_request(2); EXPECT(!rp1_gpclk_core_acquire(&core,&a),"acquire"); EXPECT(rp1_gpclk_core_acquire(&core,&a)==-EBUSY,"exclusive acquire"); }
	{ struct rp1_gpclk_program p=program(1); EXPECT(!rp1_gpclk_core_submit(&core,&p),"162-symbol submit"); EXPECT(core.symbol_count==162,"exact symbol count"); EXPECT(core.lower_div_frac==((p.tones[p.symbols[0]].lower_divider_word&0xffff)<<16),"provider first packing"); EXPECT(core.upper_div_frac==((p.tones[p.symbols[161]].upper_divider_word&0xffff)<<16),"provider last packing"); EXPECT(rp1_gpclk_core_stop(&core,0)==-ESTALE,"stale stop"); EXPECT(!rp1_gpclk_core_stop(&core,1),"finite stop"); EXPECT(rp1_gpclk_core_release(&core)==-EBUSY,"no release while draining"); EXPECT(!rp1_gpclk_core_complete(&core,1,0),"completion"); }
	{ struct rp1_gpclk_program p=program(2); p.symbol_count=161; EXPECT(rp1_gpclk_core_submit(&core,&p)==-EINVAL,"short sequence rejection"); p.symbol_count=162; p.tones[2].upper_count--; EXPECT(rp1_gpclk_core_submit(&core,&p)==-EINVAL,"malformed tone rejection"); p=program(2); p.symbols[81]=4; EXPECT(rp1_gpclk_core_submit(&core,&p)==-EINVAL,"invalid symbol index rejection"); }
	EXPECT(!rp1_gpclk_core_release(&core),"terminal release");
	if (failures)
		return 1;
	puts("RP1 GPCLK provider core tests passed");
	return 0;
}
