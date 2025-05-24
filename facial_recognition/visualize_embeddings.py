#!/usr/bin/env python3
import pickle

import numpy as np
from sklearn.manifold import TSNE
import matplotlib.pyplot as plt

# ===== Config =====
EMB_FILE = "embeddings_fnet.pkl"  # from generate_embeddings.py
# ==================

def main():
    data = pickle.load(open(EMB_FILE, 'rb'))
    X, y = data['embeddings'], data['labels']

    # t-SNE down to 2 dims
    tsne = TSNE(n_components=2, random_state=42)
    X2 = tsne.fit_transform(X)

    # Plot
    plt.figure()
    for label in sorted(set(y)):
        idx = [i for i, lab in enumerate(y) if lab == label]
        pts = X2[idx]
        plt.scatter(pts[:,0], pts[:,1], label=label)
    plt.legend()
    plt.title("t-SNE of FaceNet Embeddings")
    plt.xlabel("Dim 1")
    plt.ylabel("Dim 2")
    plt.show()

if __name__ == "__main__":
    main()
