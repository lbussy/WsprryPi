#!/usr/bin/env python3
"""Exploratory same-path comparisons; no formal RF uncertainty claim."""
import argparse,json,pathlib,hashlib,datetime
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
p=argparse.ArgumentParser();p.add_argument('directory',type=pathlib.Path);a=p.parse_args();root=a.directory
ident=json.loads((root/'identity.json').read_text());fs=ident['sample_rate_hz'];freq=ident['frequency_hz'];reference=freq+10000;center=ident['center_hz'];results={'identity':ident,'cases':{}}
fig,axes=plt.subplots(4,2,figsize=(14,12))
for row,name in enumerate(['carrier-none','transitions-none','keyed-none','keyed-raised_cosine']):
 d=root/name;m=json.loads((d/'metadata.json').read_text());raw=d/'capture.cf32';assert hashlib.sha256(raw.read_bytes()).hexdigest()==m['output']['sha256'];assert m['overflow_count']==0 and m['clipping']['sample_count']==0
 assert m['actual_settings']['center_frequency_hz']==center and m['actual_settings']['sample_rate_hz']==fs
 ready=json.loads((root/'gpsdo/ready.json').read_text());off=json.loads((root/'gpsdo/disabled.json').read_text())
 start=datetime.datetime.fromisoformat(m['timestamps']['retained_capture_start_utc'].replace('Z','+00:00')).timestamp()
 end=datetime.datetime.fromisoformat(m['timestamps']['retained_capture_complete_utc'].replace('Z','+00:00')).timestamp()
 assert ready['utc_unix']<start<end<off['utc_unix'],'Reference did not span capture'
 for status_file in (root/'gpsdo').glob('status-*.json'):
  status=json.loads(status_file.read_text());assert status['sat_lock'] and status['pll_lock'] and status['f1']==reference
 x=np.fromfile(raw,dtype='<c8');assert len(x)==m['retained_sample_count'];n=np.arange(len(x))
 def channel(offset):
  z=x*np.exp(-2j*np.pi*offset*n/fs)
  return z[:len(z)//250*250].reshape(-1,250).mean(axis=1)
 z=channel(freq-center);g=channel(reference-center);t=(np.arange(len(z))+.5)/1000
 power=np.convolve(abs(z)**2,np.ones(5)/5,mode='same');floor=np.median(power[t<.8]);high=np.percentile(power,90);mask=power>max(floor*5,high*.15)
 edges=np.flatnonzero(np.diff(np.r_[False,mask,False]));on=[(float(t[i]),float(t[j-1])) for i,j in zip(edges[::2],edges[1::2]) if j-i>30]
 tracks=[]
 for i in range(0,len(z)-199,100):
  q=z[i:i+200];gg=g[i:i+200];tt=np.arange(200)/1000
  fit=np.polyfit(tt,np.unwrap(np.angle(q)),1);gf=np.polyfit(tt,np.unwrap(np.angle(gg)),1)
  if np.mean(abs(q)**2)<max(floor*5,high*.3):continue
  resid=np.std(np.unwrap(np.angle(q))-np.polyval(fit,tt))
  if resid>.2:continue
  si=fit[0]/(2*np.pi);gps=gf[0]/(2*np.pi);tracks.append([t[i]+.0995,si,gps,si-gps*freq/reference])
 tracks=np.array(tracks);assert len(tracks)>0,'No coherent frequency observations'
 np.savetxt(d/'frequency-tracks.csv',tracks,delimiter=',',header='seconds,indicated_offset_hz,gps_offset_hz,corrected_offset_hz',comments='')
 # Welch spectrum over RF-active intervals, with fixed window/resolution.
 size=65536;window=np.hanning(size);psd=np.zeros(size);count=0
 for i in range(0,len(x)-size+1,size//2):
  ti=(i+size/2)/fs
  if name=='carrier-none':
   if not any(lo+.15<ti<hi-.15 for lo,hi in on):continue
  elif not (on[0][0]-.1<ti<on[-1][1]+.1):continue
  psd+=abs(np.fft.fft(x[i:i+size]*window))**2;count+=1
 assert count;psd/=count;bins=np.fft.fftfreq(size,1/fs)-(freq-center);region=abs(bins)<5000;carrier=abs(bins-np.median(tracks[:,1]))<20
 share=float(psd[carrier].sum()/psd[region].sum());order=np.argsort(bins[region]);rr=bins[region][order];pp=psd[region][order];pp=10*np.log10(np.maximum(pp/psd[carrier].max(),1e-15))
 record={'on_intervals_s':on,'frequency_mean_hz':float(tracks[:,3].mean()),'frequency_span_hz':float(np.ptp(tracks[:,3])),'twenty_hz_fraction_within_5khz':share,'spectrum_scope':'steady carrier interiors' if name=='carrier-none' else 'complete active sequence including transitions and key edges','floor_power':float(floor),'on_power':float(high),'overflow_count':0,'clipped_samples':0}
 if name=='transitions-none':
  start=on[0][0];gaps=[]
  for k in range(1,16):
   b=start+.5*k;sel=(t>b-.025)&(t<b+.025);gaps.append({'expected_s':b,'minimum_relative_db':float(10*np.log10(max(power[sel].min()/high,1e-15))),'samples_below_minus_10db':int(np.sum(power[sel]<high*.1))})
  record['transitions']=gaps
 # Carrier burst means are GPSDO-corrected and exclude edge windows.
 if name=='carrier-none':record['burst_means_hz']=[float(tracks[(tracks[:,0]>lo+.2)&(tracks[:,0]<hi-.2),3].mean()) for lo,hi in on if hi-lo>1]
 results['cases'][name]=record
 axes[row,0].plot(t,10*np.log10(np.maximum(power/high,1e-12)),lw=.8);axes[row,0].set(title=name,ylabel='Relative channel power (dB)',ylim=(-45,3));axes[row,1].plot(rr,pp,lw=.7);axes[row,1].set(title='Spectrum during active intervals',xlabel='Offset from requested carrier (Hz)',ylabel='dB relative to carrier bin',ylim=(-90,5))
for ax in axes.flat:ax.grid(alpha=.25)
axes[-1,0].set_xlabel('Seconds from retained capture start');fig.suptitle(ident['stage']+' / '+ident['band']+' / fixed +3.470680 ppm');fig.tight_layout();fig.savefig(root/'comparison.png',dpi=150);fig.savefig(root/'comparison.pdf');(root/'analysis.json').write_text(json.dumps(results,indent=2))
print(json.dumps({n:{k:v for k,v in r.items() if k in ['frequency_mean_hz','frequency_span_hz','twenty_hz_fraction_within_5khz','burst_means_hz']} for n,r in results['cases'].items()},indent=2))
