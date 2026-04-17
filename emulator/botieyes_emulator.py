"""
BotiEyes PC Emulator - Main Entry Point

Provides real-time visualization and control of BotiEyes library behavior
for development, testing, and AI integration workflows.

Features:
- Interactive sliders for valence, arousal, H/V position
- Emotion helper buttons (happy, sad, angry, etc.)
- Position helper buttons (lookLeft, lookRight, etc.)
- PNG frame capture for multimodal AI feedback
- JSON state export for debugging
- Matches embedded C++ rendering exactly

Usage:
    python botieyes_emulator.py
"""

import pygame
import sys
from emotion_mapper import EmotionMapper
from eye_renderer import EyeRenderer
from ui_controls import UIControls


class BotiEyesEmulator:
    """Main emulator application"""
    
    def __init__(self, width=128, height=64):
        pygame.init()
        
        # Display configuration
        self.display_width = width
        self.display_height = height
        self.scale = 8  # Scale factor for visibility (128x64 → 1024x512)
        
        # Create window
        window_width = self.display_width * self.scale + 300  # Extra space for UI
        window_height = self.display_height * self.scale
        self.screen = pygame.display.set_mode((window_width, window_height))
        pygame.display.set_caption("BotiEyes Emulator - Emotion-Driven Eye Animation")
        
        # Initialize components
        self.mapper = EmotionMapper()
        self.renderer = EyeRenderer(self.display_width, self.display_height, self.scale)
        self.ui = UIControls(self.display_width * self.scale, 0, 300, window_height)
        
        # State
        self.running = True
        self.clock = pygame.time.Clock()
    
    def handle_events(self):
        """Process user input events"""
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                self.running = False
            
            # Let UI controls handle events
            self.ui.handle_event(event)
    
    def update(self):
        """Update emulator state"""
        # Get current values from UI
        valence, arousal, h_angle, v_angle = self.ui.get_values()
        
        # Map emotion to expression parameters
        expression = self.mapper.map_emotion_to_expression(valence, arousal)
        
        # Update renderer
        self.renderer.set_emotion(valence, arousal)
        self.renderer.set_eye_position(h_angle, v_angle)
    
    def render(self):
        """Render current frame"""
        # Clear screen
        self.screen.fill((32, 32, 32))  # Dark gray background
        
        # Render eyes to display surface
        eye_surface = self.renderer.render()
        self.screen.blit(eye_surface, (0, 0))
        
        # Render UI controls
        ui_surface = self.ui.render()
        self.screen.blit(ui_surface, (self.display_width * self.scale, 0))
        
        # Update display
        pygame.display.flip()
    
    def run(self):
        """Main emulator loop"""
        while self.running:
            self.handle_events()
            self.update()
            self.render()
            self.clock.tick(60)  # 60 FPS
        
        pygame.quit()
        sys.exit()


if __name__ == "__main__":
    emulator = BotiEyesEmulator()
    emulator.run()
