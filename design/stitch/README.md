# Diseño de la nueva UI (fuente de verdad)

Pantallas de referencia de la nueva interfaz del mundo vóxel, a 960×544 reales. Sustituyen a la
exportación de Google Stitch de la que partieron (no incluida en el repositorio).

## Cómo verlas

Necesitan un servidor (las rutas son relativas y el mapa carga un JSON):

```bash
python -m http.server 8765 --bind 127.0.0.1 --directory design/stitch
```

Abrir `http://localhost:8765/<pantalla>/index.html` con la ventana a 960×544. En Claude Code la
configuración `design-stitch` de `.claude/launch.json` hace lo mismo.

## Estructura

| Carpeta | Contenido |
|---|---|
| `<pantalla>/index.html` | Versión corregida: es la referencia para implementar |
| `<pantalla>/stitch.html` | Exportación original de Stitch de la que sale (solo como historial) |
| `_shared/ui.css` | Tokens (colores, tipografía, espaciado) y componentes. El runtime los implementa tal cual |
| `_shared/specimen.html` | Muestrario tipográfico (Press Start 2P y VT323) |
| `_assets/` | Sprites **originales del GBA** exportados por `_tools/export_assets.py` (decisión D7), mapa de Hoenn compuesto y `map/hoenn_zones.json` (decisión D8) |
| `_tools/export_assets.py` | Regenera `_assets/` desde `graphics/` y `src/data/region_map/`: `python design/stitch/_tools/export_assets.py` |

## Pantallas

| Pantalla | Carpeta | Origen en Stitch |
|---|---|---|
| Hub START | `hub` | `thematic_town_signs_bottom_left_party` (sin la cápsula 3D CAM, D5) |
| Cartel de zona | `cartel_zona` | `hoenn_thematic_town_signs_showcase` (un 9-slice tintado por zona) |
| Diálogo | `dialogo` | `overworld_dialogue_box_960x544` |
| Sí / No | `si_no` | `overworld_dialogue_yes_no_choice` |
| Multichoice | `multichoice` | `pok_mart_entry_dialogue_menu_960x544` |
| Objeto obtenido | `objeto_obtenido` | `item_obtained_fanfare_960x544` |
| Llamada en campo | `llamada` | `pok_nav_match_call_dialogue_960x544` |
| Mochila | `mochila` | `bag_menu_unified_header_960x544_3` |
| Equipo (estado) | `equipo` | `party_summary_unified_hp_bars_2` |
| Equipo (movimientos) | `equipo_movimientos` | `party_moves_expanded_summary_960x544_2` |
| Equipo (acciones) | `equipo_popup` | `party_summary_popups_fixed_960x544` |
| Equipo (reordenar) | `equipo_reordenar` | `party_reorder_dynamic_lead_960x544_2` |
| Pokédex | `pokedex` | `pok_dex_dynamic_dex_switcher_960x544_2` |
| Pokédex (detalle) | `pokedex_detalle` | `pok_dex_footprint_size_compare_960x544` |
| PokéNav (cintas) | `pokenav` | `pok_nav_all_tabs_unified_960x544` (pestaña nueva) |
| PokéNav (condición) | `pokenav_condicion` | `pok_nav_condition_radar_feed_pok_block_960x544_3` |
| PokéNav (mapa) | `pokenav_mapa` | `pok_nav_accurate_routes_connections_960x544` (mapa rehecho, D8) |
| PokéNav (Match Call) | `pokenav_match_call` | `match_call_dynamic_teams_960x544` |
| Tienda (comprar) | `tienda_comprar` | `pok_mart_buy_mode_with_confirmation_960x544_4` |
| Tienda (vender) | `tienda_vender` | `pok_mart_sell_mode_unified_bag_style_2` |
| Opciones | `opciones` | `system_options_fully_interactive_960x544` |
| Ficha | `ficha` | `trainer_card_character_dossier_960x544_3` |
| Vuelo | `vuelo` | Nueva (componente de mapa en modo vuelo) |
| Equipo · usar objeto | `equipo_objetivo` | Nueva |
| Equipo · enseñar MT / olvidar movimiento | `equipo_mt` | Nueva |
| Nuzlocke | `nuzlocke` | Nueva |
| Mochila en combate | `mochila_combate` | Nueva (misma pantalla, modo combate) |
| Equipo en combate | `equipo_combate` | Nueva (misma pantalla, modo combate) |
| Equipo · elegir (tutor, guardería, scripts) | `equipo_elegir` | Nueva (ui_scr_choose.c) |
| Combate (HUD: individual, entrenador, doble, Safari; FIGHT, objetivo, SÍ/NO, subida de nivel, juez de la Arena; temas con `?theme=`) | `combate` | Diseño propio adaptado al kit |
| PC: cajas (modos MOVE / WITHDRAW / DEPOSIT / MOVE ITEMS con `?mode=`) | `pc_cajas` | Nueva (2026-11-13) |
| Teclado de nombre (`?for=mon`, `player`, `box`) | `teclado_nombre` | Nueva (2026-11-13) |
| Correo / Easy Chat (`?type=mail`, `profile`, `battle`, `interview`, `trendy`) | `correo` | Nueva (2026-11-13) |
| Mochila de la Pirámide (`?loc=field`, `battle`, `toss`, `party`) | `mochila_piramide` | Nueva (2026-11-13) |
| Equipo en combate múltiple (`?forced=1`: cambio obligado) | `equipo_multi` | Nueva (2026-11-13): `equipo_combate` en modo múltiple |
| Concurso: elección de la actuación | `concurso` | Nueva (2026-11-13) |
| Batidora de bayas: elección de baya y resultados | `batidora` | Nueva (2026-11-13) |

