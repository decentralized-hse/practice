from hashlib import sha256

f = open('dir', 'r')
bytesData = f.read()
print(sha256(bytesData.encode()).hexdigest())

