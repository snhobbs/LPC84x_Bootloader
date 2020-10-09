import zlib
import sys
f = open(sys.argv[1], 'rb')
data = f.read()
f.close()

kApplicationSize = 28 * 1024
if len(data) == 64 * 1024:
    image = data[36*1024]
elif len(data) == kApplicationSize:
    image = data
elif len(data) < kApplicationSize:
    image = data + bytes([0xde]*(kApplicationSize - len(data)))
else:
    raise UserWarning("Binary length incorrect")

crc = zlib.crc32(image, 0)
image_out = image[:-4] + crc.to_bytes(4, "little")
with open("imageout.bin", "wb") as fout:
    fout.write(image_out)

