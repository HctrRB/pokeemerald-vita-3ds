/* Screen controllers: the menu interactions of the original Stitch prototypes, rebuilt on the
   shared data (data.js) and the shared navigation/touch model (nav.js).
   Each screen is keyed by <body data-screen="...">. Selection always drives the detail panels
   (event "ui-select"), tabs change the content (event "ui-tab"), popups show the data of the
   selected element and actions change the state (give item, feed, buy, sell, toss, swap...). */
(function () {
  const G = window.GAME, $ = s => document.querySelector(s), $$ = s => [...document.querySelectorAll(s)];
  const A = window.UI_ACTIONS;
  const store = { get: (k, d) => { try { const v = sessionStorage.getItem(k); return v == null ? d : JSON.parse(v); } catch (e) { return d; } },
                  set: (k, v) => { try { sessionStorage.setItem(k, JSON.stringify(v)); } catch (e) { /* preview without storage */ } } };
  const party = () => store.get('party.v3', null) || G.party;
  const saveParty = p => store.set('party.v3', p);
  const money = () => store.get('money', G.player.money);
  const setMoney = v => { store.set('money', v); $$('.js-money').forEach(e => { e.textContent = UI.money(v); }); };
  const STATUS_ROW = { PSN: 0, PAR: 1, SLP: 2, FRZ: 3, BRN: 4, PKRS: 5, FNT: 6 };
  const statusIcon = s => s ? `<span class="status" style="background-position:0 -${STATUS_ROW[s] * 8}px"></span>` : '';
  const curTab = () => { const h = $('.header[data-nav]'); return h ? h.dataset.nav.split(':')[1] : null; };
  const onTab = fn => document.addEventListener('ui-tab', e => fn(e.detail));
  const key = (fn) => document.addEventListener('keydown', e => { if (!UI_MODALS.stack.length) fn(e.key.toLowerCase(), e); });

  // The bag survives page changes during a review session (like the party and the money)
  const savedBag = store.get('bag.v3', null); if (savedBag) G.bag = savedBag;
  G.registered = store.get('registered', G.registered);
  window.addEventListener('pagehide', () => { store.set('bag.v3', G.bag); store.set('registered', G.registered); });

  const S = {};

  /* ---------------- Hub START ---------------- */
  S.hub = () => {
    const pl = G.player, p = party(), menu = $('.menu'), hint = $('.hint');
    // Status bar of the current START menu (vita_ui.c header): clock + day/night + speed, money,
    // registered item on SELECT, repel steps or Safari balls, run toggle
    const reg = Object.values(G.bag).flat().find(it => it[0] === G.registered);
    const status = $('.status');
    const paintStatus = () => {
      const counter = pl.safari ? `SAFARI BALLS ${pl.safariBalls}` : pl.repelSteps ? `REPEL ${pl.repelSteps}` : '';
      status.innerHTML = `<span class="capsule">${pl.clock}</span>
        <span class="capsule"><span class="money">₽ ${pl.money.toLocaleString('en-US')}</span></span>
        <span class="capsule">${reg ? `<img src="../_assets/items/${reg[0]}.png" alt="" style="width:24px">` : ''}SEL: ${reg ? reg[1] : 'NO ITEM'}</span>
        ${counter ? `<span class="capsule">${counter}</span>` : ''}
        <span class="capsule run${pl.autoRun ? ' on' : ''}" data-action="toggleRun">${pl.runningShoes ? (pl.autoRun ? 'AUTO RUN · ON' : 'AUTO RUN · OFF') : 'NO SHOES'}</span>`;
    };
    A.toggleRun = () => { if (!pl.runningShoes) { UI.toast('RUNNING SHOES NOT OBTAINED'); return; } pl.autoRun = !pl.autoRun; paintStatus(); UI.toast(pl.autoRun ? 'Auto run ON' : 'Auto run OFF', 900); };
    paintStatus();
    // Menu entries from the data (NUZLOCKE only with the extra on; RETIRE only in the Safari Zone)
    const nuz = G.options.modes.find(o => o[1] === 'NUZLOCKE MODE')[4] === 1;
    const entries = G.startMenu.filter(e => e[0] !== 'NUZLOCKE' || nuz);
    if (pl.safari) entries.unshift(['RETIRE', '#retirar', 'Leave the SAFARI ZONE.']);
    const ICON = { 'POKéDEX': '<img src="../_assets/balls/poke.png" alt="" style="zoom:2">', 'POKéMON': UI.icon(p[0].sp),
      BAG: '<img src="../_assets/start/bag.png" alt="">', 'POKéNAV': '<img src="../_assets/start/pokenav.png" alt="">',
      SAVE: '<img src="../_assets/items/letter.png" alt="">', OPTION: '<img src="../_assets/items/card_key.png" alt="">',
      NUZLOCKE: '<img src="../_assets/balls/premier.png" alt="" style="zoom:2">' };
    ICON[pl.name] = '<img src="../_assets/map/brendan_head.png" alt="" style="zoom:2">';
    UI.rows(menu, entries, e => `<div class="row"><span class="ico">${ICON[e[0]] || ''}</span><span class="grow">${e[0]}</span></div>`,
      e => (e[1].startsWith('#') ? { 'data-open': e[1].slice(1) } : e[1] ? { 'data-go': e[1] } : { 'data-action': 'closeMenu' }));
    const start = Math.max(0, entries.findIndex(e => e[0] === 'POKéMON'));
    menu.querySelector('.on').classList.remove('on'); menu.children[start].classList.add('on');
    UI.onSelect(menu, i => { hint.textContent = entries[i][2]; });
    hint.textContent = entries[start][2];
    A.closeMenu = () => UI.toast('The menu closes and the game goes on.');
    // Party strip (eggs without level or HP)
    UI.rows($('.party.list'), p, m => m.egg ? `<div class="row">${UI.icon('egg')}<span class="grow">EGG</span></div>`
      : `<div class="row">${UI.icon(m.sp)}<span class="grow">${m.name}</span><span class="lv">Lv${m.lv}</span><div class="bar ${UI.hpClass(m.hp, m.maxhp)}"><i style="width:${Math.round(100 * m.hp / m.maxhp)}%"></i></div></div>`);
    // Save (vita_ui.c SAVE section): slot, summary, overwrite warning, result
    $('#guardar .win-body > div:first-child').innerHTML = `SLOT ${pl.saveSlot} · ${pl.name}<br>BADGES ${pl.badges} · POKéDEX ${pl.dexOwned} · TIME ${pl.time}` +
      (pl.saveOtherGame ? '<br><span style="color:var(--danger)">WARNING: OVERWRITING DIFFERENT SAVE!</span>' : '');
    $('#guardado .toast').innerHTML = 'GAME SAVED SAFELY! · Saved to ux0:data/POKEVITA/';
    // Field moves popup from the party
    const list = $('#mo-campo .win-body');
    const fm = p.flatMap(m => m.field.map(f => [f, m, G.player.badges >= G.hmBadge[f]]));
    UI.rows(list, fm, ([f, m, ok]) => `<div class="row">${UI.icon(m.sp)}<span class="grow${ok ? '' : ' soft'}">${f} · ${m.name}</span><span class="chip${ok ? ' lead' : ''}">${ok ? 'USABLE' : 'BADGE ' + G.hmBadge[f]}</span></div>`, () => ({ 'data-action': 'useField' }));
    A.useField = el => { const [f, m, ok] = fm[+el.dataset.idx];
      if (!ok) { UI.toast('You need the right BADGE to use it outside of battle.'); return; }
      if (f === 'FLY') { location.href = '../vuelo/index.html'; return; }
      UI_MODALS.closeTop(); UI.toast(`${m.name} used ${f}! (the menu closes, HM animation plays)`, 2200); };
    $('#mo-campo .win-foot [data-btn="cross"]').removeAttribute('data-open');
  };

  /* ---------------- Party ---------------- */
  // Gender and shiny as the console draws them: 14 px pixel art at x2 (tools/ui2/build_atlas.py GLYPH_ART)
  const genderShiny = m => (m.g === '♂' ? '<img class="gly" src="../_assets/ui_gba/gender_male.png" alt="♂">' : m.g === '♀' ? '<img class="gly" src="../_assets/ui_gba/gender_female.png" alt="♀">' : '')
    + (m.shiny ? '<img class="gly" src="../_assets/ui_gba/shiny_star.png" alt="★">' : '');
  function partyCard(m) {
    if (m.egg)
      return `<div class="card">${UI.icon('egg')}<div class="info"><div class="l1"><span class="name">EGG</span></div><div class="l2"><span class="soft">It will hatch soon.</span></div></div></div>`;
    return `<div class="card">${UI.icon(m.sp)}<div class="info"><div class="l1"><span class="name">${m.name} ${genderShiny(m)}</span>${m.item ? '<img class="ico16" src="../_assets/items/' + itemId(m.item) + '.png" alt="" style="width:24px;height:24px">' : ''}${statusIcon(m.status)}<span>Lv${m.lv}</span></div>` +
           `<div class="l2"><div class="bar ${UI.hpClass(m.hp, m.maxhp)}"><i style="width:${Math.round(100 * m.hp / m.maxhp)}%"></i></div><span class="hp">${m.hp}/${m.maxhp}</span></div></div></div>`;
  }
  const allItems = () => Object.values(G.bag).flat();
  const itemId = name => { const it = allItems().find(i => i[1] === name); return it ? it[0] : 'potion'; };

  S.equipo = (opts = {}) => {
    const battle = !!opts.battle;
    let p = party(), swapFrom = null;
    const box = $('.party'), sum = $('.summary');
    const render = (sel = UI.selected(box)) => {
      UI.rows(box, p, partyCard, () => ({ 'data-open': 'acciones' }));
      box.querySelector('.on').classList.remove('on'); box.children[sel].classList.add('on');
      if (battle) box.children[0].querySelector('.name').insertAdjacentHTML('afterend', '<span class="chip lead">IN BATTLE</span>');
      summary(sel);
    };
    // Summary pages INFO / SKILLS (MOVES / CONTEST live in equipo_movimientos), as vita_ui_party.c
    let page = battle ? 'skills' : (curTab() || 'skills');
    const ivsEvs = () => { const o = G.options.battle.find(x => x[1] === 'IVS & EVS STATS'); return !o || o[4] === 1; };
    const summary = i => {
      const m = p[i]; store.set('mon', i);
      if (m.egg) {
        sum.innerHTML = `<div class="pic"><img class="x2" src="../_assets/pokemon/egg_front.png" alt=""></div>
          <div class="head"><div class="ps">EGG</div><div class="facts" style="margin-top:12px">A MYSTERIOUS EGG.</div></div>
          <div class="stats"><div style="line-height:30px">${m.eggNote}</div></div>`;
      } else {
        const head = `<div class="pic"><img class="x2" src="../_assets/pokemon/${m.sp}_front.png" alt=""${m.shiny ? ' style="filter:hue-rotate(180deg) saturate(1.4)"' : ''}></div>
          <div class="head"><div class="ps">${m.name} ${genderShiny(m)}</div><div class="types">${m.types.map(t => `<img src="../_assets/types/${t}.png" alt="">`).join(' ')}</div>
          <div class="facts">Lv${m.lv} · Item <b>${m.item || 'NONE'}</b>${m.status ? ' · <b>' + m.status + '</b>' : ''}</div></div>`;
        const kv = (k, v) => `<div class="kv2"><span class="soft">${k}</span><span>${v}</span></div>`;
        const body = page === 'info'
          ? `<div class="stats">${kv('SPECIES', 'No.' + m.no)}${kv('OT / ID', m.ot + ' · ' + m.otId)}${kv('NATURE', m.nature)}${kv('MET AT', m.metAt + ' (Lv' + m.metLv + ')')}
              ${kv('ABILITY', m.ability)}<div class="soft" style="line-height:28px; padding:2px 0 6px">${m.abilityDesc}</div>
              <div style="display:flex; align-items:center; gap:8px"><span>EXP ${m.exp} · NEXT ${m.nextLv}</span><div class="bar exp" style="flex:1"><i style="width:${m.expPct}%"></i></div></div></div>`
          : `<div class="stats"><table><tr class="h"><td>STAT</td><td>VALUE</td>${ivsEvs() ? '<td>EV</td><td>IV</td>' : ''}</tr>${Object.entries(m.stats).map(([k, v]) =>
              `<tr><td>${k}${k === m.up ? ' ▲' : k === m.down ? ' ▼' : ''}</td><td>${k === 'HP' ? m.hp + '/' + m.maxhp : v[0]}</td>${ivsEvs() ? `<td>${v[1]}</td><td>${v[2]}</td>` : ''}</tr>`).join('')}</table></div>`;
        sum.innerHTML = head + body;
      }
      // popups follow the selection
      const pop = $('#acciones');
      pop.querySelector('.win-head').innerHTML = `<span class="ps">${m.name}</span><span class="spacer"></span><span class="vt-sm">Lv${m.lv}</span>`;
      pop.querySelector('.win').style.top = Math.min(62 + i * 70, 260) + 'px';
      const useItem = battle ? store.get('battleItem', null) : null;
      // Same actions as vita_ui_party.c: eggs only SUMMARY / SWITCH; SHARE HP with SOFTBOILED or MILK DRINK
      const shareHp = m.moves.some(mv => mv[0] === 'SOFTBOILED' || mv[0] === 'MILK DRINK') ? [['SHARE HP', 'shareHp']] : [];
      UI.rows(pop.querySelector('.list'), battle ? (useItem ? [['USE ' + findItem(useItem)[0][1], 'battleUse'], ['CANCEL', 'close']] : [['SHIFT', 'shift'], ['SUMMARY', 'close'], ['CANCEL', 'close']])
                                                 : m.egg ? [['SUMMARY', 'close'], ['SWITCH', 'startSwap'], ['CANCEL', 'close']]
                                                 : [['SUMMARY', 'close'], ['SWITCH', 'startSwap'], ['ITEM', 'item'], ...shareHp, ...m.field.map(f => [f, 'field']), ['CANCEL', 'close']],
        ([t, a]) => `<div class="row"${a === 'field' ? ' style="color:#1B6E9A"' : ''}>${t}</div>`, ([t, a]) => ({ 'data-action': 'party_' + a }));
      $('#objeto .win-head').innerHTML = `<span class="ps">HELD ITEM</span><span class="spacer"></span><span class="vt-sm">${m.name} Lv${m.lv}</span>`;
      const cur = $('#objeto .win-body > div:first-child');
      cur.innerHTML = m.item ? `<img class="x2" src="../_assets/items/${itemId(m.item)}.png" alt=""><div><b>${m.item}</b><div class="vt-sm soft">Holding now.</div></div>` : '<div><b>No item held</b><div class="vt-sm soft">Choose an item to give.</div></div>';
      // GIVE opens the whole BAG to choose the item; TAKE returns the held one to the BAG
      UI.rows($('#objeto .list'), [['GIVE', 'Choose an item from the BAG'], ...(m.item ? [['TAKE', `Put the ${m.item} in the BAG`]] : []), ['CANCEL', '']],
        ([k, d]) => `<div class="row"><span class="grow"><b>${k}</b></span><span class="vt-sm soft">${d}</span></div>`,
        ([k]) => ({ 'data-action': k === 'GIVE' ? 'giveItem' : k === 'TAKE' ? 'takeItem' : 'party_close' }));
    };
    A.party_close = () => UI_MODALS.closeTop();
    A.party_shift = () => { const i = UI.selected(box), m = p[i]; UI_MODALS.closeTop();
      if (i === 0) UI.toast(`${m.name} is already in battle!`);
      else if (m.egg) UI.toast("An EGG can't battle!");
      else if (m.hp === 0) UI.toast(`${m.name} has no energy left to battle!`);
      else UI.toast(`Go! ${m.name}! (back to the battle)`, 1800); };
    A.party_battleUse = () => { const i = UI.selected(box), m = p[i], id = store.get('battleItem', null), eff = EFFECTS[id], [it] = findItem(id); UI_MODALS.closeTop();
      if (eff[0] === 'hp' && m.hp > 0 && m.hp < m.maxhp) { m.hp = Math.min(m.maxhp, m.hp + eff[1]); it[2]--; saveParty(p); store.set('battleItem', null); render(i); UI.toast(`${m.name}'s HP was restored. (the turn is used, back to the battle)`, 2200); return; }
      if (eff[0] === 'cure' && m.status && (eff[1] === '*' || eff[1] === m.status)) { m.status = ''; it[2]--; saveParty(p); store.set('battleItem', null); render(i); UI.toast(`${m.name} was cured. (back to the battle)`, 2200); return; }
      UI.toast("It won't have any effect."); };
    A.party_item = () => UI_MODALS.open('objeto');
    A.party_shareHp = () => { UI_MODALS.closeTop(); UI.toast('Choose who gets the HP (SHARE HP: costs 1/5 of the user\'s max HP).', 2200); };
    onTab(t => { if (t === 'info' || t === 'skills') { page = t; summary(UI.selected(box)); } });
    A.party_field = el => { const m = p[UI.selected(box)], f = el.textContent;
      if (G.player.badges < G.hmBadge[f]) { UI.toast('You need the right BADGE to use it outside of battle.'); return; }
      if (f === 'FLY') { location.href = '../vuelo/index.html'; return; }
      UI_MODALS.closeTop(); UI.toast(`${m.name} used ${f}! (the menu closes, HM animation plays)`, 2200); };
    A.party_startSwap = () => { UI_MODALS.closeTop(); swapFrom = UI.selected(box); box.children[swapFrom].style.outline = '4px dashed #F6CF57'; UI.toast(`Move ${p[swapFrom].name} to where?  ✕ / tap: swap · ○: cancel`, 60000); };
    const doSwap = to => {
      if (swapFrom === null) return false;
      if (to !== swapFrom) { [p[swapFrom], p[to]] = [p[to], p[swapFrom]]; saveParty(p); }
      swapFrom = null; UI.toast('Swapped.', 900); render(to); return true;
    };
    box.addEventListener('click', e => { const c = e.target.closest('.card'); if (c && swapFrom !== null) { e.stopPropagation(); doSwap(+c.dataset.idx); } }, true);
    document.addEventListener('keydown', e => {
      const k = e.key.toLowerCase();
      if (swapFrom !== null && k === 'x') { e.stopImmediatePropagation(); doSwap(UI.selected(box)); }
      if (swapFrom !== null && (k === 'escape' || k === 'b')) { e.stopImmediatePropagation(); swapFrom = null; render(); UI.toast('Cancelled.', 700); }
      if (!UI_MODALS.stack.length && swapFrom === null && k === 's') A.party_startSwap();
    }, true);
    // drag & drop reorder (original prototype): drag a card onto another
    let dragFrom = null;
    box.addEventListener('pointerdown', e => { const c = e.target.closest('.card'); dragFrom = c ? +c.dataset.idx : null; });
    box.addEventListener('pointerup', e => {
      const c = document.elementFromPoint(e.clientX, e.clientY)?.closest('.card');
      if (dragFrom !== null && c && +c.dataset.idx !== dragFrom) { swapFrom = dragFrom; doSwap(+c.dataset.idx); }
      dragFrom = null;
    });
    A.giveItem = () => { const i = UI.selected(box);
      if (p[i].egg) { UI.toast('An EGG cannot hold items.'); return; }
      store.set('giveTo', i); location.href = '../mochila/index.html?tab=items'; };
    A.takeItem = () => {
      const i = UI.selected(box), m = p[i];
      const old = allItems().find(x => x[1] === m.item); if (old) old[2]++;
      UI.toast(`Received the ${m.item} from ${m.name}.`); m.item = ''; saveParty(p);
      UI_MODALS.closeTop(); UI_MODALS.closeTop(); render(i);
    };
    // field moves of the whole party
    const fm = () => p.flatMap(m => m.field.map(f => [f, m, G.player.badges >= G.hmBadge[f]]));
    const renderFm = () => UI.rows($('#mo .win-body'), fm(), ([f, m, ok]) => `<div class="row"><span class="grow${ok ? '' : ' soft'}">${f} · ${m.name}</span><span class="chip${ok ? ' lead' : ''}">${ok ? 'USABLE' : 'BADGE ' + G.hmBadge[f] + ' NEEDED'}</span></div>`,
      () => ({ 'data-action': 'useFm' }));
    A.useFm = el => { const [f, m, ok] = fm()[+el.dataset.idx];
      if (!ok) { UI.toast('You need the right BADGE to use it outside of battle.'); return; }
      if (f === 'FLY') { location.href = '../vuelo/index.html'; return; }
      UI_MODALS.closeTop(); UI.toast(`${m.name} used ${f}! (the menu closes, HM animation plays)`, 2200); };
    const sq = document.querySelector('.footer [data-btn="square"]'); if (sq) sq.dataset.action = 'party_startSwap';
    render(Math.min(store.get('mon', 0), p.length - 1));
    renderFm();
    if (battle) { const bi = store.get('battleItem', null); $('.header .capsule').textContent = bi ? 'USE ' + findItem(bi)[0][1] + ' ON:' : 'IN BATTLE: ' + p[0].name;
    }
    UI.onSelect(box, i => summary(i));
  };

  /* ---------------- Party · moves ---------------- */
  S.equipo_movimientos = () => {
    const p = party().filter(m => !m.egg); let mi = Math.min(store.get('mon', 0), p.length - 1), swapFrom = null;
    let page = curTab() || 'moves';   // MOVES (battle) or CONTEST (appeal / jam), as SUM_TAB_BATTLE_MOVES / CONTEST_MOVES
    // 8 fixed heart cells (sprites in the game: the fonts have no heart), filled up to n
    const hearts = (n, cls) => `<span class="hearts ${cls}">${Array.from({ length: 8 }, (_, k) => `<i${k < n ? ' class="f"' : ''}></i>`).join('')}</span>`;
    const list = $('.moves'), det = $('.detail'), cap = $('.header .capsule');
    cap.innerHTML = '<span class="step-mon" data-d="-1">◀</span> <span class="who-name"></span> <span class="step-mon" data-d="1">▶</span>';
    cap.style.cursor = 'pointer';
    const renderMon = () => {
      const m = p[mi]; store.set('mon', mi);
      cap.querySelector('.who-name').textContent = `${m.name} Lv${m.lv}`;
      // Column legend of the list: TYPE + PP (battle) or CATEGORY + APPEAL (contest)
      $('.moves .panel-head').innerHTML = page === 'contest'
        ? '<span class="ps">CONTEST MOVES</span><span class="spacer"></span><span class="vt-sm soft">CATEGORY · APPEAL</span>'
        : '<span class="ps">BATTLE MOVES</span><span class="spacer"></span><span class="vt-sm soft">TYPE · PP LEFT</span>';
      list.querySelectorAll('.mv').forEach(r => r.remove());
      m.moves.forEach((mv, i) => {
        const d = document.createElement('div');
        d.className = 'mv' + (i === 0 ? ' on' : ''); d.dataset.idx = i; d.dataset.open = 'movimiento';
        d.innerHTML = page === 'contest'
          ? `<span>${mv[0]}</span><img src="../_assets/types/${mv[8]}.png" alt=""><span class="pp">${hearts(mv[9], '')}</span>`
          : `<span>${mv[0]}</span><img src="../_assets/types/${mv[1]}.png" alt=""><span class="pp">${mv[2]}/${mv[3]}</span>`;
        list.appendChild(d);
      });
      move(0);
    };
    const move = i => {
      const m = p[mi], mv = m.moves[i];
      det.innerHTML = page === 'contest'
        ? `<div class="who"><img src="../_assets/pokemon/${m.sp}_front.png" alt="" style="width:64px"><div><div class="ps">${mv[0]}</div><img src="../_assets/types/${mv[8]}.png" alt="" style="zoom:2; margin-top:6px"></div></div>
           <div class="kv"><span>APPEAL</span>${hearts(mv[9], '')}</div><div class="kv"><span>JAM</span>${hearts(mv[10], 'jam')}</div><div class="desc">${mv[11]}</div>`
        : `<div class="who"><img src="../_assets/pokemon/${m.sp}_front.png" alt="" style="width:64px"><div><div class="ps">${mv[0]}</div><img src="../_assets/types/${mv[1]}.png" alt="" style="zoom:2; margin-top:6px"></div></div>
           <div class="kv"><span>POWER</span><span>${mv[4]}</span></div><div class="kv"><span>ACCURACY</span><span>${mv[5]}</span></div>
           <div class="kv"><span>CATEGORY</span><span>${mv[6]}</span></div><div class="desc">${mv[7]}</div>`;
      const mod = $('#movimiento');
      mod.querySelector('.win-head').innerHTML = `<span class="ps">${mv[0]}</span><span class="spacer"></span><img src="../_assets/types/${mv[1]}.png" alt="" style="zoom:2">`;
      const kv = mod.querySelectorAll('.win-body > div');
      const vals = [mv[6] + ' (extra P/S split)', mv[4], mv[5], mv[2] + ' / ' + mv[3]];
      kv.forEach((d, k) => { if (k < 4) d.lastElementChild.textContent = vals[k]; else d.textContent = mv[7]; });
    };
    UI.onSelect(list, i => { if (swapFrom === null) move(i); });
    cap.addEventListener('click', e => { const s = e.target.closest('.step-mon'); if (s) { mi = (mi + +s.dataset.d + p.length) % p.length; renderMon(); } });
    key(k => { if (k === 'arrowleft' || k === 'arrowright') { mi = (mi + (k === 'arrowleft' ? -1 : 1) + p.length) % p.length; renderMon(); } });
    A.moveSwap = () => { swapFrom = UI.selected(list); list.children[swapFrom + 1].style.outline = '4px dashed #F6CF57'; UI.toast('Reorder: pick the new position (✕ / tap) · ○ cancel', 60000); };
    list.addEventListener('click', e => { const r = e.target.closest('.mv'); if (r && swapFrom !== null) { e.stopPropagation(); finish(+r.dataset.idx); } }, true);
    const finish = to => { const mv = p[mi].moves; [mv[swapFrom], mv[to]] = [mv[to], mv[swapFrom]]; swapFrom = null; saveParty(p); renderMon(); UI.toast('Moves reordered.', 900); };
    document.addEventListener('keydown', e => { const k = e.key.toLowerCase();
      if (swapFrom !== null && k === 'x') { e.stopImmediatePropagation(); finish(UI.selected(list)); }
      if (swapFrom !== null && (k === 'escape' || k === 'b')) { e.stopImmediatePropagation(); swapFrom = null; renderMon(); }
      if (swapFrom === null && !UI_MODALS.stack.length && k === 's') A.moveSwap(); }, true);
    $('#movimiento .win-foot [data-btn="square"]').dataset.action = 'moveSwapFromModal';
    A.moveSwapFromModal = () => { UI_MODALS.closeTop(); A.moveSwap(); };
    onTab(t => { if (t === 'moves' || t === 'contest') { page = t; renderMon(); } });
    renderMon();
  };

  /* ---------------- PokéNav · condition ---------------- */
  S.pokenav_condicion = () => {
    let p = party();
    const list = $('.party.list'), svg = $('.radar svg'), stats = $('.stats');
    let blocks = store.get('blocks', G.pokeblocks);
    const KEYS = ['cool', 'beauty', 'cute', 'smart', 'tough'], ANG = KEYS.map((_, i) => -Math.PI / 2 + i * 2 * Math.PI / 5), R = 120;
    const pt = (v, i) => [Math.round(Math.cos(ANG[i]) * R * v / 255), Math.round(Math.sin(ANG[i]) * R * v / 255)];
    const p0 = p; p = p0.filter(m => !m.egg);
    UI.rows(list, p, m => `<div class="row">${UI.icon(m.sp)}<span class="grow">${m.name}</span><span>Lv${m.lv}</span></div>`);
    const shape = svg.querySelector('#cond-shape'), dots = svg.querySelector('#cond-dots');
    const show = i => {
      const m = p[i]; store.set('mon', i);
      shape.setAttribute('points', KEYS.map((k, j) => pt(Math.max(m.cond[k], 8), j).join(',')).join(' '));
      dots.innerHTML = KEYS.map((k, j) => { const [x, y] = pt(Math.max(m.cond[k], 8), j); return `<rect x="${x - 4}" y="${y - 4}" width="8" height="8"/>`; }).join('');
      const bars = stats.querySelectorAll('.st .bar > i');
      KEYS.forEach((k, j) => { bars[j].style.width = Math.round(100 * m.cond[k] / 255) + '%'; bars[j].parentElement.title = m.cond[k]; });
      stats.querySelector('.sheen .kv').textContent = `SHEEN ${m.cond.sheen} / 255 · ${m.nature}`;
      stats.querySelector('.sheen .bar > i').style.width = Math.round(100 * m.cond.sheen / 255) + '%';
      stats.querySelector('.blocks').innerHTML = `<img src="../_assets/ui_gba/pokeblock.png" alt=""> CASE: ${blocks.length} · ${m.taste}`;
      $('#pokecubos .win-head').innerHTML = `<span class="ps">POKéBLOCK CASE</span><span class="spacer"></span><span class="vt-sm">Feed ${m.name} · SHEEN ${m.cond.sheen}/255</span>`;
    };
    const HUE = { RED: 0, BLUE: 200, PINK: 300, GREEN: 100, YELLOW: 50 };
    const renderBlocks = () => UI.rows($('#pokecubos .win-body'), blocks.length ? blocks : [['', 0, [0, 0, 0, 0, 0], 0, 0]], b => b[0]
      ? `<div class="row"><img src="../_assets/ui_gba/pokeblock.png" alt="" style="zoom:3; filter:hue-rotate(${HUE[b[0]]}deg)"><span class="grow">${b[0]} POKéBLOCK Lv${b[1]}</span><span class="vt-sm">${KEYS.map((k, j) => b[2][j] ? k.toUpperCase() + ' +' + b[2][j] : '').join(' ')} · FEEL ${b[3]}</span></div>`
      : '<div class="row"><span class="grow soft">The case is empty.</span></div>', b => (b[0] ? { 'data-action': 'feed' } : {}));
    A.feed = el => {
      const i = UI.selected(list), m = p[i], b = blocks[+el.dataset.idx];
      if (m.cond.sheen >= 255) { UI.toast(`${m.name} won't eat any more...`); return; }
      KEYS.forEach((k, j) => { m.cond[k] = Math.min(255, m.cond[k] + b[2][j] * 10); });
      m.cond.sheen = Math.min(255, m.cond.sheen + b[3]);
      blocks.splice(+el.dataset.idx, 1); store.set('blocks', blocks); saveParty(p);
      UI_MODALS.closeTop(); show(i); renderBlocks();
      UI.toast(`${m.name} ate the ${b[0]} POKéBLOCK!`);
    };
    UI.onSelect(list, show);
    const start = Math.min(store.get('mon', 0), p.length - 1);
    list.querySelector('.on').classList.remove('on'); list.children[start].classList.add('on');
    show(start); renderBlocks();
  };

  /* ---------------- PokéNav · ribbons ---------------- */
  S.pokenav = () => {
    let p = party();
    const list = $('.party.list'), grid = $('.ribbons .grid'), info = $('.ribbons > div:last-child');
    p = p.filter(m => !m.egg);
    UI.rows(list, p, m => `<div class="row">${UI.icon(m.sp)}<span class="grow">${m.name}</span><span>${m.ribbons.length}</span></div>`);
    const show = i => {
      const m = p[i]; grid.innerHTML = '';
      for (let k = 0; k < 16; k++) {
        const r = m.ribbons[k], d = document.createElement('div');
        d.className = 'rb' + (r == null ? ' empty' : '') + (k === 0 && r != null ? ' on' : ''); d.dataset.idx = k;
        d.innerHTML = r == null ? '' : `<img src="../_assets/ribbons/ribbon_${r}.png" alt="" style="width:64px;height:64px;image-rendering:pixelated">`;
        grid.appendChild(d);
      }
      $('.ribbons .panel-head .ps').textContent = `${m.name} · RIBBONS`;
      rib(i, 0);
    };
    const rib = (i, k) => { const r = p[i].ribbons[k];
      info.innerHTML = r == null ? `<div class="vt-sm soft">${p[i].name} has no ribbons yet. Win contests to earn them.</div>` : `<div class="ps">${G.ribbons[r][0]}</div><div class="vt-sm" style="margin-top:8px">${G.ribbons[r][1]}</div>`; };
    UI.onSelect(list, show);
    document.addEventListener('ui-select', e => { if (grid.contains(e.detail) && !e.detail.classList.contains('empty')) rib(UI.selected(list), +e.detail.dataset.idx); });
    show(0);
  };

  /* ---------------- PokéNav · Match Call ---------------- */
  S.pokenav_match_call = () => {
    const list = $('.main .list'), det = $('.detail'); let line = 0;
    UI.rows(list, G.contacts, c => `<div class="row"><span class="grow">${c[0]}</span><span class="vt-sm">${c[1]}</span>${c[3] === 'REMATCH' ? '<span class="chip rem" title="Rematch ready">!</span>' : ''}</div>`, () => ({ 'data-open': 'llamada' }));
    $('.main .panel-head .vt-sm').textContent = G.contacts.length + ' trainers';
    const show = i => { const c = G.contacts[i];
      det.innerHTML = `<div class="pic">${c[2] ? `<img class="x2" src="../_assets/trainers/${c[2]}.png" alt="">` : ''}</div>
        <div><div class="ps">${c[0].split(' (')[0]}</div><div class="kv" style="margin-top:12px"><span>LOCATION</span><span>${c[1]}</span></div><div class="kv"><span>STATUS</span><span class="${c[3] === 'REMATCH' ? 'rem' : ''}">${c[3]}</span></div></div>
        <div class="wide"><div class="ps" style="font-size:16px">REMATCH</div><div class="${c[3] === 'REMATCH' ? 'rem' : 'soft'}">${c[3] === 'REMATCH' ? 'Ready! Meet this trainer in the area to battle again.' : 'Not ready yet.'}</div></div>`;
      const m = $('#llamada'); line = 0;
      m.querySelector('.win-head').innerHTML = `<span class="ps">MATCH CALL</span><span class="spacer"></span><span class="vt-sm">${c[0].split(' (')[0]} · ${c[1]}</span>`;
      m.querySelector('.win-body > div:first-child').innerHTML = c[2] ? `<img src="../_assets/trainers/${c[2]}.png" alt="" style="zoom:1.5">` : '';
      m.querySelector('.vt-lg').textContent = c[4][0];
    };
    $('#llamada .win-body').removeAttribute('data-close');
    $('#llamada .win-body').dataset.action = 'nextLine';
    $('#llamada .win-foot [data-btn="cross"]').dataset.action = 'nextLine';
    A.nextLine = () => { const c = G.contacts[UI.selected(list)]; line++;
      if (line >= c[4].length) { UI_MODALS.closeTop(); UI.toast('Click! (call ended)', 900); show(UI.selected(list)); return; }
      $('#llamada .vt-lg').textContent = c[4][line]; };
    UI.onSelect(list, show); show(0);
  };

  /* ---------------- PokéNav · map ---------------- */
  S.pokenav_mapa = () => {
    let zone = 'MAPSEC_ROUTE_101', method = 'GRASS', time = G.player.night ? 'NIGHT' : 'DAY', rod = 'ALL', zoom = false;
    const list = $('.enc .list'), map = $('#map');
    const METHODS = ['GRASS', 'SURF', 'FISH', 'ROCK'], TIMES = ['MORNING', 'DAY', 'NIGHT'], RODS = ['ALL', 'OLD', 'GOOD', 'SUPER'];
    const encOf = () => { const data = (G.encounters[zone] || {})[method];
      if (method === 'GRASS' && data) return data[time] || data.DAY || [];
      if (method === 'FISH' && data) return ['OLD', 'GOOD', 'SUPER'].filter(r => rod === 'ALL' || rod === r).flatMap(r => (data[r] || []).map(e => [...e, r + ' ROD']));
      return data || []; };
    const filterText = () => method === 'GRASS' ? `${method} · ${time}` : method === 'FISH' ? `${method} · ${rod === 'ALL' ? 'ALL RODS' : rod + ' ROD'}` : method;
    // Side list: icon + full name of the current method, large rows
    const paint = () => {
      const enc = encOf();
      $('#encwhat').textContent = filterText();
      UI.rows(list, enc.length ? enc : [null], e => e ? `<div class="row">${UI.icon(e[0])}<span class="grow">${e[0].toUpperCase()}</span></div>`
                                                     : '<div class="row"><span class="grow soft">NO WILD DATA</span></div>');
      if ($('#encuentros').classList.contains('open')) paintPopup();
    };
    // ENCOUNTERS popup: method tabs, time of day (grass) or rod (fish), levels and rod per row
    const tabs = (el, items, cur, set) => { el.innerHTML = items.map(t => `<span class="tab${t === cur ? ' on' : ''}" style="cursor:pointer">${t}</span>`).join('');
      el.querySelectorAll('.tab').forEach(t => { t.onclick = () => { set(t.textContent); paint(); }; }); };
    const paintPopup = () => {
      $('#enctitle').textContent = $('#zname').textContent;
      tabs($('#encmethods'), METHODS, method, v => { method = v; });
      const t2 = $('#enctimes');
      if (method === 'GRASS') { t2.style.display = ''; tabs(t2, TIMES, time, v => { time = v; }); }
      else if (method === 'FISH') { t2.style.display = ''; tabs(t2, RODS, rod, v => { rod = v; }); }
      else t2.style.display = 'none';
      const enc = encOf();
      $('#enccount').textContent = enc.length + ' POKéMON';
      UI.rows($('#enctable'), enc.length ? enc : [null], e => e ? `<div class="row">${UI.icon(e[0])}<span class="grow">${e[0].toUpperCase()}</span><span class="rod">${e[3] ? `<span class="chip">${e[3]}</span>` : ''}</span><span class="lv">Lv${e[1]}-${e[2]}</span></div>`
                                                             : '<div class="row"><span class="grow soft">NO WILD DATA FOR THIS FILTER</span></div>');
    };
    A.openEnc = () => { UI_MODALS.open('encuentros'); paintPopup(); };
    document.addEventListener('keydown', e => { if (UI_MODALS.stack[UI_MODALS.stack.length - 1] !== $('#encuentros')) return;
      const k = e.key.toLowerCase(), d = k === 'arrowleft' ? -1 : k === 'arrowright' ? 1 : 0;
      if (d && method === 'GRASS') { time = TIMES[(TIMES.indexOf(time) + d + 3) % 3]; paint(); }
      if (d && method === 'FISH') { rod = RODS[(RODS.indexOf(rod) + d + 4) % 4]; paint(); }
      if (k === 't') { e.stopImmediatePropagation(); method = METHODS[(METHODS.indexOf(method) + 1) % 4]; paint(); } }, true);
    // 2x zoom around the cursor (SELECT or X, as SELECT: 2X ZOOM in the current map)
    const applyZoom = () => {
      const cur = map.querySelector('.cursor');
      const cx = cur ? parseInt(cur.style.left, 10) + 12 : 336, cy = cur ? parseInt(cur.style.top, 10) + 12 : 180;
      map.style.transformOrigin = `${cx}px ${cy}px`; map.style.transform = zoom ? 'scale(2)' : '';
      map.parentElement.style.overflow = 'hidden';
    };
    A.mapZoom = () => { zoom = !zoom; applyZoom(); UI.toast(zoom ? '2X ZOOM' : 'FULL MAP', 700); };
    const zb = $('.footer [data-btn="cross"]'); if (zb) zb.dataset.action = 'mapZoom';
    key(k => { if (k === 'v') A.mapZoom(); });
    const obs = new MutationObserver(() => { const t = $('#ztype').textContent.split(' ')[0]; if (t && t !== zone && t.startsWith('MAPSEC')) { zone = t; paint(); if (zoom) applyZoom(); } });
    obs.observe($('#ztype'), { childList: true, characterData: true, subtree: true });
    key(k => { if (k === 't') { method = METHODS[(METHODS.indexOf(method) + 1) % 4]; paint(); } if (k === 's') A.openEnc(); });
    paint();
  };

  /* ---------------- Bag ---------------- */
  S.mochila = (opts = {}) => {
    const battle = !!opts.battle;
    const list = $('.main .list'), det = $('.detail'); let pocket = curTab() || 'balls';
    // Give mode: opened from the party (ITEM > GIVE): the only action is GIVE, ○ goes back to the party
    const giveTo = battle ? null : store.get('giveTo', null);
    if (giveTo !== null) {
      const m = party()[giveTo]; const cap = document.createElement('span'); cap.className = 'capsule'; cap.textContent = 'GIVE TO ' + m.name;
      $('.header').insertBefore(cap, $('.header .spacer').nextSibling);
      document.addEventListener('keydown', e => { const k = e.key.toLowerCase(); if (!UI_MODALS.stack.length && (k === 'escape' || k === 'b')) { e.stopImmediatePropagation(); store.set('giveTo', null); location.href = '../equipo/index.html'; } }, true);
    }
    const items = () => G.bag[pocket];
    const render = () => {
      const pk = G.pockets.find(x => x[0] === pocket);
      $('.bagimg').style.backgroundPosition = `0 -${pk[2] * 128}px`;
      $('.bagart .ps').textContent = pk[1];
      $('.bagart .vt-sm').textContent = items().length + ' kinds';
      $('.main .panel-head .ps').textContent = pk[1] + ' POCKET';
      if (!items().length) { UI.rows(list, [null], () => '<div class="row"><span class="item-ico"></span><span class="grow soft">POCKET IS EMPTY</span></div>', () => ({ 'data-action': 'closeBag' })); det.innerHTML = '<div class="desc soft">This pocket is empty.</div>'; return; }
      UI.rows(list, [...items(), null], it => it ? `<div class="row">${UI.item(it[0])}<span class="grow">${it[1]}</span>${G.registered === it[0] ? '<span class="chip lead">SEL</span>' : ''}<span class="count">${pocket === 'key' ? '' : 'x' + it[2]}</span></div>`
                                                : '<div class="row"><span class="item-ico"></span><span class="grow soft">CLOSE BAG</span></div>',
        it => (it ? { 'data-open': 'acciones' } : { 'data-action': 'closeBag' }));
      show(0);
    };
    // Reference detail card: category chip, 3-column data strip, coloured action buttons
    const CAT = { items: ['ITEM', 'YES', 'YES', 'YES'], balls: ['BALL', 'YES', 'YES', 'NO'], tms: ['TM', 'NO', 'NO', 'YES'],
                  berries: ['BERRY', 'YES', 'YES', 'YES'], key: ['KEY ITEM', 'NO', 'NO', 'YES'] };
    const ACTS = { items: [['USE', 'green'], ['GIVE', 'amber'], ['TOSS', 'red']], balls: [['GIVE', 'amber'], ['TOSS', 'red']], tms: [['USE', 'green'], ['GIVE', 'amber']],
                   berries: [['USE', 'green'], ['GIVE', 'amber'], ['TOSS', 'red']], key: [['USE', 'green'], ['REGISTER', 'amber']] };
    const show = i => { const it = items()[i]; if (!it) return;
      const c = battle ? [CAT[pocket][0], 'YES', 'YES', 'NO'] : CAT[pocket];
      const btns = battle ? [['USE', 'green']] : giveTo !== null ? (pocket === 'key' ? [] : [['GIVE', 'amber']]) : ACTS[pocket];
      det.innerHTML = `<div class="top"><div class="iconbox"><img class="x2" src="../_assets/items/${it[0]}.png" alt=""></div><div><div class="ps">${it[1].length > 12 ? it[1].slice(0, 12) : it[1]}</div>
          <div style="margin-top:6px; display:flex; gap:8px; align-items:center"><span class="chip">${c[0]}</span>${pocket === 'key' ? '' : '<span>x' + it[2] + '</span>'}</div></div></div>
        <div class="desc">${it[4]}</div>
        <div class="acts">${btns.map(([t, col]) => `<span class="act ${col}" data-action="bag_${t.toLowerCase()}">${t}</span>`).join('')}</div>`;
      const acts = battle ? ['USE', 'CANCEL'] : giveTo !== null ? (pocket === 'key' ? ['CANCEL'] : ['GIVE', 'CANCEL']) : pocket === 'key' ? ['USE', 'REGISTER', 'CANCEL'] : pocket === 'balls' ? ['GIVE', 'TOSS', 'CANCEL'] : pocket === 'tms' ? ['USE', 'GIVE', 'CANCEL'] : ['USE', 'GIVE', 'TOSS', 'CANCEL'];
      $('#acciones .win-head').innerHTML = `<span class="ps">${it[1].slice(0, 12)}</span>`;
      $('#acciones .win').style.top = Math.min(110 + i * 40, 300) + 'px';
      UI.rows($('#acciones .list'), acts, a => `<div class="row">${a}</div>`, a => ({ 'data-action': 'bag_' + a.toLowerCase() }));
      $('#cantidad .win-head').innerHTML = `<span class="ps">TOSS</span><span class="spacer"></span><span class="vt-sm">${it[1]} · in bag x${it[2]}</span>`;
      $('#cantidad .win-body img').src = `../_assets/items/${it[0]}.png`;
      $('#cantidad .stepper').stepConfig({ max: it[2], v: 1 });
    };
    const cur = () => items()[UI.selected(list)];
    A.closeBag = () => UI.toast(battle ? 'Back to the battle.' : 'The bag closes (back to the START menu).');
    A.bag_use = () => { const it = cur();
      if (battle) { UI_MODALS.closeTop();
        if (pocket === 'balls') { it[2]--; UI.toast(`BRENDAN used one ${it[1]}! (back to the battle: throw animation)`, 2400); render(); return; }
        if (EFFECTS[it[0]]) { store.set('battleItem', it[0]); location.href = '../equipo_combate/index.html'; return; }
        UI.toast("It can't be used now."); return; }
      if (pocket === 'tms') { store.set('useTm', it[0]); location.href = '../equipo_mt/index.html'; return; }
      if (EFFECTS[it[0]]) { store.set('useItem', it[0]); store.set('targetMode', 'use'); location.href = '../equipo_objetivo/index.html'; return; }
      UI_MODALS.closeTop(); UI.toast(`BRENDAN used the ${it[1]}.`); };
    A.bag_give = () => { const it = cur();
      if (giveTo !== null) { // opened from the party (ITEM > GIVE): it goes to that POKéMON
        const p = party(), m = p[giveTo];
        if (m.item) { const old = allItems().find(x => x[1] === m.item); if (old) old[2]++; }
        m.item = it[1]; it[2]--; if (it[2] <= 0) items().splice(items().indexOf(it), 1); saveParty(p);
        store.set('giveTo', null); UI_MODALS.closeTop(); UI.toast(`${m.name} was given the ${it[1]} to hold.`, 1400);
        setTimeout(() => { location.href = '../equipo/index.html'; }, 1300); return; }
      store.set('useItem', it[0]); store.set('targetMode', 'give'); location.href = '../equipo_objetivo/index.html'; };
    A.bag_toss = () => UI_MODALS.open('cantidad');
    A.bag_cancel = () => UI_MODALS.closeTop();
    A.bag_register = () => { G.registered = cur()[0]; UI_MODALS.closeTop(); render(); UI.toast(`${G.bag.key.find(k => k[0] === G.registered)[1]} registered to SELECT.`); };
    A.tossConfirm = () => { const i = UI.selected(list), it = items()[i], n = $('#cantidad .stepper').stepValue();
      it[2] -= n; UI_MODALS.closeTop(); UI_MODALS.closeTop(); UI.toast(`Threw away ${n} ${it[1]}.`);
      if (it[2] <= 0) items().splice(i, 1); render(); };
    $$('#cantidad .choice-row .row')[0].removeAttribute('data-close'); $$('#cantidad .choice-row .row')[0].dataset.action = 'tossConfirm';
    $('#cantidad .win-foot [data-btn="cross"]').dataset.action = 'tossConfirm';
    const SORTS = ['DEFAULT', 'A TO Z', 'MOST', 'LEAST']; let sort = 0;
    $('.footer [data-btn="select"]').dataset.action = 'bagSort';
    A.bagSort = () => { sort = (sort + 1) % 4;
      if (sort === 0) items().sort((a, b) => (a.order || 0) - (b.order || 0));
      if (sort === 1) items().sort((a, b) => a[1].localeCompare(b[1]));
      if (sort === 2) items().sort((a, b) => b[2] - a[2]);
      if (sort === 3) items().sort((a, b) => a[2] - b[2]);
      render(); UI.toast('POCKET SORTED! (' + SORTS[sort] + ')', 900); };
    onTab(t => { pocket = t; render(); });
    UI.onSelect(list, show);
    render();
  };

  /* ---------------- Mart · buy ---------------- */
  S.tienda_comprar = () => {
    const list = $('.main .list'), det = $('.detail'), st = $('.stepper');
    const stock = G.mart.stock.map(([id, pocket, name, price, desc]) => {
      const it = pocket ? G.bag[pocket].find(x => x[0] === id) : null;
      return { id, pocket: pocket || 'items', name: it ? it[1] : name, price: it ? it[3] : price, desc: it ? it[4] : desc };
    });
    const owned = s => { const it = G.bag[s.pocket].find(x => x[0] === s.id); return it ? it[2] : 0; };
    const render = (sel = 0) => {
      UI.rows(list, [...stock, null], s => s ? `<div class="row">${UI.item(s.id)}<span class="grow">${s.name}</span><span class="owned">${owned(s) ? 'x' + owned(s) : '-'}</span><span class="price">${UI.money(s.price)}</span></div>`
                                            : '<div class="row"><span class="item-ico"></span><span class="grow soft">QUIT</span></div>',
        s => (s ? { 'data-open': 'compra' } : { 'data-action': 'quitMart' }));
      list.querySelector('.on').classList.remove('on'); list.children[sel].classList.add('on'); show(sel);
    };
    const show = i => { const s = stock[i]; if (!s) return;
      det.innerHTML = `<div class="top"><div class="iconbox"><img class="x2" src="../_assets/items/${s.id}.png" alt=""></div><div><div class="ps">${s.name.slice(0, 12)}</div><div style="margin-top:6px">In bag: <b>x${owned(s)}</b></div></div></div><div class="sep"></div><div class="desc">${s.desc}</div>`;
      st.stepConfig({ unit: s.price, max: Math.max(1, Math.min(99, Math.floor(money() / s.price))), v: 1 });
      $('.side .panel:last-child .vt-sm').textContent = s.id === 'poke_ball' ? 'Buy 10 POKé BALLS, get a PREMIER BALL.' : 'You have ' + UI.money(money()) + '.';
    };
    const pending = () => { const s = stock[UI.selected(list)], n = st.stepValue(); return { s, n, total: s.price * n }; };
    document.addEventListener('ui-step', () => { const { s, n, total } = pending();
      $('#compra .win-head .money').textContent = '-' + UI.money(total);
      $('#compra .win-body img').src = `../_assets/items/${s.id}.png`;
      $('#compra .vt-lg').innerHTML = `${s.name}, and you want ${n}.<br>That will be ${UI.money(total)}. OK?`; });
    A.buyYes = () => { const { s, n, total } = pending();
      if (total > money()) { UI_MODALS.closeTop(); UI.toast('You don\'t have enough money.'); return; }
      setMoney(money() - total);
      let it = G.bag[s.pocket].find(x => x[0] === s.id); if (!it) { it = [s.id, s.name, 0, s.price, s.desc]; G.bag[s.pocket].push(it); } it[2] += n;
      const bonus = s.id === 'poke_ball' && n >= 10;
      $('#recibo .vt-lg').innerHTML = `Here you go! Thank you very much.${bonus ? '<br>You also get a PREMIER BALL as an added bonus.' : ''}<br><span class="vt-sm soft">In bag: ${s.name} x${it[2]} · Money ${UI.money(money())}</span><div class="choice-row"><div class="row on" data-close>OK</div></div>`;
      UI_MODALS.closeTop(); UI_MODALS.open('recibo'); render(UI.selected(list)); };
    const yes = $$('#compra .choice-row .row')[0]; yes.removeAttribute('data-open'); yes.dataset.action = 'buyYes';
    $('#compra .win-foot [data-btn="cross"]').removeAttribute('data-open'); $('#compra .win-foot [data-btn="cross"]').dataset.action = 'buyYes';
    A.quitMart = () => UI.toast('"Please come again!" (back to the field)');
    UI.onSelect(list, show);
    render(0);
  };

  /* ---------------- Mart · sell ---------------- */
  S.tienda_vender = () => {
    const list = $('.main .list'), det = $('.detail'), st = $('.stepper'); let pocket = 'items';
    const tabs = $$('.subtabs .tab'), keys = ['items', 'balls', 'tms', 'berries'];
    tabs.forEach((t, i) => { t.onclick = () => { pocket = keys[i]; tabs.forEach(x => x.classList.toggle('on', x === t)); render(); }; });
    const items = () => G.bag[pocket];
    const render = () => {
      UI.rows(list, [...items(), null], it => it ? `<div class="row">${UI.item(it[0])}<span class="grow">${it[1]}</span><span class="owned">x${it[2]}</span><span class="price">${it[3] ? UI.money(it[3] / 2) : '—'}</span></div>`
                                                : '<div class="row"><span class="item-ico"></span><span class="grow soft">QUIT</span></div>',
        it => (it ? { 'data-open': 'venta' } : { 'data-action': 'quitMart' }));
      show(0);
    };
    const show = i => { const it = items()[i]; if (!it) return;
      det.innerHTML = `<div class="top"><div class="iconbox"><img class="x2" src="../_assets/items/${it[0]}.png" alt=""></div><div><div class="ps">${it[1].slice(0, 12)}</div><div style="margin-top:6px">In bag: <b>x${it[2]}</b></div></div></div><div class="sep"></div><div class="desc">${it[4]}</div>`;
      st.stepConfig({ unit: it[3] / 2, max: it[2], v: 1 });
    };
    const pending = () => { const it = items()[UI.selected(list)], n = st.stepValue(); return { it, n, total: it[3] / 2 * n }; };
    document.addEventListener('ui-step', () => { const { it, n, total } = pending(); if (!it) return;
      $('#venta .win-head .money').textContent = '+' + UI.money(total);
      $('#venta .win-body img').src = `../_assets/items/${it[0]}.png`;
      $('#venta .vt-lg').innerHTML = it[3] ? `I can pay ${UI.money(total)}<br>for ${n} ${it[1]}. Would that be okay?` : `${it[1]}? Oh, no, I can't buy that.`; });
    A.sellYes = () => { const { it, n, total } = pending(); UI_MODALS.closeTop();
      if (!it[3]) { UI.toast('That can\'t be sold.'); return; }
      setMoney(money() + total); it[2] -= n; const i = UI.selected(list); if (it[2] <= 0) items().splice(i, 1);
      UI.toast(`Turned over the ${it[1]} and received ${UI.money(total)}.`); render(); };
    const yes = $$('#venta .choice-row .row')[0]; yes.removeAttribute('data-close'); yes.dataset.action = 'sellYes';
    $('#venta .win-foot [data-btn="cross"]').removeAttribute('data-close'); $('#venta .win-foot [data-btn="cross"]').dataset.action = 'sellYes';
    A.quitMart = () => UI.toast('"Please come again!" (back to the field)');
    UI.onSelect(list, show);
    render();
  };

  /* ---------------- Pokédex ---------------- */
  S.pokedex = () => {
    const list = $('.main .list'), entry = $('.entry'); let mode = curTab() || 'hoenn';
    const render = () => {
      const idx = (typeof view !== 'undefined' && view) ? view : G.dex.map((_, i) => i);
      UI.rows(list, idx.map(i => G.dex[i]), (d, k) => { const i = idx[k];
        const num = String(mode === 'hoenn' ? i + 1 : d[2]).padStart(3, '0');
        if (!d[3]) return `<div class="row"><span class="num">${num}</span><span class="ball"></span><span class="mon"></span><span class="grow soft">-----</span></div>`;
        return `<div class="row${d[3] === 1 ? ' seen' : ''}"><span class="num">${num}</span>${d[3] === 2 ? '<img class="ball" src="../_assets/balls/poke.png" alt="">' : '<span class="ball"></span>'}${UI.icon(d[0])}<span class="grow">${d[1]}</span></div>`;
      }, d => (d[3] === 2 ? { 'data-action': 'openDex' } : {}));
      $('.main .panel-head .ps').textContent = mode === 'hoenn' ? 'HOENN LIST' : 'NATIONAL LIST';
      show(0);
    };
    // Entry card: number + name strip, sprite well, data table (species, type, status),
    // height / weight with the footprint, flavour text box. Unknown data shows as ???.
    const show = k => { const i = (typeof view !== 'undefined' && view) ? view[k] : k; const d = G.dex[i], num = String(mode === 'hoenn' ? i + 1 : d[2]).padStart(3, '0');
      const own = d[3] === 2, seen = d[3] >= 1;
      entry.innerHTML = `<div class="panel-head"><span class="ps">No.${num}</span><span class="spacer"></span><span class="ps">${seen ? d[1] : '-----'}</span></div>
        <div class="top">
          <div class="well pic">${seen ? `<img class="x2" src="../_assets/pokemon/${d[0]}_front.png" alt=""${own ? '' : ' style="filter:brightness(0)"'}>` : '<span class="ps-lg soft">?</span>'}</div>
          <div class="kvs">
            <div><div class="k">SPECIES</div><div class="v">${seen ? d[4] || '' : '?????'}</div></div>
            <div><div class="k">TYPE</div><div class="v">${seen ? (d[5] || []).map(UI.type).join(' ') : '???'}</div></div>
            <div><div class="k">STATUS</div><div class="v">${own ? '<img src="../_assets/balls/poke.png" alt=""> CAUGHT' : seen ? 'SEEN' : 'UNKNOWN'}</div></div>
          </div>
        </div>
        <div class="row2">
          <div class="kvs h"><div><div class="k">HEIGHT</div><div class="v">${own ? d[6] : '???'}</div></div><div><div class="k">WEIGHT</div><div class="v">${own ? d[7] : '???'}</div></div></div>
          <div class="well fp">${own ? `<img class="x3" src="../_assets/pokemon/${d[0]}_footprint.png" alt="">` : ''}</div>
        </div>
        <div class="inset flavor"><div>${own ? d[8] : seen ? 'Catch it to see its POKéDEX entry.' : 'Not seen yet.'}</div></div>`;
    };
    A.openDex = el => { const k = +el.dataset.idx; store.set('dex', (typeof view !== 'undefined' && view) ? view[k] : k); location.href = '../pokedex_detalle/index.html'; };
    $('.footer [data-btn="triangle"]').dataset.action = 'cry';
    A.cry = () => { const k = UI.selected(list), d = G.dex[(typeof view !== 'undefined' && view) ? view[k] : k]; if (d[3]) { const img = entry.querySelector('.pic img'); img.animate([{ transform: 'scale(1)' }, { transform: 'scale(1.15)' }, { transform: 'scale(1)' }], 300); UI.toast(`${d[1]}'s cry!`, 900); } };
    $('.footer [data-btn="select"]').dataset.action = 'dexSearch';
    // Search popup (name initial, type, order, mode) filters and sorts the list
    const OPTS = [['NAME', ['ANY', 'ABC', 'DEF', 'GHI', 'JKL', 'MNO', 'PQR', 'STU', 'VWX', 'YZ']],
                  ['TYPE', ['ANY', 'normal', 'fire', 'water', 'grass', 'electric', 'ice', 'fight', 'poison', 'ground', 'flying', 'psychic', 'bug', 'rock', 'ghost', 'dragon', 'dark', 'steel']],
                  ['ORDER', ['NUMERICAL', 'A TO Z', 'HEAVIEST', 'LIGHTEST', 'TALLEST', 'SMALLEST']], ['MODE', ['HOENN', 'NATIONAL']]];
    const val = OPTS.map(() => 0);
    const renderOpts = (sel = 0) => { UI.rows($('#searchopts'), [...OPTS, null], (o, i) => o
      ? `<div class="row"><span class="grow">${o[0]}</span><span class="key" data-d="-1">&lt;</span><span style="width:120px;text-align:center">${o[1][val[i]].toUpperCase()}</span><span class="key" data-d="1">&gt;</span></div>`
      : '<div class="row"><span class="grow"><b>SEARCH</b></span></div>', o => ({ 'data-action': o ? 'optNext' : 'doSearch' }));
      const reset = document.createElement('div'); reset.className = 'row'; reset.dataset.action = 'resetSearch'; reset.innerHTML = '<span class="grow">RESET ALL FILTERS</span>';
      $('#searchopts').appendChild(reset);
      const l = $('#searchopts'); l.querySelector('.on').classList.remove('on'); l.children[sel].classList.add('on'); };
    const bump = (i, d) => { if (i >= OPTS.length) return; val[i] = (val[i] + d + OPTS[i][1].length) % OPTS[i][1].length; renderOpts(i); };
    $('#searchopts').addEventListener('click', e => { const k = e.target.closest('.key[data-d]'); if (k) { e.stopPropagation(); bump(+k.closest('.row').dataset.idx, +k.dataset.d); } }, true);
    document.addEventListener('keydown', e => { if (UI_MODALS.stack[UI_MODALS.stack.length - 1] !== $('#buscar')) return;
      const k = e.key.toLowerCase(); if (k === 'arrowleft' || k === 'arrowright') { e.preventDefault(); bump(UI.selected($('#searchopts')), k === 'arrowleft' ? -1 : 1); } }, true);
    A.optNext = el => bump(+el.dataset.idx, 1);
    $('#buscar .win-foot [data-btn="cross"]').dataset.action = 'doSearch';
    let view = null;
    A.doSearch = () => {
      const [ni, ti, oi, mi] = val, letters = OPTS[0][1][ni], type = OPTS[1][1][ti];
      let res = G.dex.map((d, i) => [d, i]).filter(([d]) => d[3] && (ni === 0 || letters.includes(d[1][0])) && (ti === 0 || (d[5] || []).includes(type)));
      const num = s => { const m = (s || '').match(/(\d+)'(\d+)/); return m ? +m[1] * 12 + +m[2] : 0; }, wt = s => parseFloat(s) || 0;
      const sorters = [null, (a, b) => a[0][1].localeCompare(b[0][1]), (a, b) => wt(b[0][7]) - wt(a[0][7]), (a, b) => wt(a[0][7]) - wt(b[0][7]), (a, b) => num(b[0][6]) - num(a[0][6]), (a, b) => num(a[0][6]) - num(b[0][6])];
      if (sorters[oi]) res.sort(sorters[oi]);
      mode = mi ? 'national' : 'hoenn'; view = res.map(([, i]) => i);
      UI_MODALS.closeTop(); render(); UI.toast(res.length ? res.length + ' POKéMON found.' : 'No matching POKéMON were found.');
    };
    A.dexSearch = () => { renderOpts(); UI_MODALS.open('buscar'); };
    A.resetSearch = () => { val.fill(0); view = null; mode = 'hoenn'; UI_MODALS.closeTop(); render(); UI.toast('All filters reset.', 900); };
    onTab(t => { mode = t; view = null; render(); });
    UI.onSelect(list, show);
    render();
  };

  /* ---------------- Pokédex · detail ---------------- */
  S.pokedex_detalle = () => {
    const owned = G.dex.map((d, i) => [d, i]).filter(([d]) => d[3] === 2);
    let k = Math.max(0, owned.findIndex(([, i]) => i === store.get('dex', 0)));
    const show = () => { const [d, i] = owned[k];
      $('#dexcap').textContent = `No.${String(i + 1).padStart(3, '0')} ${d[1]}`;
      $('.card .pic img').src = `../_assets/pokemon/${d[0]}_front.png`;
      $('#dexname').textContent = d[1];
      $('#dexspecies').textContent = `${d[4]} POKéMON`;
      $('#dextype').innerHTML = d[5].map(UI.type).join(' ');
      $('#dexhw').textContent = `${d[6]} · ${d[7]}`;
      $('.card .print img').src = `../_assets/pokemon/${d[0]}_footprint.png`;
      const imgs = $$('.card .size img'); imgs[1].src = `../_assets/pokemon/${d[0]}_front.png`;
      const inch = s => { const m = s.match(/(\d+)'(\d+)/); return m ? +m[1] * 12 + +m[2] : 20; };
      imgs[1].style.height = Math.round(128 * inch(d[6]) / 55) + 'px';
      $('.card .flavor').textContent = d[8];
      $('#tamano .win-head .vt-sm').textContent = `BRENDAN vs ${d[1]}`;
      const t = $$('#tamano .win-body img')[1]; t.src = `../_assets/pokemon/${d[0]}_front.png`; t.style.height = Math.round(240 * inch(d[6]) / 55) + 'px';
      $$('#tamano .win-body .vt-sm')[1].textContent = d[6];
      $('#area .win-head .ps').textContent = 'AREA · ' + d[1];
      const am = $('#areamap'); am.querySelectorAll('div').forEach(x => x.remove());
      fetch('../_assets/map/hoenn_zones.json').then(r => r.json()).then(z => {
        const zones = Object.entries(G.encounters).filter(([, m]) => Object.values(m).flat().some(e => e[0] === d[0])).map(([id]) => id);
        $('#area .win-head .vt-sm').textContent = zones.length ? zones.length + ' areas' : 'Area unknown';
        zones.forEach(id => { const q = z.zones[id]; if (!q) return; const m = document.createElement('div');
          Object.assign(m.style, { position: 'absolute', left: q.x * 24 + 'px', top: q.y * 24 + 'px', width: q.w * 24 + 'px', height: q.h * 24 + 'px', background: 'rgba(217,72,59,.55)', outline: '2px solid #fff' });
          am.appendChild(m); });
      });
      store.set('dex', i);
    };
    key(kk => { if (kk === 'arrowup' || kk === 'arrowdown') { k = (k + (kk === 'arrowup' ? -1 : 1) + owned.length) % owned.length; show(); } });
    $('.footer [data-btn="triangle"]').dataset.action = 'cry2';
    A.cry2 = () => { $('.card .pic img').animate([{ transform: 'scale(1)' }, { transform: 'scale(1.15)' }, { transform: 'scale(1)' }], 300);
      const w = $('#wave'); w.innerHTML = '';
      for (let x = 0; x < 64; x++) { const b = document.createElement('i'); const amp = Math.round(Math.abs(Math.sin(x * 0.45 + k) * Math.cos(x * 0.13)) * 100);
        b.style.height = Math.max(4, amp) + '%'; b.style.animationDelay = (x * 12) + 'ms'; w.appendChild(b); }
      $('#grito .win-head .vt-sm').textContent = owned[k][0][1]; UI_MODALS.open('grito'); };
    onTab(t => { if (t === 'cry') A.cry2(); });
    show();
  };

  /* ---------------- Options ---------------- */
  S.opciones = () => {
    // Real options (vita_ui_options.c): [group, label, kind, values, value, help]
    const panel = $('.main'), help = $('.help'); let tab = curTab() || 'general';
    panel.classList.add('list');
    const opts = () => G.options[tab];
    const valueHtml = o => o[2] === 'enum' ? `<div class="seg">${o[3].map((v, j) => `<span class="${j === o[4] ? 'v' : ''}" data-v="${j}">${v}</span>`).join('')}</div>`
      : o[2] === 'slider' ? `<div class="slider"><span class="key slide" data-d="-1">&lt;</span><div class="bar exp"><i style="width:${o[4]}%"></i></div><span>${o[4]}%</span><span class="key slide" data-d="1">&gt;</span></div>`
      : '<div class="seg"><span class="v">EXECUTE</span></div>';
    const render = () => {
      let last = '';
      panel.innerHTML = opts().map((o, i) => {
        const grp = o[0] !== last ? `<div class="grp ps">${o[0]}</div>` : '';
        last = o[0];
        return grp + `<div class="opt${i === 0 ? ' on' : ''}" data-idx="${i}"${o[2] === 'action' ? ` data-open="${o[3]}"` : ''}><span>${o[1]}</span>${valueHtml(o)}</div>`;
      }).join('');
      help.textContent = opts()[0][5];
      panel.scrollTop = 0;
      if (window.rescanLists) window.rescanLists();
    };
    const rowOf = i => panel.querySelector(`.opt[data-idx="${i}"]`);
    const change = (i, d, abs) => { const o = opts()[i];
      if (o[2] === 'enum') o[4] = abs != null ? abs : (o[4] + d + o[3].length) % o[3].length;
      if (o[2] === 'slider') o[4] = Math.min(o[3][1], Math.max(o[3][0], o[4] + d * o[3][2]));
      if (o[2] !== 'action') rowOf(i).lastElementChild.outerHTML = valueHtml(o); };
    panel.addEventListener('click', e => {
      const v = e.target.closest('.seg span[data-v]'), s2 = e.target.closest('.slide');
      const row = e.target.closest('.opt'); if (!row) return;
      if (v) { e.stopPropagation(); selectItem(row); change(+row.dataset.idx, 0, +v.dataset.v); }
      if (s2) { e.stopPropagation(); selectItem(row); change(+row.dataset.idx, +s2.dataset.d); }
    }, true);
    key(k => { if (k === 'arrowleft' || k === 'arrowright') { const r = panel.querySelector('.opt.on'); if (r) change(+r.dataset.idx, k === 'arrowleft' ? -1 : 1); } });
    document.addEventListener('ui-select', e => { if (panel.contains(e.detail)) help.textContent = opts()[+e.detail.dataset.idx][5]; });
    onTab(t => { tab = t; render(); });
    // Remap popup: the 8 actions, any of them to any button, and RESET TO DEFAULTS
    let binds = store.get('remap.v2', G.remap.map(r => r.slice()));
    const glyph = b => ({ cross: '<span class="glyph cross">✕</span>', circle: '<span class="glyph circle">○</span>', triangle: '<span class="glyph triangle">△</span>', square: '<span class="glyph square">□</span>' }[b] || `<span class="key">${b}</span>`);
    const renderRemap = (sel = 0) => { UI.rows($('#remapeo .win-body'), [...binds, null], r => r ? `<div class="row"><span class="grow">${r[0]}</span>${glyph(r[1])}</div>` : '<div class="row"><span class="grow">RESET TO DEFAULTS</span></div>',
        r => ({ 'data-action': r ? 'remapCycle' : 'remapDefaults' }));
      const l = $('#remapeo .win-body'); l.querySelector('.on').classList.remove('on'); l.children[sel].classList.add('on'); };
    A.remapCycle = el => { const i = +el.dataset.idx, c = G.remapCycle; binds[i][1] = c[(c.indexOf(binds[i][1]) + 1) % c.length]; store.set('remap.v2', binds); renderRemap(i); };
    $('#remapeo .win-foot [data-btn="triangle"]').dataset.action = 'remapDefaults';
    A.remapDefaults = () => { binds = G.remap.map(r => r.slice()); store.set('remap.v2', binds); renderRemap(); UI.toast('Default buttons restored.', 900); };
    renderRemap(); render();
  };

  /* ---------------- Trainer card ---------------- */
  S.ficha = () => {
    const body = $('.body'), front = body.innerHTML;
    const info = () => $('.badges .info');
    const bind = () => { document.addEventListener('ui-select', e => { const g = $('.badges .grid'); if (g && g.contains(e.detail)) {
      const i = +e.detail.dataset.idx, b = G.badges[i];
      info().innerHTML = i < G.player.badges ? `${b[0]} BADGE · ${b[1]} · Leader ${b[2]}<br>${b[3]}-type GYM.` : 'Not obtained yet.'; } }); };
    const tagBadges = () => $$('.badges .grid .b').forEach((b, i) => { b.dataset.idx = i; });
    onTab(t => {
      if (t === 'front') { body.innerHTML = front; tagBadges(); return; }
      body.innerHTML = `<div class="panel" style="width:520px; flex:none"><div class="panel-head"><span class="ps">TRAINER RECORDS</span></div>
        ${G.player.records.map(r => `<div class="kv2"><span>${r[0]}</span><b>${r[1]}</b></div>`).join('')}</div>
        <div class="panel" style="flex:1"><div class="panel-head"><span class="ps">FRONTIER</span><span class="spacer"></span><span>0 BP</span></div>
        <div class="grid" style="display:grid; grid-template-columns:repeat(3,1fr); gap:8px">${[1, 2, 3, 4, 5, 6, 7].map(i => `<div class="b off" style="height:76px;background:var(--paper-1);display:flex;flex-direction:column;align-items:center;justify-content:center;gap:8px"><img src="../_assets/medals/silver_${i}.png" alt="" style="zoom:3;opacity:.2"><span class="vt-sm">-</span></div>`).join('')}</div>
        <div class="vt-sm soft" style="margin-top:8px">No symbols yet.</div></div>`;
    });
    tagBadges(); bind();
  };

  /* ---------------- Field: dialogue / multichoice / yes-no ---------------- */
  const PAGES = {
    dialogo: ['BRENDAN! Before you go, say hi to<br>PROF. BIRCH next door.', 'He\'s a POKéMON researcher, so he\'s<br>always out in the field.', 'Come home safely, okay?'],
    llamada: ['Hi, BRENDAN! How\'s your POKéDEX coming<br>along? I just caught a ZIGZAGOON!', 'I\'m heading to RUSTBORO next.<br>See you around!'],
    objeto_obtenido: ['BRENDAN obtained 5 POKé BALLS!', 'BRENDAN put away the POKé BALLS<br>in the POKé BALLS POCKET.'],
  };
  function dialogue(pages) {
    const box = $('.dialog'), more = box.querySelector('.more'), name = box.querySelector('.name'), phone = box.querySelector('.phone');
    let i = 0, typing = null;
    const type = html => { clearInterval(typing); const plain = html; let n = 0;
      [...box.childNodes].forEach(c => { if (c !== more && c !== name && c !== phone) c.remove(); });
      const span = document.createElement('span'); box.insertBefore(span, more || null);
      typing = setInterval(() => { n += 2; span.innerHTML = plain.slice(0, n); if (n >= plain.length) { clearInterval(typing); typing = null; } }, 16); };
    const next = () => { if (typing) { clearInterval(typing); typing = null; box.querySelector('span:not(.more)').innerHTML = pages[i]; return; }
      i++; if (i >= pages.length) { UI.toast('(end of the message: the game continues)'); i = -1; return; } type(pages[i]); };
    box.style.cursor = 'pointer'; box.addEventListener('click', next);
    document.addEventListener('keydown', e => { if (e.key.toLowerCase() === 'x') next(); });
    type(pages[0]);
  }
  S.dialogo = () => dialogue(PAGES.dialogo);
  S.llamada = () => dialogue(PAGES.llamada);
  S.objeto_obtenido = () => dialogue(PAGES.objeto_obtenido);
  S.multichoice = () => {
    const rows = $$('.choice .row'); rows.forEach((r, i) => { r.dataset.idx = i; });
    rows[0].dataset.go = 'tienda_comprar'; rows[1].dataset.go = 'tienda_vender'; rows[2].dataset.action = 'seeYa';
    A.seeYa = () => UI.toast('"Please come again!"');
  };
  S.si_no = () => {
    const rows = $$('.choice .row'); rows[0].dataset.action = 'yes'; rows[1].dataset.action = 'no';
    A.yes = () => UI.toast('MOM: "Good! Off you go, then!"'); A.no = () => UI.toast('MOM: "Then go adjust it upstairs!"');
  };

  /* ---------------- Party as target of a bag item ---------------- */
  const EFFECTS = {
    potion: ['hp', 20], super_potion: ['hp', 50], hyper_potion: ['hp', 200], oran_berry: ['hp', 10],
    antidote: ['cure', 'PSN'], pecha_berry: ['cure', 'PSN'], paralyze_heal: ['cure', 'PAR'], awakening: ['cure', 'SLP'],
    burn_heal: ['cure', 'BRN'], ice_heal: ['cure', 'FRZ'], full_heal: ['cure', '*'], revive: ['revive'], rare_candy: ['level'], ether: ['pp', 10],
  };
  const findItem = id => { for (const [k, list] of Object.entries(G.bag)) { const it = list.find(x => x[0] === id); if (it) return [it, k]; } return [null, null]; };
  S.equipo_objetivo = () => {
    // Opened from the bag: USE (the item's effect on the chosen POKéMON) or GIVE (it holds it)
    const p = party(), box = $('.party'); const id = store.get('useItem', 'potion'), mode = store.get('targetMode', 'use');
    const verdict = (m, eff) => {
      if (mode === 'give') return m.egg ? [false, 'An EGG cannot hold items.'] : m.item ? [true, `Swap the ${m.item} for it`] : [true, 'Give it to hold'];
      if (m.egg) return [false, 'It cannot be used on an EGG.'];
      if (eff[0] === 'pp') return m.moves.some(mv => mv[2] < mv[3]) ? [true, 'Restores 10 PP of one move'] : [false, "It won't have any effect."];
      if (eff[0] === 'hp') return m.status === 'FNT' || m.hp === 0 ? [false, 'It won\'t have any effect.'] : m.hp >= m.maxhp ? [false, 'It won\'t have any effect.'] : [true, `HP ${m.hp} → ${Math.min(m.maxhp, m.hp + eff[1])}`];
      if (eff[0] === 'cure') return m.status && (eff[1] === '*' || m.status === eff[1]) ? [true, `Cures ${m.status}`] : [false, 'It won\'t have any effect.'];
      if (eff[0] === 'revive') return m.hp === 0 ? [true, `Revives with ${Math.floor(m.maxhp / 2)} HP`] : [false, 'It won\'t have any effect.'];
      if (eff[0] === 'level') return m.lv < 100 ? [true, `Lv${m.lv} → Lv${m.lv + 1}`] : [false, 'It won\'t have any effect.'];
      return [false, '-'];
    };
    const render = (sel = UI.selected(box)) => {
      const [it] = findItem(id), eff = EFFECTS[id];
      $('#itemcap').textContent = `${mode === 'give' ? 'GIVE' : 'USE'} · ${it[1]} x${it[2]}`; $('#itemimg').src = `../_assets/items/${id}.png`;
      $('#verb').textContent = mode === 'give' ? 'Give' : 'Use';
      $('#itemname').textContent = it[1]; $('#itemdesc').textContent = it[4];
      UI.rows(box, p, m => { const [ok] = verdict(m, eff);
        return partyCard(m).replace('</div></div></div>', `</div></div></div>`).replace('<span class="hp">', `<span class="tag">${ok ? 'ABLE' : '—'}</span><span class="hp">`); },
        () => ({ 'data-action': 'useOn' }));
      box.querySelector('.on').classList.remove('on'); box.children[sel].classList.add('on'); preview(sel);
    };
    const preview = i => { const [ok, txt] = verdict(p[i], EFFECTS[id]); $('#effect').innerHTML = `${p[i].name}: <b>${txt}</b>`; $('#effect').style.color = ok ? '' : 'var(--ink-soft)'; };
    A.useOn = () => {
      const i = UI.selected(box), m = p[i], eff = EFFECTS[id], [ok, txt] = verdict(m, eff), [it, pocket] = findItem(id);
      if (!ok) { UI.toast(txt); return; }
      if (mode === 'give') { // the held item goes back to the bag, then back to the bag screen
        if (m.item) { const old = allItems().find(x => x[1] === m.item); if (old) old[2]++; }
        const was = m.item; m.item = it[1]; it[2]--; if (it[2] <= 0) G.bag[pocket].splice(G.bag[pocket].indexOf(it), 1);
        saveParty(p); render(i);
        UI.toast(was ? `${m.name} was given the ${it[1]}; the ${was} went back to the BAG.` : `${m.name} was given the ${it[1]} to hold.`, 1600);
        setTimeout(() => { location.href = '../mochila/index.html?tab=' + pocket; }, 1500); return; }
      if (eff[0] === 'hp') { const before = m.hp; m.hp = Math.min(m.maxhp, m.hp + eff[1]); UI.toast(`${m.name}'s HP was restored by ${m.hp - before} point(s).`); }
      if (eff[0] === 'cure') { UI.toast(`${m.name} was cured of ${m.status}.`); m.status = ''; }
      if (eff[0] === 'revive') { m.hp = Math.floor(m.maxhp / 2); m.status = ''; UI.toast(`${m.name} was revived!`); }
      if (eff[0] === 'pp') { // like PARTY_MODE_MOVE_SELECT: choose the move to restore
        UI.rows($('#ppmoves'), m.moves, mv => `<div class="row"><span class="grow">${mv[0]}</span><span>PP ${mv[2]}/${mv[3]}</span></div>`, () => ({ 'data-action': 'restorePp' }));
        UI_MODALS.open('pp'); return; }
      if (eff[0] === 'level') { // like PARTY_MODE_DELTA_CARD: the stats before and after the level up
        const before = Object.fromEntries(Object.entries(m.stats).map(([k, v]) => [k, v[0]]));
        m.lv++; Object.entries(m.stats).forEach(([k, v], j) => { v[0] += 1 + ((m.lv + j) % 3 === 0 ? 1 : 0); });
        m.maxhp = m.stats.HP[0]; m.hp = Math.min(m.maxhp, m.hp + (m.stats.HP[0] - before.HP));
        $('#deltahead').textContent = `${m.name} grew to Lv. ${m.lv}!`;
        $('#deltabody').innerHTML = Object.entries(m.stats).map(([k, v]) => `<div class="kv2"><span>${k}</span><span>${before[k]} → <b>${v[0]}</b> <span class="accent">+${v[0] - before[k]}</span></span></div>`).join('');
        UI_MODALS.open('delta'); }
      it[2]--; if (it[2] <= 0) { G.bag[pocket].splice(G.bag[pocket].indexOf(it), 1); saveParty(p); UI.toast(`No ${it[1]} left. Back to the BAG.`); setTimeout(() => { location.href = '../mochila/index.html'; }, 1400); }
      saveParty(p); render(i);
    };
    A.restorePp = el => { const i = UI.selected(box), m = p[i], mv = m.moves[+el.dataset.idx], [it, pocket] = findItem(id);
      if (mv[2] >= mv[3]) { UI.toast("It won't have any effect."); return; }
      const before = mv[2]; mv[2] = Math.min(mv[3], mv[2] + 10); it[2]--; if (it[2] <= 0) G.bag[pocket].splice(G.bag[pocket].indexOf(it), 1);
      saveParty(p); UI_MODALS.closeTop(); UI.toast(`${mv[0]}'s PP was restored (${before} → ${mv[2]}).`); render(i); };
    UI.onSelect(box, preview);
    render(0);
  };

  /* ---------------- Party as target of a TM / HM (+ forget a move) ---------------- */
  const TM = { tm28: ['DIG', 'ground', 10, 80, 100, 'PHYSICAL', 'Digs underground the first turn and strikes the next turn.', 'contest_smart', ['mudkip', 'zigzagoon']],
               tm39: ['ROCK TOMB', 'rock', 10, 50, 80, 'PHYSICAL', 'Stops the foe\'s movement with rocks. May lower SPEED.', 'contest_smart', ['mudkip']],
               tm01: ['FOCUS PUNCH', 'fight', 20, 150, 100, 'PHYSICAL', 'Powerful, but makes the user flinch if hit first.', 'contest_tough', ['mudkip', 'zigzagoon']],
               hm01: ['CUT', 'normal', 30, 50, 95, 'PHYSICAL', 'Cuts the foe with sharp scythes or claws.', 'contest_cool', ['mudkip', 'zigzagoon']] };
  S.equipo_mt = () => {
    const p = party(), box = $('.party'); const id = store.get('useTm', 'tm28'), t = TM[id], [it] = findItem(id);
    $('#tmcap').textContent = 'TEACH · ' + (it ? it[1] : id.toUpperCase());
    const able = m => t[8].includes(m.sp) && !m.moves.some(mv => mv[0] === t[0]);
    const render = (sel = 0) => {
      UI.rows(box, p, m => `<div class="card${able(m) ? '' : ' no'}">${UI.icon(m.sp)}<span class="name">${m.egg ? m.name : m.name + ' Lv' + m.lv}</span><span class="able">${m.egg ? 'EGG' : m.moves.some(mv => mv[0] === t[0]) ? 'LEARNED' : able(m) ? 'ABLE' : 'NOT ABLE'}</span></div>`, () => ({ 'data-action': 'teach' }));
      box.querySelector('.on').classList.remove('on'); box.children[sel].classList.add('on'); panel(sel);
    };
    const panel = i => { const m = p[i];
      $('#tmpanel').innerHTML = `<div class="ps">${t[0]}</div><div>${UI.type(t[1])}</div><div class="kv"><span>POWER</span><span>${t[3]}</span></div><div class="kv"><span>ACCURACY</span><span>${t[4]}</span></div><div class="kv"><span>PP</span><span>${t[2]}</span></div><div class="vt-sm">${t[6]}</div>
        <div class="sep"></div><div class="vt-sm">${m.name}: ${able(m) ? (m.moves.length < 4 ? 'can learn it now.' : 'already knows 4 moves: one must be forgotten.') : 'can\'t learn this move.'}</div>`; };
    A.teach = () => { const i = UI.selected(box), m = p[i];
      if (!able(m)) { UI.toast(`${m.name} can't learn ${t[0]}.`); return; }
      if (m.moves.length < 4) { m.moves.push([t[0], t[1], t[2], t[2], t[3], t[4], t[5], t[6], t[7]]); saveParty(p); UI.toast(`${m.name} learned ${t[0]}!`); render(i); return; }
      $('#forgethead').textContent = `${m.name} · new: ${t[0]}`;
      $('#forgettext').textContent = `${m.name} is trying to learn ${t[0]}. Which move should be forgotten?`;
      UI.rows($('#forgetlist'), [...m.moves, null], mv => mv ? `<div class="row"><span class="grow">${mv[0]}</span><img src="../_assets/types/${mv[1]}.png" alt=""><span class="vt-sm">PP ${mv[3]}</span></div>`
                                                                : `<div class="row"><span class="grow soft">Don't learn ${t[0]}</span></div>`, mv => ({ 'data-action': mv ? 'forget' : 'stopLearning' }));
      UI_MODALS.open('olvidar'); };
    A.forget = el => { const i = UI.selected(box), m = p[i], k = +el.dataset.idx, old = m.moves[k][0];
      m.moves[k] = [t[0], t[1], t[2], t[2], t[3], t[4], t[5], t[6], t[7]]; saveParty(p); UI_MODALS.closeTop();
      UI.toast(`1, 2, and... Poof! ${m.name} forgot ${old}. And... ${m.name} learned ${t[0]}!`, 2600); render(i); };
    A.stopLearning = () => { UI_MODALS.closeTop(); UI.toast(`${p[UI.selected(box)].name} did not learn ${t[0]}.`); };
    $('#olvidar .win-foot [data-btn="cross"]').dataset.action = 'forgetSelected';
    A.forgetSelected = () => { const el = $('#forgetlist .on'); activate(el); };
    UI.onSelect(box, panel);
    render(0);
  };

  /* ---------------- Nuzlocke tracker ---------------- */
  S.nuzlocke = () => {
    const STATES = ['AVAILABLE', 'CAUGHT', 'MISSED', 'BLOCKED'];
    const CLS = { AVAILABLE: 'AVAILABLE', CAUGHT: 'CAUGHT', MISSED: 'FAILED', BLOCKED: 'BLOCKED' };
    const squad = party().filter(m => !m.egg).map(m => [m, G.nuzlocke.fallen.includes(m.name)]);
    let routes = store.get('nuzlocke.v2', [['ROUTE 101', 'zigzagoon', 1], ['ROUTE 102', 'ralts', 1], ['ROUTE 103', 'wingull', 2], ['ROUTE 104', 'taillow', 1],
      ['PETALBURG WOODS', 'wurmple', 1], ['ROUTE 116', 'lotad', 1], ['RUSTURF TUNNEL', 'whismur', 2], ['DEWFORD TOWN', null, 0], ['GRANITE CAVE', null, 0],
      ['ROUTE 109', null, 0], ['ROUTE 110', 'poochyena', 3], ['ROUTE 117', null, 0], ['ROUTE 111', null, 0], ['ROUTE 112', null, 0]]);
    const RULES = [['One encounter per area', 'Only the first wild POKéMON of each area can be caught.'], ['Fainted = released', 'A POKéMON that faints is considered dead and is boxed for good.'],
      ['Dupes clause', 'If the first encounter is a species you already caught, the next one counts (BLOCKED).'], ['Shiny clause', 'Shiny POKéMON can always be caught.'],
      ['Nicknames', 'Every caught POKéMON must be nicknamed.']];
    const list = $('#nzlist'); let tab = curTab() || 'routes';
    const counts = () => STATES.map(s => routes.filter(r => STATES[r[2]] === s).length);
    const render = (sel = 0) => {
      const c = counts(), fallen = squad.filter(x => x[1]).length;
      $('#nzsum').textContent = `NEXT: ${G.nuzlocke.nextLeader} (CAP LV.${G.nuzlocke.cap})`;
      $('#nzcount').innerHTML = [['CAUGHT AREAS', c[1]], ['MISSED AREAS', c[2]], ['FALLEN ALLIES', fallen]].map(([k, v]) => `<div class="count"><span>${k}</span><span>${v}</span></div>`).join('');
      if (tab === 'routes') {
        $('#nztitle').textContent = 'ENCOUNTERS';
        UI.rows(list, routes, r => `<div class="row">${r[1] ? UI.icon(r[1]) : '<span class="mon"></span>'}<span class="grow">${r[0]}</span><span class="st ${CLS[STATES[r[2]]]}">${STATES[r[2]]}</span></div>`, () => ({ 'data-action': 'nzCycle' }));
      } else if (tab === 'squad') {
        $('#nztitle').textContent = 'SQUAD VITAL STATUS';
        UI.rows(list, squad, ([m, dead]) => `<div class="row">${UI.icon(m.sp)}<span class="grow">${m.name}</span><span>Lv${m.lv}</span><span class="st ${dead ? 'FAILED' : 'CAUGHT'}">${dead ? 'FALLEN' : 'ALIVE'}</span></div>`);
      } else if (tab === 'settings') {
        // The CHALLENGE options of OPTIONS > MODES, plus a tracker reset (ui_scr_nuz.c)
        $('#nztitle').textContent = 'SETTINGS';
        const opts = G.options.modes.filter(o => o[0] === 'CHALLENGE');
        UI.rows(list, [...opts, null], o => o ? `<div class="row"><span class="grow">${o[1]}</span><span class="capsule">◀ ${o[3][o[4]]} ▶</span></div>`
          : `<div class="row"><span class="grow">RESET TRACKER</span><span class="capsule">EXECUTE</span></div>`);
      } else {
        $('#nztitle').textContent = 'RULES';
        UI.rows(list, RULES, r => `<div class="row"><span class="grow">${r[0]}</span><span class="st CAUGHT">ON</span></div>`);
      }
      list.querySelector('.on').classList.remove('on'); list.children[Math.min(sel, list.children.length - 1)].classList.add('on'); detail(sel);
    };
    const detail = i => {
      if (tab === 'routes') { const r = routes[i]; $('#nzdetail').innerHTML = `<div class="ps">${r[0]}</div><div style="margin-top:8px">${r[1] ? 'First encounter: <b>' + r[1].toUpperCase() + '</b>' : 'No encounter yet.'}</div><div class="vt-sm soft" style="margin-top:6px">State: ${STATES[r[2]]}</div>`; }
      else if (tab === 'squad') { const [m, dead] = squad[i]; $('#nzdetail').innerHTML = `<div class="ps">${m.name}</div><div style="margin-top:8px">${dead ? 'Fainted: it can no longer be used.' : 'Alive. Level cap: ' + G.nuzlocke.cap}</div>`; }
      else if (tab === 'settings') { $('#nzdetail').innerHTML = `<div class="ps">SETTINGS</div><div class="vt-sm" style="margin-top:8px; line-height:24px">${i < list.children.length - 1 ? 'Left / right or X to change. Saved at once.' : 'Clears the areas and fallen allies of this save slot.'}</div>`; }
      else { const r = RULES[i]; $('#nzdetail').innerHTML = `<div class="ps">RULE</div><div class="vt-sm" style="margin-top:8px; line-height:24px">${r[1]}</div>`; }
    };
    A.nzCycle = el => { const i = +el.dataset.idx; routes[i][2] = (routes[i][2] + 1) % STATES.length; store.set('nuzlocke.v2', routes); render(i); };
    $('.footer [data-btn="cross"]').dataset.action = 'nzCycleSel';
    A.nzCycleSel = () => { if (tab === 'routes') A.nzCycle(list.querySelector('.on')); };
    onTab(t => { tab = t; render(); });
    UI.onSelect(list, detail);
    render();
  };

  S.mochila_combate = () => S.mochila({ battle: true });
  S.equipo_combate = () => S.equipo({ battle: true });

  /* ---------------- Fly (map component in fly mode) ---------------- */
  S.vuelo = () => {
    const VISITED = ['MAPSEC_LITTLEROOT_TOWN', 'MAPSEC_OLDALE_TOWN', 'MAPSEC_PETALBURG_CITY', 'MAPSEC_RUSTBORO_CITY', 'MAPSEC_DEWFORD_TOWN',
                     'MAPSEC_SLATEPORT_CITY', 'MAPSEC_MAUVILLE_CITY', 'MAPSEC_VERDANTURF_TOWN', 'MAPSEC_LAVARIDGE_TOWN', 'MAPSEC_FALLARBOR_TOWN'];
    const map = $('#map'); let cur = { x: 4, y: 11 }, data = null;
    fetch('../_assets/map/hoenn_zones.json').then(r => r.json()).then(d => {
      data = d;
      const hl = document.createElement('div'); hl.className = 'zone-hl'; map.appendChild(hl);
      const cu = document.createElement('div'); cu.className = 'cursor'; map.appendChild(cu);
      for (const id of VISITED) { const z = d.zones[id]; if (!z) continue; const f = document.createElement('div'); f.className = 'fly';
        Object.assign(f.style, { left: z.x * 24 + 'px', top: z.y * 24 + 'px', width: z.w * 24 + 'px', height: z.h * 24 + 'px' }); map.appendChild(f); }
      const home = d.zones.MAPSEC_LITTLEROOT_TOWN, head = document.createElement('div'); head.className = 'head';
      head.style.left = (home.x * 24 + 4) / 1.5 + 'px'; head.style.top = (home.y * 24 + 4) / 1.5 + 'px'; map.appendChild(head);
      $('#ndest').textContent = VISITED.length;
      for (let y = 0; y < d.height; y++) for (let x = 0; x < d.width; x++) {
        const c = document.createElement('div'); c.className = 'cell'; c.style.left = x * 24 + 'px'; c.style.top = y * 24 + 'px';
        c.onclick = () => { if (cur.x === x && cur.y === y) A.flyAsk(); else move(x, y); }; map.appendChild(c); }
      const move = (x, y) => { cur = { x, y }; const id = d.grid[y][x], z = d.zones[id];
        cu.style.left = x * 24 + 'px'; cu.style.top = y * 24 + 'px';
        if (z) Object.assign(hl.style, { display: 'block', left: z.x * 24 + 'px', top: z.y * 24 + 'px', width: z.w * 24 + 'px', height: z.h * 24 + 'px' }); else hl.style.display = 'none';
        $('#zname').textContent = z ? z.name : '—';
        $('#zinfo').textContent = VISITED.includes(id) ? 'You can fly here.' : z ? 'You can\'t fly here.' : '';
      };
      window.__flyMove = move; move(4, 11);
    });
    A.flyAsk = () => { const id = data.grid[cur.y][cur.x];
      if (!VISITED.includes(id)) { UI.toast('You can\'t fly there.', 900); return; }
      $('#flytext').textContent = 'Fly to ' + data.zones[id].name + '?'; UI_MODALS.open('volar'); };
    A.flyYes = () => { UI_MODALS.closeTop(); UI.toast('TAILLOW used FLY! (the menu closes, the FLY animation plays)', 2400); };
    $('.footer [data-btn="cross"]').dataset.action = 'flyAsk';
    key(k => { const d = { arrowup: [0, -1], arrowdown: [0, 1], arrowleft: [-1, 0], arrowright: [1, 0] }[k];
      if (d && data) window.__flyMove(Math.min(27, Math.max(0, cur.x + d[0])), Math.min(14, Math.max(0, cur.y + d[1]))); });
  };

  // Every menu footer ends with the clock (HH:MM only). The money goes at the top right of the
  // header in the menus that show it (header data-money). No header shows the player's name.
  $$('.footer:not(.legend)').forEach(f => {
    if (!f.querySelector(':scope > .spacer')) f.appendChild(Object.assign(document.createElement('span'), { className: 'spacer' }));
    const st = document.createElement('span'); st.className = 'fstat';
    st.innerHTML = `<span class="capsule clock">${G.player.clock}</span>`;
    f.appendChild(st);
  });
  $$('.header[data-money]').forEach(h => {
    const c = document.createElement('span'); c.className = 'capsule';
    c.innerHTML = `<span class="money js-money">${UI.money(money())}</span>`;
    h.appendChild(c);
  });

  if (window.initSteppers) window.initSteppers();
  const run = S[document.body.dataset.screen];
  if (run) run();
})();
