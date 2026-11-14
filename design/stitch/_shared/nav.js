/* Navigation model of the new UI (one table for every full screen).
   The runtime implements the same table: it drives the header (section title + tab selector),
   L/R, touch on the tabs and B. See README.md, "Navegación".

   - sections: the menus opened from the START menu. The title is a fixed label; there is no
     section switcher (removed on 2026-11-06): to change menu, B goes back to the START menu.
   - tabs: the header tab selector of each section, in the order of the Stitch designs.
     L / R move to the previous / next tab and wrap around. A tab either opens another screen
     (page) or changes the content of the same screen (no page: the mock only moves the highlight).
   - B: back to the previous level (modal → screen → hub START). */
const UI_NAV = {
  sections: [
    { id: 'pokedex', label: 'POKéDEX', home: 'pokedex',
      tabs: [{ id: 'hoenn', label: 'HOENN' }, { id: 'national', label: 'NATIONAL' }] },
    { id: 'pokedex_detail', label: 'POKéDEX', parent: 'pokedex', home: 'pokedex_detalle', hidden: true,
      tabs: [{ id: 'info', label: 'INFO' }, { id: 'area', label: 'AREA' }, { id: 'cry', label: 'CRY' }, { id: 'size', label: 'SIZE' }] },
    { id: 'party', label: 'POKéMON', home: 'equipo',
      // The 4 summary pages of the current UI (vita_ui_party.c SummaryTab)
      tabs: [{ id: 'info', label: 'INFO', page: 'equipo' }, { id: 'skills', label: 'SKILLS', page: 'equipo' },
             { id: 'moves', label: 'MOVES', page: 'equipo_movimientos' }, { id: 'contest', label: 'CONTEST', page: 'equipo_movimientos' }] },
    { id: 'bag', label: 'BAG', home: 'mochila',
      tabs: [{ id: 'items', label: 'ITEMS' }, { id: 'balls', label: 'POKé BALLS' }, { id: 'tms', label: 'TMs & HMs' },
             { id: 'berries', label: 'BERRIES' }, { id: 'key', label: 'KEY ITEMS' }] },
    { id: 'pokenav', label: 'POKéNAV', home: 'pokenav_mapa',
      tabs: [{ id: 'map', label: 'HOENN MAP', page: 'pokenav_mapa' }, { id: 'call', label: 'MATCH CALL', page: 'pokenav_match_call' },
             { id: 'condition', label: 'CONDITION', page: 'pokenav_condicion' }, { id: 'ribbons', label: 'RIBBONS', page: 'pokenav' }] },
    { id: 'card', label: 'TRAINER CARD', home: 'ficha',
      tabs: [{ id: 'front', label: 'CARD' }, { id: 'back', label: 'PASSPORT' }] },
    { id: 'options', label: 'OPTIONS', home: 'opciones',
      // The 5 tabs of the current options (VITA_TAB_*)
      tabs: [{ id: 'general', label: 'GENERAL' }, { id: 'world', label: 'WORLD' }, { id: 'battle', label: 'BATTLE' }, { id: 'modes', label: 'MODES' }, { id: 'cheats', label: 'CHEATS' }] },
    { id: 'mart', label: 'POKé MART', home: 'tienda_comprar', hidden: true,
      tabs: [{ id: 'buy', label: 'BUY', page: 'tienda_comprar' }, { id: 'sell', label: 'SELL', page: 'tienda_vender' }] },
    // Screens opened from other screens (no START entry of their own)
    { id: 'fly', label: 'FLY', home: 'vuelo', parent: 'party', hidden: true, tabs: [{ id: 'map', label: 'HOENN' }] },
    // Opened from the bag (USE / GIVE / TM): "choose a POKéMON"; the mode is in the header capsule
    { id: 'target', label: 'POKéMON', home: 'equipo_objetivo', parent: 'bag', hidden: true,
      tabs: [{ id: 'use', label: 'CHOOSE A POKéMON' }] },
    { id: 'target_tm', label: 'POKéMON', home: 'equipo_mt', parent: 'bag', hidden: true,
      tabs: [{ id: 'tm', label: 'CHOOSE A POKéMON' }] },
    { id: 'nuzlocke', label: 'NUZLOCKE', home: 'nuzlocke', hidden: true,
      tabs: [{ id: 'routes', label: 'AREAS' }, { id: 'squad', label: 'SQUAD' }, { id: 'rules', label: 'RULES' }, { id: 'settings', label: 'SETTINGS' }] },
    // In battle: opened by the battle UI (PlayerHandleChooseItem / ChoosePokemon); B goes back to it
    { id: 'bag_battle', label: 'BAG', home: 'mochila_combate', hidden: true, back: 'battle',
      tabs: [{ id: 'items', label: 'ITEMS' }, { id: 'balls', label: 'POKé BALLS' }, { id: 'berries', label: 'BERRIES' }] },
    { id: 'party_battle', label: 'POKéMON', home: 'equipo_combate', hidden: true, back: 'battle',
      tabs: [{ id: 'status', label: 'CHOOSE A POKéMON' }] },
    // Screens still on the GBA UI (2026-11-13 designs): PC, naming, mail, Battle Pyramid bag,
    // contest appeals and Berry Blender. The multi battle party is equipo_combate in multi mode.
    // PC: the mode (WITHDRAW / DEPOSIT / MOVE POKéMON / MOVE ITEMS) is chosen in the PC menu
    // before entering, as in the game; L / R change the box. ○ goes back to the PC menu.
    { id: 'pc', label: 'PC', home: 'pc_cajas', hidden: true, back: 'screen', lr: 'screen',
      tabs: [{ id: 'move', label: 'MOVE POKéMON' }] },
    { id: 'naming', label: 'NAME', home: 'teclado_nombre', hidden: true, back: 'screen',
      tabs: [{ id: 'upper', label: 'ABC' }, { id: 'lower', label: 'abc' }, { id: 'others', label: '?!·' }] },
    { id: 'mail', label: 'MAIL', home: 'correo', hidden: true, back: 'screen',
      tabs: [{ id: 'groups', label: 'GROUPS' }, { id: 'abc', label: 'A-Z' }] },
    { id: 'bag_pyramid', label: 'BATTLE BAG', home: 'mochila_piramide', hidden: true, back: 'screen',
      tabs: [{ id: 'items', label: 'ITEMS' }] },
    { id: 'contest', label: 'CONTEST', home: 'concurso', hidden: true, back: 'screen',
      tabs: [{ id: 'appeal', label: 'APPEAL' }] },
    { id: 'blender', label: 'BERRY BLENDER', home: 'batidora', hidden: true, back: 'screen',
      tabs: [{ id: 'berry', label: 'CHOOSE A BERRY' }] },
  ],
};