## Tacto

- Primer toque **selecciona** (mueve el cursor amarillo); tocar lo ya seleccionado lo **activa**, igual que ✕.
- Las entradas del menú START abren su pantalla.
- El pie termina siempre con la **hora** (solo HH:MM). El **dinero** va arriba a la derecha de la
  cabecera en mochila, mochila de combate y tienda (`<div class="header" ... data-money>`). Las
  cabeceras no muestran el nombre del jugador. La guía L / R no va en el pie: ya la muestra el
  carril de pestañas. El texto del pie es Press Start 2P 16 (VT323 32 no cabe bien en 40 px).
- El pie es una **guía de los botones físicos**, no se toca. En los mocks el teclado hace de botones:
  X = ✕, T = △, S = □, V = SELECT, Enter = START, Esc/B = ○, Q/E o L/R = gatillos, flechas = cruceta.
- Toda lista cuyo contenido no cabe se **arrastra** (con barra de posición).
- Selector de cantidad con flechas táctiles **− / +** (mantener pulsado repite); en un popup ▲▼ ±1 y ◀▶ ±10.
- Las confirmaciones tienen sus opciones táctiles (SÍ / NO…) dentro de la ventana.

## Navegación: cabecera y pestañas

Toda la navegación sale de una tabla única, `UI_NAV` en [_shared/nav.js](_shared/nav.js), que
genera la cabecera de cada pantalla completa (`<div class="header" data-nav="sección:pestaña">`).
El runtime implementa la misma tabla.

- **Título de sección**: etiqueta fija. No hay selector de sección (quitado el 2026-11-06): para
  cambiar de menú se vuelve al menú START con ○/B.
- **Selector de pestañas `L [ … ] R`**: L / R pasan a la anterior / siguiente y dan la vuelta; también
  se tocan. Una pestaña abre otra pantalla o cambia el contenido de la misma.
- **B / ○**: cierra primero el popup abierto; si no hay ninguno, vuelve un nivel
  (detalle → lista → menú START). Con un popup abierto, L/R no cambian de pestaña.
- Secciones con `back: 'screen'` (PC, nombre, correo, Pirámide, concurso, batidora): ○ es una tecla
  de la pantalla (borrar letra, preguntar antes de salir, no se puede cancelar) y nunca sale sola.
  Con `lr: 'screen'` (PC) los gatillos son de la pantalla (cambiar de caja) y el carril no muestra L / R.

| Sección | Pestañas (orden de Stitch) | Pantallas |
|---|---|---|
| POKéDEX | HOENN · NATIONAL | `pokedex` |
| POKéDEX (detalle) | INFO · AREA · CRY · SIZE (AREA y SIZE abren su popup) | `pokedex_detalle` |
| POKéMON | STATUS · MOVES | `equipo`, `equipo_movimientos` |
| BAG | ITEMS · POKé BALLS · TMs & HMs · BERRIES · KEY ITEMS | `mochila` |
| POKéNAV | HOENN MAP · MATCH CALL · CONDITION · RIBBONS | `pokenav_mapa`, `pokenav_match_call`, `pokenav_condicion`, `pokenav` |
| TRAINER CARD | CARD · PASSPORT | `ficha` |
| OPTIONS | GENERAL · EXTRAS · WORLD · SYSTEM | `opciones` |
| POKé MART (desde el multichoice de la tienda) | BUY · SELL; al vender, bolsillos como subpestañas táctiles | `tienda_comprar`, `tienda_vender` |

