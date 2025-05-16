#!/usr/bin/env python3
import torch
from facenet_pytorch import InceptionResnetV1

# 1. Load the pretrained FaceNet (InceptionResnetV1)
model = InceptionResnetV1(pretrained='vggface2').eval()

# 2. Apply dynamic quantization to all torch.nn.Linear layers
qmodel = torch.quantization.quantize_dynamic(
    model,
    {torch.nn.Linear},
    dtype=torch.qint8
)

# 3. Trace (not script) the quantized model on a dummy input
#    to work around missing-attribute issues in scripting
example_input = torch.randn(1, 3, 160, 160)
traced = torch.jit.trace(qmodel, example_input)

# 4. Save the traced, quantized model
traced.save('fnet_quantized.pt')
print("Quantized & traced model saved → fnet_quantized.pt")