/* Popups and dropdowns: [data-open="id"] opens #id; URL #id opens it on load (for review);
   [data-close] or ○/B/Escape closes the top one. While a modal is open, L/R do nothing. */
const UI_MODALS = window.UI_MODALS = {
  stack: [],
  open(id) {
    const el = document.getElementById(id);
    if (!el) return;
    el.classList.add('open');
    this.stack.push(el);
  },
  closeTop() {
    const el = this.stack.pop();
    if (el) el.classList.remove('open');
    return !!el;
  },
};
/* ---------- Touch model (the runtime does the same with the Vita front touch panel) ----------
   - Selectable items (list rows, party cards, moves, options): the first tap selects (moves the
     yellow cursor), a tap on the already selected item activates it, exactly like ✕.
   - Activation: data-go="screen" opens that screen, data-open="id" opens a popup,
     data-close closes the top popup.
   - The footer is a GUIDE for the physical buttons, never a touch target (data-btn says which
     button; in the mocks the keyboard emulates them: X=✕, T=△, S=□, V=SELECT, Enter=START,
     Esc/B=○, Q/E or L/R = shoulders, arrows = d-pad).
   - Lists whose content does not fit scroll by dragging; a short scroll bar shows the position. */
const SELECTABLE = '.row, .card, .mv, .opt, .rb, .b';
const go = page => { location.href = '../' + page + '/index.html'; };
window.UI_ACTIONS = window.UI_ACTIONS || {};   // screen actions: data-action="name" → UI_ACTIONS.name(el)
function activate(el) {
  if (!el) return false;
  if (el.dataset.action && window.UI_ACTIONS[el.dataset.action]) { window.UI_ACTIONS[el.dataset.action](el); return true; }
  if (el.dataset.go) { go(el.dataset.go); return true; }
  if (el.dataset.open) { UI_MODALS.open(el.dataset.open); return true; }
  if (el.hasAttribute('data-close')) { UI_MODALS.closeTop(); return true; }
  return false;
}
function selectItem(el) {
  const parent = el.parentElement;
  parent.querySelectorAll(':scope > ' + SELECTABLE.split(', ').join(', :scope > ')).forEach(s => s.classList.remove('on'));
  el.classList.add('on');
  // Screens listen to this to refresh their detail panels (selection drives the content)
  document.dispatchEvent(new CustomEvent('ui-select', { detail: el }));
}
window.selectItem = selectItem;
let suppressClick = false;
document.addEventListener('click', e => {
  if (suppressClick) { suppressClick = false; e.stopPropagation(); e.preventDefault(); return; }
  if (e.target.closest('.footer, .win-foot')) return;          // button guides: not touchable
  const item = e.target.closest(SELECTABLE);
  if (item && !item.closest('.header')) {
    if (!item.classList.contains('on')) { selectItem(item); return; }
    if (activate(item)) return;
  }
  const t = e.target.closest('[data-action], [data-go], [data-open], [data-close]');
  if (t && activate(t)) { e.stopPropagation(); return; }
  if (e.target.classList.contains('modal')) UI_MODALS.closeTop();
});
window.addEventListener('load', () => { if (location.hash.length > 1) UI_MODALS.open(location.hash.slice(1)); });

