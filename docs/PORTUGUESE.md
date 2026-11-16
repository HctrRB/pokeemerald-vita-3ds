# Brazilian Portuguese build

The official source tree and default game text remain in English. The Vita
**UI LANGUAGE** setting changes only the native Vita UI and title artwork:
it does **not** swap GBA game scripts at runtime.

For a complete PT-BR game-text build, use the separately staged sources:

```sh
python tools/build_ptbr.py --stage-dir ../pokeemerald-ptbr-build
```

The script first validates the versioned PT-BR catalogs in
`tools/locales/pt_br/` against their original English strings, then copies
the English tree and game assets to the requested sibling directory, and
applies the catalogs to that copy. The original checkout is not modified.

Build the staged tree with the project's standard toolchain:

```powershell
cd ..\pokeemerald-ptbr-build
.\build_vita.ps1
```

Alternatively, add `--build` to the staging command to automatically invoke
the standard platform build script after staging.

The normal `build_vita.ps1` command in an untouched checkout continues to
produce the original English game with English UI by default.

Notes:
- Install/convert game assets using the project's normal setup before staging.
- The staging directory must not already exist; use a new path for each build.
- The source catalogs are not runtime translation files. Changing the Vita
  UI language in a PT-BR build affects native UI elements, but game-script
  text remains the build-selected Portuguese variant.
- Test the reduced dynamic sprite-cell budget on a physical PS Vita before
  declaring complete hardware compatibility. The fixed atlas currently takes
  1208 pixels, leaving 192 dynamic 64x64 cells in the 1024x2048 texture.
- A fork's customized boot artwork belongs in untracked
  `vita/sce_sys_custom/`, not the upstream code.
