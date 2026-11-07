# Compila tu propia VPK

> **En desarrollo y en pruebas.** Por favor, reporta los errores que encuentres (ver el final de esta guía).

Este repositorio no incluye los gráficos, sonidos ni datos binarios del juego, y no se
distribuyen VPK compiladas. Tú haces tu propia VPK a partir de tu copia del juego:

1. Necesitas un volcado de **tu cartucho de Pokémon Esmeralda** (`.gba`, cualquier idioma). El
   script solo lo lee para comprobar que es Pokémon Esmeralda; no copia nada de él, ni lo guarda
   ni lo sube a ningún sitio.
2. Después descarga los archivos del juego de la decompilación
   [pret/pokeemerald](https://github.com/pret/pokeemerald) (a través de
   [pokeemerald-multiplatform](https://github.com/gradenGnostic/pokeemerald-multiplatform), la
   versión en la que se basa este port) y comprueba cada uno con `tools/assets/manifest.txt`.
3. Compilas la VPK y la instalas en tu PS Vita.

No compartas la VPK ni los archivos descargados: contienen datos del juego de Nintendo.

Versión en inglés: [BUILD_YOUR_OWN.md](BUILD_YOUR_OWN.md).

## Forma rápida: un solo script

Con MSYS2 instalado en `C:\msys64` (paso 1.1) y el repositorio descargado (paso 2), esto hace
solo los pasos 1.2 a 4 (instala lo que falte) y deja `EmeraldVita.vpk` en la carpeta del
repositorio:

```powershell
powershell.exe -ExecutionPolicy Bypass -File .\build_all.ps1 -Rom "C:\ruta\a\esmeralda.gba"
```

La ROM solo hace falta la primera vez. Después de actualizar el repositorio (`git pull`), vuelve
a ejecutar `.\build_all.ps1`. Los pasos de abajo hacen lo mismo a mano.

## 1. Instala las herramientas (Windows, una vez)

1. **MSYS2** en `C:\msys64` ([msys2.org](https://www.msys2.org)). Abre *MSYS2 MSYS* y ejecuta:
   ```sh
   pacman -Syu
   pacman -S --needed make git cmake tar bzip2 xz wget curl python patch pkgconf diffutils \
       mingw-w64-x86_64-gcc mingw-w64-x86_64-libpng mingw-w64-x86_64-pkgconf
   ```
2. **VitaSDK** con SDL2, en la misma consola (añade también las dos líneas `export` a
   `~/.bash_profile`):
   ```sh
   export VITASDK=/usr/local/vitasdk PATH=/usr/local/vitasdk/bin:$PATH VITASDK_CHANNEL=2026.08
   export TAR_OPTIONS=--force-local
   mkdir -p /c/vdpm-tmp && export TMPDIR=$(cygpath -m /c/vdpm-tmp)
   git clone --depth 1 https://github.com/vitasdk/vdpm /tmp/vdpm && (cd /tmp/vdpm && ./bootstrap-vitasdk.sh)
   VDPM_NONINTERACTIVE=1 vdpm install sdl2 sdl2_image
   ```
3. **Python 3 para Windows** ([python.org](https://www.python.org), marca *Add to PATH*) y
   Pillow, desde PowerShell:
   ```powershell
   pip install pillow
   ```
4. **Git para Windows** ([git-scm.com](https://git-scm.com)) para descargar el repositorio.

Si MSYS2 o VitaSDK dan problemas, mira [BUILD.md](BUILD.md#requirements-windows).

## 2. Descarga el repositorio

```powershell
git clone https://github.com/HctrRB/pokeemerald-vita-3ds.git
cd pokeemerald-vita-3ds
```

## 3. Consigue los archivos del juego (una vez)

Pon tu volcado donde quieras y, desde la carpeta del repositorio, en PowerShell:

```powershell
powershell.exe -ExecutionPolicy Bypass -File .\fetch_assets.ps1 -Rom "C:\ruta\a\esmeralda.gba"
```

Comprueba el volcado, descarga unos 9.000 archivos y genera los archivos del port que salen de
ellos (el atlas de la interfaz y las imágenes de las maquetas). La primera vez tarda unos minutos.

## 4. Compila

En *MSYS2 MINGW64*, desde la carpeta del repositorio (`cd /c/ruta/a/pokeemerald-vita-3ds`):

```sh
make -f make_tools.mk -j8 CC="gcc -static" CXX="g++ -static"   # una vez
make -f vita/assets.mk -j8                                      # una vez
```

Y después en PowerShell:

```powershell
powershell.exe -ExecutionPolicy Bypass -File .\build_vita.ps1
```

El resultado es `build-vita\pokeemerald.vpk`.

## 5. Instala

1. Copia `pokeemerald.vpk` a la Vita (por USB o FTP con VitaShell).
2. En VitaShell, pulsa X sobre la VPK e instálala (en los ajustes de HENkaku tiene que estar
   activado *Enable unsafe homebrew*).
3. El juego aparece como **Emerald Vita**. Las partidas y los ajustes se guardan en
   `ux0:data/POKEVITA/`.

Si quieres tu propio arte en el LiveArea, ponlo en `vita/sce_sys_custom/` con los mismos nombres
que en `vita/sce_sys/` (PNG indexados de 8 bits); se usa en lugar de las imágenes neutras.

## Errores frecuentes

| Mensaje | Solución |
|---|---|
| `not a Pokémon Emerald dump` | El archivo no es una ROM de Pokémon Esmeralda o está dañado. Tiene que medir 16 MB. |
| `git is needed` | Instala Git en MSYS2: `pacman -S git`. |
| `Python 3 with Pillow is needed` | Instala Python para Windows y ejecuta `pip install pillow`. |
| `the game files are missing` | Haz el paso 3 antes de compilar. |
| `convert the game graphics first` | Ejecuta `make -f vita/assets.mk -j8` (paso 4). |

## Reportar errores

Este port **está en desarrollo y en pruebas**. Si algo falla, abre un *issue* en GitHub con la
plantilla *Bug report* e incluye:

- qué estabas haciendo y qué pasó (una foto de la pantalla ayuda);
- los archivos `perf_summary.txt`, `perf_frames.csv` y cualquier `*.log` de `ux0:data/POKEVITA/`;
- si se cerró el juego, el archivo `.psp2dmp` de `ux0:data/` (ver [DEBUGGING.md](DEBUGGING.md));
- el commit que compilaste (`git log -1 --oneline`).
