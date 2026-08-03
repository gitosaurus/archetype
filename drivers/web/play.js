//
//  play.js
//  archetype
//
//  Drives one resident universe in wasm memory, a turn at a time.  There is no
//  server: the .acx is fetched once, loaded into the module, and everything
//  after that -- including the save -- stays in this tab.
//

'use strict';

(function () {

  // ---------------------------------------------------------------- elements

  const el = {
    transcript: document.getElementById('transcript'),
    scroll: document.getElementById('scroll'),
    command: document.getElementById('command'),
    entry: document.getElementById('entry'),
    status: document.getElementById('status'),
    game: document.getElementById('game'),
    width: document.getElementById('width'),
    restart: document.getElementById('restart'),
    download: document.getElementById('download'),
    upload: document.getElementById('upload'),
    ruler: document.getElementById('ruler')
  };

  let arch = null;      // the instantiated module
  let manifest = null;
  let current = null;   // the entry from games.json being played
  let columns = 80;
  let busy = false;

  // --------------------------------------------------------------- ANSI text

  // games/intrptr.arch builds its prompt out of \e[2m, \e[1m and \e[0m, so SGR
  // sequences arrive inside the narrative.  Anything else the CSI grammar
  // allows is dropped rather than printed.
  const CSI = /\x1b\[([0-9;]*)([A-Za-z])/g;

  function escapeHtml(text) {
    return text.replace(/[&<>]/g, (ch) => (
      ch === '&' ? '&amp;' : ch === '<' ? '&lt;' : '&gt;'
    ));
  }

  function ansiToHtml(text) {
    let html = '';
    let cursor = 0;
    let bold = false;
    let dim = false;

    function emit(run) {
      if (!run) return;
      const escaped = escapeHtml(run);
      if (!bold && !dim) {
        html += escaped;
        return;
      }
      const classes = (bold ? 'b' : '') + (bold && dim ? ' ' : '') + (dim ? 'd' : '');
      html += '<span class="' + classes + '">' + escaped + '</span>';
    }

    CSI.lastIndex = 0;
    let match;
    while ((match = CSI.exec(text)) !== null) {
      emit(text.slice(cursor, match.index));
      cursor = CSI.lastIndex;
      if (match[2] !== 'm') continue;
      const params = (match[1] === '' ? '0' : match[1]).split(';');
      for (const param of params) {
        const code = parseInt(param === '' ? '0' : param, 10);
        if (code === 0) { bold = false; dim = false; }
        else if (code === 1) bold = true;
        else if (code === 2) dim = true;
        else if (code === 22) { bold = false; dim = false; }
      }
    }
    emit(text.slice(cursor));
    return html;
  }

  // Kept alongside the save so that resuming restores what the player had been
  // reading, not just the world they were standing in.  Stored as the
  // interpreter emitted it, escapes and all, and re-rendered on the way back.
  let narrativeSoFar = '';

  function append(text, remember) {
    if (!text) return;
    if (remember !== false) narrativeSoFar += text;
    el.transcript.insertAdjacentHTML('beforeend', ansiToHtml(text));
    el.scroll.scrollTop = el.scroll.scrollHeight;
  }

  // Text from a turn that has not happened yet: the game printed a prompt and
  // stopped to ask a question.  It is shown but never remembered, because the
  // next attempt replays the turn from the top and produces it again -- that
  // time with the player's answer echoed into it.  Kept in an element of its
  // own so it can be taken back off without disturbing the transcript.
  let provisional = null;

  function showProvisional(text) {
    dropProvisional();
    provisional = document.createElement('span');
    provisional.innerHTML = ansiToHtml(text);
    el.transcript.append(provisional);
    el.scroll.scrollTop = el.scroll.scrollHeight;
  }

  function dropProvisional() {
    if (!provisional) return;
    provisional.remove();
    provisional = null;
  }

  // Transient by default: a note about what just happened has said its piece
  // once the player has read it, and leaving it up makes it look like a
  // permanent condition of the game.  Errors and the end-of-game notice are
  // sticky, since those describe a state rather than an event.
  const STATUS_TTL = 5000;
  let statusTimer = null;
  let statusSticky = false;

  function status(message, sticky) {
    clearTimeout(statusTimer);
    statusTimer = null;
    statusSticky = Boolean(message && sticky);
    el.status.textContent = message || '';
    if (message && !sticky) {
      statusTimer = setTimeout(() => status(''), STATUS_TTL);
    }
  }

  // The other half of "goes away by itself": acting is as good as waiting.
  function clearTransientStatus() {
    if (!statusSticky) status('');
  }

  // ------------------------------------------------------------------ layout

  // WrappedOutput wraps to a column count, so the page has to tell it how many
  // columns the transcript actually has.  Measuring a run of characters beats
  // assuming a ratio, since the font is whatever the platform's monospace is.
  //
  // What fits is only half the answer: a 200-column measure is miserable to
  // read even on a monitor that can show it, so the player picks a ceiling and
  // the window supplies the rest.  A fixed choice is still capped by what fits,
  // because exceeding it would scroll the transcript sideways.
  const WIDTH_KEY = 'archetype.columns';
  let widthChoice = '120';

  function measureColumns() {
    el.ruler.textContent = 'M'.repeat(100);
    const perCharacter = el.ruler.getBoundingClientRect().width / 100;
    el.ruler.textContent = '';
    if (!perCharacter) return columns;
    const styles = getComputedStyle(el.scroll);
    const usable = el.scroll.clientWidth
      - parseFloat(styles.paddingLeft) - parseFloat(styles.paddingRight);
    // The floor is deliberate: on a phone, wrapping to a dozen columns reads
    // worse than letting the transcript scroll.
    const fits = Math.max(40, Math.floor(usable / perCharacter));
    if (widthChoice === 'fit') return fits;
    return Math.min(parseInt(widthChoice, 10) || 120, fits);
  }

  let resizeTimer = null;
  window.addEventListener('resize', () => {
    clearTimeout(resizeTimer);
    // Only the next turn's text is affected; what is already on screen was
    // wrapped at the old width and stays that way.
    resizeTimer = setTimeout(() => { columns = measureColumns(); }, 150);
  });

  // ----------------------------------------------------------------- storage

  // A .acx is binary and a whole game's state, which rules out localStorage.
  const DB_NAME = 'archetype';
  const STORE = 'saves';

  function openDb() {
    return new Promise((resolve, reject) => {
      const request = indexedDB.open(DB_NAME, 1);
      request.onupgradeneeded = () => {
        request.result.createObjectStore(STORE);
      };
      request.onsuccess = () => resolve(request.result);
      request.onerror = () => reject(request.error);
    });
  }

  function withStore(mode, action) {
    return openDb().then((db) => new Promise((resolve, reject) => {
      const tx = db.transaction(STORE, mode);
      const request = action(tx.objectStore(STORE));
      tx.oncomplete = () => { db.close(); resolve(request && request.result); };
      tx.onerror = () => { db.close(); reject(tx.error); };
    }));
  }

  // Persistence is best-effort: a browser in private mode, or one that has
  // walled off storage, should still be playable for the length of a visit.
  function putSave(slug, bytes) {
    return withStore('readwrite', (store) => store.put(bytes, slug))
      .catch((error) => { console.warn('Could not store save:', error); });
  }

  function getSave(slug) {
    return withStore('readonly', (store) => store.get(slug))
      .catch((error) => { console.warn('Could not read save:', error); return undefined; });
  }

  function dropSave(slug) {
    return withStore('readwrite', (store) => store.delete(slug))
      .catch(() => undefined);
  }

  function autosave() {
    const bytes = arch.arch_save();
    if (!bytes) {
      console.warn('Could not serialize state:', arch.arch_last_error());
      return Promise.resolve();
    }
    return putSave(current.slug, { acx: bytes, narrative: narrativeSoFar });
  }

  // -------------------------------------------------------------------- play

  // Only the command line closes: a finished game is still worth saving, and
  // the ended state is exactly what the desktop interpreter should see.
  function setPlayable(playable) {
    el.command.disabled = !playable;
    if (playable) el.command.focus();
  }

  function finish() {
    setPlayable(false);
    status('The game has ended.  Start over, or load a save.', true);
  }

  // A mid-turn question, waiting to be answered.  While one is up the player is
  // answering the game rather than commanding it, so the entry box is routed
  // here instead of starting a new turn.
  let pending = null;

  function askPlayer(want) {
    return new Promise((resolve) => {
      pending = { want: want, resolve: resolve };
      status(want === 'key'
        ? 'The game is waiting for a keystroke — Esc to decline.'
        : 'The game is waiting for an answer — Esc to decline.', true);
      setPlayable(true);
    });
  }

  // `value` is null when the player declines, which the interpreter reports as
  // end-of-input: the same thing ^D does at a console prompt, so the game sees
  // UNDEFINED and carries on rather than asking again.
  function answerPlayer(value) {
    const waiting = pending;
    pending = null;
    status('');
    if (waiting) waiting.resolve(value);
  }

  // A turn is a conversation rather than a call.  The game may stop partway
  // through at a 'read' or a 'key' the player has not answered yet; each answer
  // is appended to the list and the turn run again from the top.  The
  // interpreter rolls the universe back before handing the question over, so
  // replaying is exactly as though the whole turn had been typed at once.
  //
  // Only a game that reads without ever being satisfied can run out of
  // attempts, and one declined answer is always enough to end a turn.
  const MAX_PROMPTS = 64;

  async function takeTurn(input) {
    if (busy || arch.arch_ended()) return;
    busy = true;
    setPlayable(false);
    const items = [input];
    try {
      for (let attempt = 0; attempt <= MAX_PROMPTS; attempt++) {
        const turn = arch.arch_turn(items, columns);
        if (turn.status === 'error') {
          dropProvisional();
          status('Error: ' + (arch.arch_last_error() || 'turn failed'), true);
          return;
        }
        if (turn.status === 'complete') {
          dropProvisional();
          append(turn.text);
          await autosave();
          // A finished game is worth keeping: reloading the page should show
          // how it ended rather than silently starting again.
          if (arch.arch_ended()) finish();
          return;
        }
        showProvisional(turn.text);
        items.push(await askPlayer(turn.status === 'needs_key' ? 'key' : 'line'));
      }
      dropProvisional();
      status('The game kept asking for input; the turn was abandoned.', true);
    } finally {
      busy = false;
      if (!arch.arch_ended()) setPlayable(true);
    }
  }

  el.entry.addEventListener('submit', (event) => {
    event.preventDefault();
    if (pending) {
      // Answering a question, not issuing a command.  Sent untrimmed and blank
      // lines included, because that is what the game would have read from a
      // console -- and what keeps a browser transcript replayable as a script.
      if (pending.want !== 'line') return;
      const answer = el.command.value;
      el.command.value = '';
      answerPlayer(answer);
      return;
    }
    const input = el.command.value.trim();
    // An empty command would reach 'UPDATE' as UNDEFINED and end the game with
    // "EOF; goodbye." (games/intrptr.arch), so it never gets sent.
    if (!input || busy) return;
    el.command.value = '';
    clearTransientStatus();
    takeTurn(input);
  });

  document.addEventListener('keydown', (event) => {
    if (!pending || event.altKey || event.ctrlKey || event.metaKey) return;
    if (event.key === 'Escape') {
      event.preventDefault();
      el.command.value = '';
      answerPlayer(null);
      return;
    }
    if (pending.want !== 'key') return;
    // One printable character answers 'key'.  Return counts as a keystroke too,
    // but is sent as a space: the interpreter echoes whatever it reads, and a
    // newline there would break the line the prompt is sitting on.
    if (event.key.length === 1) {
      event.preventDefault();
      answerPlayer(event.key);
    } else if (event.key === 'Enter') {
      event.preventDefault();
      answerPlayer(' ');
    }
  });

  // ------------------------------------------------------------------- games

  function fetchBytes(url) {
    return fetch(url, { cache: 'no-cache' }).then((response) => {
      if (!response.ok) throw new Error(url + ': ' + response.status);
      return response.arrayBuffer();
    }).then((buffer) => new Uint8Array(buffer));
  }

  function loadBytes(bytes) {
    if (!arch.arch_load(bytes)) {
      throw new Error(arch.arch_last_error() || 'Could not load game');
    }
  }

  // `resume` is a stored session -- {acx, narrative} -- or a bare Uint8Array of
  // .acx bytes from a file the player picked.  Without one the game starts from
  // the pristine binary.
  function start(game, resume) {
    current = game;
    el.transcript.textContent = '';
    narrativeSoFar = '';
    el.game.value = game.slug;
    document.title = game.title + ' — Archetype';
    columns = measureColumns();
    busy = true;

    const resumeBytes = resume && (resume.acx || resume);
    const source = resumeBytes
      ? Promise.resolve(resumeBytes)
      : fetchBytes(game.acx);

    return source.then((bytes) => {
      loadBytes(bytes);
      busy = false;
      if (resume && resume.narrative) {
        // Put back what the player had been reading.  It was wrapped at
        // whatever width they had then, which is why it is replayed verbatim
        // rather than re-wrapped.
        append(resume.narrative);
      }
      if (arch.arch_ended()) {
        // A resumed save of a game that was already over.
        append('\nThis game has ended.  Start over to play again.\n', false);
        finish();
        return;
      }
      setPlayable(true);
      status(resumeBytes ? 'Resumed your save.' : '');
      if (resumeBytes) {
        // The universe is mid-game; replaying a turn would advance it, so just
        // hand control back to the player.
        append('\n[ Resumed. ]\n', false);
        return;
      }
      // The first 'UPDATE' is what runs the game's setup and prints the intro
      // and opening room (games/intrptr.arch).  It still needs a command to
      // consume: "wait" costs one harmless line, where "look" would describe
      // the room a second time.  This is what the Cloud Run driver defaults to.
      return takeTurn('wait');
    }).catch((error) => {
      busy = false;
      status('Error: ' + error.message, true);
      console.error(error);
    });
  }

  function chosenSlug() {
    const asked = new URLSearchParams(location.search).get('game');
    if (asked && manifest.games.some((g) => g.slug === asked)) return asked;
    return manifest.default;
  }

  function gameBySlug(slug) {
    return manifest.games.find((g) => g.slug === slug);
  }

  // ----------------------------------------------------------------- buttons

  // Guards the controls that swap the universe out.  A turn used to be over in
  // an instant; now it can span however long a player takes to answer a
  // question, and the turn waiting on that answer will replay itself against
  // whatever universe is loaded when it arrives.  Saving is left alone, since a
  // turn that stopped to ask has been rolled back and the state is sound.
  function interrupting() {
    if (!busy) return false;
    status(pending
      ? 'Answer the question first, or press Esc to decline it.'
      : 'A turn is still running.', true);
    return true;
  }

  el.game.addEventListener('change', () => {
    if (interrupting()) {
      el.game.value = current.slug;
      return;
    }
    const game = gameBySlug(el.game.value);
    const url = new URL(location.href);
    url.searchParams.set('game', game.slug);
    history.replaceState(null, '', url);
    getSave(game.slug).then((saved) => start(game, saved));
  });

  el.restart.addEventListener('click', () => {
    if (interrupting()) return;
    dropSave(current.slug).then(() => start(current, null));
  });

  el.width.addEventListener('change', () => {
    widthChoice = el.width.value;
    try {
      localStorage.setItem(WIDTH_KEY, widthChoice);
    } catch (ignored) {
      // Private browsing can refuse; the choice just will not outlive the tab.
    }
    columns = measureColumns();
    // Text already on screen was wrapped by the interpreter as it was emitted,
    // and cannot be reflowed without re-running the game -- WrappedOutput also
    // centers titles and sizes banner rules to the column count, so the wrap is
    // not something CSS could redo.
    status('Now ' + columns + ' columns, from the next turn on.');
  });

  el.download.addEventListener('click', () => {
    const bytes = arch.arch_save();
    if (!bytes) {
      status('Error: ' + (arch.arch_last_error() || 'could not save'), true);
      return;
    }
    const url = URL.createObjectURL(new Blob([bytes], { type: 'application/octet-stream' }));
    const link = document.createElement('a');
    link.href = url;
    link.download = current.slug + '.save.acx';
    link.click();
    URL.revokeObjectURL(url);
    status('Saved ' + link.download + ' — resume it here or with --perform.');
  });

  el.upload.addEventListener('change', () => {
    if (interrupting()) {
      el.upload.value = '';
      return;
    }
    const file = el.upload.files && el.upload.files[0];
    if (!file) return;
    file.arrayBuffer().then((buffer) => {
      const bytes = new Uint8Array(buffer);
      // A save carries no record of which game it came from, so it is loaded
      // against whichever game is on screen.  It arrives with no transcript --
      // it may well have come from the desktop interpreter.
      return start(current, { acx: bytes, narrative: '' })
        .then(() => autosave());
    }).catch((error) => {
      status('Error: ' + error.message, true);
    }).finally(() => { el.upload.value = ''; });
  });

  // ------------------------------------------------------------------- start

  try {
    const stored = localStorage.getItem(WIDTH_KEY);
    if (stored) widthChoice = stored;
  } catch (ignored) {
    // Nothing stored is a perfectly good answer.
  }
  el.width.value = widthChoice;
  if (!el.width.value) {
    // A stored value no longer offered as an option.
    widthChoice = '120';
    el.width.value = widthChoice;
  }

  status('Loading interpreter…');

  Promise.all([
    createArchetype(),
    fetch('games.json', { cache: 'no-cache' }).then((r) => r.json())
  ]).then(([module, games]) => {
    arch = module;
    manifest = games;

    for (const game of manifest.games) {
      const option = document.createElement('option');
      option.value = game.slug;
      option.textContent = game.title;
      el.game.append(option);
    }

    const game = gameBySlug(chosenSlug());
    return getSave(game.slug).then((saved) => start(game, saved));
  }).catch((error) => {
    status('Could not start: ' + error.message, true);
    console.error(error);
  });

})();
