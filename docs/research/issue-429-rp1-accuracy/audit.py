"""Offline audit of this exact GPIO20 campaign; never controls hardware."""
import json
import math
import pathlib
import re
import sys

BASE = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path('/private/tmp')
MODULE = '8ca08a2ba510f1720ccc47fd667cf8e8e749ded182dacb96ab488ed445b9a4ed'
RETRY_MODULE = '895f2d74a2143541775451b1fdbc0c7ec61eacd311e5b838d8273654430d0c0f'
FREQUENCIES = [137500,475700,1838100,3570100,5288700,7040100,10140200,
               14097100,18106100,21096100,24926100,28126100,50294500,70092500]

def clock_state(text):
    stack=[]
    for line in text.splitlines():
        fields=line.split()
        if len(fields)<5:
            continue
        depth=len(line)-len(line.lstrip())
        while stack and stack[-1][0]>=depth:
            stack.pop()
        if fields[0]=='clk_gp0':
            return {'parent':stack[-1][1],'parent_rate':int(stack[-1][2]),
                    'enabled':int(fields[1]),'rate':int(fields[4])}
        stack.append((depth,fields[0],fields[4]))
    raise AssertionError('GPCLK0 absent')

def collect():
    rows = []
    for root in sorted(BASE.glob('issue429-rp1-accuracy-20260830T*-*')):
        if root.name < 'issue429-rp1-accuracy-20260830T203918Z-':
            continue
        estimate = root/'analysis-carrier-peak.json'
        if not estimate.exists():
            estimate = root/'analysis.json'
        if not estimate.exists():
            continue
        analysis = json.loads(estimate.read_text())
        assert 'detector' in analysis, estimate
        metadata = json.loads((root/'metadata.json').read_text())
        transaction = json.loads((root/'tone.json').read_text())
        assert metadata['primary_outcome'] == 'success'
        plan_path=root/'acquisition-plan.json'
        plan=json.loads(plan_path.read_text()) if plan_path.exists() else {}
        expected_samples=plan.get('requested_sample_count',11000000)
        assert expected_samples in (11000000,30000000)
        assert metadata['retained_sample_count'] == metadata['requested_sample_count'] == expected_samples
        assert metadata['overflow_count'] == metadata['timeout_count'] == metadata['clipping']['sample_count'] == 0
        assert metadata['cleanup']['outcome'] == 'verified'
        assert metadata['resolved_device'] == {'driver':'sdrplay','serial':'2404058C60'}
        assert metadata['actual_settings']['sample_rate_hz'] == 250000
        assert metadata['actual_settings']['center_frequency_hz'] == analysis['target_hz']-25000
        before_clock=clock_state((root/'clock-before.txt').read_text())
        during_clock=clock_state((root/'clock-during-tone.txt').read_text())
        after_clock=clock_state((root/'clock-after.txt').read_text())
        assert before_clock == after_clock and after_clock['enabled'] == 0
        assert during_clock['parent'] == 'pll_sys' and during_clock['enabled'] > 0
        assert during_clock['parent_rate'] == 200000000
        assert transaction['completed'] and transaction['terminal_response']['scheduler_restored']
        assert transaction['frequency_hz'] == analysis['target_hz']
        assert (root/'rp1-final.txt').read_text().split() == ['N','0']
        assert len(re.findall(r'Output [12]:\s+---',(root/'gps-final.txt').read_text())) == 2
        for label in ('before','after'):
            reference_text=(root/f'gps-{label}.txt').read_text()
            assert 'SAT lock:     LOCKED' in reference_text
            assert 'PLL lock:     LOCKED' in reference_text
            assert 'Mode:         PLL' in reference_text
            assert re.search(r'Output 1:\s+'+str(int(analysis['target_hz']))+r' Hz\s+level: LOW',reference_text)
            assert re.search(r'Output 2:\s+---',reference_text)
        chrony = {}
        for label in ('before','after'):
            text = (root/f'chrony-{label}.txt').read_text()
            assert 'Leap status     : Normal' in text
            assert re.search(r'^\^\*\s+69\.197\.177\.234\s',text,re.M)
            chrony[label] = {key:float(re.search(r'^'+key+r'\s*:\s*([-+.0-9]+)',text,re.M)[1])
                             for key in ('Residual freq','Skew','Frequency')}
            assert abs(chrony[label]['Residual freq']) <= .5 and chrony[label]['Skew'] <= .5
        manifest_path = root/'manifest.json'
        if manifest_path.exists():
            manifest = json.loads(manifest_path.read_text())
            assert manifest['module_sha256'] in (MODULE, RETRY_MODULE)
            assert manifest['binary_sha256'] == '8c0e73b9429905b3424a3242c92adc0a720c155f36b03d3fb727c1ffebd19336'
            assert manifest['gpio'] == 20 and manifest['manual_ppm'] == 0
            module_sha = manifest['module_sha256']
        else:
            # This bracket's first analysis was rejected, before manifest write;
            # its unchanged raw samples were subsequently reviewed offline.
            assert root.name == 'issue429-rp1-accuracy-20260830T204012Z-475700'
            module_sha = MODULE
        a,b,c = (analysis[key] for key in ('gps_before','rp1','gps_after'))
        fraction = (b['mid_s']-a['mid_s'])/(c['mid_s']-a['mid_s'])
        assert 0 < fraction < 1
        error = b['residual_hz']-((1-fraction)*a['residual_hz']+fraction*c['residual_hz'])
        assert math.isclose(error,analysis['bracketed_error_hz'],abs_tol=1e-9)
        assert math.isclose(error/analysis['target_hz']*1e6,analysis['bracketed_error_ppm'],abs_tol=1e-9)
        fft_error = b['fft_residual_hz']-((1-fraction)*a['fft_residual_hz']+fraction*c['fft_residual_hz'])
        assert abs(fft_error-error) < .5, ('FFT versus phase estimate mismatch',root)
        rows.append({'run':root.name,'frequency_hz':analysis['target_hz'],'module_sha256':module_sha,
                     'campaign':'post_usb_reset_paced' if plan else 'pre_usb_reset',
                     'gpsdo_control_sha256':manifest.get('gpsdo_control_sha256') if manifest_path.exists() else None,
                     'error_ppm':analysis['bracketed_error_ppm'],
                     'fft_error_ppm':fft_error/analysis['target_hz']*1e6,
                     'error_hz':error,'reference_drift_hz':analysis['reference_drift_hz'],
                     'max_phase_residual_rad':max(point['phase_fit_residual_rms_rad'] for point in (a,b,c)),
                     'clock_before':before_clock,'clock_during':during_clock,'clock_after':after_clock,
                     'chrony':chrony,'capture_sha256':metadata['output']['sha256'],
                     'remote_capture':metadata['output']['path'],'analysis':analysis})
    return {'schema':1,'current_module_sha256':RETRY_MODULE,'gpio':20,'manual_ppm':0,
            'expected_frequencies_hz':FREQUENCIES,'measurements':rows,
            'scope':'single-bracket diagnostics, not a calibration constant or spectral qualification'}

if __name__ == '__main__':
    print(json.dumps(collect(),indent=2))
