"""
EmotionMapper - Valence-Arousal to Expression Parameters

Ports the C++ EmotionMapper logic for PC emulator.
Maps continuous valence-arousal coordinates to visual expression parameters.

Valence: -0.5 (negative) to +0.5 (positive)
Arousal: 0.0 (calm) to 1.0 (excited)
"""


class ExpressionParameters:
    """Expression visual parameters (matches C++ struct)"""
    
    def __init__(self):
        self.eye_width = 28        # 20-35 pixels
        self.eye_height = 30       # 22-40 pixels
        self.lid_top_coverage = 0.0     # 0.0-0.5
        self.lid_bottom_coverage = 0.0  # 0.0-0.5
        self.y_offset = 0          # -5 to +8 pixels
        self.spacing_adjust = 0    # -4 to +4 pixels
        self.asymmetry = 0.0       # -0.5 to +0.5


class EmotionMapper:
    """Maps valence-arousal to expression parameters (shape-based finalized design)"""
    
    def map_emotion_to_expression(self, valence, arousal):
        """
        Convert valence-arousal coordinates to expression parameters.
        
        Args:
            valence: -0.5 (negative) to +0.5 (positive)
            arousal: 0.0 (calm) to 1.0 (excited)
        
        Returns:
            ExpressionParameters with computed visual settings
        """
        params = ExpressionParameters()
        
        # Clamp inputs
        valence = max(-0.5, min(0.5, valence))
        arousal = max(0.0, min(1.0, arousal))
        
        # Eye dimensions (arousal-driven)
        # High arousal → wider, taller eyes
        # Low arousal → narrower, shorter eyes
        params.eye_width = int(20 + arousal * 15)     # 20-35 pixels
        params.eye_height = int(22 + arousal * 18)    # 22-40 pixels
        
        # Eyelid coverage (valence-driven for minimalist design)
        if valence < 0:
            # Negative emotions: top lid droops
            params.lid_top_coverage = abs(valence)    # 0.0-0.5
            params.lid_bottom_coverage = 0.0
        else:
            # Positive emotions: bottom lid curves up (smile effect)
            params.lid_top_coverage = 0.0
            params.lid_bottom_coverage = valence * 0.6  # 0.0-0.3
        
        # Vertical offset (arousal-driven with valence modifier)
        # High arousal + positive valence → eyes up
        # Low arousal → eyes down
        params.y_offset = int((arousal - 0.5) * 8 + valence * 6)  # -5 to +8
        
        # Spacing adjustment (arousal-driven)
        # High arousal → eyes closer (intensity)
        # Low arousal → eyes farther apart (relaxed)
        params.spacing_adjust = int((0.5 - arousal) * 8)  # -4 to +4
        
        # Asymmetry (reserved for specific emotions like thinking/confused)
        # Default: 0.0 (symmetric)
        params.asymmetry = 0.0
        
        return params
