import subprocess
import re

timings_file = "timings.txt"
opticks_file = "Opticks.txt"

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
    with open(timings_file, "w") as tf, open(opticks_file, "w") as of:
        for threads in range(1, 21):
            times = {}
            sim_time_true = None
            for flag in ['true', 'false']:
                # Write run.mac with current flag
                with open("run.mac", "w") as rm:
                    rm.write(run_mac_template.format(threads=threads, flag=flag))
                # Run with time in bash to capture real/user/sys
                result = subprocess.run(
                    ["bash", "-c", "time simg4oxmt -g tests/geom/pfrich_min_FINAL.gdml -m run.mac"],
                    capture_output=True, text=True
                )
                stdout = result.stdout
                stderr = result.stderr

                # Save simulation time for true run only
                if flag == 'true':
                    sim_time_true = parse_sim_time(stdout + stderr)
                    if sim_time_true is not None:
                        of.write(f"{threads} {sim_time_true}\n")
                        of.flush()

                # Extract real time
                real_match = re.search(r"real\s+\d+m[\d.]+s", stderr)
                if real_match:
                    real_sec = parse_real_time(real_match.group())
                    times[flag] = real_sec
                else:
                    print(f"[!] Could not find 'real' time for threads={threads} flag={flag}")

            # Write the difference to timings.txt (true - false)
            if 'true' in times and 'false' in times:
                diff = times['true'] - times['false']
                tf.write(f"{threads} {diff}\n")
                tf.flush()
            else:
                print(f"[!] Missing times for threads={threads}")

    print("Done.")


if __name__ == '__main__':
    main()
