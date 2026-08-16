---
name: Wsprry Pi Console
description: A precise transmitter-appliance interface for configuring, operating, and monitoring WSPR-family radio workflows.
colors:
  transmitter-slate: "#405e7a"
  transmitter-slate-dark: "#344a60"
  slate-muted: "#6b8096"
  signal-text-light: "#ecf1f6"
  signal-text-dark: "#e5ecf2"
  zephyr-primary: "#3459e6"
  body-text-light: "#495057"
  body-bg-light: "#ffffff"
  secondary-bg-light: "#e9ecef"
  border-light: "#dee2e6"
  body-text-dark: "#dee2e6"
  body-bg-dark: "#212529"
  secondary-bg-dark: "#343a40"
  border-dark: "#495057"
  state-success: "#5f8f74"
  state-warning: "#b79256"
  state-danger: "#b66a76"
  state-active: "#c8a85d"
typography:
  display:
    fontFamily: "Barlow Semi Condensed, Segoe UI, sans-serif"
    fontSize: "clamp(1.5rem, 2vw, 2rem)"
    fontWeight: 600
    lineHeight: 1.05
    letterSpacing: "0.012em"
  headline:
    fontFamily: "Barlow Semi Condensed, Segoe UI, sans-serif"
    fontSize: "1.5625rem"
    fontWeight: 600
    lineHeight: 1.12
    letterSpacing: "0.012em"
  title:
    fontFamily: "Barlow Semi Condensed, Segoe UI, sans-serif"
    fontSize: "1.25rem"
    fontWeight: 600
    lineHeight: 1.12
    letterSpacing: "0.012em"
  body:
    fontFamily: "Source Sans 3, Segoe UI, sans-serif"
    fontSize: "1rem"
    fontWeight: 400
    lineHeight: 1.55
  label:
    fontFamily: "Barlow Semi Condensed, Segoe UI, sans-serif"
    fontSize: "0.75rem"
    fontWeight: 700
    lineHeight: 1.3
    letterSpacing: "0.08em"
rounded:
  sm: "0.25rem"
  md: "0.375rem"
  lg: "0.5rem"
  tab: "0.9rem 0.9rem 0 0"
  pill: "999px"
spacing:
  xs: "0.5rem"
  sm: "0.75rem"
  md: "1rem"
  lg: "1.5rem"
  xl: "2rem"
  xxl: "clamp(2.5rem, 4vw, 3.5rem)"
components:
  button-primary:
    backgroundColor: "{colors.zephyr-primary}"
    textColor: "{colors.body-bg-light}"
    rounded: "{rounded.md}"
    padding: "0.5rem 1rem"
  nav-primary:
    backgroundColor: "{colors.transmitter-slate}"
    textColor: "{colors.signal-text-light}"
    rounded: "{rounded.lg}"
    padding: "0.5rem 0.8rem"
  panel:
    backgroundColor: "{colors.body-bg-light}"
    textColor: "{colors.body-text-light}"
    rounded: "{rounded.md}"
    padding: "1.05rem 1.15rem"
  chip:
    backgroundColor: "{colors.body-bg-light}"
    textColor: "{colors.body-text-light}"
    rounded: "{rounded.pill}"
    padding: "0.45rem 0.75rem"
  footer-state-badge:
    backgroundColor: "{colors.transmitter-slate}"
    textColor: "{colors.signal-text-light}"
    rounded: "{rounded.sm}"
    padding: "0.05rem 0.35rem"
---

# Design System: Wsprry Pi Console

## 1. Overview

**Creative North Star: "The Bench Instrument"**

Wsprry Pi Console is an operational control surface for technically capable ham radio operators. It should feel like a clean bench instrument: compact, readable, and specific, with clear state changes and enough typographic hierarchy to scan transmitter status, configuration, logs, maintenance, and spots without theatrical styling.

The system uses Bootstrap 5 with the Zephyr theme as its base, then narrows the personality through a custom slate transmitter accent, local Source Sans 3 and Barlow Semi Condensed fonts, restrained tonal panels, and explicit state color tokens. `PRODUCT.md` says the interface should feel "trustworthy and utilitarian rather than decorative"; that sentence is the governing rule for new screens.

**Key Characteristics:**

- Precise, clean, technical, and operational.
- Dense enough for repeated appliance use, but not visually cramped.
- Light and dark themes share the same layout and interaction model.
- State, mode, and transmission context are visually more important than brand expression.

## 2. Colors

The palette is restrained: a transmitter-slate shell, neutral Bootstrap surfaces, Zephyr primary for default Bootstrap actions, and muted operational state colors for live system feedback.

### Primary

