import torch
import torch.nn.utils.prune as prune
import pickle

# load your PyTorch model (e.g. ArcFace or FaceNet backbone)
model = torch.load('arcface_backbone.pt', map_location='cpu')

# prune 20% of weights in each linear layer
for name, module in model.named_modules():
    if isinstance(module, torch.nn.Linear):
        prune.l1_unstructured(module, name='weight', amount=0.2)

# remove reparameterization to make masks permanent
for module in model.modules():
    if hasattr(module, 'weight_mask'):
        prune.remove(module, 'weight')

torch.save(model, 'pruned_model.pt')
