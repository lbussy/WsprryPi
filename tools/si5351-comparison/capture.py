#!/usr/bin/env python3
"""Run on wspr5 only: bounded, explicitly authorized conducted comparisons."""
import argparse,hashlib,json,pathlib,shlex,subprocess,time
p=argparse.ArgumentParser();p.add_argument('--enable-rf',action='store_true');p.add_argument('--band',choices=['40m','2m'],required=True);p.add_argument('--stage',required=True);p.add_argument('--source-revision',required=True);p.add_argument('--binary',required=True);p.add_argument('--extra',action='append',default=[]);a=p.parse_args()
assert a.enable_rf,'Explicit RF enable required'
assert a.stage.replace('-','').replace('_','').isalnum()
root=pathlib.Path('/home/pi/si5351-iterations')/(a.stage+'-'+a.band);root.mkdir(parents=True,exist_ok=False)
base=pathlib.Path('/home/pi/wsprrypi-qualification-runs/complete-test-deployment-b5bbe83c75ba346005d589da')
ssh=str(base/'ssh-wspr4');capture=str(base/'wspq-capture-soapy')
f=7040100 if a.band=='40m' else 144490500

def tx(args,timeout=25):
 r=subprocess.run([ssh,'wspr4',shlex.join(args)],capture_output=True,text=True,timeout=timeout)
 assert r.returncode==0,r.stderr+r.stdout
 return r.stdout

def snapshot():
 code="import fcntl,os,json;fd=os.open('/dev/i2c-1',os.O_RDWR);fcntl.ioctl(fd,0x0703,0x60);os.write(fd,bytes([3]));v=os.read(fd,1)[0];os.close(fd);print(json.dumps({'output_enable_register':v}))"
 return json.loads(tx(['python3','-c',code]))

before=snapshot();assert before['output_enable_register']==255
service=tx(['systemctl','is-active','wsprrypi.service']).strip();assert service=='active'
identity={'stage':a.stage,'source_revision':a.source_revision,'band':a.band,'binary':a.binary,'binary_sha256':tx(['sha256sum',a.binary]).split()[0],'frequency_hz':f,'gpsdo_frequency_hz':f+10000,'reference_hz':27000000,'ppm':3.470680,'center_hz':f-25000,'sample_rate_hz':250000,'bandwidth_hz':200000,'gain_db':20,'extra':a.extra,'before':before,'installed_hashes':tx(['sha256sum','/usr/local/bin/wsprrypi','/usr/local/etc/wsprrypi.ini'])}
(root/'identity.json').write_text(json.dumps(identity,indent=2))
guard=None;cap=None
try:
 tx(['sudo','-n','systemctl','stop','wsprrypi.service'])
 guard_source=(pathlib.Path(__file__).parent/'gpsdo_guard.py').read_text().replace('7050100',str(f+10000))
 (root/'gpsdo_guard.py').write_text(guard_source);(root/'gpsdo').mkdir()
 log=(root/'gpsdo.log').open('w')
 guard=subprocess.Popen(['sudo','-n','/home/pi/lbgpsdo/.venv/bin/python',str(root/'gpsdo_guard.py'),str(root/'gpsdo')],stdout=log,stderr=subprocess.STDOUT,start_new_session=True)
 deadline=time.monotonic()+50
 while not (root/'gpsdo/ready.json').exists() and guard.poll() is None and time.monotonic()<deadline:time.sleep(1)
 assert (root/'gpsdo/ready.json').exists(),(root/'gpsdo.log').read_text()
 for scenario,fade in [('carrier','none'),('transitions','none'),('keyed','none'),('keyed','raised_cosine')]:
  name=scenario+'-'+fade;out=root/name;out.mkdir();secs=18 if scenario=='carrier' else 16
  argv=[capture,'--enable-physical-sdr','sdrplay','2404058C60',str(f-25000),str(secs*250000),'20','250000','200000','0','false','false','500000',str(secs+6),str(out/'capture.cf32'),str(out/'metadata.json'),name]
  (out/'capture-command.json').write_text(json.dumps(argv));clog=(out/'capture.log').open('w');cap=subprocess.Popen(argv,stdout=clog,stderr=subprocess.STDOUT)
  time.sleep(1)
  assert cap.poll() is None,'Capture failed before TX'
  assert guard.poll() is None,'GPSDO guard stopped'
  command=['timeout','--signal=TERM','--kill-after=2','18',a.binary,'--scenario',scenario,'--fade',fade,'--freq',str(f),'--ppm','3.470680','--power-level','1']+a.extra
  (out/'transmitter-command.json').write_text(json.dumps(command));started=time.time()
  try:result=tx(command,timeout=23);(out/'transmitter.log').write_text(result)
  finally:(out/'transmitter-times.json').write_text(json.dumps({'start_unix':started,'end_unix':time.time()}))
  assert cap.wait(timeout=secs+6)==0,(out/'capture.log').read_text();cap=None
  meta=json.loads((out/'metadata.json').read_text());assert meta['retained_sample_count']==secs*250000
  assert meta['overflow_count']==0 and meta['clipping']['sample_count']==0, 'Capture overflow/clipping'
  assert snapshot()['output_enable_register']==255
  print(json.dumps({'stage':a.stage,'band':a.band,'case':name,'captured':True}),flush=True)
finally:
 if cap is not None and cap.poll() is None:cap.terminate();cap.wait(timeout=10)
 (root/'gpsdo/stop').touch()
 # Force RF off even if timeout terminated the benchmark before C++ cleanup.
 off="import fcntl,os;f=os.open('/dev/i2c-1',os.O_RDWR);fcntl.ioctl(f,0x0703,0x60);os.write(f,bytes([3,255]));os.close(f)"
 tx(['python3','-c',off]);after=snapshot();assert after['output_enable_register']==255
 tx(['sudo','-n','systemctl','start','wsprrypi.service'])
 if guard is not None:guard.wait(timeout=40)
 after['service']=tx(['systemctl','is-active','wsprrypi.service']).strip()
 after['installed_hashes']=tx(['sha256sum','/usr/local/bin/wsprrypi','/usr/local/etc/wsprrypi.ini']);assert after['installed_hashes']==identity['installed_hashes']
 if (root/'gpsdo/after.json').exists():
  after['gpsdo']=json.loads((root/'gpsdo/after.json').read_text());assert not after['gpsdo']['out1'] and not after['gpsdo']['out2'] and not after['gpsdo']['pps1']
 else:raise RuntimeError('GPSDO shutdown unconfirmed')
 (root/'cleanup.json').write_text(json.dumps(after,indent=2))