// Physical buttons (keyboard in the mocks). ✕ activates the selected item of the top layer, or
// the guide action of that layer's footer (data-btn) when it has one.
const KEYS = { x: 'cross', t: 'triangle', s: 'square', v: 'select', enter: 'start' };
function topLayer() { return UI_MODALS.stack[UI_MODALS.stack.length - 1] || document.querySelector('.stage'); }
document.addEventListener('keydown', e => {
  const k = e.key.toLowerCase();
  if ((k === 'escape' || k === 'b') && UI_MODALS.stack.length) { e.stopImmediatePropagation(); UI_MODALS.closeTop(); return; }
  const layer = topLayer();
  if (k === 'arrowup' || k === 'arrowdown') {
    const sel = [...layer.querySelectorAll('.on')].find(el => el.matches(SELECTABLE) && !el.closest('.header') && el.offsetParent);
    if (!sel) return;
    const sibs = [...sel.parentElement.children].filter(c => c.matches(SELECTABLE));
    const n = sibs[sibs.indexOf(sel) + (k === 'arrowup' ? -1 : 1)];
    if (n) { selectItem(n); n.scrollIntoView({ block: 'nearest' }); }
    e.preventDefault();
    return;
  }
  const btn = KEYS[k];
  if (!btn) return;
  const guide = [...layer.querySelectorAll('[data-btn="' + btn + '"]')].find(el => !UI_MODALS.stack.length || el.closest('.modal') === layer);
  if (guide && activate(guide)) return;
  if (btn === 'cross') {
    const sel = [...layer.querySelectorAll('.on')].find(el => el.matches(SELECTABLE) && !el.closest('.header') && el.offsetParent);
    activate(sel);
  }
}, true);

