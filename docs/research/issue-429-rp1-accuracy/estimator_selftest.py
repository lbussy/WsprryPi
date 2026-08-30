"""Synthetic sanity check for the offline bracket estimator (no hardware)."""
import json
import sys
import subprocess
import tempfile
from pathlib import Path
import numpy as np

root=Path(tempfile.mkdtemp(prefix='issue429-estimator-selftest-'))
fs=250000
x=np.memmap(root/'capture.cf32',dtype='<c8',mode='w+',shape=(11000000,))
rng=np.random.default_rng(429)
for i in range(0,len(x),250000):
    n=min(250000,len(x)-i)
    x[i:i+n]=.0002*(rng.normal(size=n)+1j*rng.normal(size=n))
for a,b,offset in [(2,10,15.),(13,15,-654.),(18,26,15.1)]:
    n=np.arange((b-a)*fs)
    x[a*fs:b*fs]+=.05*np.exp(2j*np.pi*(25000+offset)*n/fs)
x.flush()
(root/'metadata.json').write_text(json.dumps({'primary_outcome':'success','retained_sample_count':11000000,
 'requested_sample_count':11000000,'overflow_count':0,'timeout_count':0,'clipping':{'sample_count':0},
 'cleanup':{'outcome':'verified'},'actual_settings':{'sample_rate_hz':fs,'center_frequency_hz':14072100}}))
result=json.loads(subprocess.check_output([sys.executable,str(Path(__file__).with_name('estimate.py')),str(root),'14097100'],text=True))
# Reference midpoints are 6 s and 22 s; RP1 midpoint is 14 s.
assert abs(result['bracketed_error_hz']-(-669.05))<.002,result
print(json.dumps({'synthetic_test':'passed','expected_hz':-669.05,'measured_hz':result['bracketed_error_hz'],'artifact':str(root)}))
n=np.arange(2*fs)
x[32*fs:34*fs]+=.03*np.exp(2j*np.pi*30000*n/fs)
x.flush()
extra=json.loads(subprocess.check_output([sys.executable,str(Path(__file__).with_name('estimate.py')),str(root),'14097100'],text=True))
assert len(extra['all_detected_intervals_seconds'])==4
assert abs(extra['bracketed_error_hz']-(-669.05))<.002,extra
print('Extra post-bracket burst isolation passed')
for a,b,offset in [(10,12,15.),(26,28,15.1)]:
    n=np.arange((b-a)*fs)+8*fs
    # Preserve phase continuity with the existing reference burst.
    x[a*fs:b*fs]+=.05*np.exp(2j*np.pi*(25000+offset)*n/fs)
x.flush()
(root/'reference-bounds.json').write_text(json.dumps({label:{'min_active_seconds':9.9,
 'max_active_seconds':10.3} for label in ('before','after')}))
extended=json.loads(subprocess.check_output([sys.executable,str(Path(__file__).with_name('estimate.py')),str(root),'14097100'],text=True))
assert abs(extended['bracketed_error_hz']-(-669.04375))<.002,extended
print('Measured reference-command latency bounds passed')
x[12*fs:int(12.6*fs)]+=.02*(rng.normal(size=int(.6*fs))+1j*rng.normal(size=int(.6*fs)))
x.flush()
broadband=json.loads(subprocess.check_output([sys.executable,str(Path(__file__).with_name('estimate.py')),str(root),'14097100'],text=True))
assert abs(broadband['bracketed_error_hz']-(-669.04375))<.002,broadband
assert broadband['intervals_seconds'][0][1]==12,broadband
print('Broadband tail does not extend the reference carrier interval')

# Explicit paced acquisition plan, not relaxed legacy timing constraints.
longroot=Path(tempfile.mkdtemp(prefix='issue429-paced-estimator-selftest-'))
longx=np.memmap(longroot/'capture.cf32',dtype='<c8',mode='w+',shape=(30000000,))
for a,b,offset in [(12,34,15.),(50,52,-654.),(70,92,15.1)]:
    for second in range(a,b):
        n=np.arange(fs)+(second-a)*fs
        longx[second*fs:(second+1)*fs]=.05*np.exp(2j*np.pi*(25000+offset)*n/fs)
longx.flush()
meta=json.loads((root/'metadata.json').read_text())
meta.update(retained_sample_count=30000000,requested_sample_count=30000000)
(longroot/'metadata.json').write_text(json.dumps(meta))
(longroot/'reference-bounds.json').write_text(json.dumps({label:{'min_active_seconds':12,
 'max_active_seconds':38} for label in ('before','after')}))
(longroot/'acquisition-plan.json').write_text(json.dumps({'requested_sample_count':30000000,
 'max_reference_seconds':45,'max_gap_seconds':30}))
paced=json.loads(subprocess.check_output([sys.executable,str(Path(__file__).with_name('estimate.py')),str(longroot),'14097100'],text=True))
expected=-654-(15+(.1*(51-23)/(81-23)))
assert abs(paced['bracketed_error_hz']-expected)<.002,paced
print('Paced 120-second capture, longer references and quiet gaps passed')
