import zlib
import sys
f = open(sys.argv[1], 'rb')
data = f.read()
f.close()

kTotalSections = 64
kSectorSize = 1024

kBootloaderSections = 14

kTotalSize = kTotalSections * kSectorSize
kBootloaderSize = kBootloaderSections * kSectorSize
kApplicationSectors = kTotalSections - kBootloaderSections
kApplicationSize = (kApplicationSectors) * kSectorSize

if len(data) == kTotalSize:
    image = data[kBootloaderSize:]
elif len(data) == kApplicationSize:
    image = data
elif len(data) < kApplicationSize:
    image = data + bytes([0xde]*(kApplicationSize - len(data)))
else:
    raise UserWarning("Binary length incorrect")

assert(len(image) == kApplicationSize)
image = image[:-4] # remove last 4 bytes
crc = zlib.crc32(image, 0)
image_out = image + crc.to_bytes(4, "little")
assert(len(image_out) == kApplicationSize)
assert(zlib.crc32(image_out[:-4], 0) == crc)
print("Image CRC ", crc)

with open("imageout.bin", "wb") as fout:
    fout.write(image_out)

