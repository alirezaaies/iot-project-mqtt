# Project Report

This directory contains the Persian, implementation-oriented project report. It is not a technology encyclopedia: each chapter records only the decisions, commands, implementation notes, tests, and evidence required to reproduce one project phase.

## Build

Requirements:

- XeLaTeX
- `latexmk`
- a Persian font (`Vazirmatn` is preferred; `Noto Naskh Arabic` is the fallback)
- `DejaVu Sans` and `DejaVu Sans Mono`

From this directory, run:

```bash
make
```

The generated PDF is `main.pdf`, next to `main.tex`. This is also compatible with the normal TeXstudio workflow. Run `make clean` to remove generated files.

## Writing workflow

1. Start the relevant project phase.
2. Copy only the useful sections from `templates/chapter-template.tex` if a new chapter is needed.
3. Record the chosen configuration and commands while implementing.
4. Add the isolated test, expected result, and actual evidence.
5. Mark completion criteria only after the code and test exist.
6. Put reusable figures, tables, and citations in the repository-level directories.

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
