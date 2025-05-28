#!/usr/bin/env python3
# esp32_auth/blockchain_manual.py

import os
import json
import time
import hashlib

from config import BLOCKCHAIN_DIR

CHAIN_PATH = os.path.join(BLOCKCHAIN_DIR, "chain.json")

class Blockchain:
    def __init__(self):
        # ensure the directory exists
        os.makedirs(BLOCKCHAIN_DIR, exist_ok=True)

        # load existing chain or create a new one
        if os.path.exists(CHAIN_PATH):
            with open(CHAIN_PATH, "r") as f:
                self.chain = json.load(f)
        else:
            # start with an empty list, so index=0 works correctly
            self.chain = []
            # genesis block
            genesis = self._make_block(data="genesis", prev_hash="0")
            self.chain.append(genesis)
            self._save()

    def _make_block(self, data, prev_hash):
        """
        Build a block dict. Uses current len(self.chain) as the index.
        """
        index     = len(self.chain)
        timestamp = time.time()
        block = {
            "index":     index,
            "timestamp": timestamp,
            "data":      data,
            "prev_hash": prev_hash
        }
        # compute SHA-256 over the JSON (sorted keys) of the block (without the hash)
        block_str     = json.dumps(block, sort_keys=True).encode()
        block["hash"] = hashlib.sha256(block_str).hexdigest()
        return block

    def add_block(self, data):
        """
        Append a new block containing `data` (must be JSON-serializable).
        """
        prev_hash = self.chain[-1]["hash"]
        block     = self._make_block(data, prev_hash)
        self.chain.append(block)
        self._save()
        return block

    def prune(self, keep_last=1000):
        """
        Keep only the last `keep_last` blocks (drop older), reindex + rehash.
        """
        if len(self.chain) <= keep_last:
            return

        # trim and then re-build the chain from scratch
        trimmed = self.chain[-keep_last:]
        self.chain = []
        prev = "0"
        for blk in trimmed:
            # rebuild each block in order
            new_blk = {
                "index":     len(self.chain),
                "timestamp": blk["timestamp"],
                "data":      blk["data"],
                "prev_hash": prev
            }
            # hash it
            block_str     = json.dumps(new_blk, sort_keys=True).encode()
            new_blk["hash"] = hashlib.sha256(block_str).hexdigest()
            self.chain.append(new_blk)
            prev = new_blk["hash"]

        self._save()

    def _save(self):
        with open(CHAIN_PATH, "w") as f:
            json.dump(self.chain, f, indent=2)

    def get_chain(self):
        return self.chain
