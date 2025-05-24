#!/usr/bin/env python3
import pickle
from sklearn.svm import SVC

# Load embeddings
data = pickle.load(open('embeddings_fnet.pkl', 'rb'))
X, y = data['embeddings'], data['labels']

# Train a linear SVM
clf = SVC(kernel='linear', probability=True)
clf.fit(X, y)

# Save model
with open('face_recognizer_svm.pkl', 'wb') as f:
    pickle.dump(clf, f)

print("Trained and saved SVM classifier → face_recognizer_svm.pkl")
