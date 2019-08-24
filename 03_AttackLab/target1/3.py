s = "59b997fa"

r = ""

for c in s:
    r += str(hex(ord(c)))[2:] + " "

print(r)
