# Getting started
```
git clone --recurse-submodules https://github.com/amb/stm32-midi-ocm3.git
cd stm32-midi-ocm3
make -C libopencm3
make -C project
```

# Updates
```
make -C project
```

# Flashing
```
make -C project flash
```