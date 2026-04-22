"""
EyeRenderer - Pygame-based Eye Rendering

Matches C++ BotiEyes::renderEyes() logic using Pygame primitives.
- Filled base ellipses
- Eyelid overlays carved in background color
- Gaze offset from h/v angles
- Asymmetry scaling per-eye (for Thinking/Confused)
- Blink/wink vertical openness scaling
"""

import pygame


class EyeRenderer:
    """Renders eyes with emotion and position (matches C++ renderEyes())"""

    def __init__(self, display_width, display_height, scale):
        self.display_width = display_width
        self.display_height = display_height
        self.scale = scale

        # Low-res surface drawn at native OLED resolution, then scaled up
        self._low = pygame.Surface((display_width, display_height))
        self.surface = pygame.Surface((display_width * scale, display_height * scale))

        # State
        self.valence = 0.0
        self.arousal = 0.5
        self.h_angle = 0
        self.v_angle = 0
        self.openness_left = 1.0
        self.openness_right = 1.0
        self.params = None  # ExpressionParameters injected by emulator

    def set_emotion(self, valence, arousal):
        self.valence = valence
        self.arousal = arousal

    def set_eye_position(self, h_angle, v_angle):
        self.h_angle = max(-90, min(90, h_angle))
        self.v_angle = max(-45, min(45, v_angle))

    def set_openness(self, left, right):
        self.openness_left = max(0.0, min(1.0, left))
        self.openness_right = max(0.0, min(1.0, right))

    def set_expression_params(self, params):
        """Inject ExpressionParameters computed by EmotionMapper."""
        self.params = params

    # ---- Eyelid overlay: carves a curved shape in background color ----
    def _draw_eyelid(self, surf, cx, cy, w, h, coverage, is_top, color):
        if coverage <= 0.0:
            return
        coverage = min(coverage, 0.5)
        half_w = w // 2
        half_h = h // 2
        cov_h = int(half_h * coverage * 2.0)
        if cov_h <= 0:
            return

        if is_top:
            y1 = cy - half_h
            y2 = y1 + cov_h
            pygame.draw.rect(surf, color, (cx - half_w, y1, w, cov_h))
            # Curved edge: filled ellipse centered at y2, lower half bulges down into the eye
            pygame.draw.ellipse(surf, color,
                                (cx - half_w, y2 - cov_h // 2, w, cov_h))
        else:
            y2 = cy + half_h
            y1 = y2 - cov_h
            pygame.draw.rect(surf, color, (cx - half_w, y1, w, cov_h))
            # Curved edge: filled ellipse centered at y1, upper half bulges up into the eye (smile)
            pygame.draw.ellipse(surf, color,
                                (cx - half_w, y1 - cov_h // 2, w, cov_h))

    def render(self):
        low = self._low
        low.fill((0, 0, 0))

        if self.params is None:
            # Fallback: simple outline if no params injected yet
            cx = self.display_width // 2
            cy = self.display_height // 2
            pygame.draw.ellipse(low, (255, 255, 255), (cx - 35, cy - 15, 28, 30), 1)
            pygame.draw.ellipse(low, (255, 255, 255), (cx +  7, cy - 15, 28, 30), 1)
        else:
            p = self.params
            cx = self.display_width // 2
            cy = self.display_height // 2

            spacing = (self.display_width // 4) + p.spacing_adjust
            left_x  = cx - spacing
            right_x = cx + spacing

            # gaze offset (angle -> pixels), mirrors C++ mapping
            gaze_x = int(self.h_angle * 16 / 90)
            gaze_y = int(-self.v_angle * 10 / 45)

            eye_y = cy + p.y_offset + gaze_y

            lW = p.eye_width
            lH = p.eye_height
            rW = p.eye_width
            rH = p.eye_height

            if p.asymmetry != 0.0:
                a = p.asymmetry
                lH = max(4, min(60, int(lH * (1.0 + a))))
                rH = max(4, min(60, int(rH * (1.0 - a))))

            lH = max(2, int(lH * self.openness_left))
            rH = max(2, int(rH * self.openness_right))

            # Filled ellipses (white)
            pygame.draw.ellipse(low, (255, 255, 255),
                                (left_x + gaze_x - lW // 2, eye_y - lH // 2, lW, lH))
            pygame.draw.ellipse(low, (255, 255, 255),
                                (right_x + gaze_x - rW // 2, eye_y - rH // 2, rW, rH))

            # Eyelid overlays (drawn in black to carve the shape)
            if p.lid_top_coverage > 0.0:
                self._draw_eyelid(low, left_x  + gaze_x, eye_y, lW, lH, p.lid_top_coverage, True,  (0, 0, 0))
                self._draw_eyelid(low, right_x + gaze_x, eye_y, rW, rH, p.lid_top_coverage, True,  (0, 0, 0))
            if p.lid_bottom_coverage > 0.0:
                self._draw_eyelid(low, left_x  + gaze_x, eye_y, lW, lH, p.lid_bottom_coverage, False, (0, 0, 0))
                self._draw_eyelid(low, right_x + gaze_x, eye_y, rW, rH, p.lid_bottom_coverage, False, (0, 0, 0))

        # Nearest-neighbour upscale to final surface
        pygame.transform.scale(low, self.surface.get_size(), self.surface)
        return self.surface
