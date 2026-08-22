# Project Report

This directory contains the Persian practical guide. It documents implemented
work in full and keeps future work as a short roadmap. A new chapter is added
only when implementation of that phase begins.

## Build

Requirements:

- XeLaTeX
- `Arial` for Persian text and `Consolas` for code on the current machine

From this directory, run:

```bash
make
```

The generated PDF is `main.pdf`, next to `main.tex`. This is also compatible with the normal TeXstudio workflow. Run `make clean` to remove generated files.

The selected fonts are the only machine-specific settings. On Linux or macOS,
install those fonts or replace the two `set...font` lines in `config.tex` with
the local font names. No chapter file needs to change.

## Writing workflow

1. Start one small project phase.
2. Add a chapter only when that phase has real implementation work.
3. Record exact configuration and commands while implementing.
4. Add the isolated test, expected result, and observed evidence.
5. Compile with XeLaTeX and visually inspect the generated PDF.

## Content rules

- Write report content in Persian; keep technical identifiers in English.
- Prefer exact commands, paths, payloads, and observed results over general background.
- Cite primary documentation for protocol or hardware facts.
- Do not duplicate component setup instructions from a component README; reference them and record project-specific results here.
- Keep planned work clearly separate from completed work.

## Persian and English text

Wrap every English word or phrase inside Persian text with `\lr{...}`. Put the English fragment on a separate source line, then continue the Persian text on the next source line. For example:

```tex
این بخش برای ارتباط با
\lr{MQTT}
استفاده می‌شود.
```

Wrap a complete English paragraph or a code block in an `LTR` environment. This prevents direction and font problems in Persian output.
