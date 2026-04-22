"""
UIControls - Interactive Control Panel

Provides sliders and buttons for controlling valence, arousal, and eye position.
Includes emotion helper buttons (T059) and position helper buttons (T060).

Coordinate convention: all rects and mouse positions are expressed in
PANEL-LOCAL coordinates. The caller (emulator) forwards pygame events
as-is; this class converts global mouse positions into panel-local
coordinates using (self.x, self.y).
"""

import pygame


# 13 emotion presets matching BotiEyes C++ helpers (intensity = 1.0)
# (label, valence, arousal, asymmetry)
EMOTION_PRESETS = [
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
    ("Neutral",   0.00, 0.00,  0.00),
]

# 5 position helpers matching BotiEyes C++ (h, v)
POSITION_PRESETS = [
    ("Left",    -45,   0),
    ("Right",    45,   0),
    ("Up",        0,  30),
    ("Down",      0, -30),
    ("Center",    0,   0),
]


class _Slider:
    def __init__(self, rect, label, vmin, vmax, initial, fmt="{:+.2f}"):
        self.rect = pygame.Rect(rect)
        self.label = label
        self.vmin = vmin
        self.vmax = vmax
        self.value = initial
        self.fmt = fmt
        self.dragging = False

    def _track_rect(self):
        return pygame.Rect(self.rect.x, self.rect.y + 18, self.rect.w, 6)

    def _handle_rect(self):
        track = self._track_rect()
        t = (self.value - self.vmin) / (self.vmax - self.vmin)
        t = max(0.0, min(1.0, t))
        hx = int(track.x + t * track.w) - 5
        return pygame.Rect(hx, track.y - 4, 10, 14)

    def hit(self, pos):
        return self.rect.collidepoint(pos) or self._track_rect().collidepoint(pos)

    def set_from_pos(self, pos):
        track = self._track_rect()
        t = (pos[0] - track.x) / max(1, track.w)
        t = max(0.0, min(1.0, t))
        self.value = self.vmin + t * (self.vmax - self.vmin)

    def draw(self, surf, font):
        label = f"{self.label}: {self.fmt.format(self.value)}"
        surf.blit(font.render(label, True, (220, 220, 220)), (self.rect.x, self.rect.y))
        track = self._track_rect()
        pygame.draw.rect(surf, (70, 70, 70), track, border_radius=3)
        handle = self._handle_rect()
        pygame.draw.rect(surf, (120, 200, 255), handle, border_radius=3)


class _Button:
    def __init__(self, rect, label, action):
        self.rect = pygame.Rect(rect)
        self.label = label
        self.action = action    # (kind, payload)
        self.hover = False

    def draw(self, surf, font, active=False):
        if active:
            bg = (70, 110, 70)
        elif self.hover:
            bg = (80, 80, 80)
        else:
            bg = (60, 60, 60)
        pygame.draw.rect(surf, bg, self.rect, border_radius=4)
        pygame.draw.rect(surf, (30, 30, 30), self.rect, width=1, border_radius=4)
        text = font.render(self.label, True, (230, 230, 230))
        tx = self.rect.x + (self.rect.w - text.get_width()) // 2
        ty = self.rect.y + (self.rect.h - text.get_height()) // 2
        surf.blit(text, (tx, ty))


