// Menu themes on the battle HUD (OPTIONS > MENU COLOR: RED, EMERALD, CRYSTAL, TROPICAL, ROYAL).
// ?theme=red|emerald|crystal|tropical|royal (none: the original EMERALD look of this mock)
// ?battle=wild|trainer|double|safari  ?mode=cmd|fight|target|msg|yesno|lvup|judge
// Colours, corner shape, edge, shadow, gloss and fonts are the runtime's (src/platform/ui/ui_theme.c).
// Every box is drawn as an SVG background the size of its element, as BevelBoxR draws it: hard
// shadow, edge, vertical gradient, inner highlight / shade, glass on the upper half.
(() => {
const P = new URLSearchParams(location.search);
const id = (P.get('theme') || '').toLowerCase();

const rgb = (r, g, b) => `rgb(${r},${g},${b})`;
const TH = {
  red: {
    title: ['800', 'normal', 'Saira Condensed', 20 / 16], body: ['700', 'normal', 'Saira Condensed', 26 / 32, 32 / 40],
    r: 10, chamfer: true, edgeW: 2, sh: [6, 6, 'rgba(0,0,0,.35)'], gloss: false, bevel: false,
    ink: rgb(38, 38, 46), inkSoft: rgb(138, 138, 150), em: rgb(212, 32, 44), title_c: rgb(255, 255, 255), soft: rgb(183, 183, 194),
    paper: [rgb(250, 250, 250), rgb(240, 240, 242), rgb(43, 43, 51), '#fff', rgb(213, 213, 220)],
    dlg: [rgb(250, 250, 250), rgb(240, 240, 242), rgb(43, 43, 51), '#fff', rgb(213, 213, 220)],
    dark: [rgb(50, 50, 60), rgb(36, 36, 44), rgb(20, 20, 26), rgb(70, 70, 80), rgb(36, 36, 44)],
    rowA: rgb(250, 250, 250), rowB: rgb(242, 242, 245), rowEdge: rgb(43, 43, 51),
    sel: [rgb(242, 64, 74), rgb(212, 32, 44), rgb(142, 14, 23)], selInk: '#fff', gold: rgb(255, 201, 60), accent: rgb(224, 48, 58),
    chip: [rgb(224, 48, 58), rgb(190, 30, 40), rgb(120, 14, 22), '#fff'], gaugeBg: rgb(20, 20, 26),
    cmd: { fight: [rgb(242, 64, 74), rgb(212, 32, 44)], bag: [rgb(255, 184, 64), rgb(232, 146, 20)],
           mon: [rgb(60, 60, 70), rgb(43, 43, 51)], run: [rgb(150, 150, 162), rgb(110, 110, 122)] },
    cmdEdge: rgb(20, 20, 26), cmdInk: '#fff', stripe: true,
  },
  emerald: {
    title: ['400', 'normal', 'Press Start 2P', 1], body: ['400', 'normal', 'VT323', 1, 1],
    r: 6, chamfer: false, edgeW: 3, sh: [0, 4, 'rgba(0,0,0,.45)'], gloss: false, bevel: true,
    ink: rgb(22, 39, 28), inkSoft: rgb(114, 130, 119), em: '#B45309', title_c: '#99F6E4', soft: '#7EA69C',
    paper: ['#FBF8EE', '#EFE8D5', '#0C1626', '#fff', '#C5BEAA', '#34D399'],
    dlg: ['#FBFAF2', '#EBE7D3', '#26272D', '#fff', '#9E9784'],
    dark: ['#0E2B25', '#061A16', '#041210', '#1B544B', '#061A16', '#34D399'],
    rowA: '#FFFFFF', rowB: '#F1ECDD', rowEdge: '#26272D',
    sel: ['#FFF7D6', '#FDE68A', '#D97706'], selInk: '#3A2A00', gold: '#FCD34D', accent: '#34D399',
    chip: ['#D97706', '#D97706', '#7C3A00', '#fff'], gaugeBg: '#0C1626',
    cmd: { fight: ['#F07070', '#D94444'], bag: ['#F59E2B', '#D97706'], mon: ['#2D6B57', '#1B4D3E'], run: ['#9B6BF2', '#7C3AED'] },
    cmdEdge: '#0C1626', cmdInk: '#fff',
  },
  crystal: {
    title: ['800', 'normal', 'M PLUS Rounded 1c', 20 / 16], body: ['800', 'normal', 'M PLUS Rounded 1c', 20 / 32, 25 / 40],
    r: 12, chamfer: false, edgeW: 3, sh: [0, 5, 'rgba(0,0,0,.35)'], gloss: true, bevel: true,
    ink: rgb(18, 50, 74), inkSoft: rgb(108, 141, 163), em: rgb(10, 127, 166), title_c: rgb(159, 234, 255), soft: rgb(156, 199, 219),
    paper: [rgb(255, 255, 255), rgb(221, 239, 247), rgb(27, 79, 114), '#fff', rgb(188, 214, 227)],
    dlg: [rgb(255, 255, 255), rgb(221, 239, 247), rgb(11, 62, 99), '#fff', rgb(188, 214, 227)],
    dark: [rgb(17, 80, 124), rgb(7, 42, 68), rgb(79, 179, 214), rgb(143, 224, 255), rgb(7, 42, 68)],
    rowA: rgb(255, 255, 255), rowB: rgb(238, 246, 250), rowEdge: rgb(27, 79, 114),
    sel: [rgb(217, 250, 255), rgb(76, 201, 234), rgb(10, 127, 166)], selInk: rgb(6, 40, 58), gold: rgb(255, 225, 107), accent: rgb(79, 179, 214),
    chip: [rgb(47, 143, 192), rgb(21, 94, 140), rgb(207, 244, 255), '#fff'], gaugeBg: rgb(3, 20, 36),
    cmd: { fight: [rgb(238, 106, 114), rgb(204, 68, 80)], bag: [rgb(255, 205, 90), rgb(228, 160, 30)],
           mon: [rgb(69, 192, 138), rgb(42, 154, 104)], run: [rgb(58, 166, 214), rgb(29, 126, 176)] },
    cmdEdge: '#fff', cmdInk: '#fff', pill: true,
  },
  tropical: {
    title: ['600', 'normal', 'Fredoka', 20 / 16], body: ['500', 'normal', 'Fredoka', 24 / 32, 29 / 40],
    r: 14, chamfer: false, edgeW: 4, sh: [6, 6, rgb(42, 30, 59)], gloss: false, bevel: false,
    ink: rgb(42, 30, 59), inkSoft: rgb(155, 138, 120), em: rgb(224, 106, 16), title_c: rgb(255, 210, 63), soft: rgb(255, 233, 207),
    paper: [rgb(255, 248, 236), rgb(255, 241, 216), rgb(42, 30, 59), '#fff', rgb(242, 212, 156)],
    dlg: [rgb(255, 248, 236), rgb(255, 241, 216), rgb(42, 30, 59), '#fff', rgb(242, 212, 156)],
    dark: [rgb(42, 30, 59), rgb(42, 30, 59), rgb(42, 30, 59), rgb(70, 56, 90), rgb(42, 30, 59)],
    rowA: rgb(255, 248, 236), rowB: rgb(255, 242, 220), rowEdge: rgb(42, 30, 59),
    sel: [rgb(255, 177, 59), rgb(255, 138, 31), rgb(42, 30, 59)], selInk: rgb(42, 30, 59), gold: rgb(255, 210, 63), accent: rgb(53, 208, 195),
    chip: [rgb(53, 208, 195), rgb(53, 208, 195), rgb(42, 30, 59), rgb(42, 30, 59)], gaugeBg: rgb(42, 30, 59),
    cmd: { fight: [rgb(255, 122, 127), rgb(236, 96, 102)], bag: [rgb(255, 210, 63), rgb(240, 186, 40)],
           mon: [rgb(53, 208, 195), rgb(36, 184, 172)], run: [rgb(255, 177, 59), rgb(255, 138, 31)] },
    cmdEdge: rgb(42, 30, 59), cmdInk: rgb(42, 30, 59), pill: true,
  },
  royal: {
    title: ['800', 'italic', 'Exo 2', 20 / 16], body: ['700', 'normal', 'Exo 2', 22 / 32, 26 / 40],
    r: 10, chamfer: true, edgeW: 3, sh: [0, 4, 'rgba(0,0,0,.45)'], gloss: false, bevel: true,
    ink: rgb(16, 38, 29), inkSoft: rgb(106, 133, 119), em: rgb(62, 122, 14), title_c: rgb(246, 231, 168), soft: rgb(147, 184, 166),
    paper: [rgb(255, 255, 255), rgb(238, 244, 235), rgb(11, 58, 44), '#fff', rgb(197, 210, 201), rgb(201, 166, 70)],
    dlg: [rgb(255, 255, 255), rgb(238, 244, 235), rgb(11, 58, 44), '#fff', rgb(201, 166, 70)],
    dark: [rgb(15, 74, 55), rgb(6, 40, 31), rgb(201, 166, 70), rgb(46, 122, 94), rgb(6, 40, 31)],
    rowA: rgb(244, 248, 242), rowB: rgb(234, 242, 231), rowEdge: rgb(11, 58, 44),
    sel: [rgb(244, 255, 201), rgb(155, 219, 58), rgb(62, 122, 14)], selInk: rgb(19, 48, 10), gold: rgb(243, 207, 91), accent: rgb(201, 166, 70),
    chip: [rgb(21, 94, 70), rgb(11, 58, 44), rgb(201, 166, 70), rgb(246, 231, 168)], gaugeBg: rgb(3, 26, 19),
    cmd: { fight: [rgb(205, 82, 82), rgb(140, 36, 36)], bag: [rgb(241, 203, 90), rgb(185, 138, 28)],
           mon: [rgb(46, 154, 112), rgb(17, 85, 63)], run: [rgb(96, 118, 140), rgb(52, 70, 86)] },
    cmdEdge: rgb(11, 58, 44), cmdInk: '#fff',
  },
};
const T = TH[id];

// ---- Shapes ----
let uid = 0;
function path(x, y, w, h, r, chamfer) {
  r = Math.max(0, Math.min(r, w / 2, h / 2));
  if (r === 0) return `M${x},${y}h${w}v${h}h${-w}Z`;
  if (chamfer) return `M${x + r},${y}H${x + w - r}L${x + w},${y + r}V${y + h - r}L${x + w - r},${y + h}H${x + r}L${x},${y + h - r}V${y + r}Z`;
  return `M${x + r},${y}H${x + w - r}A${r},${r} 0 0 1 ${x + w},${y + r}V${y + h - r}A${r},${r} 0 0 1 ${x + w - r},${y + h}H${x + r}A${r},${r} 0 0 1 ${x},${y + h - r}V${y + r}A${r},${r} 0 0 1 ${x + r},${y}Z`;
}
// opts: top, bot, edge, hi, shade, ring (gold / accent outline), r, flatBottom
function box(el, o) {
  const w = el.offsetWidth, h = el.offsetHeight;
  if (!w || !h) return;
  const e = o.edgeW ?? T.edgeW, r = o.r ?? (h < 24 ? Math.min(T.r, 4) : T.r);
  const [sx, sy, sc] = o.noShadow ? [0, 0, 'none'] : T.sh;
  const ringW = o.ring ? 3 : 0, pad = ringW;
  const W = w + 2 * pad + sx, H = h + 2 * pad + sy, g = 'g' + (++uid);
  let s = `<svg xmlns="http://www.w3.org/2000/svg" width="${W}" height="${H}"><defs><linearGradient id="${g}" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="${o.top}"/><stop offset="1" stop-color="${o.bot}"/></linearGradient></defs>`;
  if (sx || sy) s += `<path d="${path(pad + sx, pad + sy, w, h, r, T.chamfer)}" fill="${sc}"/>`;
  if (ringW) s += `<path d="${path(0, 0, w + 2 * pad, h + 2 * pad, r + ringW, T.chamfer)}" fill="${o.ring}"/>`;
  s += `<path d="${path(pad, pad, w, h, r, T.chamfer)}" fill="${o.edge}"/>`;
  const fr = Math.max(0, r - e);
  s += `<path d="${path(pad + e, pad + e, w - 2 * e, h - 2 * e, fr, T.chamfer)}" fill="url(#${g})"/>`;
  if (T.bevel && o.hi) {
    const b = 2, x0 = pad + e + fr, x1 = pad + w - e - fr, y0 = pad + e + fr, y1 = pad + h - e - fr;
    s += `<rect x="${x0}" y="${pad + e}" width="${x1 - x0}" height="${b}" fill="${o.hi}"/><rect x="${pad + e}" y="${y0}" width="${b}" height="${y1 - y0}" fill="${o.hi}"/>`;
    s += `<rect x="${x0}" y="${pad + h - e - b}" width="${x1 - x0}" height="${b}" fill="${o.shade}"/><rect x="${pad + w - e - b}" y="${y0}" width="${b}" height="${y1 - y0}" fill="${o.shade}"/>`;
  }
  if ((T.gloss || o.gloss) && h > 2 * e + 6)
    s += `<path d="${path(pad + e + 2, pad + e + 1, w - 2 * e - 4, (h - 2 * e) / 2 - 1, Math.max(0, fr - 2), T.chamfer)}" fill="rgba(255,255,255,.28)"/>`;
  if (o.accentBar) s += `<rect x="${pad}" y="${pad + (T.chamfer ? r : 0)}" width="6" height="${h - (T.chamfer ? 2 * r : 0)}" fill="${o.accentBar}"/>`;
  s += '</svg>';
  el.style.background = 'none';
  el.style.border = 'none';
  el.style.boxShadow = 'none';
  el.style.borderRadius = '0';
  el.style.outline = 'none';
  el.style.overflow = 'visible';
  // The SVG overhangs the element by the ring and the shadow: drawn by a ::before-like layer
  const layer = document.createElement('u');
  layer.className = 'th-bg';
  layer.style.cssText = `position:absolute;left:${-pad - (getComputedStyle(el).borderLeftWidth ? 0 : 0)}px;top:${-pad}px;width:${W}px;height:${H}px;z-index:-1;pointer-events:none;background:url("data:image/svg+xml,${encodeURIComponent(s)}") no-repeat;`;
  if (getComputedStyle(el).position === 'static') el.style.position = 'relative';
  el.style.zIndex = el.style.zIndex || '0';
  el.style.isolation = 'isolate';
  el.prepend(layer);
}

// ---- Fonts ----
function fonts(root) {
  // Read every element's font first: once a parent changes, its children would inherit the new
  // family and be skipped
  const all = [...root.querySelectorAll('*')].filter(el => !el.classList.contains('th-bg'))
    .map(el => { const cs = getComputedStyle(el); return [el, cs.fontFamily, parseFloat(cs.fontSize)]; });
  all.forEach(([el, fam, px]) => {
    if (/Press Start/.test(fam)) {
      el.style.fontFamily = `"${T.title[2]}"`; el.style.fontWeight = T.title[0]; el.style.fontStyle = T.title[1];
      el.style.fontSize = Math.round(px * T.title[3]) + 'px';
    } else if (/VT323/.test(fam)) {
      el.style.fontFamily = `"${T.body[2]}"`; el.style.fontWeight = T.body[0]; el.style.fontStyle = T.body[1];
      el.style.fontSize = Math.round(px * (px >= 36 ? T.body[4] : T.body[3])) + 'px';
    }
  });
}

function apply() {
  const st = document.getElementById('stage');
  st.style.setProperty('--ink', T.ink);
  const q = s => st.querySelectorAll(s);
  // Fonts first: the sizes of the boxes do not depend on them, but the text has to be in place
  fonts(st);
  q('.plate').forEach(el => {
    const turn = el.classList.contains('turn');
    box(el, { top: T.paper[0], bot: T.paper[1], edge: T.paper[2], hi: T.paper[3], shade: T.paper[4], ring: turn ? T.gold : T.paper[5] });
  });
  q('.plate.turn').forEach(el => el.style.setProperty('--turn', T.gold));
  q('.plate .lv').forEach(el => el.style.color = T.inkSoft);
  q('.hpchip').forEach(el => box(el, { top: T.chip[0], bot: T.chip[1], edge: T.chip[2], r: 3, noShadow: true, edgeW: 2 }) || (el.style.color = T.chip[3]));
  q('.hpchip').forEach(el => el.style.color = T.chip[3]);
  q('.gauge, .exp').forEach(el => { el.style.background = T.gaugeBg; el.style.borderColor = T.gaugeBg; el.style.borderRadius = T.chamfer ? '0' : Math.min(T.r, 6) + 'px'; });
  q('.balls').forEach(el => box(el, { top: T.dark[0], bot: T.dark[1], edge: T.dark[2], hi: T.dark[3], shade: T.dark[4], ring: T.dark[5] }));
  q('.box').forEach(el => box(el, { top: T.dlg[0], bot: T.dlg[1], edge: T.dlg[2], hi: T.dlg[3], shade: T.dlg[4], noShadow: true,
                                    accentBar: id === 'red' ? T.accent : null }));
  q('.msg em').forEach(el => el.style.color = T.em);
  q('.more').forEach(el => el.style.color = T.em);
  q('.cmd').forEach(el => {
    const k = ['fight', 'bag', 'mon', 'run'].find(c => el.classList.contains(c)), c = T.cmd[k], on = el.classList.contains('on');
    box(el, { top: c[0], bot: c[1], edge: T.cmdEdge, hi: 'rgba(255,255,255,.45)', shade: 'rgba(0,0,0,.25)', ring: on ? T.gold : null,
              r: T.pill ? 28 : undefined });
    el.style.color = T.cmdInk;
  });
  q('.mv').forEach((el, i) => {
    const on = el.classList.contains('on');
    box(el, on ? { top: T.sel[0], bot: T.sel[1], edge: T.sel[2], hi: 'rgba(255,255,255,.6)', shade: 'rgba(0,0,0,.15)', r: T.pill ? 28 : undefined }
               : { top: i % 2 ? T.rowB : T.rowA, bot: T.rowB, edge: T.rowEdge, hi: '#fff', shade: T.paper[4], noShadow: true, r: T.pill ? 28 : undefined });
    el.style.color = on ? T.selInk : T.ink;
  });
  const darkBox = el => box(el, { top: T.dark[0], bot: T.dark[1], edge: T.dark[2], hi: T.dark[3], shade: T.dark[4], ring: T.dark[5] });
  q('.mvinfo, .lvup, .judge').forEach(darkBox);
  q('.mvinfo .k small, .lvup .hd, .judge .t').forEach(el => el.style.color = T.title_c);
  q('.lvup .k small, .judge .row3 span:nth-child(2)').forEach(el => el.style.color = T.soft);
  q('.lvup .k, .judge .row3').forEach(el => el.style.borderColor = T.dark[3]);
  q('.judge .tot').forEach(el => el.style.borderColor = T.accent);
  q('.choice').forEach(el => box(el, { top: T.dlg[0], bot: T.dlg[1], edge: T.dlg[2], hi: T.dlg[3], shade: T.dlg[4] }));
  q('.choice .row').forEach(el => {
    el.style.background = 'transparent'; el.style.borderColor = 'transparent';
    if (el.classList.contains('on')) box(el, { top: T.sel[0], bot: T.sel[1], edge: T.sel[2], noShadow: true, r: T.pill ? 18 : 4, edgeW: 2 });
    el.style.color = el.classList.contains('on') ? T.selInk : T.ink;
  });
  q('.tchip').forEach(el => { el.style.borderRadius = T.pill ? '12px' : T.chamfer ? '0' : '3px'; });
}

if (T) {
  const css = document.createElement('style');
  css.textContent = `@import url('https://fonts.googleapis.com/css2?family=Saira+Condensed:wght@700;800&family=M+PLUS+Rounded+1c:wght@500;800&family=Fredoka:wght@500;600&family=Exo+2:ital,wght@0,700;1,800&display=block');
    .plate.turn::before { color: ${T.gold}; }
    .cmd.on::before { color: ${T.cmdInk === '#fff' ? T.gold : T.cmdInk}; z-index: 1; }
    .mv.on::before { color: ${T.selInk}; z-index: 1; }
    .choice .row.on::before { color: ${T.selInk}; z-index: 1; }
    u.th-bg { text-decoration: none; }
    .stage::after { content: '${id.toUpperCase()}'; position: absolute; right: 10px; top: 8px; font: 12px monospace; color: rgba(255,255,255,.6); }`;
  document.head.appendChild(css);
}

// URL state for the captures
if (P.get('battle')) S.battle = P.get('battle');
if (P.get('mode')) S.mode = P.get('mode');
if (P.get('cmd')) S.cmd = +P.get('cmd');
if (P.get('mv')) S.mv = +P.get('mv');
if (P.get('target')) S.target = +P.get('target');
if (P.get('active')) S.active = +P.get('active');
const base = render;
render = function () { base(); if (T) apply(); };
document.fonts.ready.then(() => render());
render();
})();
