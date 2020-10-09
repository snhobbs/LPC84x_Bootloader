import zlib
import sys
f = open(sys.argv[1], 'rb')
data = f.read()
f.close()

image = data[:-4]
crc = zlib.crc32(image, 0)
image_out = image + crc.to_bytes(4, "little")
with open("imageout.bin", "wb") as fout:
    fout.write(image_out)

