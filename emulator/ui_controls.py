"""
UIControls - Interactive Control Panel

Provides sliders and buttons for controlling valence, arousal, and eye position.
Includes emotion helper buttons and position helper buttons.
"""

import pygame


class UIControls:
    """UI control panel for emulator"""
    
    def __init__(self, x, y, width, height):
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        
        # Create UI surface
        self.surface = pygame.Surface((width, height))
        
        # Current values
        self.valence = 0.0      # -0.5 to +0.5
        self.arousal = 0.5      # 0.0 to 1.0
        self.h_angle = 0        # -90 to +90
        self.v_angle = 0        # -45 to +45
        
        # UI state
        self.dragging_slider = None
        self.font = pygame.font.SysFont('Arial', 14)
    
    def handle_event(self, event):
        """Handle mouse/keyboard events"""
        # TODO: Implement slider dragging
        # TODO: Implement button clicks for emotion helpers
        pass
    
    def get_values(self):
        """Get current control values"""
        return self.valence, self.arousal, self.h_angle, self.v_angle
    
    def render(self):
        """Render UI controls"""
        # Clear surface
        self.surface.fill((48, 48, 48))  # Medium gray background
        
        # TODO: Draw sliders
        # TODO: Draw emotion helper buttons
        # TODO: Draw position helper buttons
        # TODO: Draw current values
        
        # Placeholder text
        title = self.font.render("BotiEyes Controls", True, (255, 255, 255))
        self.surface.blit(title, (10, 10))
        
        valence_text = self.font.render(f"Valence: {self.valence:.2f}", True, (200, 200, 200))
        self.surface.blit(valence_text, (10, 40))
        
        arousal_text = self.font.render(f"Arousal: {self.arousal:.2f}", True, (200, 200, 200))
        self.surface.blit(arousal_text, (10, 60))
        
        h_text = self.font.render(f"H Angle: {self.h_angle}°", True, (200, 200, 200))
        self.surface.blit(h_text, (10, 80))
        
        v_text = self.font.render(f"V Angle: {self.v_angle}°", True, (200, 200, 200))
        self.surface.blit(v_text, (10, 100))
        
        return self.surface
