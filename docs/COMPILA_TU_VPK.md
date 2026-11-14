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
3. Compila la VPK, y tú la instalas en tu PS Vita.

No compartas la VPK ni los archivos descargados: contienen datos del juego de Nintendo.

Versión en inglés: [BUILD_YOUR_OWN.md](BUILD_YOUR_OWN.md).

## Qué necesitas

- Un PC con Windows 10 u 11, conexión a internet y unos 5 GB libres.
- Tu volcado de Pokémon Esmeralda (`.gba`, 16 MB).
- Una PS Vita con HENkaku o Ensō y VitaShell.

Todo lo demás (MSYS2, VitaSDK, Python, Git) lo instala el script.

## 1. Descarga el repositorio

Clónalo con Git:

```powershell
git clone https://github.com/HctrRB/pokeemerald-vita-3ds.git C:\Emerald\pokeemerald-vita-3ds
```

o, sin Git, usa **Code > Download ZIP** en GitHub y descomprímelo.

Ponlo en una carpeta cuya ruta **no tenga espacios ni tildes**, por ejemplo
`C:\Emerald\pokeemerald-vita-3ds`. Las herramientas de compilación no los soportan (el script lo
comprueba y se para), así que `C:\Users\Tu Nombre\Downloads\...` no sirve.

## 2. Compila

Abre PowerShell en la carpeta del repositorio (en el Explorador, clic derecho dentro de la
carpeta > *Abrir en Terminal*; en Windows 10, *Archivo > Abrir Windows PowerShell*) y ejecuta:

```powershell
powershell.exe -ExecutionPolicy Bypass -File .\build_all.ps1
```

Una ventana te pide tu `.gba`. También puedes indicarlo en la línea de comandos:
`.\build_all.ps1 -Rom "C:\ruta\a\esmeralda.gba"` (cualquier ruta, con espacios incluidos).

El script:

1. instala MSYS2 en `C:\msys64` si no está, y los paquetes que necesita la compilación;
2. instala VitaSDK con SDL2 en MSYS2 (`/usr/local/vitasdk`) si no está;
3. comprueba tu volcado y descarga los archivos del juego (unos 9.000);
4. compila las herramientas, convierte los archivos del juego y compila la VPK.

La primera vez descarga 1–2 GB y puede tardar 20–40 minutos; las siguientes, unos minutos. El
resultado es **`EmeraldVita.vpk`** en la carpeta del repositorio. Los pasos ya hechos se
saltan, así que puedes volver a ejecutarlo después de un fallo.

**Actualizar:** `git pull` (o vuelve a descargar el ZIP en la misma carpeta) y ejecuta
`.\build_all.ps1` otra vez. La ROM solo se pide la primera vez.

## 3. Instala

1. Copia `EmeraldVita.vpk` a la Vita (por USB o FTP con VitaShell).
2. En VitaShell, pulsa X sobre la VPK e instálala (en los ajustes de HENkaku tiene que estar
   activado *Enable unsafe homebrew*).
3. El juego aparece como **Emerald Vita**. Las partidas y los ajustes se guardan en
   `ux0:data/POKEVITA/`.

Si quieres tu propio arte en el LiveArea, ponlo en `vita/sce_sys_custom/` con los mismos nombres
que en `vita/sce_sys/` (PNG indexados de 8 bits); se usa en lugar de las imágenes neutras.

## Errores frecuentes

| Mensaje | Solución |
|---|---|
| `This folder's path has spaces or special characters` | Mueve la carpeta del repositorio a una ruta sencilla, como `C:\Emerald\pokeemerald-vita-3ds`. |
| `Could not download MSYS2` / `MSYS2 did not install` | Instala MSYS2 a mano desde [msys2.org](https://www.msys2.org) en `C:\msys64` y vuelve a ejecutar el script. |
| `not a Pokémon Emerald dump` | El archivo no es una ROM de Pokémon Esmeralda o está dañado. Tiene que medir 16 MB. |
| `VitaSDK did not install` | Casi siempre es una descarga que falló: vuelve a ejecutar el script. Si no, mira [BUILD.md](BUILD.md#requirements-windows). |
| `la ejecución de scripts está deshabilitada en este sistema` | Lánzalo exactamente como arriba (`powershell.exe -ExecutionPolicy Bypass -File ...`). |

## Compilar a mano

El script hace estos pasos; puedes hacerlos tú en la consola *MSYS2 MINGW64*, desde la carpeta
del repositorio (`cd /c/Emerald/pokeemerald-vita-3ds`):

1. Paquetes y VitaSDK, una vez (los comandos exactos y los problemas típicos están en
   [BUILD.md](BUILD.md#requirements-windows)).
2. Los archivos del juego, una vez:
   ```sh
   ./fetch_assets.sh /c/ruta/a/esmeralda.gba
   ```
3. Las herramientas y los archivos del juego convertidos:
   ```sh
   make -f make_tools.mk -j8 CC="gcc -static" CXX="g++ -static"
   make -f vita/assets.mk -j8
   ```
4. La VPK: `./build_vita.sh` → `build-vita/pokeemerald.vpk`.

## Reportar errores

Este port **está en desarrollo y en pruebas**. Si algo falla, abre un *issue* en GitHub con la
plantilla *Bug report* e incluye:

- qué estabas haciendo y qué pasó (una foto de la pantalla ayuda);
- los archivos `perf_summary.txt`, `perf_frames.csv` y cualquier `*.log` de `ux0:data/POKEVITA/`;
- si se cerró el juego, el archivo `.psp2dmp` de `ux0:data/` (ver [DEBUGGING.md](DEBUGGING.md));
- el commit que compilaste (`git log -1 --oneline`);
- si falló la compilación, las últimas líneas que mostró el script.
