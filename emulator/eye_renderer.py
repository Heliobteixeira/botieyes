"""
EyeRenderer - Pygame-based Eye Rendering

Matches C++ rendering logic using Pygame primitives.
Renders base ellipse + eyelid overlays with position transformations.
"""

import pygame
import math


class EyeRenderer:
    """Renders eyes with emotion and position (matches C++ renderEyes())"""
    
    def __init__(self, display_width, display_height, scale):
        self.display_width = display_width
        self.display_height = display_height
        self.scale = scale
        
        # Create rendering surface
        self.surface = pygame.Surface((display_width * scale, display_height * scale))
        
        # State
        self.valence = 0.0
        self.arousal = 0.5
        self.h_angle = 0  # Horizontal angle (-90 to +90)
        self.v_angle = 0  # Vertical angle (-45 to +45)
        
        # Eye base positions (centered, two eyes)
        self.eye_spacing = 40  # Base spacing in pixels
    
    def set_emotion(self, valence, arousal):
        """Set current emotion state"""
        self.valence = valence
        self.arousal = arousal
    
    def set_eye_position(self, h_angle, v_angle):
        """Set coupled eye position (both eyes move together)"""
        self.h_angle = max(-90, min(90, h_angle))
        self.v_angle = max(-45, min(45, v_angle))
    
    def render(self):
        """Render current eye state to surface"""
        # Clear surface (black background like OLED)
        self.surface.fill((0, 0, 0))
        
        # TODO: Implement actual rendering with ellipses and eyelids
        # For now, placeholder visualization
        
        # Calculate eye centers with position offset
        h_offset = int(self.h_angle * 0.3)  # Horizontal movement
        v_offset = int(self.v_angle * 0.2)  # Vertical movement
        
        center_y = (self.display_height // 2 + v_offset) * self.scale
        
        left_eye_x = (self.display_width // 2 - self.eye_spacing // 2 + h_offset) * self.scale
        right_eye_x = (self.display_width // 2 + self.eye_spacing // 2 + h_offset) * self.scale
        
        # Simple ellipse rendering (will be replaced with full implementation)
        eye_width = int((20 + self.arousal * 15) * self.scale)
        eye_height = int((22 + self.arousal * 18) * self.scale)
        
        # Draw both eyes (white ellipses on black background)
        pygame.draw.ellipse(self.surface, (255, 255, 255),
                          (left_eye_x - eye_width // 2, center_y - eye_height // 2,
                           eye_width, eye_height), 2)
        pygame.draw.ellipse(self.surface, (255, 255, 255),
                          (right_eye_x - eye_width // 2, center_y - eye_height // 2,
                           eye_width, eye_height), 2)
        
        return self.surface
