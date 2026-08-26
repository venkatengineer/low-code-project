import math
import struct
import wave

SAMPLE_RATE = 44100
TOTAL_DURATION = 7.5
NUM_SAMPLES = int(SAMPLE_RATE * TOTAL_DURATION)

def clamp(v, min_v, max_v):
    return max(min_v, min(max_v, v))

# Generate sample buffer
samples_left = [0.0] * NUM_SAMPLES
samples_right = [0.0] * NUM_SAMPLES

# 1. Base ambient space room hum (0.0 - 7.5s)
for i in range(NUM_SAMPLES):
    t = i / SAMPLE_RATE
    hum = (math.sin(2 * math.pi * 55.0 * t) * 0.03 +
           math.sin(2 * math.pi * 110.0 * t) * 0.015 +
           math.sin(2 * math.pi * 165.0 * t + math.sin(t * 1.5)) * 0.01)
    lfo = 0.8 + 0.2 * math.sin(t * 1.2)
    samples_left[i] += hum * lfo
    samples_right[i] += hum * lfo

# 2. Fast Cartoony Keyboard Typing Clicks (0.2s - 5.2s)
# 45 fast playful mechanical keystrokes
import random
random.seed(42)

typing_times = []
curr_t = 0.25
while curr_t < 5.2:
    typing_times.append(curr_t)
    curr_t += 0.07 + random.uniform(0.02, 0.11)

for idx, st in enumerate(typing_times):
    start_samp = int(st * SAMPLE_RATE)
    click_dur = int(0.035 * SAMPLE_RATE)
    # Pitch variation for keys
    pitch = 1200.0 + random.uniform(-300.0, 400.0)
    pan = 0.4 + random.uniform(-0.2, 0.2)
    for j in range(click_dur):
        if start_samp + j < NUM_SAMPLES:
            dt = j / SAMPLE_RATE
            env = math.exp(-dt * 120.0)
            # High click + woody body pop
            click = (math.sin(2 * math.pi * pitch * dt) * 0.15 +
                     math.sin(2 * math.pi * 320.0 * dt) * 0.10) * env
            samples_left[start_samp + j] += click * (1.0 - pan)
            samples_right[start_samp + j] += click * pan

# 3. Playful Alien Chirps & Electronic Glitch Beeps (1.2s, 2.6s, 4.1s)
chirp_points = [
    (1.2, 880.0, 1320.0, 0.12),
    (2.6, 950.0, 1600.0, 0.15),
    (4.1, 780.0, 1400.0, 0.14)
]
for (c_time, f_start, f_end, dur) in chirp_points:
    s_idx = int(c_time * SAMPLE_RATE)
    d_len = int(dur * SAMPLE_RATE)
    for j in range(d_len):
        if s_idx + j < NUM_SAMPLES:
            dt = j / SAMPLE_RATE
            p = dt / dur
            freq = f_start + (f_end - f_start) * p
            env = math.sin(p * math.pi)
            chirp = math.sin(2 * math.pi * freq * dt) * env * 0.08
            samples_left[s_idx + j] += chirp * 0.6
            samples_right[s_idx + j] += chirp * 0.4

# 4. Compilation Progress Success Chimes (5.1s - 5.5s)
success_start = int(5.15 * SAMPLE_RATE)
success_dur = int(0.6 * SAMPLE_RATE)
success_notes = [659.25, 783.99, 1046.50, 1318.51] # E5, G5, C6, E6
for idx, sn in enumerate(success_notes):
    n_offset = int(idx * 0.06 * SAMPLE_RATE)
    for j in range(success_dur - n_offset):
        pos = success_start + n_offset + j
        if pos < NUM_SAMPLES:
            dt = j / SAMPLE_RATE
            env = math.exp(-dt * 6.0)
            note = math.sin(2 * math.pi * sn * dt) * env * 0.12
            samples_left[pos] += note * 0.5
            samples_right[pos] += note * 0.5

# 5. Happy Alien Greeting Chirp & Wave (5.4s - 6.2s)
greet_start = int(5.45 * SAMPLE_RATE)
greet_dur = int(0.35 * SAMPLE_RATE)
for j in range(greet_dur):
    if greet_start + j < NUM_SAMPLES:
        dt = j / SAMPLE_RATE
        p = dt / 0.35
        # Whistle wobble
        freq = 1200.0 + math.sin(p * math.pi * 6.0) * 180.0
        env = math.sin(p * math.pi) * 0.10
        sound = math.sin(2 * math.pi * freq * dt) * env
        samples_left[greet_start + j] += sound * 0.4
        samples_right[greet_start + j] += sound * 0.6

# 6. Grand Solaris OS Crystal Chord (6.4s - 7.5s)
chord_start = int(6.4 * SAMPLE_RATE)
chord_dur = int(1.1 * SAMPLE_RATE)
chord_freqs = [261.63, 392.00, 523.25, 659.25, 783.99, 1046.50] # C major orchestral fullness
for idx, cf in enumerate(chord_freqs):
    note_offset = int(idx * 0.03 * SAMPLE_RATE)
    for j in range(chord_dur - note_offset):
        pos = chord_start + note_offset + j
        if pos < NUM_SAMPLES:
            dt = j / SAMPLE_RATE
            env = math.exp(-dt * 2.2)
            note = (math.sin(2 * math.pi * cf * dt) * 0.10 +
                    math.sin(2 * math.pi * (cf * 2) * dt) * 0.04) * env
            samples_left[pos] += note * 0.5
            samples_right[pos] += note * 0.5

# Write WAV file
wav_path = os.path.join(os.path.dirname(__file__), "../output/solaris_boot_audio.wav")
with wave.open(wav_path, "wb") as wf:
    wf.setnchannels(2)
    wf.setsampwidth(2)
    wf.setframerate(SAMPLE_RATE)
    packed_frames = bytearray()
    for l, r in zip(samples_left, samples_right):
        l_clamped = int(clamp(l, -0.95, 0.95) * 32767)
        r_clamped = int(clamp(r, -0.95, 0.95) * 32767)
        packed_frames.extend(struct.pack("<hh", l_clamped, r_clamped))
    wf.writeframes(packed_frames)

print(f"Generated 2D Cartoon Alien audio soundtrack: {wav_path}")
