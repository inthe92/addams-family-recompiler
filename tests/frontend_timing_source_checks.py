"""Frontend integration contract and real diagnostic JSON serialization.

Source checks protect call placement in the actual SDL loop; native differential
and graphical probes provide execution evidence separately.
"""
import json
from pathlib import Path
import re
import subprocess
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1]
BINARY=sys.argv.pop(1) if len(sys.argv)>1 else None

class FrontendContract(unittest.TestCase):
    def setUp(self):
        self.source=(ROOT/'app/desktop.cpp').read_text()
        self.compact=re.sub(r'\s+','',self.source)
        self.active=self.compact.split('FrontendTiming::Scopesimulation_measure',1)[1].split('if(wav)wav->finish()',1)[0]

    def test_active_execution_order_is_preserved(self):
        calls=['bus.set_buttons(0,ui.game_input_enabled()', 'mods.frame(cpu)', 'god.begin_frame()',
               'constuint64_tbefore=bus.video().frames()', 'while(bus.video().frames()==before)',
               'game_execute_slice(&cpu,&services,&budget)', 'diagnostic.slice(cpu)', 'diagnostic.frame(cpu,',
               'bus.video().consume_frame()', 'bus.audio().take_samples()', 'audio_output.submit(samples)',
               'wav->append(samples)', 'ui.start(seconds())', 'presenter.present(bus.video())',
               'presenter.present_overlay(ui,seconds())', '++nes_frames', 'presenter.capture(capture)', 'SDL_GL_SwapWindow(window.get())']
        indices=[self.active.index(c) for c in calls];self.assertEqual(indices,sorted(indices))
        for c in ['game_execute_slice(', 'take_samples()', 'mods.frame(cpu)', 'god.begin_frame()']:
            self.assertEqual(self.active.count(c),1,c)
        self.assertIn('uint32_tbudget=1000;',self.active)

    def test_pause_has_no_hooks_logic_or_audio_collection(self):
        paused=self.compact.split('if(ui.paused()){',1)[1].split('continue;',1)[0]
        for c in ['game_execute_slice','mods.frame','god.begin_frame','take_samples','submit(samples)','++nes_frames','consume_frame']:
            self.assertNotIn(c,paused)
        self.assertIn('presenter.present(snapshot)',paused)
        self.assertIn('presentation_clock->presentation_complete(',paused)
        self.assertIn('if(presentation_clock)presenter.present(snapshot);elsepresenter.present(bus.video())',self.compact)
        self.assertIn('SDL_Delay(1)',paused)
        legacy=self.compact.split('if(ui.paused()){',1)[1].split('continue;',1)[1].split('continue;',1)[0]
        calls=['bus.set_buttons(0,0)', 'presenter.present(bus.video())', 'presenter.present_overlay(ui,seconds())',
               'SDL_GL_SwapWindow(window.get())', 'next_deadline=static_cast<double>(SDL_GetPerformanceCounter())', 'SDL_Delay(10)']
        self.assertEqual([legacy.index(c) for c in calls],sorted(legacy.index(c) for c in calls))
        self.assertIn('audio_output.set_paused(ui.paused())',self.compact)

    def test_frame_limit_capture_and_fast_path(self):
        self.assertIn('while(running&&(!limit||nes_frames<limit))',self.compact)
        self.assertIn('if(!nes_frames)ui.start(seconds())',self.active)
        self.assertIn('if(limit&&nes_frames==limit&&!capture.empty())',self.active)
        self.assertEqual(self.compact.count('++nes_frames'),1)
        self.assertIn('if(simulate)++nes_frames',self.active)
        self.assertIn('final_frame||presentation_clock->poll(',self.active)
        self.assertIn('if(experimental_refresh&&!fast)presentation_clock.emplace(',self.compact)
        self.assertIn('snapshot.complete(bus.video().frame(),',self.active)
        self.assertLess(self.active.index('wav->append(samples)'),self.active.index('snapshot.complete('))
        self.assertIn('constboolsimulate=!presentation_clock||presentation_clock->poll(',self.compact)
        wait=self.active.split('if(!fast){',1)[1]
        self.assertIn('next_pal_deadline(next_deadline,',wait)
        self.assertIn('while(static_cast<double>(SDL_GetPerformanceCounter())<next_deadline)SDL_Delay(1)',wait)
        self.assertIn('Presented',self.source) # Legacy summary retained.

    def test_diagnostic_is_advisory_and_off_by_default(self):
        self.assertIn('timing_requested=false',self.compact)
        self.assertIn('if(option=="--timing-diagnostic"){timing_requested=true;continue;}',self.compact)
        self.assertIn('if(timing_requested)timing.start(',self.compact)
        lib=(ROOT/'runtime/src/frontend_timing.cpp').read_text()
        self.assertNotRegex(lib,r'\b(Bus|CPUState|RuntimeServices|SDL_Delay|next_pal_deadline)\b')
        self.assertLess(self.compact.rindex('SDL_GL_SwapWindow'),self.compact.rindex('timing.report'))

    def test_report_round_trip_with_fake_clock(self):
        self.assertIsNotNone(BINARY)
        output=subprocess.check_output([BINARY,'json'],text=True)
        self.assertTrue(output.startswith('Timing diagnostic: '))
        data=json.loads(output.removeprefix('Timing diagnostic: '))
        self.assertEqual(data['schema'],1);self.assertTrue(data['unthrottled'])
        self.assertEqual(data['clock_frequency'],1000);self.assertEqual(data['wall_seconds'],0.025)
        self.assertEqual(data['legacy_frame_units'],1);self.assertEqual(data['counts']['nes_frames_executed'],1)
        self.assertEqual(data['counts']['nes_draw_commands'],3);self.assertEqual(data['counts']['buffer_swaps_completed'],0)
        self.assertEqual(data['stages']['render']['ticks'],25);self.assertEqual(data['clock_reads'],4)
        self.assertEqual(data['invalid_clock_samples'],0)

if __name__=='__main__':unittest.main()