Nota: los bolsillos son los 5 de Esmeralda; el `MEDICINE` de Stitch no existe en el juego.

## Popups y desplegables

Un componente único (`.modal` en `ui.css`, `UI_MODALS` en `nav.js`). Se abren con su botón o
tocando el elemento, se cierran con ○/B, y para revisarlos se abren directamente con `#id` en la
URL (por ejemplo `equipo/index.html#acciones`).

| Pantalla | Popup (`#id`) | Origen en Stitch | Se abre con |
|---|---|---|---|
| Hub | Movimientos de campo (`#mo-campo`) + aviso (`#toast-mo`) | `FieldMovesModal`, `hm-action-toast` | △ |
| Equipo | Acciones del Pokémon (`#acciones`) | Contextual actions pop-up | ✕ o toque en la tarjeta |
| Equipo | Objeto equipado (`#objeto`) | Bag / item modal | △ o ITEM |
| Equipo | Movimientos de campo del equipo (`#mo`) | Field HM modal | SELECT o el MO del popup |
| Equipo · movimientos | Detalle del movimiento (`#movimiento`) | Move details modal | ✕ o toque en el movimiento |
| Mochila | Acciones del objeto (`#acciones`) | (bolsa: estados de `design.md`) | ✕ o toque en la fila |
| Mochila | Cantidad (`#cantidad`) | Quantity selector | TOSS |
| Tienda · comprar | Confirmación (`#compra`) y recibo (`#recibo`) | Purchase confirmation modal | ✕ |
| Tienda · vender | Confirmación de venta (`#venta`) | Sale dialogue modal | ✕ |
| Opciones | Confirmación (`#confirmar`) + aviso (`#aviso`) | Confirm / prompt modal, toast | ○ (guardar y salir) |
| Opciones | Remapeo de botones (`#remapeo`) | Remap controls modal | SELECT |
| Opciones | Salir al LiveArea (`#salir`) | LiveArea shutdown modal | START |
| Pokédex · detalle | Área en el mapa (`#area`) | Habitat modal | Pestaña AREA |
| Pokédex · detalle | Comparación de tamaño (`#tamano`) | Trainer height modal | Pestaña SIZE |
| PokéNav · condición | Estuche de Pokécubos (`#pokecubos`) | Pokéblock case modal | ✕ |
| PokéNav · Match Call | Llamada en curso (`#llamada`) | Match call dialogue modal | ✕ o toque en un contacto |

Excluidos a propósito: el desplegable de zonas del menú START (`hoenn-location-dropdown`) y el
selector de sección de la cabecera, quitados por decisión del 2026-11-06; los submenús de combate (`MoveSelectionSubmenu`), porque el combate queda
fuera, y la «Purchase confirmation» que Stitch copió por error dentro de la ficha de entrenador.
`equipo_popup` y `equipo_reordenar` se quedan como capturas de estado; el popup real vive en `equipo`.

## Interacciones de menú (las de los prototipos de Stitch)

Los originales de Stitch eran prototipos interactivos. Sus interacciones se han recuperado sobre
datos comunes ([_shared/data.js](_shared/data.js), con la misma forma que lo que el runtime lee
del juego en Core 1) y un controlador por pantalla ([_shared/screens.js](_shared/screens.js)).
Regla general: **la selección manda** (evento `ui-select` → se rehacen los paneles de detalle y
los popups), **las pestañas cambian el contenido** (evento `ui-tab`) y **las acciones cambian el
estado** (se conserva entre pantallas durante la sesión de revisión).

