"""
BotiEyes PC Emulator - Main Entry Point

Runs the same 12 emotion cycle as BasicEmotion.ino, rendered with the
ported renderEyes() logic.

Usage:
    python botieyes_emulator.py
"""

import pygame
import sys
from emotion_mapper import EmotionMapper
from eye_renderer import EyeRenderer
from ui_controls import UIControls


# 12 emotion presets matching BotiEyes C++ helpers (intensity = 1.0)
# (name, valence, arousal, asymmetry)
EMOTIONS = [
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

EMOTION_DURATION_MS = 2000      # time per emotion (matches BasicEmotion.ino)
TRANSITION_MS       = 400       # smooth transition between emotions


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

        window_width = self.display_width * self.scale + 300
        window_height = self.display_height * self.scale
        self.screen = pygame.display.set_mode((window_width, window_height))
        pygame.display.set_caption("BotiEyes Emulator - Emotion-Driven Eye Animation")

        self.mapper = EmotionMapper()
        self.renderer = EyeRenderer(self.display_width, self.display_height, self.scale)
        self.ui = UIControls(self.display_width * self.scale, 0, 300, window_height)

        # Interpolation state
        self.current_idx = 0
        self.prev_v, self.prev_a, self.prev_asym = EMOTIONS[0][1:]
        self.target_v, self.target_a, self.target_asym = EMOTIONS[0][1:]
        self.transition_start = pygame.time.get_ticks()
        self.last_emotion_change = pygame.time.get_ticks()

        self.font = pygame.font.SysFont('Arial', 16, bold=True)
        self.small = pygame.font.SysFont('Consolas', 13)

        self.running = True
        self.clock = pygame.time.Clock()

    def _advance_emotion(self):
        self.current_idx = (self.current_idx + 1) % len(EMOTIONS)
        name, v, a, asym = EMOTIONS[self.current_idx]
        # Snapshot current interpolated values as the new "from"
        self.prev_v, self.prev_a, self.prev_asym = self._current_interp()
        self.target_v, self.target_a, self.target_asym = v, a, asym
        self.transition_start = pygame.time.get_ticks()
        self.last_emotion_change = self.transition_start
        print(f"Emotion: {name} (v={v:+.2f}, a={a:.2f}, asym={asym:+.2f})")

    def _current_interp(self):
        now = pygame.time.get_ticks()
        elapsed = now - self.transition_start
        t = min(1.0, elapsed / TRANSITION_MS) if TRANSITION_MS > 0 else 1.0
        t = _ease_in_out_cubic(t)
        v = self.prev_v + (self.target_v - self.prev_v) * t
        a = self.prev_a + (self.target_a - self.prev_a) * t
        asym = self.prev_asym + (self.target_asym - self.prev_asym) * t
        return v, a, asym

    def handle_events(self):
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.running = False
            elif event.type == pygame.KEYDOWN:
                if event.key == pygame.K_SPACE:
                    self._advance_emotion()
                elif event.key == pygame.K_ESCAPE:
                    self.running = False
            self.ui.handle_event(event)

    def update(self):
        now = pygame.time.get_ticks()
        if now - self.last_emotion_change >= EMOTION_DURATION_MS:
            self._advance_emotion()

        v, a, asym = self._current_interp()

        # Map to expression params and inject asymmetry
        params = self.mapper.map_emotion_to_expression(v, a)
        params.asymmetry = asym

        self.renderer.set_emotion(v, a)
        self.renderer.set_eye_position(0, 0)
        self.renderer.set_expression_params(params)

    def render(self):
        self.screen.fill((32, 32, 32))
        eye_surface = self.renderer.render()
        self.screen.blit(eye_surface, (0, 0))

        # Overlay: current emotion name and v/a
        name = EMOTIONS[self.current_idx][0]
        v, a, asym = self._current_interp()
        label = self.font.render(name, True, (0, 255, 120))
        self.screen.blit(label, (10, 10))
        info = self.small.render(
            f"V:{v:+.2f}  A:{a:.2f}  Asym:{asym:+.2f}", True, (180, 180, 180))
        self.screen.blit(info, (10, 34))
        hint = self.small.render("SPACE: next emotion   ESC: quit",
                                 True, (120, 120, 120))
        self.screen.blit(hint, (10, self.display_height * self.scale - 22))

        ui_surface = self.ui.render()
        self.screen.blit(ui_surface, (self.display_width * self.scale, 0))

        pygame.display.flip()

    def run(self):
        print("BotiEyes Emulator - cycling 12 emotions")
        while self.running:
            self.handle_events()
            self.update()
            self.render()
            self.clock.tick(60)
        pygame.quit()
        sys.exit()


if __name__ == "__main__":
    BotiEyesEmulator().run()
