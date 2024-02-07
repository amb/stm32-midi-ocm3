
for ph in range(256):
    if ph < 128:
        sample = ph * 2
    else:
        sample = 255 - (ph - 128) * 2

    print(sample)
