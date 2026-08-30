"""Offline GPSDO-bracketed frequency estimate; no RF or device access."""
import json
import sys
from pathlib import Path
import numpy as np

root=Path(sys.argv[1]); target=float(sys.argv[2]); center=target-25000
meta=json.loads((root/'metadata.json').read_text())
plan_path=root/'acquisition-plan.json'
plan=json.loads(plan_path.read_text()) if plan_path.exists() else {}
expected_samples=plan.get('requested_sample_count',11000000)
assert expected_samples in (11000000,30000000)
max_reference=plan.get('max_reference_seconds',15)
max_gap=plan.get('max_gap_seconds',5)
assert 15<=max_reference<=45 and 5<=max_gap<=30
assert meta['primary_outcome']=='success'
assert meta['retained_sample_count']==meta['requested_sample_count']==expected_samples
assert meta['overflow_count']==meta['timeout_count']==meta['clipping']['sample_count']==0
assert meta['cleanup']['outcome']=='verified'
assert meta['actual_settings']['sample_rate_hz']==250000
assert meta['actual_settings']['center_frequency_hz']==center
x=np.memmap(root/'capture.cf32',dtype='<c8',mode='r')
assert len(x)==expected_samples
fs=250000.; block=25000
detector_window=np.hanning(block)
detector_frequency=np.fft.fftfreq(block,1/fs)
detector_bins=np.flatnonzero(abs(detector_frequency-(target-center))<5000)
envelope=np.array([np.max(abs(np.fft.fft(x[i:i+block]*detector_window)[detector_bins]))/detector_window.sum()
                   for i in range(0,len(x),block)])
baseline=float(np.percentile(envelope,20))
active=envelope>max(.004,baseline*3)
runs=[]; start=None
for i,on in enumerate(np.r_[active,False]):
    if on and start is None: start=i
    if not on and start is not None:
        if (i-start)*block/fs>=1: runs.append((start*block,i*block))
        start=None
all_runs=list(runs)
bound_path=root/'reference-bounds.json'
bounds=json.loads(bound_path.read_text()) if bound_path.exists() else None
def reference_duration_matches(duration,label):
    if bounds is None:
        return 7.5<=duration<=9
    lower=bounds[label]['min_active_seconds']
    upper=bounds[label]['max_active_seconds']
    assert 7.5<=lower<=upper<=max_reference,bounds
    return lower-.3<=duration<=upper+.3
candidates=[]
for i in range(len(runs)-2):
    triplet=runs[i:i+3]
    durations=[(b-a)/fs for a,b in triplet]
    gaps=[(triplet[j+1][0]-triplet[j][1])/fs for j in (0,1)]
    if reference_duration_matches(durations[0],'before') and 1.5<=durations[1]<=2.5 and reference_duration_matches(durations[2],'after') and all(.5<=gap<=max_gap for gap in gaps):
        candidates.append(triplet)
assert len(candidates)==1,{'intervals':runs,'pattern_candidates':len(candidates),'baseline':baseline}
runs=candidates[0]

def estimate(a,b):
    trim=int(min(1.,(b-a)/fs/8)*fs)
    a+=trim; b-=trim
    y=np.asarray(x[a:b]); n=np.arange(len(y))
    shifted=y*np.exp(-2j*np.pi*(target-center)*n/fs)
    nfft=1 << (len(y)-1).bit_length()
    power=abs(np.fft.fft(shifted*np.hanning(len(y)),nfft))**2
    freq=np.fft.fftfreq(nfft,1/fs)
    idx=np.flatnonzero(abs(freq)<5000)
    k=idx[np.argmax(power[idx])]
    q=np.log(power[[(k-1)%nfft,k,(k+1)%nfft]]+1e-300)
    delta=.5*(q[0]-q[2])/(q[0]-2*q[1]+q[2])
    coarse=freq[k]+delta*fs/nfft
    mixed=shifted*np.exp(-2j*np.pi*coarse*n/fs)
    z=mixed[:len(mixed)//1024*1024].reshape(-1,1024).mean(1)
    t=(np.arange(len(z))+.5)*1024/fs
    phase=np.unwrap(np.angle(z)); weights=abs(z)
    design=np.c_[t,np.ones(len(t))]
    fit=np.linalg.lstsq(design*weights[:,None],phase*weights,rcond=None)[0]
    residual=float(coarse+fit[0]/(2*np.pi))
    return {'mid_s':(a+b)/2/fs,'used_seconds':(b-a)/fs,'residual_hz':residual,
            'fft_residual_hz':float(coarse),'phase_fit_residual_rms_rad':float(np.sqrt(np.mean((phase-design@fit)**2)))}

before,rp1,after=[estimate(a,b) for a,b in runs]
alpha=(rp1['mid_s']-before['mid_s'])/(after['mid_s']-before['mid_s'])
assert 0<alpha<1
reference=before['residual_hz']+alpha*(after['residual_hz']-before['residual_hz'])
error=rp1['residual_hz']-reference
print(json.dumps({'target_hz':target,'gpio':20,'manual_ppm':0,'intervals_seconds':[[a/fs,b/fs] for a,b in runs],
 'all_detected_intervals_seconds':[[a/fs,b/fs] for a,b in all_runs],
 'selection':'unique reference/tone/reference duration and gap pattern with measured command-latency bounds when present',
 'reference_command_bounds':bounds,
 'acquisition_plan':plan,
 'detector':'100 ms Hann-window carrier peak within +/-5 kHz, threshold max(0.004, 3x baseline)',
 'gps_before':before,'rp1':rp1,'gps_after':after,'reference_drift_hz':after['residual_hz']-before['residual_hz'],
 'bracketed_error_hz':error,'bracketed_error_ppm':error/target*1e6,
 'scope':'single bracket clock accuracy diagnostic, not a calibration constant or spectral qualification'},indent=2))
