#!/usr/bin/env python3
import torch
from facenet_pytorch import InceptionResnetV1

# ===== Config =====
FULL_MODEL_NAME     = 'vggface2'        # for pretrained InceptionResnetV1
QUANTIZED_MODEL_F   = 'fnet_quantized.pt'
TS_EXPORT_F         = 'fnet_ts.pt'
# ==================

def main():
    # Try loading the quantized version first
    try:
        model = torch.jit.load(QUANTIZED_MODEL_F, map_location='cpu')
        print(f"Loaded quantized model → {QUANTIZED_MODEL_F}")
    except (RuntimeError, FileNotFoundError):
        # Fallback: full-precision from facenet-pytorch
        model = InceptionResnetV1(pretrained=FULL_MODEL_NAME).eval()
        print("Loaded full-precision FaceNet")

    # Trace to TorchScript
    example = torch.randn(1, 3, 160, 160)
    traced = torch.jit.trace(model, example)
    traced.save(TS_EXPORT_F)
    print(f"Exported TorchScript model → {TS_EXPORT_F}")

if __name__ == "__main__":
    main()
