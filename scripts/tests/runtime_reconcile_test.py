#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Offline startup orchestration tests; fake public provider, no system effects."""
import contextlib
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import runtime_reconcile as recovery


def digest(value):
    return recovery.installer.sha256_bytes(recovery.installer.canonical(value))


class System:
    def __init__(self, route='gpio4', history=True):
        self.record = {'runtime': {'bindingSha256': 'a'*64, 'artifactSetSha256': 'b'*64}}
        self.config = {'backend': 'rp1-gpclk', 'route': route, 'transmit': False}
        self.saved = None
        self.env = None
        self.calls = []
        self.state = 'activation_required'
        self.service_state = {'LoadState': 'loaded', 'ActiveState': 'active', 'UnitFileState': 'enabled'}
        self.current_boot = '00000000-0000-0000-0000-000000000002'
        self.plan = {'version': 3, 'activationContext': 'post-reboot',
            'bindingSha256': 'a'*64, 'artifactSetSha256': 'b'*64,
            'bootId': self.current_boot, 'rebootEvidence': {'transactions': {
                'transaction.json': {'phase': 'complete-inhibited', 'target': 1 if route == 'gpio4' else 2} if history else None}}}
        self.plan_hash = digest(self.plan)
        self.journal = {'phase': 'complete-neutral', 'plan': {'bootId': '00000000-0000-0000-0000-000000000001'}}
        self.active = None
        self.interrupt = None

    def bootstrap_capable(self): pass
    def lock(self): return contextlib.nullcontext()
    def ownership(self): return copy.deepcopy(self.record)
    def configuration(self): return copy.deepcopy(self.config)
    def checkpoint(self): return copy.deepcopy(self.saved)
    def activation_journal(self, name='activation.json'):
        return copy.deepcopy(self.journal) if name == 'activation.json' else None
    def boot(self): return self.current_boot
    def service(self): return copy.deepcopy(self.service_state)
    def environment(self, idle): self.env = idle
    def save(self, value):
        self.saved = copy.deepcopy(value)
        if self.interrupt == value['phase']: raise KeyboardInterrupt()
    def call(self, operation, arguments=()):
        self.calls.append((operation, tuple(arguments)))
        if operation == 'inspect':
            return {'result': self.state, 'identities': {'installedBinding': {
                'status': 'valid', 'sha256': 'a'*64, 'value': {'artifactSetSha256': 'b'*64}}},
                'activation': {'value': {'activationJournal': copy.deepcopy(self.journal)}},
                'routes': {'active': self.active}, 'safety': {'owner': False, 'lease': False}}
        if operation == 'activation-plan':
            return {'activationPlan': dict(self.plan, planSha256=self.plan_hash)}
        if operation == 'activation-ensure':
            assert arguments == ['--plan-sha256', self.plan_hash]
            self.journal = {'phase': 'complete-neutral', 'plan': copy.deepcopy(self.plan), 'planSha256': self.plan_hash}
            self.state = 'neutral_ready'
            if self.interrupt == operation: raise KeyboardInterrupt()
            return {'operation': operation, 'response': {'status': 'activated-neutral'}}
        if operation == 'route-plan':
            body = {'route': self.config['route'], 'bindingSha256': 'a'*64}
            return {'routePlan': dict(body, planSha256=digest(body))}
        if operation == 'route-ensure':
            assert arguments[:8] == ['--route', self.config['route'], '--requested-route', self.config['route'],
                                     '--configured-route', self.config['route'], '--persisted-route', self.config['route']]
            self.state = 'exact_ready'
            self.active = self.config['route']
            if self.interrupt == operation: raise KeyboardInterrupt()
            return {'response': {'status': 'restored'}}
        raise AssertionError(operation)


