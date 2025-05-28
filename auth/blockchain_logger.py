#!/usr/bin/env python3
import os
import json
import time
from config import BLOCKCHAIN_DIR

# ensure the blockchain directory exists
os.makedirs(BLOCKCHAIN_DIR, exist_ok=True)

def log_entry(entry: dict):
    """
    Write one “transaction” (any dict) to a timestamped JSON file.
    """
    ts = int(time.time() * 1000)
    path = os.path.join(BLOCKCHAIN_DIR, f"entry_{ts}.json")
    with open(path, "w") as f:
        json.dump(entry, f, indent=2)
