#!/bin/bash
set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_DIR="$PROJECT_DIR/output"
PLYMOUTH_DIR="$PROJECT_DIR/plymouth-theme/solaris-alien-boot"

mkdir -p "$OUTPUT_DIR/frames_60fps" "$PLYMOUTH_DIR" "$PROJECT_DIR/web-preview"

echo "=== 1. Compiling Render Engine ==="
gcc -O3 "$PROJECT_DIR/src/render_bootsplash.c" -o "$PROJECT_DIR/src/render_bootsplash" $(pkg-config --cflags --libs cairo) -lm

echo "=== 2. Generating Audio Soundtrack ==="
python3 "$PROJECT_DIR/src/generate_audio.py"

echo "=== 3. Rendering 30 FPS Frames for Plymouth Theme (225 Frames) ==="
"$PROJECT_DIR/src/render_bootsplash" "$PLYMOUTH_DIR" 30 1920 1080

echo "=== 4. Rendering 60 FPS Frames for Smooth Full HD Video (450 Frames) ==="
"$PROJECT_DIR/src/render_bootsplash" "$OUTPUT_DIR/frames_60fps" 60 1920 1080

echo "=== 5. Encoding MP4 Video (H.264 + AAC Audio) ==="
ffmpeg -y -framerate 60 -i "$OUTPUT_DIR/frames_60fps/progress-%d.png" -i "$OUTPUT_DIR/solaris_boot_audio.wav" \
  -c:v libx264 -pix_fmt yuv420p -preset slow -crf 17 -c:a aac -b:a 192k -shortest "$OUTPUT_DIR/solaris-boot.mp4"

echo "=== 6. Encoding WebM Video (VP9) ==="
ffmpeg -y -i "$OUTPUT_DIR/solaris-boot.mp4" -c:v libvpx-vp9 -b:v 0 -crf 24 -c:a libopus "$OUTPUT_DIR/solaris-boot.webm"

echo "=== 7. Generating Animated GIF Preview ==="
ffmpeg -y -i "$OUTPUT_DIR/solaris-boot.mp4" -vf "fps=15,scale=960:-1:flags=lanczos,split[s0][s1];[s0]palettegen=max_colors=128[p];[s1][p]paletteuse=dither=bayer" "$OUTPUT_DIR/solaris-boot.gif"

echo "=== 8. Creating Plymouth Theme Files ==="
cat << 'PLYMOUTH_EOF' > "$PLYMOUTH_DIR/solaris-alien-boot.plymouth"
[Plymouth Theme]
Name=Solaris Alien Boot
Description=Stylized 3D Sci-Fi Alien Spacecraft Boot Splash for Solaris OS
ModuleName=script

[script]
ImageDir=/usr/share/plymouth/themes/solaris-alien-boot
ScriptFile=/usr/share/plymouth/themes/solaris-alien-boot/solaris-alien-boot.script
PLYMOUTH_EOF

cat << 'SCRIPT_EOF' > "$PLYMOUTH_DIR/solaris-alien-boot.script"
# Solaris OS Alien Boot Animation Script - 225 Frames (30 FPS)
total_frames = 225;
for (i = 0; i < total_frames; i++) {
  progress_images[i] = Image("progress-" + i + ".png");
}

sprite = Sprite();
sprite.SetZ(10);

frame_idx = 0;
counter = 0;

fun refresh_callback () {
  counter++;
  if (frame_idx < total_frames - 1) {
    frame_idx++;
  } else {
    # Keep final status frame visible or pulse
    frame_idx = total_frames - 1;
  }

  img = progress_images[frame_idx];
  sprite.SetImage(img);

  screen_w = Window.GetWidth();
  screen_h = Window.GetHeight();
  img_w = img.GetWidth();
  img_h = img.GetHeight();

  sprite.SetX((screen_w - img_w) / 2);
  sprite.SetY((screen_h - img_h) / 2);
}

Plymouth.SetRefreshFunction(refresh_callback);
SCRIPT_EOF

cat << 'INSTALL_EOF' > "$PLYMOUTH_DIR/install.sh"
#!/bin/bash
set -e
THEME_NAME="solaris-alien-boot"
TARGET_DIR="/usr/share/plymouth/themes/$THEME_NAME"

echo "Installing $THEME_NAME to $TARGET_DIR..."
sudo mkdir -p "$TARGET_DIR"
sudo cp -r ./* "$TARGET_DIR/"
sudo plymouth-set-default-theme -R "$THEME_NAME"
echo "Successfully installed and configured $THEME_NAME as default Plymouth boot splash!"
INSTALL_EOF
chmod +x "$PLYMOUTH_DIR/install.sh"

echo "=== BUILD COMPLETE! ==="
