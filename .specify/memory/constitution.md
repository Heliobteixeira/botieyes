<!--
SYNC IMPACT REPORT
==================
Version Change: Initial → 1.0.0 → 1.1.0 → 2.0.0 → 3.0.0 → 4.0.0 (MAJOR)
Modified Principles:
  - ADDED three new foundational principles (MAJOR change):
    I. Pragmatic Simplicity (avoid over-engineering, justify complexity)
    II. Maintainable Code (modularity, readability, refactorability)
    VIII. Continuous Learning (document clarifications, integrate feedback)
  - REORDERED principles by priority (simplicity and maintainability first)
  - Previous principles renumbered: Performance → III, Hardware Abstraction → IV, etc.
  - Updated governance with new priority order and learning integration note
Added Sections: Learning Integration in governance
Removed Sections: None
Templates Requiring Updates:
  - ✅ All templates already aligned
Follow-up TODOs: None
-->

# BotiEyes Constitution

This constitution defines the core architectural principles and governance for BotiEyes.

## Core Principles

### I. Pragmatic Simplicity
Every implementation must have a clear justification. Start with the simplest solution that works; add complexity only when necessary and documented. Avoid over-engineering—YAGNI (You Aren't Gonna Need It) is a core value.

### II. Maintainable Code
Code is written once but read many times. Prioritize modularity, human readability, and ease of refactoring over cleverness. Clear structure and naming enable future contributors to understand and evolve the codebase.

### III. Performance-First Design
Performance is a first-class concern. Smooth animations are essential to emotional expressiveness. Profile before optimizing; measure real bottlenecks.

### IV. Hardware Abstraction
Core emotion and animation logic is independent of display hardware. Platform-specific code lives in adapters. This enables cross-platform development and PC emulation.

### V. Emotion-Driven Design
Expressions derive from a continuous valence-arousal emotional model, enabling smooth interpolation between states. This provides natural, coherent emotional expression.

### VI. Cross-Platform Emulation
PC-based emulation is a first-class development target for rapid iteration and accessibility. No hardware required to contribute.

### VII. Extensible Architecture
The architecture supports extension without core modifications. Clear extension points for emotion models, rendering backends, and input processors.

### VIII. Continuous Learning
User clarifications and feedback are valuable. Document insights, review against specifications, and integrate learnings into project memory for future consistency.

## Governance

**Principle Priority** (when principles conflict):
1. Pragmatic Simplicity
2. Maintainable Code
3. Hardware Abstraction
4. Performance-First
5. Emotion-Driven
6. Cross-Platform Emulation
7. Extensible Architecture
8. Continuous Learning

**Amendments**: Require 2/3 vote from active maintainers (commit access in past 90 days). Voting period: minimum 7 days.

**Scope**: This constitution covers architectural principles and governance. Implementation details (metrics, procedures, tooling) are not constitutional.

**Learning Integration**: Clarifications, insights, and design decisions are documented in project memory (`.specify/memory/`) and reviewed against specifications to ensure consistency across features.

---

**Version**: 4.0.0 | **Ratified**: 2026-04-16 | **Last Amended**: 2026-04-16