| Pantalla | Interacciones |
|---|---|
| Menú START | La descripción sigue a la entrada elegida; cada entrada abre su pantalla (GUARDAR abre su popup, SALIR cierra el menú); equipo y movimientos de campo con datos del equipo |
| Equipo | La ficha (sprite, tipos, naturaleza, objeto, estadísticas con ▲▼, EXP) sigue a la tarjeta elegida; popup de acciones con los MO reales de ese Pokémon; **reordenar** con SWITCH / □ (elegir destino) o **arrastrando** una tarjeta sobre otra; **dar / quitar objeto** (actualiza tarjeta, ficha y mochila); movimientos de campo de todo el equipo con requisito de medalla |
| Equipo · movimientos | Lista de movimientos del Pokémon elegido; ◀▶ o las flechas de la cápsula cambian de Pokémon; el detalle y el popup siguen al movimiento; **reordenar movimientos** con □ |
| PokéNav · condición | El **radar y las barras reflejan los atributos del Pokémon elegido**; brillo, naturaleza y gustos; estuche de Pokécubos con los del jugador; **dar un Pokécubo** sube los atributos y el brillo, lo gasta y redibuja el radar (no come con brillo 255) |
| PokéNav · cintas | Las cintas del Pokémon elegido (icono original); la descripción sigue a la cinta elegida |
| PokéNav · Match Call | Ficha del contacto elegido (sprite si existe, lugar, estado, REVANCHA); la llamada muestra sus frases y avanza con ✕ / toque |
| PokéNav · mapa | Zona elegida en el mapa → nombre, tipo y encuentros; pestañas GRASS / SURF / FISH / ROCK (toque o △) cambian la lista |
| Mochila | Las pestañas cambian de **bolsillo** (lista, dibujo de la mochila, nº de tipos); el detalle sigue al objeto; acciones según bolsillo (clave: USAR / REGISTRAR; Poké Balls: DAR / TIRAR…); **registrar** en SELECT; **tirar** con cantidad (máx. lo que hay); **ordenar** con SELECT |
| Tienda · comprar | El detalle y el precio siguen al artículo; cantidad con − / + (máx. lo que alcanza el dinero); total en vivo; confirmación con el texto real; **compra** resta dinero y suma a la mochila; PREMIER BALL de regalo con 10 POKé BALLS |
| Tienda · vender | Bolsillos por toque; precio de venta = mitad; cantidad máx. = lo que hay; **venta** suma dinero y resta de la mochila; los objetos sin precio no se venden |
| Pokédex | SELECT abre la **búsqueda** (inicial, tipo, orden, modo) que filtra y ordena la lista; HOENN / NATIONAL cambian la numeración; la ficha sigue a la selección (visto = silueta, desconocido = «?»); los capturados abren el detalle; △ grito |
| Pokédex · detalle | ▲▼ pasa al anterior / siguiente capturado; AREA (mapa con sus zonas reales de encuentro) y SIZE (comparación con el jugador) siguen al Pokémon |
| Opciones | Cada pestaña muestra sus opciones; la ayuda sigue a la opción; ◀▶ o toque en un valor lo cambia (interruptores, listas, deslizador); las de SISTEMA abren remapeo, guardar, valores por defecto o salir; **remapeo**: tocar / ✕ cicla el botón, △ restaura |
| Ficha | CARD / PASSPORT cambian la cara (medallas / símbolos del Frente Batalla); la información sigue a la medalla elegida |
| Vuelo | Mapa en modo vuelo: los pueblos visitados se marcan; cursor con la cruceta o toque; ✕ / tocar dos veces → «Fly to X?» → animación de Vuelo del juego. Se llega desde FLY en el equipo o en los movimientos de campo del menú START (requiere la medalla 6) |
| Equipo · usar objeto | Desde BAG → USE: tarjetas con ABLE / —, el panel muestra el efecto en el elegido (PS antes → después, cura, revivir, nivel); usar gasta el objeto y vuelve a la mochila al acabarse |
| Equipo · MT | Desde BAG → TMs & HMs → USE: ABLE / NOT ABLE / LEARNED; con 4 movimientos abre «olvidar movimiento» (elegir cuál o no aprenderlo) |
| Nuzlocke | Estado por zona (AVAILABLE → CAUGHT → FAILED → BLOCKED) con ✕ o toque; recuento; pestaña de reglas |
| Mochila / equipo en combate | Solo ITEMS, POKé BALLS y BERRIES; USAR una Poké Ball la lanza; una curación pasa al equipo en modo combate («USE POTION ON:»); en el equipo, SHIFT con «IN BATTLE» y sin energía; ○ vuelve al combate |
| Movimientos de campo | Cada MO exige su medalla (CUT 1, FLASH 2, ROCK SMASH 3, STRENGTH 4, SURF 5, FLY 6, DIVE 7, WATERFALL 8) |
| Diálogo, llamada, objeto obtenido | El texto se escribe letra a letra; ✕ / toque completa la línea o pasa a la siguiente |
| Multichoice / Sí-No | Cursor y elección: BUY / SELL abren la tienda, SEE YA cierra |