// Drag to scroll: every .list whose rows do not fit
function enableDragScroll(list) {
  if (list.dataset.drag) return;
  list.dataset.drag = '1';
  list.classList.add('scrolls');
  const bar = document.createElement('i');
  bar.className = 'scrollbar';
  list.appendChild(bar);
  const paintBar = () => {
    const h = list.clientHeight, sh = list.scrollHeight;
    bar.style.height = Math.max(24, h * h / sh) + 'px';
    bar.style.top = (list.scrollTop + (h - parseFloat(bar.style.height)) * list.scrollTop / (sh - h)) + 'px';
  };
  let y0 = null, s0 = 0, moved = false;
  list.addEventListener('pointerdown', e => { y0 = e.clientY; s0 = list.scrollTop; moved = false; });
  window.addEventListener('pointermove', e => {
    if (y0 === null) return;
    const dy = e.clientY - y0;
    if (Math.abs(dy) > 6) moved = true;
    if (moved) { list.scrollTop = s0 - dy; paintBar(); }
  });
  window.addEventListener('pointerup', () => { if (y0 !== null && moved) suppressClick = true; y0 = null; setTimeout(() => { suppressClick = false; }, 0); });
  list.addEventListener('scroll', paintBar);
  paintBar();
}
function scanLists() {
  document.querySelectorAll('.list').forEach(l => {
    if (l.scrollHeight > l.clientHeight + 2) enableDragScroll(l);
    else if (l.dataset.drag) { const b = l.querySelector('.scrollbar'); if (b) b.style.display = 'none'; }
    if (l.dataset.drag) { const b = l.querySelector('.scrollbar'); if (b && l.scrollHeight > l.clientHeight + 2) { b.style.display = ''; l.dispatchEvent(new Event('scroll')); } }
  });
}
window.rescanLists = () => requestAnimationFrame(scanLists);
window.addEventListener('load', () => { scanLists(); document.fonts.ready.then(scanLists); });

// Quantity stepper: [−] value [+] tappable (press and hold repeats); d-pad ▲▼ ±1, ◀▶ ±10.
function stepper(el) {
  if (el.stepAdd) return;
  const cfg = el.cfg = { min: +el.dataset.min || 1, max: +el.dataset.max || 99, unit: +el.dataset.unit || 0, v: +el.dataset.value || 1 };
  const out = el.querySelector('.n');
  const total = el.dataset.total ? document.querySelector(el.dataset.total) : null;
  const paint = () => {
    out.textContent = 'x' + String(cfg.v).padStart(2, '0');
    if (total) total.textContent = '₽ ' + (cfg.v * cfg.unit).toLocaleString('en-US');
    el.dispatchEvent(new CustomEvent('ui-step', { detail: cfg.v, bubbles: true }));
  };
  const add = d => { cfg.v = Math.min(cfg.max, Math.max(cfg.min, cfg.v + d)); paint(); };
  el.stepConfig = o => { Object.assign(cfg, o); cfg.v = Math.min(cfg.max, Math.max(cfg.min, cfg.v)); paint(); };
  el.stepValue = () => cfg.v;
  for (const b of el.querySelectorAll('.step')) {
    let timer = null;
    const d = +b.dataset.step;
    b.addEventListener('pointerdown', e => { e.preventDefault(); add(d); timer = setInterval(() => add(d), 120); });
    ['pointerup', 'pointerleave'].forEach(t => b.addEventListener(t, () => clearInterval(timer)));
  }
  el.stepAdd = add;
  paint();
}
window.initSteppers = () => document.querySelectorAll('.stepper[data-max]').forEach(stepper);
document.addEventListener('DOMContentLoaded', window.initSteppers);
// Steppers: inside a popup ▲▼ ±1 and ◀▶ ±10; on a screen with a list (shop), ◀▶ ±1 and ▲▼ move the list.
document.addEventListener('keydown', e => {
  const layer = topLayer();
  const st = [...layer.querySelectorAll('.stepper[data-max]')].find(s => s.offsetParent);
  if (!st || !st.stepAdd) return;
  const inModal = layer.classList.contains('modal');
  const map = inModal ? { arrowup: 1, arrowdown: -1, arrowright: 10, arrowleft: -10 } : { arrowright: 1, arrowleft: -1 };
  const d = map[e.key.toLowerCase()];
  if (d) { st.stepAdd(d); e.preventDefault(); e.stopImmediatePropagation(); }
}, true);

