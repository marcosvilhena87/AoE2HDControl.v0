import csv
import tempfile
import unittest
from pathlib import Path
from analyze_economy import analyze


class AnalyzerTests(unittest.TestCase):
    def test_legacy_and_new_csv_sessions(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "events.csv"
            with path.open("w", newline="", encoding="utf-8") as f:
                w = csv.writer(f)
                w.writerow(["run_id","elapsed_ms","session","event","world","local_player",
                            "resource_table","resource","previous","current","delta","details"])
                w.writerow(["runA",1000,1,"initial",1,2,3,"Food","",200,"","initial"])
                w.writerow(["runA",1000,1,"initial",1,2,3,"Wood","",200,"","initial"])
                w.writerow(["runA",11000,1,"change",1,2,3,"Food",200,150,-50,"spent"])
                w.writerow(["runA",21000,1,"change",1,2,3,"Food",150,180,30,"earned"])
                w.writerow(["runA",21000,1,"change",1,2,3,"Wood",200,230,30,"earned"])
                w.writerow(["runA",25000,1,"unavailable",1,2,3,"","","","","closed"])
                w.writerow(["runA",26000,2,"initial",4,5,6,"Food","",100,"","new"])
                w.writerow(["runB",0,1,"initial",7,8,9,"Food","",300,"","new run"])
            rows = analyze(path)
            food = next(x for x in rows if x["run_id"] == "runA" and x["session"] == 1 and x["resource"] == "Food")
            self.assertEqual((food["initial"],food["final"],food["increases_observed"],food["decreases_observed"]), (200,180,30,50))
            self.assertEqual(food["net"],-20)
            self.assertEqual(food["net_per_minute"],-50)
            self.assertEqual(len(rows),4)
            self.assertEqual(len({(r["run_id"],r["session"]) for r in rows}),3)

    def test_legacy_format(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "legacy.csv"
            path.write_text("elapsed_ms,session,event,resource,previous,current,delta\n0,1,initial,Gold,0.00,100.00,0.00\n60000,1,change,Gold,100,130,30\n", encoding="utf8")
            result = analyze(path)
            self.assertEqual(result[0]["net_per_minute"],30)
            self.assertEqual(result[0]["run_id"],"legacy")


if __name__ == "__main__":
    unittest.main()
