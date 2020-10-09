import zlib
import sys
f = open(sys.argv[1], 'rb')
data = f.read()
f.close()

if len(data) == 64 * 1024:
    image = data[36*1024:-4]
elif len(data) == 28 * 1024:
    image = data[:-4]
else:
    raise UserWarning("Binary length incorrect")

crc = zlib.crc32(image, 0)
image_out = image + crc.to_bytes(4, "little")
with open("imageout.bin", "wb") as fout:
    fout.write(image_out)

