import argparse
import subprocess
import re

from pathlib import Path

run_mac_template = """
/run/numberOfThreads {threads}
/run/verbose 1
/process/optical/cerenkov/setStackPhotons {flag}
/run/initialize
/run/beamOn 50000
"""

def parse_real_time(time_str):
    # Parses 'real\t0m41.149s' to seconds
    match = re.search(r'real\s+(\d+)m([\d.]+)s', time_str)
    if match:
        minutes = int(match.group(1))
        seconds = float(match.group(2))
        return minutes * 60 + seconds
    return None

def parse_sim_time(output):
    match = re.search(r"Simulation time:\s*([\d.]+)\s*seconds", output)
    if match:
        return float(match.group(1))
    return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-g', '--gdml', type=Path, default=Path('tests/geom/pfrich_min_FINAL.gdml'), help="Path to a custom GDML geometry file")
    parser.add_argument('-o', '--outpath', type=Path, default=Path('./'), help="Path where the output file will be saved")

    args = parser.parse_args()

    geant_file = args.outpath/"timing_geant.txt"
    optix_file = args.outpath/"timing_optix.txt"

    with open(geant_file, "w") as gfile, open(optix_file, "w") as ofile:
        for threads in range(50, 0, -1):
            times = {}
            sim_time_true = None
            for flag in ['true', 'false']:
                # Write run.mac with current flag
                with open("run.mac", "w") as rm:
                    rm.write(run_mac_template.format(threads=threads, flag=flag))
                # Run with time in bash to capture real/user/sys
                cmd = f"time simg4oxmt -g {args.gdml} -m run.mac"
                print(f"Running {threads} threads: {cmd}")
                result = subprocess.run(
                    ["bash", "-c", cmd],
                    capture_output=True, text=True
                )
                stdout = result.stdout
                stderr = result.stderr

                # Save simulation time for true run only
                if flag == 'true':
                    sim_time_true = parse_sim_time(stdout + stderr)
                    if sim_time_true is not None:
                        ofile.write(f"{threads} {sim_time_true}\n")
                        ofile.flush()

                # Extract real time
                real_match = re.search(r"real\s+\d+m[\d.]+s", stderr)
                if real_match:
                    real_sec = parse_real_time(real_match.group())
                    times[flag] = real_sec
                else:
                    print(f"[!] Could not find 'real' time for threads={threads} flag={flag}")

            # Write the difference to timing_geant.txt (true - false)
            if 'true' in times and 'false' in times:
                diff = times['true'] - times['false']
                gfile.write(f"{threads} {diff}\n")
                gfile.flush()
            else:
                print(f"[!] Missing times for threads={threads}")

    print("Done.")


if __name__ == '__main__':
    main()
