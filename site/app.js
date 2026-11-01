"use strict";
(() => {
  const latest = "https://github.com/masarray/arson-comp/releases/download/v0.2.0-rc.1/";
  const downloads = {
    windows: latest + "ArSonComp-0.2.0-Windows-x64-Setup.exe",
    macos: latest + "ArSonComp-0.2.0-macOS-Universal-AdHoc.pkg"
  };
  const osName = (navigator.userAgentData && navigator.userAgentData.platform) ||
    navigator.platform || navigator.userAgent || "";
  const detect = /Win/i.test(osName) ? "windows" :
    (/Mac|iPad|iPhone/i.test(osName) ? "macos" :
    (/Linux|X11/i.test(osName) ? "linux" : ""));
  const primary = document.getElementById("primary-download");
  const primaryLabel = document.getElementById("primary-download-label");
  if (detect === "windows" || detect === "macos") {
    primary.href = downloads[detect];
    primaryLabel.textContent = detect === "windows" ? "Download for Windows" : "Download for macOS";
  } else if (detect === "linux") {
    primaryLabel.textContent = "Choose your Linux download";
  }
  const chosenCard = detect && document.getElementById("download-" + detect);
  if (chosenCard) chosenCard.classList.add("recommended");

  const views = {
    full: {src:"./assets/arsoncomp-full.png",alt:"Full ArSonComp dynamics interface",width:612,height:580,caption:"Full Dynamics — transfer graph, gain reduction and primary controls.",counter:"01 / 04"},
    sidechain: {src:"./assets/arsoncomp-sidechain.png",alt:"ArSonComp sidechain filter and detector panel",width:580,height:540,caption:"Sidechain — shape the detector with focused controls.",counter:"02 / 04"},
    mini: {src:"./assets/arsoncomp-mini.png",alt:"ArSonComp compact MINI editor",width:612,height:320,caption:"MINI — key controls in a compact live workspace.",counter:"03 / 04"},
    presets: {src:"./assets/arsoncomp-presets.png",alt:"ArSonComp factory preset browser",width:612,height:580,caption:"Factory presets — a fast starting point for every session.",counter:"04 / 04"}
  };
  const tabs = Array.from(document.querySelectorAll('.gallery-tabs [role="tab"]'));
  const panel = document.getElementById("gallery-panel");
  const image = document.getElementById("gallery-image");
  const caption = document.getElementById("gallery-caption");
  const counter = document.getElementById("gallery-counter");
  function select(tab, focus) {
    const item = views[tab.dataset.view];
    if (!item) return;
    tabs.forEach(b => {
      const active = b === tab;
      b.setAttribute("aria-selected", String(active));
      b.tabIndex = active ? 0 : -1;
    });
    panel.setAttribute("aria-labelledby", tab.id);
    image.src = item.src;
    image.alt = item.alt;
    image.width = item.width;
    image.height = item.height;
    caption.textContent = item.caption;
    counter.textContent = item.counter;
    if (focus) tab.focus();
  }
  tabs.forEach((tab, index) => {
    tab.addEventListener("click", () => select(tab, false));
    tab.addEventListener("keydown", e => {
      let next = index;
      if (e.key === "ArrowRight" || e.key === "ArrowDown") next = (index + 1) % tabs.length;
      else if (e.key === "ArrowLeft" || e.key === "ArrowUp") next = (index + tabs.length - 1) % tabs.length;
      else if (e.key === "Home") next = 0;
      else if (e.key === "End") next = tabs.length - 1;
      else return;
      e.preventDefault();
      select(tabs[next], true);
    });
  });
})();
