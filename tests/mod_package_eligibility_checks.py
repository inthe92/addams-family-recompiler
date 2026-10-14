"""Package eligibility preserves failing suites and rejects missing/new failures."""
import contextlib,io,unittest
import verify_ctest_suite_checks as fixture
import mod_package_eligibility as eligibility
class Checks(unittest.TestCase):
 def setUp(self):self.f=fixture.GateChecks();self.f.setUp();self.addCleanup(self.f.doCleanups);self.f.write_inventory()
 def assess(self):
  with contextlib.redirect_stdout(io.StringIO()):return eligibility.assess(self.f.inventory,self.f.junit)
 def test_green(self):self.f.results();r=self.assess();self.assertTrue(r['suite_pass'] and r['package_eligible'])
 def test_a02_remains_failed_but_package_possible(self):self.f.results(failure='apu_status_irq_collision_reference');r=self.assess();self.assertFalse(r['suite_pass']);self.assertTrue(r['package_eligible']);self.assertEqual(r['failures'],['apu_status_irq_collision_reference'])
 def test_new_failure_blocks(self):self.f.results(failure='mod_completion02_store');self.assertFalse(self.assess()['package_eligible'])
 def test_skip_blocks(self):
  self.f.results(skip='mod_completion02_store')
  with self.assertRaises(ValueError):self.assess()
 def test_missing_blocks(self):
  self.f.results(missing='mod_completion02_store')
  with self.assertRaises(ValueError):self.assess()
 def test_wrong_count_blocks(self):
  self.f.outputs['mod_completion02_store']='Completion02 native checks=0 PASS';self.f.results()
  with self.assertRaises(ValueError):self.assess()
if __name__=='__main__':unittest.main()
