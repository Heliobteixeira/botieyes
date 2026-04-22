"""
BotiEyes PC Emulator - Main Entry Point

Runs a cycle of 12 emotions (matches BasicEmotion.ino) in AUTO mode, and
switches to MANUAL mode when the user interacts with the control panel:
dragging sliders or clicking emotion / position preset buttons takes over
the current target. Press SPACE to return to AUTO.

Usage:
    python botieyes_emulator.py
"""

import pygame
import sys
import io
import os
import json
import datetime
from emotion_mapper import EmotionMapper
from eye_renderer import EyeRenderer
from ui_controls import UIControls


# 12 emotion presets for the auto-cycle (intensity = 1.0)
# (name, valence, arousal, asymmetry)
AUTO_EMOTIONS = [
    ("Happy",     0.35, 0.55,  0.00),
    ("Sad",      -0.35, 0.35,  0.00),
    ("Angry",    -0.30, 0.80,  0.00),
    ("Calm",      0.00, 0.10,  0.00),
    ("Excited",   0.30, 0.90,  0.00),
    ("Tired",     0.05, 0.10,  0.00),
    ("Surprised", 0.15, 0.85,  0.00),
    ("Anxious",  -0.20, 0.75,  0.00),
    ("Content",   0.25, 0.40,  0.00),
    ("Curious",   0.15, 0.60,  0.00),
    ("Thinking",  0.00, 0.45, -0.20),
    ("Confused", -0.15, 0.55, -0.30),
]

EMOTION_DURATION_MS = 2000      # time per emotion in auto-cycle
TRANSITION_MS       = 400       # smooth transition between target changes


def _ease_in_out_cubic(t):
    if t < 0.5:
        return 4 * t * t * t
    f = (2 * t - 2)
    return 0.5 * f * f * f + 1


