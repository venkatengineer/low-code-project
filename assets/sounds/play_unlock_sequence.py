#!/usr/bin/env python3
"""
🛸 SOLARIS SPACECRAFT // VOICE & UNLOCK AUDIO TELEMETRY DISPATCHER
Plays the original spaceship engine startup audio, gathers live CPU temperatures
and telemetry in parallel, and delivers the dynamic AI cockpit voice briefing.
"""

import os
import sys
import glob
import time
import subprocess
import asyncio
import threading
import signal

# Ensure audio runtime environment
os.environ["XDG_RUNTIME_DIR"] = "/run/user/1000"
os.environ["PULSE_SERVER"] = "/run/user/1000/pulse/native"

# Ignore SIGHUP so parent shell termination doesn't kill audio playback
signal.signal(signal.SIGHUP, signal.SIG_IGN)

HOME_DIR = os.path.expanduser("~")
SOUND_ENGINE = f"{HOME_DIR}/.local/share/sounds/spaceship_engine_start.wav"
FALLBACK_TTS = f"{HOME_DIR}/.local/share/sounds/solaris_nominal_telemetry.wav"
TELEMETRY_CACHE = "/tmp/solaris_telemetry.mp3"
LOCK_FILE = "/tmp/solaris_unlock_sequence.lock"
LOG_FILE = "/tmp/solaris_unlock_sequence.log"

def log(msg):
    ts = time.strftime("%Y-%m-%d %H:%M:%S")
    formatted = f"[{ts}] [Unlock Audio] {msg}\n"
    try:
        with open(LOG_FILE, "a") as f:
            f.write(formatted)
    except Exception:
        pass
    print(formatted.strip(), flush=True)

def check_and_acquire_lock():
    """Prevent overlapping duplicate playback within 3.5 seconds."""
    now = time.time()
    try:
        if os.path.exists(LOCK_FILE):
            with open(LOCK_FILE, "r") as f:
                last_time = float(f.read().strip())
                if now - last_time < 3.5:
                    log("Duplicate unlock trigger detected within 3.5s — skipping duplicate.")
                    return False
    except Exception:
        pass

    try:
        with open(LOCK_FILE, "w") as f:
            f.write(f"{now}\n")
    except Exception:
        pass
    return True

def ensure_audio_unmuted():
    """Ensure audio output sink & ALSA hardware controls are unmuted and at full volume."""
    try:
        subprocess.run(["wpctl", "set-mute", "@DEFAULT_AUDIO_SINK@", "0"],
                       check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        subprocess.run(["wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", "1.0"],
                       check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        for ctrl in ['Master', 'Headphone', 'Speaker', 'PCM']:
            subprocess.run(["amixer", "-c", "0", "sset", ctrl, "100%", "unmute"],
                           check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except Exception as e:
        log(f"Unmute error: {e}")

def get_cpu_temp():
    try:
        temps = []
        for zone in glob.glob("/sys/class/thermal/thermal_zone*/temp"):
            try:
                with open(zone, "r") as f:
                    val = int(f.read().strip())
                    if 10000 < val < 110000:
                        temps.append(val / 1000.0)
            except Exception:
                pass
        if temps:
            avg_temp = int(sum(temps) / len(temps))
            status = "normal" if avg_temp < 75 else "elevated"
            return f"CPU temperature {status}, {avg_temp} degrees"
    except Exception:
        pass
    return "CPU temperature normal, 52 degrees"

def get_battery_info():
    try:
        for bat in glob.glob("/sys/class/power_supply/BAT*/capacity"):
            try:
                with open(bat, "r") as f:
                    pct = int(f.read().strip())
                    return f"Battery at {pct} percent"
            except Exception:
                pass
    except Exception:
        pass
    return "Power cell nominal"

def get_weather_info():
    try:
        res = subprocess.run(
            ["curl", "-s", "--max-time", "0.6", "wttr.in/?format=%C+%t"],
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True
        )
        if res.returncode == 0 and res.stdout.strip():
            w_text = res.stdout.strip().replace("+", "")
            return f"Weather: {w_text}"
    except Exception:
        pass
    return "Atmospheric sensors online"

async def generate_speech(text, out_path):
    import edge_tts
    communicate = edge_tts.Communicate(text, voice="en-US-AriaNeural", rate="+15%")
    await communicate.save(out_path)

def play_audio_file(path):
    if not os.path.exists(path):
        log(f"Audio file not found: {path}")
        return
    log(f"Playing audio: {path}")
    env = os.environ.copy()
    env["XDG_RUNTIME_DIR"] = "/run/user/1000"
    env["PULSE_SERVER"] = "/run/user/1000/pulse/native"

    for player in [
        ["paplay", path],
        ["pw-play", path],
        ["aplay", "-D", "default", "-q", path],
        ["ffplay", "-nodisp", "-autoexit", "-loglevel", "quiet", path]
    ]:
        try:
            res = subprocess.run(player, env=env, check=False)
            if res.returncode == 0:
                log(f"{player[0]} finished successfully for {path}")
                return
        except Exception:
            continue

def main():
    if not check_and_acquire_lock():
        sys.exit(0)

    log("Starting original unlock audio sequence...")

    # 1. Unmute output immediately
    ensure_audio_unmuted()

    # 2. Start playing original engine sound IMMEDIATELY
    engine_proc = None
    if os.path.exists(SOUND_ENGINE):
        log(f"Launching original engine audio: {SOUND_ENGINE}")
        env = os.environ.copy()
        env["XDG_RUNTIME_DIR"] = "/run/user/1000"
        env["PULSE_SERVER"] = "/run/user/1000/pulse/native"
        for p_cmd in [["paplay", SOUND_ENGINE], ["pw-play", SOUND_ENGINE], ["aplay", "-D", "default", "-q", SOUND_ENGINE]]:
            try:
                engine_proc = subprocess.Popen(p_cmd, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                break
            except Exception:
                continue

    # 3. Trigger visual cockpit lighting-up & gauge calibration sweep
    try:
        with open("/tmp/solaris_awakening_time", "w") as f:
            f.write(f"{time.time()}\n")
    except Exception:
        pass

    # 4. Prepare live telemetry voice briefing in background with real CPU temps & battery
    tts_ready = False

    def prepare_telemetry():
        nonlocal tts_ready
        temp_str = get_cpu_temp()
        bat_str = get_battery_info()
        weather_str = get_weather_info()
        speech_text = f"Ignition verified. {temp_str}. {bat_str}. {weather_str}. All systems operational."
        log(f"Telemetry prompt: {speech_text}")
        try:
            asyncio.run(generate_speech(speech_text, TELEMETRY_CACHE))
            tts_ready = True
            log("Telemetry audio synthesized successfully via edge-tts.")
        except Exception as e:
            log(f"edge-tts error: {e}")

    telemetry_thread = threading.Thread(target=prepare_telemetry)
    telemetry_thread.start()

    # 5. Wait for engine startup sound to complete
    if engine_proc:
        try:
            engine_proc.wait(timeout=5.2)
            log("Engine audio finished.")
        except Exception:
            pass

    telemetry_thread.join(timeout=1.5)

    # 6. Play the AI voice telemetry report with live temps!
    if tts_ready and os.path.exists(TELEMETRY_CACHE):
        play_audio_file(TELEMETRY_CACHE)
    elif os.path.exists(FALLBACK_TTS):
        log("Using fallback pre-rendered telemetry audio.")
        play_audio_file(FALLBACK_TTS)

    log("Unlock sequence completed.")

if __name__ == "__main__":
    main()
