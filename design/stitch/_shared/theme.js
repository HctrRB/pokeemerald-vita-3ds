// Menu themes on the design mocks: ?theme=red|emerald|crystal|tropical|royal (loaded by nav.js).
// Colours and fonts of src/platform/ui/ui_theme.c: the CSS tokens are set, every colour of the
// EMERALD design is swapped for the theme's, and the two pixel fonts for the theme's fonts.
(() => {
const TH = {"red": {"faceTitle": "2", "faceBody": "3", "frameA": "rgba(30,30,38,0.84)", "barTop": "#2e2e38", "barBot": "#1e1e26", "barLine": "#e0303a", "onFrame": "#ffffff", "onSoft": "#b7b7c2", "title": "#ffffff", "paper0": "#fafafa", "paper1": "#f0f0f2", "paper2": "#e2e2e6", "paperEdge": "#2b2b33", "paperShade": "#d5d5dc", "paperStrip": "#2b2b33", "stripInk": "#ffffff", "stripSoft": "#b7b7c2", "rowA": "#fafafa", "rowB": "#f2f2f5", "rowLine": "#e4e4e9", "ink": "#26262e", "inkSoft": "#8a8a96", "selTop": "#f2404a", "selBot": "#d4202c", "selEdge": "#8e0e17", "selInk": "#ffffff", "selArrow": "#ffffff", "tabTop": "#f2404a", "tabBot": "#d4202c", "tabEdge": "#8e0e17", "tabHi": "#ff7880", "tabInk": "#ffffff", "rail": "#14141a", "railEdge": "#464652", "keyTop": "#e0303a", "keyBot": "#be1e28", "keyLight": "#ff6e76", "keyDark": "#780e16", "keyInk": "#ffffff", "capTop": "#2b2b33", "capBot": "#1e1e26", "capEdge": "#14141a", "capHi": "#464650", "darkTop": "#32323c", "darkBot": "#24242c", "darkEdge": "#14141a", "darkHi": "#464650", "winTop": "#2b2b33", "winBot": "#2b2b33", "winInk": "#ffffff", "chipTop": "#2b2b33", "chipBot": "#2b2b33", "chipEdge": "#14141a", "chipInk": "#ffffff", "dlgEdge": "#2b2b33", "dlgTop": "#fafafa", "dlgBot": "#f0f0f2", "dlgShade": "#d5d5dc", "btnTop": ["#3c3c46", "#e0303a", "#7a7a86"], "btnBot": ["#2b2b33", "#be1e28", "#60606c"], "btnEdge": "#14141a", "btnInk": "#ffffff", "accent": "#e0303a", "gold": "#ffc93c", "dim": "rgba(20,20,26,0.60)", "shadowCol": "rgba(0,0,0,0.35)", "plateTop": "#fafafa", "plateBot": "#f0f0f2", "plateEdge": "#2b2b33", "plateShade": "#d5d5dc", "plateRing": "0", "lvInk": "#8a8a96", "hpInk": "#e0303a", "gauge": "#14141a", "cmdTop": ["#f2404a", "#ffb840", "#3c3c46", "#9696a2"], "cmdBot": ["#d4202c", "#e89214", "#2b2b33", "#6e6e7a"], "cmdEdge": "#14141a", "cmdInk": "#ffffff", "em": "#d4202c", "radius": "10", "selRadius": "0", "edge": "2", "shadow": "6", "shadowR": "6", "chamfer": "true", "gloss": "false", "bevel": "false", "stripes": "true"}, "emerald": {"faceTitle": "0", "faceBody": "1", "frameA": "rgba(4,20,16,0.82)", "barTop": "#0e2b25", "barBot": "#061a16", "barLine": "#2b5849", "onFrame": "#ffffff", "onSoft": "#7ea69c", "title": "#99f6e4", "paper0": "#f7f4e9", "paper1": "#ebe4cf", "paper2": "#dfd7bf", "paperEdge": "#1a3024", "paperShade": "#d1c8af", "paperStrip": "#e7dfc6", "stripInk": "#16271c", "stripSoft": "#728277", "rowA": "#ebe4cf", "rowB": "#f1ebd8", "rowLine": "#d8cfb7", "ink": "#16271c", "inkSoft": "#728277", "selTop": "#fff7d6", "selBot": "#fde68a", "selEdge": "#d97706", "selInk": "#3a2a00", "selArrow": "#d97706", "tabTop": "#fceda6", "tabBot": "#e2ab24", "tabEdge": "#8c6006", "tabHi": "#fffbe0", "tabInk": "#16271c", "rail": "#041612", "railEdge": "#1c5547", "keyTop": "#1b3a32", "keyBot": "#0c201a", "keyLight": "#366d5f", "keyDark": "#061511", "keyInk": "#6ee7b7", "capTop": "#123a31", "capBot": "#0a241e", "capEdge": "#041210", "capHi": "#2e6b5b", "darkTop": "#0c2f27", "darkBot": "#071d18", "darkEdge": "#041210", "darkHi": "#1b544b", "winTop": "#1c382f", "winBot": "#0b231d", "winInk": "#99f6e4", "chipTop": "#245244", "chipBot": "#163a30", "chipEdge": "#0b2e26", "chipInk": "#d1fae5", "dlgEdge": "#26272d", "dlgTop": "#fbfaf2", "dlgBot": "#ebe7d3", "dlgShade": "#9e9784", "btnTop": ["#2f7a5c", "#e08a2a", "#a83a3a"], "btnBot": ["#1d5a43", "#b8661a", "#7a2626"], "btnEdge": "#172a22", "btnInk": "#ffffff", "accent": "0", "gold": "#f5d162", "dim": "rgba(4,20,16,0.60)", "shadowCol": "rgba(0,0,0,0.40)", "plateTop": "#fbf8ee", "plateBot": "#efe8d5", "plateEdge": "#0c1626", "plateShade": "#c5beaa", "plateRing": "#34d399", "lvInk": "#574500", "hpInk": "#d97706", "gauge": "#0c1626", "cmdTop": ["#f07070", "#f59e2b", "#2d6b57", "#9b6bf2"], "cmdBot": ["#d94444", "#d97706", "#1b4d3e", "#7c3aed"], "cmdEdge": "#0c1626", "cmdInk": "#ffffff", "em": "#e07a1f", "radius": "6", "selRadius": "0", "edge": "3", "shadow": "4", "shadowR": "0", "chamfer": "false", "gloss": "false", "bevel": "true", "stripes": "false"}, "crystal": {"faceTitle": "4", "faceBody": "5", "frameA": "rgba(8,36,62,0.84)", "barTop": "#2f8fc0", "barBot": "#0b3e63", "barLine": "#4fb3d6", "onFrame": "#ffffff", "onSoft": "#9cc7db", "title": "#9feaff", "paper0": "#ffffff", "paper1": "#eef6fa", "paper2": "#dcebf2", "paperEdge": "#1b4f72", "paperShade": "#bcd6e3", "paperStrip": "#e3f1f7", "stripInk": "#0e5e84", "stripSoft": "#6c8da3", "rowA": "#ffffff", "rowB": "#f4fafc", "rowLine": "#e6f0f5", "ink": "#12324a", "inkSoft": "#6c8da3", "selTop": "#d9faff", "selBot": "#4cc9ea", "selEdge": "#0a7fa6", "selInk": "#06283a", "selArrow": "#ffffff", "tabTop": "#ffffff", "tabBot": "#9fe4f6", "tabEdge": "#ffffff", "tabHi": "#ffffff", "tabInk": "#0a3550", "rail": "#031424", "railEdge": "#2b7faa", "keyTop": "#3aa6d6", "keyBot": "#0e4c70", "keyLight": "#cff4ff", "keyDark": "#0b3e63", "keyInk": "#ffffff", "capTop": "#0e4a73", "capBot": "#072c48", "capEdge": "#4fb3d6", "capHi": "#7fd3f0", "darkTop": "#11507c", "darkBot": "#072a44", "darkEdge": "#4fb3d6", "darkHi": "#8fe0ff", "winTop": "#2f8fc0", "winBot": "#1b6c9e", "winInk": "#ffffff", "chipTop": "#2f8fc0", "chipBot": "#155e8c", "chipEdge": "#cff4ff", "chipInk": "#ffffff", "dlgEdge": "#0b3e63", "dlgTop": "#ffffff", "dlgBot": "#ddeff7", "dlgShade": "#bcd6e3", "btnTop": ["#45c08a", "#3aa6d6", "#ee6a72"], "btnBot": ["#2a9a68", "#1d7eb0", "#cc4450"], "btnEdge": "#ffffff", "btnInk": "#ffffff", "accent": "0", "gold": "#ffe16b", "dim": "rgba(4,22,40,0.60)", "shadowCol": "rgba(0,0,0,0.35)", "plateTop": "#ffffff", "plateBot": "#ddeff7", "plateEdge": "#1b4f72", "plateShade": "#bcd6e3", "plateRing": "0", "lvInk": "#6c8da3", "hpInk": "#2f8fc0", "gauge": "#031424", "cmdTop": ["#ee6a72", "#ffcd5a", "#45c08a", "#3aa6d6"], "cmdBot": ["#cc4450", "#e4a01e", "#2a9a68", "#1d7eb0"], "cmdEdge": "#ffffff", "cmdInk": "#ffffff", "em": "#0a7fa6", "radius": "12", "selRadius": "18", "edge": "3", "shadow": "5", "shadowR": "0", "chamfer": "false", "gloss": "true", "bevel": "true", "stripes": "false"}, "tropical": {"faceTitle": "6", "faceBody": "7", "frameA": "rgba(12,104,112,0.84)", "barTop": "#ff9a2a", "barBot": "#f07a14", "barLine": "#2a1e3b", "onFrame": "#ffffff", "onSoft": "#ffe9cf", "title": "#ffd23f", "paper0": "#fff8ec", "paper1": "#fff1d8", "paper2": "#ffe3b5", "paperEdge": "#2a1e3b", "paperShade": "#f2d49c", "paperStrip": "#35d0c3", "stripInk": "#2a1e3b", "stripSoft": "#165e5a", "rowA": "#fff8ec", "rowB": "#fff2dc", "rowLine": "#f6e2be", "ink": "#2a1e3b", "inkSoft": "#9b8a78", "selTop": "#ffb13b", "selBot": "#ff8a1f", "selEdge": "#2a1e3b", "selInk": "#2a1e3b", "selArrow": "#fff8ec", "tabTop": "#fff8ec", "tabBot": "#ffeacd", "tabEdge": "#2a1e3b", "tabHi": "#ffffff", "tabInk": "#2a1e3b", "rail": "#e06a10", "railEdge": "#2a1e3b", "keyTop": "#35d0c3", "keyBot": "#1eaaa0", "keyLight": "#8cf0e6", "keyDark": "#2a1e3b", "keyInk": "#2a1e3b", "capTop": "#2a1e3b", "capBot": "#2a1e3b", "capEdge": "#2a1e3b", "capHi": "#46385a", "darkTop": "#2a1e3b", "darkBot": "#2a1e3b", "darkEdge": "#2a1e3b", "darkHi": "#46385a", "winTop": "#ff8a1f", "winBot": "#f07a14", "winInk": "#2a1e3b", "chipTop": "#35d0c3", "chipBot": "#35d0c3", "chipEdge": "#2a1e3b", "chipInk": "#2a1e3b", "dlgEdge": "#2a1e3b", "dlgTop": "#fff8ec", "dlgBot": "#fff1d8", "dlgShade": "#f2d49c", "btnTop": ["#35d0c3", "#ffd23f", "#ff7a7f"], "btnBot": ["#24b8ac", "#f0ba28", "#ec6066"], "btnEdge": "#2a1e3b", "btnInk": "#2a1e3b", "accent": "0", "gold": "#ffd23f", "dim": "rgba(42,30,59,0.60)", "shadowCol": "#2a1e3b", "plateTop": "#fff8ec", "plateBot": "#fff1d8", "plateEdge": "#2a1e3b", "plateShade": "#f2d49c", "plateRing": "0", "lvInk": "#9b8a78", "hpInk": "#1ea096", "gauge": "#2a1e3b", "cmdTop": ["#ff7a7f", "#ffd23f", "#35d0c3", "#ffb13b"], "cmdBot": ["#ec6066", "#f0ba28", "#24b8ac", "#ff8a1f"], "cmdEdge": "#2a1e3b", "cmdInk": "#2a1e3b", "em": "#e06a10", "radius": "14", "selRadius": "16", "edge": "4", "shadow": "6", "shadowR": "6", "chamfer": "false", "gloss": "false", "bevel": "false", "stripes": "false"}, "royal": {"faceTitle": "8", "faceBody": "9", "frameA": "rgba(4,34,26,0.84)", "barTop": "#11553f", "barBot": "#073326", "barLine": "#c9a646", "onFrame": "#ffffff", "onSoft": "#93b8a6", "title": "#f6e7a8", "paper0": "#f4f8f2", "paper1": "#e7f0e4", "paper2": "#d6e5d1", "paperEdge": "#0b3a2c", "paperShade": "#c5d2c9", "paperStrip": "#155e46", "stripInk": "#f6e7a8", "stripSoft": "#93b8a6", "rowA": "#f4f8f2", "rowB": "#eaf2e7", "rowLine": "#d3e2ce", "ink": "#10261d", "inkSoft": "#6a8577", "selTop": "#f4ffc9", "selBot": "#9bdb3a", "selEdge": "#3e7a0e", "selInk": "#13300a", "selArrow": "#3e7a0e", "tabTop": "#f4ffc9", "tabBot": "#82bc24", "tabEdge": "#3e7a0e", "tabHi": "#ffffff", "tabInk": "#13300a", "rail": "#031a13", "railEdge": "#8a9a92", "keyTop": "#ffffff", "keyBot": "#9aa8a1", "keyLight": "#ffffff", "keyDark": "#3b4a43", "keyInk": "#10261d", "capTop": "#0e4635", "capBot": "#062a20", "capEdge": "#c9a646", "capHi": "#4f9478", "darkTop": "#0f4a37", "darkBot": "#06281f", "darkEdge": "#c9a646", "darkHi": "#2e7a5e", "winTop": "#155e46", "winBot": "#0b3a2c", "winInk": "#f6e7a8", "chipTop": "#155e46", "chipBot": "#0b3a2c", "chipEdge": "#c9a646", "chipInk": "#f6e7a8", "dlgEdge": "#0b3a2c", "dlgTop": "#ffffff", "dlgBot": "#eef4eb", "dlgShade": "#c9a646", "btnTop": ["#2e9a70", "#f1cb5a", "#c14848"], "btnBot": ["#11553f", "#b98a1c", "#7a2020"], "btnEdge": "#0b3a2c", "btnInk": "#ffffff", "accent": "0", "gold": "#f3cf5b", "dim": "rgba(2,18,14,0.60)", "shadowCol": "rgba(0,0,0,0.45)", "plateTop": "#ffffff", "plateBot": "#eef4eb", "plateEdge": "#0b3a2c", "plateShade": "#c5d2c9", "plateRing": "#c9a646", "lvInk": "#6a8577", "hpInk": "#155e46", "gauge": "#031a13", "cmdTop": ["#cd5252", "#f1cb5a", "#2e9a70", "#60768c"], "cmdBot": ["#8c2424", "#b98a1c", "#11553f", "#344656"], "cmdEdge": "#0b3a2c", "cmdInk": "#ffffff", "em": "#3e7a0e", "radius": "10", "selRadius": "0", "edge": "3", "shadow": "4", "shadowR": "0", "chamfer": "true", "gloss": "false", "bevel": "true", "stripes": "false"}};
const id = (new URLSearchParams(location.search).get('theme') || '').toLowerCase(), T = TH[id];
if (!T) return;
const FONTS = { red: [['800', 'normal', 'Saira Condensed', 20 / 16], ['700', 'normal', 'Saira Condensed', 26 / 32, 32 / 40]],
  emerald: null,
  crystal: [['800', 'normal', 'M PLUS Rounded 1c', 20 / 16], ['800', 'normal', 'M PLUS Rounded 1c', 20 / 32, 25 / 40]],
  tropical: [['600', 'normal', 'Fredoka', 20 / 16], ['500', 'normal', 'Fredoka', 24 / 32, 29 / 40]],
  royal: [['800', 'italic', 'Exo 2', 20 / 16], ['700', 'normal', 'Exo 2', 22 / 32, 26 / 40]] }[id];
// CSS tokens of ui.css
const VARS = { '--frame-0-a': 'frameA', '--frame-top': 'barTop', '--frame-bot': 'barBot', '--line-dark': 'barLine',
  '--edge-dark': 'darkEdge', '--hi-dark': 'darkHi', '--on-frame': 'onFrame', '--on-frame-soft': 'onSoft', '--mint': 'title',
  '--mint-2': 'keyInk', '--paper-0': 'paper0', '--paper-1': 'paper1', '--paper-2': 'paper2', '--paper-strip': 'paperStrip',
  '--paper-edge': 'paperEdge', '--paper-shade': 'paperShade', '--row-line': 'rowLine', '--ink': 'ink', '--ink-soft': 'inkSoft',
  '--accent': 'em', '--sel-top': 'selTop', '--sel-bot': 'selBot', '--sel-edge': 'selEdge', '--sel-ink': 'selInk',
  '--strip-ink': 'stripInk', '--tab-top': 'tabTop', '--tab-bot': 'tabBot', '--tab-edge': 'tabEdge', '--gold': 'gold', '--frame-1': 'capBot', '--frame-2': 'darkTop' };
// Colours written as such in ui.css and the pages (the EMERALD design)
const HARD = { '#123a31': 'capTop', '#0a241e': 'capBot', '#2e6b5b': 'capHi', '#041612': 'rail', '#1c5547': 'railEdge', '#fffbe0': 'tabHi',
  '#1b3a32': 'keyTop', '#0c201a': 'keyBot', '#366d5f': 'keyLight', '#061511': 'keyDark', '#1c382f': 'winTop', '#0b231d': 'winBot',
  '#0c2f27': 'darkTop', '#071d18': 'darkBot', '#051614': 'darkBot', '#f1ebd8': 'rowB', '#245244': 'chipTop', '#163a30': 'chipBot',
  '#d1fae5': 'chipInk', '#0b2e26': 'chipEdge', '#3c7a68': 'chipTop', '#2f7a5c': ['btnTop', 0], '#1d5a43': ['btnBot', 0],
  '#e08a2a': ['btnTop', 1], '#b8661a': ['btnBot', 1], '#a83a3a': ['btnTop', 2], '#7a2626': ['btnBot', 2], '#172a22': 'btnEdge',
  '#fbfaf2': 'dlgTop', '#ebe7d3': 'dlgBot', '#9e9784': 'dlgShade', '#26272d': 'dlgEdge', '#fffae0': 'selTop', '#fed87b': 'selBot',
  '#d64d1f': 'selEdge', '#0e2b25': 'barTop', '#061a16': 'barBot', '#2b5849': 'barLine', '#041210': 'darkEdge', '#1b544b': 'darkHi',
  '#99f6e4': 'title', '#6ee7b7': 'keyInk', '#7ea69c': 'onSoft', '#16271c': 'ink', '#728277': 'inkSoft', '#f7f4e9': 'paper0',
  '#ebe4cf': 'paper1', '#dfd7bf': 'paper2', '#e7dfc6': 'paperStrip', '#1a3024': 'paperEdge', '#d1c8af': 'paperShade', '#d8cfb7': 'rowLine',
  '#b45309': 'em', '#fff7d6': 'selTop', '#fde68a': 'selBot', '#d97706': 'selEdge', '#3a2a00': 'selInk', '#fceda6': 'tabTop',
  '#e2ab24': 'tabBot', '#8c6006': 'tabEdge', '#fcd34d': 'gold', '#09211d': 'capBot', '#34d399': 'barLine' };
const val = k => Array.isArray(k) ? T[k[0]][k[1]] : T[k];
const root = document.documentElement.style;
for (const [v, k] of Object.entries(VARS)) if (T[k]) root.setProperty(v, T[k]);
root.setProperty('--r', Math.min(+T.radius, 10) + 'px');
const hex = /#[0-9a-fA-F]{6}\b/g;
const swap = s => s.replace(hex, h => { const k = HARD[h.toLowerCase()]; return k && val(k) ? val(k) : h; });
function sheets() {
  for (const ss of document.styleSheets) {
    let rules;
    try { rules = ss.cssRules; } catch (e) { continue; }
    const walk = rs => {
      for (const r of rs) {
        if (r.cssRules) walk(r.cssRules);
        if (!r.style) continue;
        for (let i = 0; i < r.style.length; i++) {
          const p = r.style[i], v = r.style.getPropertyValue(p);
          if (p.startsWith('--')) continue;
          const n = swap(v);
          if (n !== v) r.style.setProperty(p, n, r.style.getPropertyPriority(p));
        }
      }
    };
    walk(rules);
  }
}
function inline() {
  document.querySelectorAll('[style]').forEach(el => { const s = el.getAttribute('style'), n = swap(s); if (n !== s) el.setAttribute('style', n); });
}
function fonts() {
  if (!FONTS) return;
  const all = [...document.querySelectorAll('.stage *')].map(el => { const cs = getComputedStyle(el); return [el, cs.fontFamily, parseFloat(cs.fontSize)]; });
  all.forEach(([el, fam, px]) => {
    if (el.dataset.thf) return;
    let f = null;
    if (/Press Start/.test(fam)) f = [FONTS[0], px * FONTS[0][3]];
    else if (/VT323/.test(fam)) f = [FONTS[1], px * (px >= 36 ? FONTS[1][4] : FONTS[1][3])];
    if (!f) return;
    el.dataset.thf = 1;
    el.style.fontFamily = '"' + f[0][2] + '"'; el.style.fontWeight = f[0][0]; el.style.fontStyle = f[0][1]; el.style.fontSize = Math.round(f[1]) + 'px';
  });
}
const link = document.createElement('link'); link.rel = 'stylesheet';
link.href = 'https://fonts.googleapis.com/css2?family=Saira+Condensed:wght@700;800&family=M+PLUS+Rounded+1c:wght@500;800&family=Fredoka:wght@500;600&family=Exo+2:ital,wght@0,700;1,800&display=block';
document.head.appendChild(link);
const run = () => { sheets(); inline(); fonts(); };
run(); window.addEventListener('load', run); setTimeout(run, 300); setTimeout(run, 1200);
new MutationObserver(() => { inline(); fonts(); }).observe(document.body, { childList: true, subtree: true });
})();
