#!/usr/bin/env python3
# esp32_auth/gui/blockchain_tab.py

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
from datetime import datetime

from blockchain_manual import Blockchain

class BlockchainTab(ttk.Frame):
    DEFAULT_DISPLAY_COUNT = 10

    def __init__(self, parent):
        super().__init__(parent)
        # instantiate blockchain client (will load chain.json)
        self.bc = Blockchain()

        self._build_ui()
        self._refresh()  # initial load

    def _build_ui(self):
        # Row 0: Refresh & Prune buttons + range label
        self.btn_refresh = ttk.Button(self, text="Refresh", command=self._refresh)
        self.btn_refresh.grid(row=0, column=0, padx=5, pady=5, sticky='w')

        self.btn_prune = ttk.Button(self, text="Prune Old", command=self._prune)
        self.btn_prune.grid(row=0, column=1, padx=5, pady=5, sticky='w')

        self.lbl_range = ttk.Label(self, text="")
        self.lbl_range.grid(row=0, column=2, columnspan=2, padx=5, pady=5, sticky='e')

        # Row 1: list of recent blocks
        self.listbox = tk.Listbox(self, height=10)
        self.listbox.grid(row=1, column=0, columnspan=4, sticky='nsew', padx=5, pady=5)
        self.listbox.bind("<<ListboxSelect>>", self._on_select)

        # Row 2: block detail viewer
        self.txt = scrolledtext.ScrolledText(self, height=10, state='disabled')
        self.txt.grid(row=2, column=0, columnspan=4, sticky='nsew', padx=5, pady=5)

        # layout weights
        self.grid_rowconfigure(1, weight=1)
        self.grid_rowconfigure(2, weight=1)
        for c in range(4):
            self.grid_columnconfigure(c, weight=1)

    def _refresh(self):
        """
        Reload chain from disk and update UI list of the last DEFAULT_DISPLAY_COUNT blocks.
        """
        # reload blockchain instance to pick up any new blocks added elsewhere
        self.bc = Blockchain()
        chain = self.bc.get_chain()
        total = len(chain)
        start = max(0, total - self.DEFAULT_DISPLAY_COUNT)

        # update range label
        if total:
            self.lbl_range.config(text=f"Blocks {start}–{total-1}  (total {total})")
        else:
            self.lbl_range.config(text="No blocks")

        # populate listbox with recent blocks
        self.listbox.delete(0, 'end')
        for blk in chain[start:]:
            ts = datetime.fromtimestamp(blk["timestamp"]).strftime("%Y-%m-%d %H:%M:%S")
            self.listbox.insert('end', f"{blk['index']}: {ts}")

        # clear detail pane
        self.txt.configure(state='normal')
        self.txt.delete('1.0', 'end')
        self.txt.configure(state='disabled')

    def _on_select(self, event):
        """
        Display the JSON of the selected block.
        """
        sel = self.listbox.curselection()
        if not sel:
            return
        entry = self.listbox.get(sel[0])
        try:
            idx = int(entry.split(':', 1)[0])
        except ValueError:
            return

        # reload chain to ensure fresh data
        chain = self.bc.get_chain()
        # find block by index (should exist)
        blk = next((b for b in chain if b["index"] == idx), None)
        if not blk:
            return

        import json
        s = json.dumps(blk, indent=2)
        self.txt.configure(state='normal')
        self.txt.delete('1.0', 'end')
        self.txt.insert('end', s)
        self.txt.configure(state='disabled')

    def _prune(self):
        """
        Prune to keep only the last 1000 blocks, then refresh UI.
        """
        self.bc.prune(keep_last=1000)
        messagebox.showinfo("Pruned", "Old blocks removed; keeping last 1000.")
        self._refresh()
