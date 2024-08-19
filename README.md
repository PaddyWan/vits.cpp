This is a fork of the main: https://github.com/maxilevi/vits.cpp
All the main changes are in different branches with pull request to the main repo. My main branch combines all these changes and add some additional changes for my specific desktop linux settup.

# vits-cpp

a cpp ggml port of "VITS: Conditional Variational Autoencoder with Adversarial Learning for End-to-End Text-to-Speech." for use in mobile devices. 

# How to use

Clone and fetch all submodules
```
git@github.com:maxilevi/vits.cpp.git
cd vits.cpp
git submodule update --init --recursive
```

Fetch the models
```
git lfs pull
```

Alternatively you can export the models yourself from huggingface 
```
pip install -r scripts/requirements.txt
python3 scripts/export_vits.py
```

To build and run the program you can run: 

```
make
```