Los pequeños avisos (toasts) indican lo que haría el juego fuera de la UI (animación de MO,
cerrar el menú, volver al campo).

## Cobertura de la UI actual (paridad funcional)

Cada función que ya existe en la UI del juego (`vita_ui*.c`, `vita_extras.c`) tiene su sitio en
las maquetas. La UI de combate queda fuera (se mantiene la actual).

| UI actual | Función | Pantalla nueva |
|---|---|---|
| Menú START (`vita_ui.c`) | Entradas según progreso, NUZLOCKE con el extra activo, RETIRE en la Zona Safari | `hub` (menú) |
| Barra de estado | Reloj día/noche y velocidad, dinero, objeto registrado (SELECT), pasos de Repelente / balls Safari, correr (Deportivas) | `hub` (cápsulas) |
| Guardar | Ranura, resumen, aviso de sobrescribir otra partida, resultado | `hub` → popup Guardar |
| MO de campo | Solo con la medalla que la habilita | `hub` → equipo, popup |
| Equipo (`vita_ui_party.c`) | Resumen INFO / SKILLS / MOVES / CONTEST, IV/EV solo con su opción, huevo, variocolor ★ | `equipo`, `equipo_movimientos` |
| Popup de equipo | SUMMARY / SWITCH / ITEM / MO; huevo; SHARE HP (Amortiguador, Batido); SHIFT en combate | `equipo_popup`, `equipo` |
| Usar objeto en el equipo | Regla del huevo, Éter → elegir movimiento, Caramelo raro → tarjeta de subida de nivel | `equipo_objetivo` (popups `#pp`, `#delta`) |
| MT / MO | Compatibilidad, olvidar movimiento | `equipo_mt` |
| Mochila (`vita_ui_bag.c`) | Bolsillos, ordenar (DEFAULT, A–Z, MÁS, MENOS), bolsillo vacío, cantidad con − / +, registrar | `mochila`, `mochila_combate` |
| Pokédex (`vita_ui_pokedex.c`) | Lista, búsqueda con filtros y RESET, huella, área por método, grito con onda, tamaño | `pokedex`, `pokedex_detalle` |
| PokéNav (`vita_ui_pokenav.c`) | Mapa, Match Call, Condición (araña del Pokémon elegido), Cintas | `pokenav*` |
| Mapa (`vita_ui_map.c`) | MAPSEC 28×15, zoom ×2, encuentros por método, hora del día y caña | `pokenav_mapa`, `vuelo` |
| Ficha | Anverso, reverso con récords y símbolos del Frente Batalla | `ficha` |
| Opciones (`vita_ui_options.c`) | Pestañas GENERAL / WORLD / BATTLE / MODES / CHEATS, grupos, acciones EXECUTE, reasignar botones con RESET | `opciones` |
| Nuzlocke (`vita_extras.c`) | Zonas AVAILABLE / CAUGHT / MISSED / BLOCKED, equipo vivo/caído, tope de nivel del próximo líder, contadores | `nuzlocke` (AREAS / SQUAD / RULES) |
| Tienda | Comprar y vender con cantidad | `tienda_comprar`, `tienda_vender` |
| Campo | Diálogo, sí/no, multielección, objeto obtenido, llamada, cartel de zona | pantallas de campo |

## Reglas del sistema (resumen de `_shared/ui.css`)

- **Lienzo**: 960×544, píxeles enteros, márgenes de 16 px, ritmo de 8 px, filas táctiles ≥ 40 px.
- **Tipografía** (D2, decidida en la consola): Press Start 2P a 16 px (títulos, etiquetas, botones)
  y 24 px; VT323 a 32 px para todo el texto (28 se descartó: poco legible en la consola). Nada más.
- **Color**: tokens `--frame-*`, `--paper-*`, `--ink*`, `--select`, `--accent-money`, `--hp-*`.
  Nada de colores sueltos fuera de los tokens, salvo los de los sprites del GBA.
