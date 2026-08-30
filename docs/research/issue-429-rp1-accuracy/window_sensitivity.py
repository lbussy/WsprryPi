"""Read-only analysis-window sensitivity check for retained captures."""
import contextlib
import io
import json
from pathlib import Path
import runpy
import sys

arguments=sys.argv[1:]
results=[]
for directory in arguments:
    root=Path(directory)
    target=float(root.name.rsplit('-',1)[1])
    sys.argv=['estimate.py',str(root),str(target)]
    with contextlib.redirect_stdout(io.StringIO()):
        state=runpy.run_path(str(Path(__file__).with_name('estimate.py')))
    intervals=state['runs']; fs=state['fs']; estimate=state['estimate']
    first=estimate(intervals[0][1]-int(6*fs),intervals[0][1])
    last=estimate(intervals[2][0],intervals[2][0]+int(6*fs))
    tx=state['rp1']
    def error(tx):
        alpha=(tx['mid_s']-first['mid_s'])/(last['mid_s']-first['mid_s'])
        return (tx['residual_hz']-((1-alpha)*first['residual_hz']+alpha*last['residual_hz']))/target*1e6
    a,b=intervals[1]
    inset=(b-a)//4
    middle=estimate(a+inset,b-inset)
    results.append({'frequency_hz':target,'run':root.name,
        'full_reference_ppm':state['error']/target*1e6,
        'near_reference_ppm':error(tx),'central_tone_near_reference_ppm':error(middle),
        'scope':'window sensitivity only, not a formal uncertainty budget'})
print(json.dumps(results,indent=2))