class Tests(unittest.TestCase):
    def test_normal_reboot_recovers_only_each_explicit_selected_route(self):
        for route in ('gpio4', 'gpio20'):
            with self.subTest(route=route):
                system = System(route)
                recovery.prepare(system)
                self.assertTrue(system.env)
                self.assertEqual(system.calls, [], 'ExecStartPre must never query the manager socket')
                self.assertEqual(recovery.reconcile(system), 'reconciled-idle')
                self.assertEqual(system.active, route)
                self.assertEqual(system.saved['phase'], 'complete')
                calls = copy.deepcopy(system.calls)
                self.assertEqual(recovery.reconcile(system), 'already-complete')
                self.assertEqual(system.calls, calls)
                self.assertFalse(system.config['transmit'])

    def test_neutral_reboot_and_completed_removal_do_not_select_a_route(self):
        for removed in (False, True):
            system = System(history=False)
            if removed:
                system.plan['rebootEvidence']['transactions']['transaction.json'] = {'phase': 'recovered-inhibited', 'target': 1}
                system.plan_hash = digest(system.plan)
            recovery.reconcile(system)
            self.assertIsNone(system.active)
            self.assertFalse(any(name.startswith('route-') for name, unused in system.calls))

    def test_blocked_history_reports_provider_refusal_without_planning(self):
        for state in ('recovery_required', 'conflict'):
            system = System()
            system.state = state
            with self.assertRaisesRegex(recovery.installer.ContractError,
                                        'provider refuses reboot activation: '+state):
                recovery.reconcile(system)
            self.assertEqual([name for name, unused in system.calls], ['inspect'])
            self.assertIsNone(system.saved)

    def test_saved_route_mismatch_stopped_masked_and_transmission_are_preserved(self):
        changes = (
            lambda s: s.config.update(route='gpio20'),
            lambda s: s.config.update(transmit=True),
            lambda s: s.service_state.update(ActiveState='inactive'),
            lambda s: s.service_state.update(LoadState='masked', UnitFileState='masked'),
            lambda s: s.record['runtime'].update(bindingSha256='c'*64),
            lambda s: setattr(s, 'plan_hash', '0'*64),
            lambda s: s.plan.update(version=2),
        )
        for mutate in changes:
            with self.subTest(mutate=mutate):
                system = System(); mutate(system)
                with self.assertRaises(recovery.installer.ContractError): recovery.reconcile(system)
                self.assertFalse(any(name.endswith('-ensure') for name, unused in system.calls))

    def test_each_worker_checkpoint_boundary_resumes_without_changing_route(self):
        for route in ('gpio4', 'gpio20'):
            for boundary in ('pending', 'activation-ensure', 'route-ensure', 'complete'):
                with self.subTest(route=route, boundary=boundary):
                    system = System(route); system.interrupt = boundary
                    with self.assertRaises(KeyboardInterrupt): recovery.reconcile(system)
                    recovery.prepare(system)
                    system.interrupt = None
                    recovery.reconcile(system)
                    self.assertEqual(system.active, route)
                    self.assertEqual(system.saved['phase'], 'complete')
                    self.assertFalse(system.config['transmit'])

    def test_checkpoint_never_adopts_new_boot_binding_route_or_failed_activation(self):
        changes = (
            lambda s: setattr(s, 'current_boot', '00000000-0000-0000-0000-000000000003'),
            lambda s: s.record['runtime'].update(bindingSha256='c'*64),
            lambda s: s.config.update(route='gpio20'),
            lambda s: setattr(s, 'state', 'recovery_required'),
        )
        for mutate in changes:
            with self.subTest(mutate=mutate):
                system = System(); system.interrupt = 'pending'
                with self.assertRaises(KeyboardInterrupt): recovery.reconcile(system)
                system.interrupt = None; system.calls.clear(); mutate(system)
                with self.assertRaises(recovery.installer.ContractError): recovery.reconcile(system)
                self.assertFalse(any(name.endswith('-ensure') for name, unused in system.calls))

    def test_alternate_backend_and_same_boot_neutral_do_not_select_route(self):
        system = System(); system.config['backend'] = 'si5351'
        self.assertEqual(recovery.reconcile(system), 'not-applicable')
        self.assertFalse(system.calls)
        system = System(); system.journal['plan']['bootId'] = system.boot()
        self.assertEqual(recovery.reconcile(system), 'no-reboot-reconciliation')
        self.assertFalse(any(name.endswith('-ensure') for name, unused in system.calls))

    def test_preparation_preserves_boot_policy_and_requires_no_socket(self):
        source = (Path(__file__).resolve().parents[2]/'src/config_handler.cpp').read_text()
        override = source.split('if (std::getenv("WSPRRYPI_ROUTE_RESTORE_IDLE") != nullptr ||', 1)[1].split('switch (config.enable_on_boot)', 1)[0]
        self.assertIn('WSPRRYPI_RP1_REBOOT_IDLE', override)
        self.assertIn('config.transmit = false', override)
        self.assertNotIn('config.enable_on_boot =', override)
        for old in (True, False):
            system = System(); system.config['transmit'] = old
            recovery.prepare(system)
            self.assertTrue(system.env)
            self.assertEqual(system.config['transmit'], old)
            self.assertFalse(system.calls)

    def test_bootstrap_flag_does_not_hide_current_route_restoration_acknowledgement(self):
        system = System(); system.interrupt = 'activation-ensure'
        with self.assertRaises(KeyboardInterrupt): recovery.reconcile(system)
        with patch.object(system, 'activation_journal', side_effect=lambda name='activation.json':
                system.journal if name == 'activation.json' else {'boot': system.boot(), 'phase': 'start-intent'}):
            recovery.prepare(system)
        self.assertFalse(system.env)
        source = (Path(__file__).resolve().parents[2]/'src/scheduling_runtime.cpp').read_text()
        self.assertIn('restoration && std::getenv("WSPRRYPI_RP1_REBOOT_IDLE") == nullptr', source)
        self.assertIn('!acknowledge_rp1_restoration(restoration, config.transmit)', source)

    def test_two_reboots_each_require_new_boot_plan_and_route_request(self):
        for route in ('gpio4', 'gpio20'):
            system = System(route)
            recovery.reconcile(system)
            first = copy.deepcopy(system.saved)
            system.current_boot = '00000000-0000-0000-0000-000000000003'
            system.plan['bootId'] = system.current_boot
            system.plan_hash = digest(system.plan)
            system.state = 'activation_required'
            system.active = None
            recovery.prepare(system)
            self.assertTrue(system.env)
            recovery.reconcile(system)
            self.assertNotEqual(first['activationPlanSha256'], system.saved['activationPlanSha256'])
            self.assertNotEqual(first['bootId'], system.saved['bootId'])
            self.assertEqual(system.active, route)

    def test_startup_installer_upgrade_removal_and_interrupted_retry(self):
        import install_runtime_reconcile as installation
        for boundary in (None, 0, 1, 2):
            with self.subTest(boundary=boundary), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                (root/'source').write_bytes(b'first')
                targets = {str(root/('installed'+str(i))): ('source', 0o644) for i in range(3)}
                state = root/'state.json'
                def read(path): return path.read_bytes() if path.exists() else None
                replace = installation.os.replace
                calls = []
                def interrupted(source, target):
                    replace(source, target)
                    if str(target) in targets:
                        calls.append(str(target))
                        if len(calls)-1 == boundary: raise KeyboardInterrupt()
                with patch.object(installation, 'ROOT', root), patch.object(installation, 'STATE', state), \
                     patch.object(installation, 'FILES', targets), patch.object(installation, 'read', read), \
                     patch.object(installation.subprocess, 'run') as command:
                    with patch.object(installation.os, 'replace', interrupted):
                        if boundary is None: installation.apply()
                        else:
                            with self.assertRaises(KeyboardInterrupt): installation.apply()
                    installation.apply()
                    self.assertTrue(all(Path(p).read_bytes() == b'first' for p in targets))
                    (root/'source').write_bytes(b'upgrade')
                    installation.apply()
                    self.assertTrue(all(Path(p).read_bytes() == b'upgrade' for p in targets))
                    installation.apply(remove=True)
                    installation.apply(remove=True)
                    self.assertTrue(all(not Path(p).exists() for p in targets))
                    self.assertTrue(all(call.args[0] == ['/usr/bin/systemctl', 'daemon-reload'] for call in command.call_args_list))

    def test_provider_replacement_or_conflict_starts_diagnostics_idle_without_administration(self):
        for error in (FileNotFoundError('binding retired'), recovery.installer.ContractError('ownership differs')):
            system = System()
            with patch.object(system, 'ownership', side_effect=error):
                recovery.prepare(system)
                self.assertTrue(system.env)
                with self.assertRaises(type(error)): recovery.reconcile(system)
            self.assertFalse(system.calls)
        system = System()
        with patch.object(system, 'bootstrap_capable', side_effect=ValueError('old binary')):
            with self.assertRaises(ValueError): recovery.prepare(system)
        self.assertIsNone(system.env)

    def test_duplicate_or_malformed_checkpoint_is_rejected(self):
        with self.assertRaises(recovery.installer.ContractError): recovery.strict_json('{"a":1,"a":2}')
        for value in ({}, {'schema': recovery.SCHEMA}):
            with self.assertRaises(recovery.installer.ContractError): recovery.validate_checkpoint(value)


if __name__ == '__main__': unittest.main()
