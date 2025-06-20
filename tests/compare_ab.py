import numpy as np

a = np.load("/tmp/fakeuser/opticks/GEOM/fakegeom/simg4ox/ALL0_none/A000/record.npy")
b = np.load("/tmp/fakeuser/opticks/GEOM/fakegeom/simg4ox/ALL0_none/B000/f000/record.npy")

print(a.shape)
print(b.shape)

assert a.shape == b.shape

diff = [i for i, (a, b) in enumerate(zip(a[:, 1:], b[:, 0:-1])) if not np.allclose(a, b, rtol=0, atol=1e-5)]
print(diff)

assert diff == [6, 14, 22, 40, 50, 81, 91]
