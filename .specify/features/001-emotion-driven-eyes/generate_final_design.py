"""
Generate final BotiEyes design: NO EYEBROWS, morphing ellipses only.

Approach F: Extreme morphing + spacing + asymmetry for expression.
No pupils, no highlights, no eyebrows - pure minimalist futuristic design.
"""

from PIL import Image, ImageDraw
import math


class BotiEyes:
    """BotiEyes renderer - Option F: No eyebrows, morphing only."""
    
    def __init__(self):
        self.bg_color = (0, 0, 0)  # Black OLED background
        self.fg_color = (255, 255, 255)  # White eyes
    
    def draw_eye(self, draw, center_x, center_y, params):
        """Draw a single eye with morphing parameters."""
        
        width = params.get('width', 28)
        height = params.get('height', 32)
        y_offset = params.get('y_offset', 0)
        spacing_adjust = params.get('spacing_adjust', 0)
        lid_top = params.get('lid_top', 0.0)
        lid_bottom = params.get('lid_bottom', 0.0)
        lid_angle = params.get('lid_angle', 0.5)
        asymmetry = params.get('asymmetry', 0)
        
        # Adjust position for emotion
        center_y += y_offset
        center_x += spacing_adjust
        
        # Draw base ellipse
        bbox = [center_x - width//2, center_y - height//2,
                center_x + width//2, center_y + height//2]
        draw.ellipse(bbox, fill=self.fg_color)
        
        # Apply eyelid overlays for angular expression
        if lid_top > 0:
            lid_height = int(height * lid_top)
            
            if lid_angle > 0.6:  # Angry-style (sharp angle)
                triangle = [
                    (bbox[0], bbox[1]),
                    (bbox[2], bbox[1]),
                    (bbox[2], bbox[1] + lid_height + asymmetry)
                ]
                draw.polygon(triangle, fill=self.bg_color)
            elif lid_angle < 0.4:  # Sad-style (droop)
                triangle = [
                    (bbox[0], bbox[1]),
                    (bbox[2], bbox[1]),
                    (bbox[0], bbox[1] + lid_height + asymmetry)
                ]
                draw.polygon(triangle, fill=self.bg_color)
            else:  # Neutral - curved overlay
                lid_bbox = [bbox[0], bbox[1] - height//4, 
                           bbox[2], bbox[1] + lid_height]
                draw.ellipse(lid_bbox, fill=self.bg_color)
        
        # Bottom eyelid for happy
        if lid_bottom > 0:
            lid_height = int(height * lid_bottom)
            bottom_bbox = [bbox[0], bbox[3] - lid_height, 
                          bbox[2], bbox[3] + height//2]
            draw.ellipse(bottom_bbox, fill=self.bg_color)
    
    def render_emotion(self, emotion_name, params, width=300, height=150):
        """Render both eyes for a given emotion."""
        
        img = Image.new('RGB', (width, height), self.bg_color)
        draw = ImageDraw.Draw(img)
        
        # Draw left eye
        left_x = width // 3
        left_y = height // 2
        self.draw_eye(draw, left_x, left_y, params)
        
        # Draw right eye (mirror spacing adjustments)
        right_x = 2 * width // 3
        right_y = height // 2
        right_params = params.copy()
        right_params['spacing_adjust'] = -params.get('spacing_adjust', 0)
        self.draw_eye(draw, right_x, right_y, right_params)
        
        # Add emotion label
        draw.text((10, 10), emotion_name.upper(), fill=(200, 200, 0))
        
        return img


def create_emotion_parameters():
    """Define all emotion parameters for BotiEyes - SYMMETRY + CUTENESS OPTIMIZED."""
    return {
        'neutral': {
            'width': 31, 'height': 34,  # Rounder (+3w) + 10% larger
            'lid_top': 0.0, 'lid_bottom': 0.0,
            'lid_angle': 0.5,
            'y_offset': 4, 'spacing_adjust': -2, 'asymmetry': 0  # Lower + closer
        },
        'happy': {
            'width': 33, 'height': 36,  # Rounder (+3w) + 10% larger
            'lid_top': 0.0, 'lid_bottom': 0.50,  # Stronger smile curve
            'lid_angle': 0.5,
            'y_offset': 0, 'spacing_adjust': -2, 'asymmetry': 0  # Closer together
        },
        'sad': {
            'width': 26, 'height': 28,  # Rounder (+4w) + larger, cute-sad
            'lid_top': 0.40, 'lid_bottom': 0.0,  # Softer droop (was 0.35)
            'lid_angle': 0.30,  # Softer angle (was 0.25)
            'y_offset': 6, 'spacing_adjust': -1, 'asymmetry': 0  # Lower + closer
        },
        'angry': {
            'width': 20, 'height': 26,  # Rounder (+4w) + larger, cute-angry
            'lid_top': 0.45, 'lid_bottom': 0.40,  # Softer (was 0.5/0.45)
            'lid_angle': 0.30,  # Softer angle (was 0.2)
            'y_offset': 4, 'spacing_adjust': -3, 'asymmetry': 0  # SYMMETRY FIX (was -5)
        },
        'surprised': {
            'width': 35, 'height': 40,  # Rounder (+3w) + larger, biggest eyes
            'lid_top': 0.0, 'lid_bottom': 0.0,
            'lid_angle': 0.5,
            'y_offset': 1, 'spacing_adjust': -1, 'asymmetry': 0  # Lower + closer
        },
        'anxious': {
            'width': 26, 'height': 38,  # Rounder (+2w) + larger
            'lid_top': 0.08, 'lid_bottom': 0.0,
            'lid_angle': 0.45,
            'y_offset': 2, 'spacing_adjust': 1, 'asymmetry': -1  # Keep asymmetry
        },
        'excited': {
            'width': 33, 'height': 36,  # Rounder (+3w) + larger
            'lid_top': 0.0, 'lid_bottom': 0.18,  # Subtle smile
            'lid_angle': 0.5,  # Softer (was 0.7)
            'y_offset': 0, 'spacing_adjust': -2, 'asymmetry': 0  # SYMMETRY FIX (was 2)
        },
        'tired': {
            'width': 31, 'height': 22,  # Rounder (+3w) + larger
            'lid_top': 0.5, 'lid_bottom': 0.0,
            'lid_angle': 0.5,
            'y_offset': 8, 'spacing_adjust': -2, 'asymmetry': 0  # Lower + closer
        },
        'confused': {
            'width': 26, 'height': 31,  # Rounder (+3w) + larger
            'lid_top': 0.18, 'lid_bottom': 0.08,
            'lid_angle': 0.35,
            'y_offset': 4, 'spacing_adjust': -1, 'asymmetry': -3  # Keep asymmetry
        },
        'thinking': {
            'width': 23, 'height': 30,  # Rounder (+3w) + larger
            'lid_top': 0.12, 'lid_bottom': 0.0,
            'lid_angle': 0.4,
            'y_offset': 3, 'spacing_adjust': 0, 'asymmetry': -2  # Keep asymmetry
        },
        'content': {
            'width': 31, 'height': 32,  # Rounder (+3w) + larger
            'lid_top': 0.0, 'lid_bottom': 0.35,  # Gentle smile
            'lid_angle': 0.5,
            'y_offset': 3, 'spacing_adjust': -2, 'asymmetry': 0  # Lower + closer
        },
        'curious': {
            'width': 32, 'height': 36,  # MUCH rounder (+6w) + larger, peak cuteness
            'lid_top': 0.0, 'lid_bottom': 0.0,  # Wide open (remove lift)
            'lid_angle': 0.5,  # Neutral (was 0.6)
            'y_offset': 2, 'spacing_adjust': -3, 'asymmetry': 0  # SYMMETRY FIX (was 1)
        }
    }


def generate_all_emotions():
    """Generate individual images for each emotion."""
    
    renderer = BotiEyes()
    emotions = create_emotion_parameters()
    
    print("Generating BotiEyes emotions (NO EYEBROWS design)...")
    print("-" * 60)
    
    for emotion_name, params in emotions.items():
        img = renderer.render_emotion(emotion_name, params)
        output_path = f'c:/git/botieyes/.specify/features/001-emotion-driven-eyes/emotion-{emotion_name}.png'
        img.save(output_path)
        print(f"Generated: emotion-{emotion_name}.png")
    
    print("-" * 60)


def generate_master_grid():
    """Generate master grid showing all emotions."""
    
    renderer = BotiEyes()
    emotions = create_emotion_parameters()
    
    # Create grid: 4 columns × 3 rows
    cols = 4
    rows = 3
    cell_width = 300
    cell_height = 150
    
    canvas_width = cols * cell_width
    canvas_height = rows * cell_height + 50
    
    master = Image.new('RGB', (canvas_width, canvas_height), (20, 20, 20))
    draw = ImageDraw.Draw(master)
    
    # Title
    title = "BotiEyes - Final Design (NO EYEBROWS)"
    draw.text((20, 15), title, fill=(255, 255, 255))
    
    subtitle = "Pure morphing ellipses - Minimalist futuristic aesthetic"
    draw.text((20, 35), subtitle, fill=(150, 150, 150))
    
    # Draw emotions in grid
    emotion_list = list(emotions.items())
    for idx, (emotion_name, params) in enumerate(emotion_list):
        row = idx // cols
        col = idx % cols
        
        x_offset = col * cell_width
        y_offset = row * cell_height + 50
        
        # Render emotion
        emotion_img = renderer.render_emotion(emotion_name, params, cell_width, cell_height)
        master.paste(emotion_img, (x_offset, y_offset))
    
    # Save
    output_path = 'c:/git/botieyes/.specify/features/001-emotion-driven-eyes/all-emotions-grid.png'
    master.save(output_path)
    print(f"\nGenerated master grid: all-emotions-grid.png")


def generate_valence_arousal_mapping():
    """Generate visual mapping of emotions on valence-arousal space."""
    
    width = 600
    height = 600
    img = Image.new('RGB', (width, height), (30, 30, 30))
    draw = ImageDraw.Draw(img)
    
    # Draw axes
    center_x = width // 2
    center_y = height // 2
    
    # Arousal axis (vertical)
    draw.line([(center_x, 50), (center_x, height - 50)], fill=(100, 100, 100), width=2)
    draw.text((center_x + 10, 40), "HIGH AROUSAL", fill=(150, 150, 150))
    draw.text((center_x + 10, height - 40), "LOW AROUSAL", fill=(150, 150, 150))
    
    # Valence axis (horizontal)
    draw.line([(50, center_y), (width - 50, center_y)], fill=(100, 100, 100), width=2)
    draw.text((60, center_y + 10), "NEGATIVE", fill=(150, 150, 150))
    draw.text((width - 140, center_y + 10), "POSITIVE", fill=(150, 150, 150))
    
    # Emotion positions (valence, arousal) normalized to -1 to +1
    emotion_positions = {
        'angry': (-0.8, 0.7),
        'anxious': (-0.6, 0.6),
        'surprised': (0.0, 0.8),
        'excited': (0.7, 0.7),
        'happy': (0.8, 0.4),
        'content': (0.6, -0.2),
        'curious': (0.3, 0.3),
        'neutral': (0.0, 0.0),
        'thinking': (0.0, 0.2),
        'confused': (-0.3, 0.3),
        'sad': (-0.6, -0.4),
        'tired': (-0.2, -0.7)
    }
    
    # Draw emotion points
    renderer = BotiEyes()
    emotions = create_emotion_parameters()
    
    for emotion_name, (valence, arousal) in emotion_positions.items():
        # Map to canvas coordinates
        x = center_x + int(valence * (width - 100) / 2)
        y = center_y - int(arousal * (height - 100) / 2)
        
        # Draw small eyes at position
        if emotion_name in emotions:
            mini_img = renderer.render_emotion(emotion_name, emotions[emotion_name], 80, 40)
            mini_img = mini_img.resize((60, 30))
            img.paste(mini_img, (x - 30, y - 15))
            
            # Label
            draw.text((x - 20, y + 18), emotion_name, fill=(200, 200, 0))
    
    # Title
    draw.text((20, 20), "Valence-Arousal Emotion Space", fill=(255, 255, 255))
    
    # Save
    output_path = 'c:/git/botieyes/.specify/features/001-emotion-driven-eyes/valence-arousal-map.png'
    img.save(output_path)
    print(f"Generated valence-arousal map: valence-arousal-map.png")


def main():
    """Generate all final design images."""
    
    generate_all_emotions()
    generate_master_grid()
    generate_valence_arousal_mapping()
    
    print("\n" + "=" * 60)
    print("✓ ALL IMAGES GENERATED SUCCESSFULLY!")
    print("=" * 60)
    print("\nDesign: NO EYEBROWS (SYMMETRY + CUTENESS OPTIMIZED)")
    print("Total emotions: 12")
    print("Primitives per eye: 1-3 (ellipse + 0-2 overlay shapes)")
    print("\nExpressiveness: 8.0/10 (target after symmetry fixes)")
    print("Cuteness: 8.0-8.5/10 (target after shape/position optimization)")
    print("\nKey improvements:")
    print("  SYMMETRY FIXES:")
    print("    - Angry: Removed asymmetry (was -5) - now pure symmetric rage")
    print("    - Excited: Removed asymmetry (was +2) - now symmetric joy")
    print("    - Curious: Removed asymmetry (was +1) - now open symmetric wonder")
    print("    - Kept: Confused (-3), Thinking (-2) for cognitive complexity")
    print("  CUTENESS OPTIMIZATIONS:")
    print("    - 10% larger eyes (baby schema)")
    print("    - Rounder shapes (+3-6px width, aspect ratio ~0.95-1.0)")
    print("    - Positioned lower (+4px baseline) and closer (-2px baseline)")
    print("    - Softer eyelid angles (reduced harsh triangles)")
    print("    - Curious: Peak cuteness with wide round eyes (32×36)")
    print("\nReady for expert re-review.")


if __name__ == "__main__":
    main()