(function () {
  const header = document.querySelector('.header[data-nav]');
  if (!header) return;
  const [secId, tabId] = header.dataset.nav.split(':');
  const sec = UI_NAV.sections.find(s => s.id === secId);
  const qTab = new URLSearchParams(location.search).get('tab');
  let cur = Math.max(0, sec.tabs.findIndex(t => t.id === (qTab || tabId)));
  if (qTab) header.dataset.nav = secId + ':' + sec.tabs[cur].id;

  // Section title (fixed label)
  const title = document.createElement('div');
  title.className = 'nav-title ps';
  title.innerHTML = '<img class="ball" src="../_assets/balls/poke.png" alt="">';
  title.appendChild(document.createTextNode(sec.label));

  // Tab selector: L [tabs] R
  const tabs = document.createElement('div');
  tabs.className = 'tabs';
  const keyL = Object.assign(document.createElement('span'), { className: 'key nav-lr', textContent: 'L' });
  const keyR = Object.assign(document.createElement('span'), { className: 'key nav-lr', textContent: 'R' });
  if (sec.lr !== 'screen') tabs.appendChild(keyL);
  const els = sec.tabs.map((t, i) => {
    const el = Object.assign(document.createElement('span'), { className: 'tab', textContent: t.label });
    el.onclick = () => select(i);
    tabs.appendChild(el);
    return el;
  });
  if (sec.lr !== 'screen') tabs.appendChild(keyR);
  function paint() { els.forEach((el, i) => el.classList.toggle('on', i === cur)); }
  function select(i) {
    const n = sec.tabs.length;
    i = ((i % n) + n) % n;
    const t = sec.tabs[i];
    if (t.page && !location.pathname.includes('/' + t.page + '/')) { location.href = '../' + t.page + '/index.html?tab=' + t.id; return; }
    cur = i; paint();
    document.dispatchEvent(new CustomEvent('ui-tab', { detail: t.id }));
  }
  keyL.onclick = () => select(cur - 1);
  keyR.onclick = () => select(cur + 1);
  document.addEventListener('keydown', e => {
    const k = e.key.toLowerCase();
    if (UI_MODALS.stack.length) return;   // modal open: L/R and B belong to it
    // lr: 'screen' → the shoulders belong to the screen (PC boxes), the rail shows no L / R keys
    const lr = k === 'q' || k === 'l' ? -1 : k === 'e' || k === 'r' ? 1 : 0;
    if (lr && sec.lr === 'screen') document.dispatchEvent(new CustomEvent('ui-lr', { detail: lr }));
    else if (lr) select(cur + lr);
    if (k === 'escape' || k === 'b') {
      // back: 'screen' → ○ is the screen's own key (delete a letter, ask before quitting...)
      if (sec.back === 'screen') { document.dispatchEvent(new CustomEvent('ui-back')); return; }
      if (sec.back === 'battle') { if (window.UI) UI.toast('Back to the battle (current battle UI).'); return; }
      if (sec.back === 'field') { if (window.UI) UI.toast('Back to the field.'); return; }
      location.href = '../' + (sec.parent ? UI_NAV.sections.find(s => s.id === sec.parent).home : 'hub') + '/index.html';
    }
  });

  header.prepend(title, tabs);
  paint();
})();

// Menu themes on the mocks (?theme=red|crystal|tropical|royal): _shared/theme.js
(() => {
  if (!new URLSearchParams(location.search).get('theme')) return;
  const s = document.createElement('script');
  s.src = '../_shared/theme.js';
  document.head.appendChild(s);
})();
