# PT-BR v0.5 — local UI and camera improvements

The English baseline and the optional PT-BR catalogs remain separate.

## Localization / UI
- Native keyboard footer: Type -> Digitar; Delete -> Apagar.
- Naming warning: Deixar vazio usará o padrão.
- Registered item hint: Registre um item da BOLSA para usá-lo rapidamente.
- Bag menu hint: Vasculhe os bolsos da sua MOCHILA.
- Locked -> Bloqueado (case-sensitive forms).
- Status capsules now center and clip translated labels, and choose among
  VT323 24/20/18 px when they are too long.
- City name popup once again displays the town slogan under the name plate,
  using the translated phrases in the native UI dictionary.

## Experimental overworld camera
Under Options > World > Renderer, immediately below 3D VOXEL DIORAMA:
- RIGHT STICK CAMERA: defaults OFF. Move the **right analog stick** directly
  to orbit/pitch the voxel camera, without a shoulder-button modifier.
  Turning it OFF resets yaw and pitch to the original angle immediately.
- FIRST PERSON: defaults OFF. Switches the overworld view to an eye-level
  voxel camera. Optionally combine with RIGHT STICK CAMERA to look around.
- FACE CAMERA SPRITES: shown only when RIGHT STICK CAMERA is ON. Opt-in
  camera-facing foliage: tree/grass sprite cards rotate in the existing
  chunk VBO as yaw changes, based on a saved copy of the original vertices.
  Disabling it restores the original unrotated geometry. Requires Vita GPU
  performance and seam testing, especially on large multi-card trees.
- The native status labels and city slogans now use scrolling marquee
  animations for text wider than their allotted areas, instead of cutting
  off the ending. City slogans use PT-BR translations when selected, and
  have no additional background capsule or drop shadow.
- Camera changes do not modify the default battle camera or 2D field view.
  Broader 3D culling for free-look is experimental and requires performance
  testing on physical PS Vita hardware.
- Suppress fixed screen-space sun/moon shafts in classic (60 FPS) mode.
  HD-2D mode retains its existing cinematic effect.

## Local PS Vita packaging
- Custom LiveArea art can be copied into vita/sce_sys_custom using:
  icon0.png, pic0.png, livearea/contents/bg.png,
  livearea/contents/startup.png and livearea/contents/template.xml.
  The VPK CMake rules prefer this local, uncommitted override directory.
- The startup graphic is converted to an indexed PNG for Vita compatibility.
- The user-supplied LiveArea XML is staged from Post PTBR/Updatev0.5/template.xml
  into sce_sys/livearea/contents/template.xml in both VPK variants.
- Optional manual: sce_sys/manual/001.png through 080.png inside the VPK.
- To remove the vitaGL splash, a separate local vitaGL library was rebuilt
  with NO_SPLASHSCREEN=1 (SKIP_SPLASHSCREEN), without modifying the installed
  SDK permanently or committing the library binary. The build used a
  local-only compatibility shim for an optional VitaShaRK API absent from the
  local SDK. Rebuild this library to reproduce the no-splash variant.

## Validation status
- Translation catalogs: 14,253 entries across 405 files validated.
- New atlas: 35 fonts, 4,235 glyphs, 129 sprites, 1024x1168 fixed pixels.
- Docker VitaSDK compilation and both VPK integrity checks passed.
- Vita3K v0.2.1 still aborts at startup with ACCESS_VIOLATION after opening
  ui2/ui_atlas.bin, so **gameplay, hardware camera operation, first-person
  usability, and clipping have not yet been visually tested**. Requires
  a PS Vita hardware test before a public release.
- No ROM, manual, custom LiveArea assets or binary VPK is committed.