class BotiEyesEmulator:
    def __init__(self, width=128, height=64):
        pygame.init()

        self.display_width = width
        self.display_height = height
        self.scale = 8

        panel_w = 300
        window_width = self.display_width * self.scale + panel_w
        window_height = self.display_height * self.scale
        self.screen = pygame.display.set_mode((window_width, window_height))
        pygame.display.set_caption("BotiEyes Emulator - Emotion-Driven Eye Animation")

        self.mapper = EmotionMapper()
        self.renderer = EyeRenderer(self.display_width, self.display_height, self.scale)
        self.ui = UIControls(self.display_width * self.scale, 0, panel_w, window_height)

        # Interpolation state (generic: from -> to over TRANSITION_MS)
        self.current_idx = 0
        self.prev_v, self.prev_a, self.prev_asym = AUTO_EMOTIONS[0][1:]
        self.target_v, self.target_a, self.target_asym = AUTO_EMOTIONS[0][1:]
        self.transition_start = pygame.time.get_ticks()
        self.last_emotion_change = pygame.time.get_ticks()

        # Gaze target (H, V degrees) with smooth interpolation
        self.prev_h = 0
        self.prev_v_pos = 0
        self.target_h = 0
        self.target_v_pos = 0
        self.pos_transition_start = pygame.time.get_ticks()
        self.pos_transition_ms = TRANSITION_MS

        self.mode = "auto"          # 'auto' or 'manual'
        self.manual_emotion_label = None
        self.manual_position_label = None

        self.font = pygame.font.SysFont('Arial', 16, bold=True)
        self.small = pygame.font.SysFont('Consolas', 13)

        self.running = True
        self.clock = pygame.time.Clock()

    # ----- interpolation helpers -----

    def _current_emotion(self):
        now = pygame.time.get_ticks()
        elapsed = now - self.transition_start
        t = min(1.0, elapsed / TRANSITION_MS) if TRANSITION_MS > 0 else 1.0
        t = _ease_in_out_cubic(t)
        v = self.prev_v + (self.target_v - self.prev_v) * t
        a = self.prev_a + (self.target_a - self.prev_a) * t
        asym = self.prev_asym + (self.target_asym - self.prev_asym) * t
        return v, a, asym

    def _current_position(self):
        now = pygame.time.get_ticks()
        elapsed = now - self.pos_transition_start
        t = min(1.0, elapsed / self.pos_transition_ms) if self.pos_transition_ms > 0 else 1.0
        t = _ease_in_out_cubic(t)
        h = self.prev_h + (self.target_h - self.prev_h) * t
        v = self.prev_v_pos + (self.target_v_pos - self.prev_v_pos) * t
        return h, v

    def _set_emotion_target(self, v, a, asym, duration_ms=TRANSITION_MS):
        cv, ca, casym = self._current_emotion()
        self.prev_v, self.prev_a, self.prev_asym = cv, ca, casym
        self.target_v, self.target_a, self.target_asym = v, a, asym
        self.transition_start = pygame.time.get_ticks()

    def _set_position_target(self, h, v, duration_ms=TRANSITION_MS):
        ch, cv = self._current_position()
        self.prev_h, self.prev_v_pos = ch, cv
        self.target_h, self.target_v_pos = h, v
        self.pos_transition_start = pygame.time.get_ticks()
        self.pos_transition_ms = duration_ms

    # ----- auto cycle -----

    def _advance_auto(self):
        self.current_idx = (self.current_idx + 1) % len(AUTO_EMOTIONS)
        name, v, a, asym = AUTO_EMOTIONS[self.current_idx]
        self._set_emotion_target(v, a, asym)
        self.last_emotion_change = pygame.time.get_ticks()
        print(f"[AUTO] Emotion: {name} (v={v:+.2f}, a={a:.2f}, asym={asym:+.2f})")

    def _enter_manual(self):
        if self.mode != "manual":
            self.mode = "manual"
            print("[MODE] MANUAL  (SPACE to resume auto)")

    def _enter_auto(self):
        if self.mode != "auto":
            self.mode = "auto"
            self.manual_emotion_label = None
            self.manual_position_label = None
            self.ui.active_emotion = None
            self.ui.active_position = None
            self.last_emotion_change = pygame.time.get_ticks()
            print("[MODE] AUTO")

    # ----- main loop plumbing -----

    def handle_events(self):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    self.running = False
                elif event.key == pygame.K_SPACE:
                    self._enter_auto()
                elif event.key == pygame.K_c:
                    path = self.save_frame()
                    print(f"[CAPTURE] Saved frame + state to {path}")
            self.ui.handle_event(event)

    def update(self):
        now = pygame.time.get_ticks()

        # Consume UI requests first; they always switch to manual.
        emo_req = self.ui.consume_emotion_request()
        pos_req = self.ui.consume_position_request()
        slider_dirty = self.ui.consume_slider_dirty()

        if emo_req is not None:
            label, v, a, asym = emo_req
            self._enter_manual()
            self.manual_emotion_label = label
            self._set_emotion_target(v, a, asym)
            print(f"[MANUAL] Emotion: {label} (v={v:+.2f}, a={a:.2f}, asym={asym:+.2f})")

        if pos_req is not None:
            label, h, v = pos_req
            self._enter_manual()
            self.manual_position_label = label
            self._set_position_target(h, v)
            print(f"[MANUAL] Position: {label} (h={h}, v={v})")

        if slider_dirty:
            self._enter_manual()
            sv, sa, sh, svv = self.ui.slider_values()
            # Sliders drive targets live. Clear asymmetry when user is
            # adjusting raw valence/arousal so the shape matches the sliders.
            self._set_emotion_target(sv, sa, 0.0, duration_ms=120)
            self._set_position_target(sh, svv, duration_ms=120)
            self.manual_emotion_label = None
            self.manual_position_label = None
            self.ui.active_emotion = None
            self.ui.active_position = None

        # Auto-cycle only when in auto mode
        if self.mode == "auto":
            if now - self.last_emotion_change >= EMOTION_DURATION_MS:
                self._advance_auto()

        v, a, asym = self._current_emotion()
        h_pos, v_pos = self._current_position()

        # Reflect current state in sliders (when not being dragged)
        self.ui.sync_from_state(v, a, h_pos, v_pos)

        params = self.mapper.map_emotion_to_expression(v, a)
        params.asymmetry = asym

        self.renderer.set_emotion(v, a)
        self.renderer.set_eye_position(h_pos, v_pos)
        self.renderer.set_expression_params(params)

    def render(self):
        self.screen.fill((32, 32, 32))
        eye_surface = self.renderer.render()
        self.screen.blit(eye_surface, (0, 0))

        # Overlay current state on the eye canvas
        if self.mode == "auto":
            name = AUTO_EMOTIONS[self.current_idx][0]
        else:
            name = self.manual_emotion_label or "(custom)"
        v, a, asym = self._current_emotion()
        h_pos, v_pos = self._current_position()

        label = self.font.render(name, True, (0, 255, 120))
        self.screen.blit(label, (10, 10))
        info = self.small.render(
            f"V:{v:+.2f}  A:{a:.2f}  Asym:{asym:+.2f}", True, (180, 180, 180))
        self.screen.blit(info, (10, 34))
        pos_info = self.small.render(
            f"H:{int(round(h_pos)):+3d}deg  V:{int(round(v_pos)):+3d}deg",
            True, (180, 180, 180))
        self.screen.blit(pos_info, (10, 52))
        hint = self.small.render("SPACE: auto   C: capture   ESC: quit",
                                 True, (120, 120, 120))
        self.screen.blit(hint, (10, self.display_height * self.scale - 22))

        ui_surface = self.ui.render(mode=self.mode)
        self.screen.blit(ui_surface, (self.display_width * self.scale, 0))

        pygame.display.flip()

    # ----- AI feedback hooks (User Story 2) -----

    def captureFrame(self):
        """Return the current 128x64 OLED frame as PNG bytes.

        Uses the low-resolution source surface (not the upscaled one) so AI
        tools see exactly what the embedded display would show.
        """
        buf = io.BytesIO()
        low = self.renderer._low
        pygame.image.save(low, buf, "PNG")
        return buf.getvalue()

    def getExpressionState(self):
        """Return a JSON-serializable dict describing the current render state."""
        v, a, asym = self._current_emotion()
        h_pos, v_pos = self._current_position()
        p = self.mapper.map_emotion_to_expression(v, a)
        p.asymmetry = asym
        if self.mode == "auto":
            label = AUTO_EMOTIONS[self.current_idx][0]
        else:
            label = self.manual_emotion_label or "(custom)"
        return {
            "mode": self.mode,
            "emotion_label": label,
            "valence": round(v, 4),
            "arousal": round(a, 4),
            "position": {
                "h_deg": int(round(h_pos)),
                "v_deg": int(round(v_pos)),
            },
            "expression_params": {
                "eye_width": p.eye_width,
                "eye_height": p.eye_height,
                "lid_top_coverage": round(p.lid_top_coverage, 4),
                "lid_bottom_coverage": round(p.lid_bottom_coverage, 4),
                "y_offset": p.y_offset,
                "spacing_adjust": p.spacing_adjust,
                "asymmetry": round(p.asymmetry, 4),
            },
        }

    def save_frame(self, out_dir="captures"):
        """Save current frame as PNG + state as JSON. Returns the PNG path."""
        os.makedirs(out_dir, exist_ok=True)
        ts = datetime.datetime.now().strftime("%Y%m%d_%H%M%S_%f")[:-3]
        png_path = os.path.join(out_dir, f"frame_{ts}.png")
        json_path = os.path.join(out_dir, f"frame_{ts}.json")
        with open(png_path, "wb") as f:
            f.write(self.captureFrame())
        with open(json_path, "w", encoding="utf-8") as f:
            json.dump(self.getExpressionState(), f, indent=2)
        return png_path

    def run(self):
        print("BotiEyes Emulator")
        print("  AUTO mode cycles 12 emotions.")
        print("  Click a button / drag a slider to enter MANUAL mode.")
        print("  SPACE = resume AUTO,  C = capture frame+state,  ESC = quit.")
        while self.running:
            self.handle_events()
            self.update()
            self.render()
            self.clock.tick(60)
        pygame.quit()
        sys.exit()


if __name__ == "__main__":
    BotiEyesEmulator().run()
