# AXE documentation

| File | Contents |
|---|---|
| `AXE-Reference.md` | The Library Reference, editable source of the PDF |
| `reference-template.html`, `reference.css` | Page layout of the PDF: pandoc template and print stylesheet |
| `linebreaks.lua` | pandoc filter for formats other than HTML, e.g. Word |
| `AXE-Reference-v2.2.pdf` | The published v2.2 reference, generated from `AXE-Reference.md` with `make docs` |
| `AXE-Reference-v2.1.pdf` | The v2.1 reference, made with Microsoft Word before the Markdown source existed |
| `AXE-Reference-Review.md` | Review of the v2.1 reference against the code, the fixes, and how each finding was resolved |

## Editing the reference

`AXE-Reference.md` is GitHub-flavored Markdown, so GitHub shows it the way the PDF prints it:

- Tables are pipe tables. A line break inside a cell is `<br>`, and a `|` inside a cell, also inside a code span, is written `\|`.
- Footnotes are `[^name]` with the text in `[^name]: text`; they print at the end of their top level section.
- C++ code is in ```` ```cpp ```` fenced blocks.
- The title, subtitle and the copyright line printed at the bottom of every page are in the front matter at the top of the file.

## Building the PDF

Requirements:

- [pandoc](https://pandoc.org/installing.html) 2.19 or later;
- a Chromium-based browser 131 or later: Google Chrome, Chromium or Microsoft Edge. It prints the page footer and numbers with CSS `@page` margin boxes, which older versions ignore.

### Linux and macOS

```sh
make docs                                         # build/docs/AXE-Reference.html and build/docs/AXE-Reference.pdf
make docs CHROME="/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"
make docs CHROME_FLAGS=--no-sandbox               # when running as root, e.g. in a container
```

`make docs` finds `google-chrome`, `chromium`, `chromium-browser` or `microsoft-edge` on the `PATH`; `CHROME=` names another browser. It doesn't need a C++ compiler or Yadro.

To publish a new version, update the title and copyright line in the front matter of `AXE-Reference.md`, run `make docs`,
and copy `build/docs/AXE-Reference.pdf` to `docs/AXE-Reference-v<version>.pdf`.

### Windows

In PowerShell, from the repository root:

```powershell
New-Item -ItemType Directory -Force build\docs | Out-Null
pandoc docs/AXE-Reference.md -f gfm -t html5 --standalone --embed-resources `
    --template=docs/reference-template.html --css=docs/reference.css `
    --toc --toc-depth=2 --reference-location=section --highlight-style=tango -o build/docs/AXE-Reference.html
& "${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe" --headless --disable-gpu `
    --no-pdf-header-footer --generate-pdf-document-outline `
    --print-to-pdf="$PWD\build\docs\AXE-Reference.pdf" "$PWD\build\docs\AXE-Reference.html"
```

### Word

pandoc also writes a Word document from the same source, e.g. for review with tracked changes:

```sh
pandoc docs/AXE-Reference.md -f gfm --toc --lua-filter=docs/linebreaks.lua -o build/docs/AXE-Reference.docx
```

Changes made in Word have to be carried back to `AXE-Reference.md`, which stays the source of the PDF.
