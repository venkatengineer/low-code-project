import numpy as np
import math

res = 360
size = 240.0
x = np.linspace(-size*0.5, size*0.5, res)
y = np.linspace(-size*0.5, size*0.5, res)
X, Y = np.meshgrid(x, y)

# Fast vectorized multiscale fractal noise using harmonic sinusoids with irrational phase shifts
Z = np.zeros_like(X)

# 1. Broad rift valley / canyon chasm running diagonally
canyon_axis = 18.0 * np.sin(Y * 0.02) + 12.0 * np.cos(Y * 0.045)
d_canyon = np.abs(X - canyon_axis)
canyon_depth = -18.0 / (1.0 + (d_canyon / 22.0)**2.5)

# 2. Stepped basalt mesa plateaus
plateau = 12.0 * np.sin(X * 0.028 + 0.4) * np.cos(Y * 0.025 - 0.6)
# Stepping
terrace = np.round(plateau / 4.0) * 4.0

# 3. Outer mountain amphitheater
r = np.sqrt(X**2 + Y**2)
rim = np.clip((r - 55.0) / 45.0, 0.0, 1.0)
m1 = 1.0 - np.abs(np.sin(X * 0.04 + 0.2) * np.cos(Y * 0.04 + 0.8))
m2 = 1.0 - np.abs(np.sin(X * 0.08 - 0.5) * np.cos(Y * 0.08 + 0.3))
mountains = (m1 * 32.0 + m2 * 16.0) * rim

# 4. Fractured surface detail
detail = 1.5 * np.sin(X * 0.25) * np.cos(Y * 0.25) + 0.7 * np.sin(X * 0.5 + Y * 0.3)

Z = canyon_depth + terrace * (1.0 - rim * 0.5) + mountains + detail
print("Z min:", Z.min(), "Z max:", Z.max(), "Z mean:", Z.mean())
