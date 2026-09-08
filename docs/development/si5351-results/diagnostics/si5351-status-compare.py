import ctypes,os,fcntl,time,json,collections,pathlib
class Msg(ctypes.Structure):
 _fields_=[('addr',ctypes.c_uint16),('flags',ctypes.c_uint16),('len',ctypes.c_uint16),('buf',ctypes.POINTER(ctypes.c_uint8))]
class Transfer(ctypes.Structure):
 _fields_=[('msgs',ctypes.POINTER(Msg)),('nmsgs',ctypes.c_uint32)]
f=os.open('/dev/i2c-1',os.O_RDWR);fcntl.ioctl(f,0x0703,0x60);os.write(f,bytes([3]));assert os.read(f,1)[0]==255
libc=ctypes.CDLL(None,use_errno=True);pointer=(ctypes.c_uint8*1)(0);data=(ctypes.c_uint8*1)();msgs=(Msg*2)(Msg(0x60,0,1,pointer),Msg(0x60,1,1,data));transfer=Transfer(msgs,2);values={'split':[],'combined':[]}
for i in range(500):
 os.write(f,bytes([0]));values['split'].append(os.read(f,1)[0])
 rc=libc.ioctl(f,0x0707,ctypes.byref(transfer));assert rc==2,(rc,ctypes.get_errno());values['combined'].append(data[0]);time.sleep(.002)
os.write(f,bytes([3]));assert os.read(f,1)[0]==255;os.close(f)
result={'samples':values,'counts':{k:dict(collections.Counter(v)) for k,v in values.items()}}
pathlib.Path('/home/pi/si5351-iterations/status-read-comparison.json').write_text(json.dumps(result,indent=2));print(json.dumps(result['counts']))