- **Pantallas completas** (D3): marco translúcido sobre el mundo; los paneles son opacos.
- **Acabado** (el de la referencia de Stitch): cajas con biselado doble (borde oscuro, brillo arriba
  a la izquierda, sombra abajo a la derecha), sombra dura sin desenfoque, esquinas de 4 px;
  degradados verticales de dos colores (pestaña activa y selección ámbar, barras, botones, cabecera
  y pie); carril de pestañas hundido con teclas L/R en relieve; franja de encabezado en los paneles;
  botones de acción de colores; círculos de los botones de PlayStation. **Sin líneas de escaneo**.
- **Sprites** (D7): solo los del GBA, a escala entera (×1, ×2, ×3) con `image-rendering: pixelated`.
  Lo nuevo es el «cromo» de la UI (paneles, pestañas, barras, glifos de botones PlayStation).
- **Mapa** (D8): imagen del mapa regional del GBA ×3 (672×360) y zonas de la rejilla 28×15 del
  juego; celda = 24 px; toque → `GetRegionMapSecIdAt(x + 1, y + 2)`.
- **Datos**: solo los que existen en el juego (G9). Nada de «Pocket slots», «Started OCT 2002»…

## Decisiones de sprites (D11)

- Diálogos **sin retratos** (el GBA no los tiene): solo la pestaña con el nombre.
- Iconos del menú START con **sprites originales**: POKéDEX = Poké Ball, POKéMON = icono del primer
  Pokémon del equipo, BAG = mochila (`bag_male`, ½), POKéNAV = `pokenav/nav_icon`, ficha = cabeza del
  jugador del mapa, SAVE = carta (`LETTER`), OPTION = `CARD_KEY`, EXIT sin icono.
- Cursor del diseño: fila amarilla con ▶.
- Las medallas de `graphics/trainer_card/badges.png` son grises en el original: se usan tal cual.

## Sincronización con la consola (2026-11-06)

Las maquetas se han ajustado a lo que ya dibuja la consola:

- **Diálogo y llamada:**
  - Texto en VT323 40 y sin pestaña de nombre, porque el juego no tiene ese dato.
  - En la llamada, el teléfono animado (los 8 fotogramas de `nav_icon`) va a la derecha del texto.
- **Cartel de zona:** el lema es el texto del cartel del GBA y baja bajo la franja del nombre, con fondo semitransparente y letra más pequeña.
- **Hub:**
  - Sin el contador 6/6.
  - Sin los atajos SELECT / △ en el pie.
  - Reloj HH:MM sin DAY/NIGHT.
  - AUTO RUN en su propia línea bajo las cápsulas.
- **Match Call:**
  - Un chip compacto "!" para la revancha, que no desborda la fila.
  - Bloque REMATCH en el detalle.
- **Nuzlocke:** pestaña SETTINGS, con las opciones de CHALLENGE y el borrado del registro.
- **Género y variocolor:** se dibujan con el pixel art de `tools/ui2/build_atlas.py` (`_assets/ui_gba/gender_*.png` y `shiny_star.png`).
- **Pokédex, AREA:** en la consola es una pestaña con el mapa en el panel. La maqueta lo sigue mostrando en su popup, con el mismo contenido.
- **CONDITION:** el botón FEED POKéBLOCK, o □, abre la lista de la caja de Pokécubos.

## Combate (fase 0 del plan de combate)

En `combate/index.html` hay un selector de estados, fuera del lienzo de 960×544.

- **Qué se dibuja:** solo el HUD. Los Pokémon, el mundo y los efectos de la maqueta están solo como referencia: en el juego los sigue dibujando el GBA (pase 6).
- **Desplegable de FIGHT:**
  - Se abre hacia la izquierda, sobre la caja de mensajes, y no a la derecha como en el diseño de Stitch. Así tus placas y tus PS siguen a la vista mientras eliges.
  - En dobles, la cabecera muestra el objetivo con los chips FOE 1 / FOE 2 / ALLY.
- **Placas:** gráficas con bisel, ▶ en el Pokémon que elige acción, y la barra de EXP y los PS en números en las tuyas.
- **Poké Balls del equipo:** en combates contra entrenadores, una tira bajo las placas con 6 bolas: sano, con estado, debilitado o vacío (`ball_display` del GBA).
- **Colores de PP del GBA:** amarillo con la mitad o menos, naranja con un cuarto o menos, rojo a 0.
