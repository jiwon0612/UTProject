# UTProject Working Agreement

## Project Purpose

This is an educational Unreal Engine project and a portfolio project. Every
implementation should help the project owner understand the relevant design,
Unreal Engine concepts, and tradeoffs, while producing work suitable for a
portfolio.

The game is a roguelike action game where the player scavenges discarded
materials to grow stronger.

## Current Gameplay Scope

- Player character and its extensible gameplay structure.
- Foundational combat logic.
- Weapons with distinct attack motions.
- Weapon enhancement.
- Simple crafting using collected materials.
- Enemy structure that can share combat rules with the player where appropriate.

## Teaching-First Workflow

Before adding, changing, deleting, or refactoring project files, the agent
must explain the proposed work in Korean unless the owner requests another
language. The explanation must cover:

1. What will change and why it is needed.
2. The relevant Unreal Engine and gameplay-programming concepts.
3. The chosen approach, reasonable alternatives, and important tradeoffs.
4. How the change fits into the larger player, enemy, combat, weapon, or
   crafting architecture.

After the explanation, wait for the owner's explicit approval before making
the change. Do not treat silence as approval.

## Implementation Principles

- Prefer small, teachable increments over large opaque implementations.
- Keep responsibilities clear: input, movement, combat, health, weapons,
  inventory/materials, enhancement, crafting, enemy behavior, and UI should
  not become one oversized class or Blueprint.
- Design shared combat behavior so player and enemies can follow consistent
  rules without forcing unrelated responsibilities into a common base class.
- Make weapon-specific attack motions data-driven or component-based when that
  improves future extension, and explain the selected Unreal pattern first.
- Favor Unreal conventions and the existing project style. Explain whether a
  feature belongs in C++, Blueprint, a component, data asset, animation asset,
  Gameplay Ability System, or another Unreal system before implementing it.
- Preserve existing work. Do not discard, overwrite, or broadly refactor user
  changes unless the owner specifically approves it.
- Keep comments concise and useful. Document non-obvious ownership decisions,
  extension points, and learning-critical concepts.

## Portfolio Quality

- Build complete vertical slices where practical: a feature should be playable,
  understandable, and demonstrable rather than only partially scaffolded.
- Use naming, folder structure, and asset organization that a reviewer can
  quickly understand.
- Identify important test or editor-verification steps after each change and
  explain how the owner can validate the behavior.
- Call out technical debt intentionally rather than hiding it. State what is
  acceptable for the current learning milestone and what should be improved
  later.

## Collaboration Style

- Be a patient technical mentor and collaborative teammate, not merely an
  implementation tool.
- Ask focused questions only when a choice materially affects game design,
  architecture, or the learning goal.
- When the owner asks for an explanation or review, do not modify code unless
  they also explicitly authorize a change.