- **Transmitter Slate** (#405e7a): The custom navigation and appliance accent in light theme, defined as `--wspr-accent-rgb: 64, 94, 122`. Use for shell-level chrome, not large decorative fills.
- **Low-Light Transmitter Slate** (#344a60): The dark-theme navigation accent, defined as `--wspr-accent-rgb: 52, 74, 96`.
- **Zephyr Action Blue** (#3459e6): Bootstrap's primary action and link color. Use where native Bootstrap components already expect `--bs-primary`; do not replace the transmitter-slate shell with this brighter blue.

### Secondary

- **Slate Muted** (#6b8096): Soft accent and focus-ring source, defined as `--wspr-accent-soft-rgb: 107, 128, 150`. Use for focus affordances and subtle active treatment.

### Tertiary

- **Signal Active Gold** (#c8a85d): Connected or transmitting-ready status, defined as `--wspr-state-active`. Reserve for live radio state, not decoration.

### Neutral

- **Light Body Text** (#495057): Default Zephyr body text in light mode.
- **Light Canvas** (#ffffff): Bootstrap body background in light mode.
- **Light Secondary Surface** (#e9ecef): Subtle surface mix-in for panels and backgrounds.
- **Light Border** (#dee2e6): Standard divider and panel stroke.
- **Dark Body Text** (#dee2e6): Default body text in dark mode.
- **Dark Canvas** (#212529): Bootstrap body background in dark mode.
- **Dark Secondary Surface** (#343a40): Subtle surface mix-in for dark panels.
- **Dark Border** (#495057): Standard dark-mode divider and panel stroke.
- **Signal Text Light** (#ecf1f6) and **Signal Text Dark** (#e5ecf2): Text colors inside transmitter-slate navigation and alert surfaces.

### Named Rules

**The State Color Rule.** Success (#5f8f74), warning (#b79256), danger (#b66a76), and active (#c8a85d) are operational vocabulary. Do not use them for decorative emphasis.

**The Slate Shell Rule.** The navigation bar owns the transmitter-slate identity. Interior panels should stay neutral, with `color-mix()` tints only where they clarify status or grouping.

**The Slate State Badge Rule.** Warning and danger text on transmitter slate must not use raw state colors as foregrounds. Use signal text (#ecf1f6 light, #e5ecf2 dark) on a state-tinted slate badge so footer status remains WCAG AA readable.

## 3. Typography

**Display Font:** Barlow Semi Condensed, with Segoe UI and sans-serif fallbacks  
**Body Font:** Source Sans 3, with Segoe UI and sans-serif fallbacks  
**Label/Mono Font:** Barlow Semi Condensed for labels; Bootstrap's monospace stack only for code or logs

**Character:** The pairing reads technical without becoming terminal-like. Barlow Semi Condensed gives headings, nav labels, and mode controls an instrument-panel cadence; Source Sans 3 keeps long help text, logs, tables, and form guidance readable.

### Hierarchy

- **Display** (600, `clamp(1.5rem, 2vw, 2rem)`, 1.05): Page-level configuration titles and major state values.
- **Headline** (600, `1.5625rem`, 1.12): Navbar page title and high-level view headings.
- **Title** (600, `1.25rem`, 1.12): Card titles and section headers.
- **Body** (400, `1rem`, 1.55): Forms, operational summaries, alerts, and normal copy. Paragraphs and legends should stay near 68ch maximum.
- **Label** (700, `0.75rem`, 0.08em, uppercase): Eyebrows, state labels, panel labels, and compact technical metadata.

### Named Rules

**The Label Discipline Rule.** Uppercase Barlow labels are for scan anchors only. Do not apply them to paragraphs, helper text, or button bodies.

## 4. Elevation

The system uses a hybrid of light structural shadows and tonal layering. Bootstrap cards, tables, list groups, and modals carry a small two-layer shadow (`0 1px 3px 0 rgba(0,0,0,.1), 0 1px 2px 0 rgba(0,0,0,.06)`), while interior operational panels rely on borders and `color-mix()` surface tints instead of additional shadow.

### Shadow Vocabulary

- **Base Surface Shadow** (`box-shadow: 0 1px 3px 0 rgba(0,0,0,.1), 0 1px 2px 0 rgba(0,0,0,.06)`): Cards, tables, list groups, and modals.
- **Navbar Shadow** (`box-shadow: 0 0.5rem 1.35rem rgba(20, 27, 35, 0.12)`): Fixed top navigation only.
- **Alert Shadow** (`box-shadow: 0 0.35rem 0.8rem rgba(0,0,0,0.16)`): Temporary connection alert banner.
- **Focus Ring** (`box-shadow: 0 0 0 0.2rem rgba(var(--wspr-accent-soft-rgb), 0.22)`): Keyboard focus on nav, buttons, fields, selects, and switches.

### Named Rules

**The Interior Flatness Rule.** Panels inside cards should use a 1px border, neutral tint, and spacing before they use another shadow.

## 5. Components

### Buttons

- **Shape:** Bootstrap medium radius (`0.375rem`) unless a control is a segmented tab, chip, or switch.
- **Primary:** Bootstrap `btn-primary` uses Zephyr Action Blue (#3459e6) with standard Bootstrap contrast and `0.5rem 1rem` action padding in custom operational actions.
- **Hover / Focus:** Hover follows Bootstrap or the component's explicit background/border transition. Focus uses the slate-derived `--focus-ring`.
- **Secondary / Ghost / Tertiary:** Outline and link buttons are acceptable for utility actions. Destructive actions use Bootstrap danger, but only for real stop, shutdown, delete, or failure contexts.

### Chips

- **Style:** Hostname and signal status chips use pill radius (`999px`), compact padding, neutral or transmitter-slate-tinted backgrounds, and medium-weight text.
- **State:** Use chips for compact metadata and live status only. They should not replace form controls or table filters.

### Cards / Containers

- **Corner Style:** Bootstrap card radius (`0.375rem`), with `shadow-sm` on the outer page card.
- **Background:** Bootstrap body background for primary cards; interior panels use `color-mix(in srgb, var(--bs-body-bg) 91%, var(--bs-secondary-bg) 9%)` or nearby mixes.
- **Shadow Strategy:** The page card may use the base surface shadow. Interior panels stay flat.
- **Border:** 1px `var(--bs-border-color)` or a subtle `color-mix()` border.
- **Internal Padding:** Operational panels use about `1.05rem 1.15rem`; page spacing follows `--page-space-*` from `0.5rem` to `clamp(2.5rem, 4vw, 3.5rem)`.

### Inputs / Fields

- **Style:** Bootstrap form controls and selects, with extra right padding for validation affordances where needed.
- **Focus:** Slate-derived focus ring across `.form-control`, `.form-select`, and `.form-check-input`.
- **Error / Disabled:** Validation messages appear below the field, wrap long technical values, and hide when the field is disabled.

### Navigation

- **Style:** Fixed top navbar in transmitter slate, with Barlow page title, Source Sans kicker, wrapped nav links, and compact Bootstrap icons.
- **Active State:** `rgba(var(--wspr-accent-text-rgb), 0.12)` background, 1px translucent border, and a subtle inset line.
- **Mobile Treatment:** Navbar collapses into a full-width stacked menu, preserving icon/text alignment and allowing long labels to wrap.

### Footer Status

- **Style:** Fixed bottom footer uses transmitter slate with signal text for base content.
- **Update Available:** Render version/update indicators as compact state badges using signal text on `color-mix(in srgb, var(--wspr-state-warning) 30%, rgb(var(--wspr-accent-rgb)) 70%)`, with a 1px full-border state tint.
- **Update Failed:** Use the same badge structure with `--wspr-state-danger`; do not place raw danger text directly on the slate footer.
- **Hover / Focus:** Keep signal text and deepen the state tint only slightly, around 32% state color, so warning/danger links remain above 4.5:1 contrast.

### Tabs and Segmented Controls

- **Style:** Configuration tabs use Barlow Semi Condensed, `0.9rem 0.9rem 0 0` top corners, strong min-height, and active tonal background.
- **State:** Active segments use subtle primary/body color mixes, not saturated fills.

### Operational Panels

- **Style:** Grid-based panels with uppercase labels, semibold values, neutral tint, 1px border, and wrap-safe values.
- **State:** Live transmitter state values may use success, warning, danger, or primary mixes, always tied to actual backend state.

### Tables

- **Style:** Bootstrap tables with fixed layout for spots, sticky headers, uppercase table headings, and 0.875rem table text.
- **Behavior:** Long call signs, grid locators, frequencies, and messages should wrap safely rather than overflow.

## 6. Do's and Don'ts

### Do:

- **Do** preserve both light and dark themes with the same layout and interaction model.
- **Do** use transmitter slate (#405e7a light, #344a60 dark) for shell identity and status chrome.
- **Do** use Source Sans 3 for normal reading and Barlow Semi Condensed for headings, labels, tabs, and state values.
- **Do** keep controls and terminology direct, consistent, and pragmatic.
- **Do** make system state, mode, and transmission context easy to scan quickly.
- **Do** let technical values wrap with `overflow-wrap: anywhere` when they can be long or user supplied.
- **Do** use 1px borders and neutral surface mixes for interior grouping before adding more shadows.
- **Do** render footer warning and danger states as accessible state badges, not raw colored text on slate.

### Don't:

- **Don't** make the interface flashy, ornamental, or marketing-like.
- **Don't** simplify technical workflows into vague abstractions when precise controls are needed.
- **Don't** use state colors for decoration; keep them tied to actual success, warning, danger, active, loading, or offline states.
- **Don't** replace the restrained transmitter-slate shell with saturated blue across the whole interface.
- **Don't** create nested cards or repeated identical card grids for dense operational screens.
- **Don't** use gradient text, decorative glassmorphism, or colored side-stripe borders.
- **Don't** use modal dialogs as the first answer for normal configuration flows; prefer inline progressive controls.
- **Don't** put low-contrast warning or danger foreground colors directly on the transmitter-slate footer.

### Excluded Pages

The following files must NOT be modified by automated tools (including Codex + Impeccable):

- data/view_diag_logs.php

These pages are considered stable or externally managed.
