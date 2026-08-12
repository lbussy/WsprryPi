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

static struct rp1_gpclk_event_program event_program(unsigned long long generation)
{
	struct rp1_gpclk_event_program r = { .version=2,.size=sizeof(r),
		.fractional_bits=16,.tick_divider=511,.tone_count=2,.event_count=3,
		.generation=generation,.total_duration_ns=60 };
	r.tones[0]=(struct rp1_gpclk_symbol){.lower_divider_word=232445,
		.upper_divider_word=232446,.lower_count=12,.upper_count=13};
	r.tones[1]=(struct rp1_gpclk_symbol){.lower_divider_word=232444,
		.upper_divider_word=232445,.lower_count=11,.upper_count=14};
	r.events[0]=(struct rp1_gpclk_event){.duration_ns=10,.tone_index=0,
		.flags=RP1_GPCLK_EVENT_RF_ON};
	r.events[1]=(struct rp1_gpclk_event){.duration_ns=20,.tone_index=1,
		.flags=RP1_GPCLK_EVENT_RF_ON};
	r.events[2]=(struct rp1_gpclk_event){.duration_ns=30};
	return r;
}

struct adapter_state { unsigned applied; unsigned closed; unsigned fail_at; };
static int apply_event(void *context, const struct rp1_gpclk_event *event,
	unsigned index)
{
	struct adapter_state *state = context;
	(void)event;
	state->applied++;
	return state->fail_at == index + 1;
}
static void close_event(void *context)
{
	((struct adapter_state *)context)->closed++;
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
	{ struct rp1_gpclk_acquire a=acquire_request(2); struct rp1_gpclk_program p=program(1); EXPECT(!rp1_gpclk_core_acquire(&core,&a),"second owner acquire"); EXPECT(core.generation==0,"new lease resets generation"); EXPECT(!rp1_gpclk_core_submit(&core,&p),"second owner initial generation submit"); EXPECT(rp1_gpclk_core_submit(&core,&p)==-EBUSY,"same lease rejects submit while running"); EXPECT(!rp1_gpclk_core_complete(&core,1,0),"second owner completion"); EXPECT(rp1_gpclk_core_submit(&core,&p)==-EINVAL,"same lease rejects stale generation"); EXPECT(!rp1_gpclk_core_release(&core),"second owner release"); }
	{
		struct rp1_gpclk_acquire a=acquire_request(2);
		struct rp1_gpclk_event_program p=event_program(1), bad;
		struct adapter_state state={0};
		struct rp1_gpclk_event_adapter adapter={apply_event,close_event,&state};
		EXPECT(!rp1_gpclk_core_acquire(&core,&a),"event owner acquire");
		EXPECT(rp1_gpclk_core_valid_event_program(&p,0),"valid event program");
		bad=p; bad.version=1; EXPECT(!rp1_gpclk_core_valid_event_program(&bad,0),"event version rejection");
		bad=p; bad.size--; EXPECT(!rp1_gpclk_core_valid_event_program(&bad,0),"event size rejection");
		bad=p; bad.flags=1; EXPECT(!rp1_gpclk_core_valid_event_program(&bad,0),"program flags rejection");
		bad=p; bad.reserved=1; EXPECT(!rp1_gpclk_core_valid_event_program(&bad,0),"program reserved rejection");
		bad=p; bad.event_count=0; EXPECT(!rp1_gpclk_core_valid_event_program(&bad,0),"zero event rejection");
		bad=p; bad.event_count=RP1_GPCLK_EVENT_MAX_EVENTS+1; EXPECT(!rp1_gpclk_core_valid_event_program(&bad,0),"event bound rejection");
		bad=p; bad.tone_count=RP1_GPCLK_EVENT_MAX_TONES+1; EXPECT(!rp1_gpclk_core_valid_event_program(&bad,0),"tone bound rejection");
		bad=p; bad.events[0].duration_ns=0; EXPECT(!rp1_gpclk_core_valid_event_program(&bad,0),"zero duration rejection");
		bad=p; bad.events[0].reserved=1; EXPECT(!rp1_gpclk_core_valid_event_program(&bad,0),"event reserved rejection");
		bad=p; bad.events[0].flags=2; EXPECT(!rp1_gpclk_core_valid_event_program(&bad,0),"event flags rejection");
		bad=p; bad.events[0].tone_index=2; EXPECT(!rp1_gpclk_core_valid_event_program(&bad,0),"tone index rejection");
		bad=p; bad.total_duration_ns--; EXPECT(!rp1_gpclk_core_valid_event_program(&bad,0),"total duration rejection");
		bad=p; bad.events[0].duration_ns=~0ULL; EXPECT(!rp1_gpclk_core_valid_event_program(&bad,0),"duration overflow rejection");
		EXPECT(!rp1_gpclk_core_submit_events(&core,&p),"event submit");
		EXPECT(!rp1_gpclk_core_start_events(&core,&adapter,100),"event start");
		EXPECT(core.current_event==0 && state.applied==1,"first event applied");
		EXPECT(rp1_gpclk_core_advance_events(&core,&adapter,0,110)==-ESTALE,"stale event callback");
		EXPECT(!rp1_gpclk_core_advance_events(&core,&adapter,1,109),"early callback ignored");
		EXPECT(!rp1_gpclk_core_advance_events(&core,&adapter,1,110),"absolute first deadline");
		EXPECT(core.current_event==1 && state.applied==2,"second event applied");
		EXPECT(!rp1_gpclk_core_advance_events(&core,&adapter,1,130),"absolute second deadline");
		EXPECT(core.current_event==2 && state.applied==3,"RF-off event applied");
		EXPECT(!rp1_gpclk_core_advance_events(&core,&adapter,1,160),"event completion");
		EXPECT(core.state==RP1_GPCLK_STATE_COMPLETE && core.terminal_reason==RP1_GPCLK_TERMINAL_COMPLETE,"completion reason");
		EXPECT(state.closed==1,"completion fails closed once");
		EXPECT(rp1_gpclk_core_stop_events(&core,&adapter,1,RP1_GPCLK_TERMINAL_STOPPED)==-EALREADY,"stop after complete");
		EXPECT(state.closed==1,"repeated cleanup is idempotent");
		EXPECT(!rp1_gpclk_core_release(&core),"event owner release");
	}
	{
		struct rp1_gpclk_acquire a=acquire_request(2);
		struct rp1_gpclk_event_program p=event_program(1);
		struct adapter_state state={0};
		struct rp1_gpclk_event_adapter adapter={apply_event,close_event,&state};
		EXPECT(!rp1_gpclk_core_acquire(&core,&a),"prestart owner acquire");
		EXPECT(!rp1_gpclk_core_submit_events(&core,&p),"prestart submit");
		EXPECT(!rp1_gpclk_core_stop_events(&core,&adapter,1,RP1_GPCLK_TERMINAL_STOPPED),"stop before start");
		EXPECT(core.terminal_reason==RP1_GPCLK_TERMINAL_STOPPED && state.applied==0 && state.closed==1,"prestart stop fails closed without transition");
		EXPECT(!rp1_gpclk_core_release(&core),"prestart owner release");
	}
	{
		struct rp1_gpclk_acquire a=acquire_request(2);
		struct rp1_gpclk_event_program p=event_program(1);
		struct adapter_state state={0};
		struct rp1_gpclk_event_adapter adapter={apply_event,close_event,&state};
		EXPECT(!rp1_gpclk_core_acquire(&core,&a),"deadline owner acquire");
		EXPECT(!rp1_gpclk_core_submit_events(&core,&p),"deadline submit");
		EXPECT(!rp1_gpclk_core_start_events(&core,&adapter,100),"deadline start");
		EXPECT(rp1_gpclk_core_advance_events(&core,&adapter,1,111)==-EIO,"missed deadline fails");
		EXPECT(core.terminal_reason==RP1_GPCLK_TERMINAL_DEADLINE_MISSED && state.closed==1,"deadline fail closed");
		EXPECT(!rp1_gpclk_core_release(&core),"deadline owner release");
	}
	{
		struct rp1_gpclk_acquire a=acquire_request(2);
		struct rp1_gpclk_event_program p=event_program(1);
		struct adapter_state state={0};
		struct rp1_gpclk_event_adapter adapter={apply_event,close_event,&state};
		EXPECT(!rp1_gpclk_core_acquire(&core,&a),"stop owner acquire");
		EXPECT(!rp1_gpclk_core_submit_events(&core,&p),"stop submit");
		EXPECT(!rp1_gpclk_core_start_events(&core,&adapter,100),"stop start");
		EXPECT(!rp1_gpclk_core_stop_events(&core,&adapter,1,RP1_GPCLK_TERMINAL_OWNER_CLOSED),"synchronous stop");
		EXPECT(core.state==RP1_GPCLK_STATE_COMPLETE && core.terminal_reason==RP1_GPCLK_TERMINAL_OWNER_CLOSED && state.closed==1,"owner close terminal state");
		EXPECT(rp1_gpclk_core_advance_events(&core,&adapter,1,110)==-EALREADY,"stale terminal callback cannot reactivate");
		EXPECT(!rp1_gpclk_core_release(&core),"stop owner release");
	}
	{
		struct rp1_gpclk_acquire a=acquire_request(2);
		struct rp1_gpclk_event_program p=event_program(1);
		struct adapter_state state={.fail_at=1};
		struct rp1_gpclk_event_adapter adapter={apply_event,close_event,&state};
		EXPECT(!rp1_gpclk_core_acquire(&core,&a),"adapter owner acquire");
		EXPECT(!rp1_gpclk_core_submit_events(&core,&p),"adapter submit");
		EXPECT(rp1_gpclk_core_start_events(&core,&adapter,100)==-EIO,"adapter failure");
		EXPECT(core.terminal_reason==RP1_GPCLK_TERMINAL_ADAPTER_FAILED && state.closed==1,"adapter failure closes");
		EXPECT(!rp1_gpclk_core_release(&core),"adapter owner release");
	}
	{
		struct rp1_gpclk_acquire a=acquire_request(2);
		struct rp1_gpclk_event_program p=event_program(1);
		struct adapter_state state={.fail_at=2};
		struct rp1_gpclk_event_adapter adapter={apply_event,close_event,&state};
		EXPECT(!rp1_gpclk_core_acquire(&core,&a),"transition failure owner acquire");
		EXPECT(!rp1_gpclk_core_submit_events(&core,&p),"transition failure submit");
		EXPECT(!rp1_gpclk_core_start_events(&core,&adapter,100),"transition failure start");
		EXPECT(rp1_gpclk_core_advance_events(&core,&adapter,1,110)==-EIO,"later adapter failure");
		EXPECT(core.terminal_reason==RP1_GPCLK_TERMINAL_ADAPTER_FAILED && state.closed==1,"later adapter failure closes");
		EXPECT(!rp1_gpclk_core_release(&core),"transition failure owner release");
	}
	if (failures)
		return 1;
	puts("RP1 GPCLK provider core tests passed");
	return 0;
}