class UIControls:
    """UI control panel for emulator.

    The emulator owns a mode flag ('auto' or 'manual'). This panel exposes:
      - sliders for valence, arousal, H angle, V angle (T057)
      - emotion preset buttons (T059)
      - position preset buttons (T060)

    User interaction sets pending_emotion / pending_position / slider_dirty
    so the emulator can consume them each frame and switch out of auto-cycle.
    """

    def __init__(self, x, y, width, height):
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.surface = pygame.Surface((width, height))

        self.font_title = pygame.font.SysFont('Arial', 14, bold=True)
        self.font = pygame.font.SysFont('Arial', 12)
        self.font_small = pygame.font.SysFont('Consolas', 11)

        pad = 12
        slider_w = width - 2 * pad
        y0 = 28

        self.s_val = _Slider((pad, y0 +   0, slider_w, 30), "Valence", -0.5, 0.5, 0.0)
        self.s_aro = _Slider((pad, y0 +  34, slider_w, 30), "Arousal",  0.0, 1.0, 0.5, fmt="{:.2f}")
        self.s_h   = _Slider((pad, y0 +  68, slider_w, 30), "H angle", -90.0, 90.0, 0.0, fmt="{:+.0f}")
        self.s_v   = _Slider((pad, y0 + 102, slider_w, 30), "V angle", -45.0, 45.0, 0.0, fmt="{:+.0f}")
        self.sliders = [self.s_val, self.s_aro, self.s_h, self.s_v]

        # Emotion buttons: 2 columns x 7 rows
        btn_w = (width - 2 * pad - 5) // 2
        btn_h = 22
        emo_y0 = y0 + 140
        self.emotion_buttons = []
        for i, preset in enumerate(EMOTION_PRESETS):
            col = i % 2
            row = i // 2
            bx = pad + col * (btn_w + 5)
            by = emo_y0 + row * (btn_h + 4)
            self.emotion_buttons.append(_Button((bx, by, btn_w, btn_h), preset[0], ("emotion", preset)))

        # Position buttons: 5 -> 2 columns x 3 rows
        pos_y0 = emo_y0 + 7 * (btn_h + 4) + 22
        self.position_buttons = []
        for i, preset in enumerate(POSITION_PRESETS):
            col = i % 2
            row = i // 2
            bx = pad + col * (btn_w + 5)
            by = pos_y0 + row * (btn_h + 4)
            self.position_buttons.append(_Button((bx, by, btn_w, btn_h), preset[0], ("position", preset)))

        self._emo_header_y = emo_y0 - 18
        self._pos_header_y = pos_y0 - 18

        # Outgoing requests (consumed once per frame by the emulator).
        self.pending_emotion = None   # (label, v, a, asym)
        self.pending_position = None  # (label, h, v)
        self.slider_dirty = False
        self.active_emotion = None    # for highlight
        self.active_position = None

    # ----- event handling -----

    def _to_local(self, pos):
        return (pos[0] - self.x, pos[1] - self.y)

    def handle_event(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            lp = self._to_local(event.pos)
            for s in self.sliders:
                if s.hit(lp):
                    s.dragging = True
                    s.set_from_pos(lp)
                    self.slider_dirty = True
                    return
            for b in self.emotion_buttons:
                if b.rect.collidepoint(lp):
                    _, preset = b.action
                    label, v, a, asym = preset
                    self.pending_emotion = (label, v, a, asym)
                    self.active_emotion = label
                    self.s_val.value = v
                    self.s_aro.value = a
                    return
            for b in self.position_buttons:
                if b.rect.collidepoint(lp):
                    _, preset = b.action
                    label, h, v = preset
                    self.pending_position = (label, h, v)
                    self.active_position = label
                    self.s_h.value = h
                    self.s_v.value = v
                    return

        elif event.type == pygame.MOUSEBUTTONUP and event.button == 1:
            for s in self.sliders:
                s.dragging = False

        elif event.type == pygame.MOUSEMOTION:
            lp = self._to_local(event.pos)
            any_drag = False
            for s in self.sliders:
                if s.dragging:
                    s.set_from_pos(lp)
                    self.slider_dirty = True
                    any_drag = True
            if any_drag:
                return
            for b in self.emotion_buttons:
                b.hover = b.rect.collidepoint(lp)
            for b in self.position_buttons:
                b.hover = b.rect.collidepoint(lp)

    # ----- state accessors -----

    def slider_values(self):
        """Slider-derived target state: (valence, arousal, h, v)."""
        return (self.s_val.value, self.s_aro.value,
                int(round(self.s_h.value)), int(round(self.s_v.value)))

    def consume_emotion_request(self):
        req = self.pending_emotion
        self.pending_emotion = None
        return req

    def consume_position_request(self):
        req = self.pending_position
        self.pending_position = None
        return req

    def consume_slider_dirty(self):
        d = self.slider_dirty
        self.slider_dirty = False
        return d

    def sync_from_state(self, valence, arousal, h, v):
        """Reflect externally-driven state in the sliders (auto mode),
        unless the user is currently dragging one of them."""
        if not any(s.dragging for s in self.sliders):
            self.s_val.value = valence
            self.s_aro.value = arousal
            self.s_h.value = h
            self.s_v.value = v

    # ----- rendering -----

    def render(self, mode="auto"):
        self.surface.fill((48, 48, 48))

        title = self.font_title.render("BotiEyes Controls", True, (255, 255, 255))
        self.surface.blit(title, (10, 6))

        mode_color = (180, 220, 120) if mode == "auto" else (220, 180, 120)
        mode_label = self.font_small.render(
            f"Mode: {mode.upper()}  (SPACE=auto)", True, mode_color)
        self.surface.blit(mode_label, (10, self.height - 20))

        for s in self.sliders:
            s.draw(self.surface, self.font)

        self.surface.blit(self.font_title.render("Emotions", True, (200, 200, 200)),
                          (12, self._emo_header_y))
        for b in self.emotion_buttons:
            active = (mode == "manual" and b.label == self.active_emotion)
            b.draw(self.surface, self.font, active=active)

        self.surface.blit(self.font_title.render("Positions", True, (200, 200, 200)),
                          (12, self._pos_header_y))
        for b in self.position_buttons:
            active = (mode == "manual" and b.label == self.active_position)
            b.draw(self.surface, self.font, active=active)

        return self.surface
