# Adding a language

Type: guide · Audience: developers and translators · Status: current
Verified against: `178fe60f`

A build has one language, and everything on screen follows it: the game's own screens (dialogue,
mini-games, berries, contests, credits) and the native interface. The repository's sources stay in
English; a translated build is made from data only, with no change to the C code or the tools. Only
Latin-script languages are supported.

## What a language is made of

Everything a language brings lives in one row of `tools/locales/languages.json` and one folder,
`tools/locales/<code>/`:

| Piece | Where | What |
|---|---|---|
| Registry row | `tools/locales/languages.json` | `code` (folder and `VITA_LOCALE` value), `gameLanguage` (the game's `LANGUAGE_*`), `romCode` (4th letter of the cartridge's game code), `romSha1`, `source` (`rom`: the game text comes from the player's official cartridge; `catalog`: from files in the folder), `base`, `letters` (letters beyond ASCII its fonts need), `units` (`metric` or `imperial`) |
| Interface text | `ui.json` | Every English string of the native interface (`tools/locales/en/ui.json`) and its translation |
| Game text | `manifest.json.gz` | For a `rom` language: which spans of the sources hold text and where that text lives in the official ROM. No game text is stored |
| Layout fixes | `patches/` | Patches for the game's screens whose windows or word order were made for English text, applied only to that language's builds |

## Steps

1. **Register it.** Add its row to `tools/locales/languages.json`. List in `letters` every letter
   beyond ASCII its text uses (`é É · ×` are always there). Then bake them from the real fonts:
   `python tools/ui2/bake_locale_glyphs.py` (needs a Chromium browser and the internet) and commit
   `tools/ui2/fonts/locale_glyphs.*`.
2. **Translate the interface.** Copy the keys of `tools/locales/en/ui.json` into
   `tools/locales/<code>/ui.json` as `{"strings": {"English": "Translation"}}`. Keep every `%s`,
   `%u`, `%d` in the same order; translate a sentence with a `%s` as a whole, putting the words in
   the language's own order. A missing entry is shown in English.
3. **Game text.** For a `rom` language, make its manifest: `python tools/locales/remap_manifest.py
   --locale <code> --source <manifest> --reference <folder> --symbols <symbols>` imports one made
   for another tree (this is how the Spanish one came from Emerald 3DS, see `NOTICE.md`). See
   "Importing a manifest" below. A `catalog` language (no official cartridge: its game text kept in
   its folder, over a ROM of its `base` language) is not staged yet: `tools/locales/localize.py`
   only handles `rom` languages.
4. **Build it.** `.\build_vita.ps1 -Locale <code> -Rom <your ROM of that language>` builds in
   `build-vita-<code>\`. `tools/locales/compile_ui.py` stops the build on a translation whose
   `printf` conversions differ from the English ones or that uses a letter not listed for it, and
   writes `build-vita-<code>/ui2/locale.bin.txt` with the missing strings and the ones much wider
   than the English text.
5. **Fix the layout.** Play it. Where a game screen cuts or overlaps the longer text, or puts words
   in the wrong order, add a patch to `tools/locales/<code>/patches/`; for the native interface,
   shorten the translation.
6. **Test on the console** with the checklist of [UI_HARDWARE_TESTS.md](UI_HARDWARE_TESTS.md),
   plus the first gyms, a contest, a berry tag and the credits.

## Importing a manifest

Another project's manifest counts its spans in *its* sources, so `remap_manifest.py` needs those
sources (`--reference`) to read the text of each span, then finds that same text in ours. Only the
files whose text differs from ours are needed: for the others the tool uses our own file. For
Emerald 3DS (how the Spanish manifest was made):

1. List the files its manifest covers whose text differs from ours (compare each file's `sha256`
   in the manifest with our file read with LF line endings).
2. Fetch those files from pret/pokeemerald at the commit its `upstream.lock` pins.
3. Apply its `patches/pokeemerald/*.patch` in order, only to those files
   (`git apply --include=<file>`), after converting the patches to LF line endings.
4. Put the language's own layout patches in `tools/locales/<code>/patches/` first (the manifest is
   remapped onto our sources *with* them applied), then run the import with that folder as
   `--reference`, the project's manifest as `--source` and its ROM symbol list (`*-symbols.s.gz`)
   as `--symbols`, which gives the offsets of the Easy Chat ID lists.

The rebuilt files do not need to match the other project's sources byte for byte (Emerald 3DS's
did not): a span is only placed where its exact text and its surroundings are found once in our
file, and the tool lists any span it could not place. Line endings must be LF everywhere: the spans
count characters. Commit the code the manifest is made from first, then the manifest, so its
`sourceCommit` holds the language's patches (the later remaps read them from git).

## When the English sources change

`tools/locales/localize.py` refuses to build a language whose manifest was made for other
sources, and lists the files. Run `python tools/locales/remap_manifest.py --locale <code>`: it
finds every span again by its exact text and surroundings, so an edit never lands on other text,
and lists any span it cannot place (that text stays English). When the interface text changes,
`python tools/locales/extract_ui.py` updates `tools/locales/en/ui.json` (the build checks it is
current) and reports, per language, the strings left to translate.
