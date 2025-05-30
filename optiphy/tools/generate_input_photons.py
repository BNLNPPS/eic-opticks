#!/usr/bin/env python3

from collections import OrderedDict as odict
import argparse, logging, os, json
import numpy as np
from optiphy.ana.sample import sample_trig, sample_normals, sample_reject, sample_linear, sample_linspace, sample_disc
from optiphy.ana.sample import xy_grid_coordinates

log = logging.getLogger(__name__)
np.set_printoptions(linewidth=200, suppress=True, precision=3)

def vnorm(v):
    norm = np.sqrt((v*v).sum(axis=1))
    norm3 = np.repeat(norm, 3).reshape(-1,3)
    v /=  norm3
    return v


class InputPhotons:
    """
    The "WEIGHT" has never been used as such.
    The (1,3) sphoton.h slot is used for the iindex integer,
    hence set the "WEIGHT" to 0.f in order that the int32
    becomes zero.
    """

    WEIGHT = 0.

    DEFAULT_BASE = os.path.expanduser("~/.opticks/InputPhotons")
    DTYPE = np.float64

    X = np.array( [1., 0., 0.], dtype=DTYPE )
    Y = np.array( [0., 1., 0.], dtype=DTYPE )
    Z = np.array( [0., 0., 1.], dtype=DTYPE )

    POSITION = [0.,0.,0.]
    TIME = 0.1
    WAVELENGTH  = 440.

    @classmethod
    def BasePath(cls, name=None):
        if name is None:
            name = os.environ.get("OPTICKS_INPUT_PHOTON", "RandomSpherical100_f4.npy")

        return os.path.join(cls.DEFAULT_BASE, name)

    @classmethod
    def Path(cls, name, ext=".npy"):
        prec = None
        if cls.DTYPE == np.float32: prec = "_f4"
        if cls.DTYPE == np.float64: prec = "_f8"
        return os.path.join(cls.DEFAULT_BASE, "%s%s%s" % (name, prec, ext))


    @classmethod
    def CubeCorners(cls):
        """
        :return dir: (8,3) array of normalized direction vectors

        000  0   (-1,-1,-1)/sqrt(3)
        001  1
        010  2
        011  3
        100  4
        101  5
        110  6
        111  7   (+1,+1,+1)/sqrt(3)
        """
        v = np.zeros((8, 3), dtype=cls.DTYPE)
        for i in range(8): v[i] = list(map(float,[ bool(i & 1), bool(i & 2), bool(i & 4)]))
        v = 2.*v - 1.
        return vnorm(v)

    @classmethod
    def GenerateCubeCorners(cls):
        direction = cls.CubeCorners()
        polarization = vnorm(np.cross(direction, cls.Y))

        p = np.zeros( (8, 4, 4), dtype=cls.DTYPE )
        n = len(p)
        p[:,0,:3] = cls.POSITION + direction  # offset start position by direction vector for easy identification purposes
        p[:,0, 3] = cls.TIME*(1. + np.arange(n))
        p[:,1,:3] = direction
        p[:,1, 3] = cls.WEIGHT
        p[:,2,:3] = polarization
        p[:,2, 3] = cls.WAVELENGTH
        return p

    @classmethod
    def OutwardsCubeCorners(cls):
        direction = cls.CubeCorners()
        polarization = vnorm(np.cross(direction, cls.Y))

        p = np.zeros( (8, 4, 4), dtype=cls.DTYPE )
        n = len(p)
        p[:,0,:3] = cls.POSITION + direction  # offset start position by direction vector for easy identification purposes
        p[:,0, 3] = cls.TIME*(1. + np.arange(n))
        p[:,1,:3] = direction
        p[:,1, 3] = cls.WEIGHT
        p[:,2,:3] = polarization
        p[:,2, 3] = cls.WAVELENGTH
        return p


    @classmethod
    def InwardsCubeCorners(cls, radius):
        """
        :param radius: of start position
        :return p: (8,4,4) array of photons
        """
        log.info(" radius %s " % radius )
        direction = cls.CubeCorners()
        polarization = vnorm(np.cross(-direction, cls.Y))

        p = np.zeros( (8, 4, 4), dtype=cls.DTYPE )
        n = len(p)
        p[:,0,:3] = radius*direction
        p[:,0, 3] = cls.TIME*(1. + np.arange(n))
        p[:,1,:3] = -direction
        p[:,1, 3] = cls.WEIGHT
        p[:,2,:3] = polarization
        p[:,2, 3] = cls.WAVELENGTH
        return p

    @classmethod
    def Axes(cls):
        """

               Z   -X
               |  .
               | .
               |.
       -Y......O------ Y  1
              /.
             / .
            /  .
           X   -Z
          0
        """
        v = np.zeros((6, 3), dtype=cls.DTYPE)
        v[0] = [1,0,0]
        v[1] = [0,1,0]
        v[2] = [0,0,1]
        v[3] = [-1,0,0]
        v[4] = [0,-1,0]
        v[5] = [0,0,-1]
        return v


    @classmethod
    def GenerateAxes(cls):
        direction = cls.Axes()
        polarization = np.zeros((6, 3), dtype=cls.DTYPE)
        polarization[:-1] = direction[1:]
        polarization[-1] = direction[0]

        p = np.zeros( (6, 4, 4), dtype=cls.DTYPE )
        n = len(p)
        p[:,0,:3] = cls.POSITION
        p[:,0, 3] = cls.TIME*(1. + np.arange(n))
        p[:,1,:3] = direction
        p[:,1, 3] = cls.WEIGHT
        p[:,2,:3] = polarization
        p[:,2, 3] = cls.WAVELENGTH
        return p


    @classmethod
    def Parallelize1D(cls, p, r, offset=True):
        """
        :param p: photons array of shape (num, 4, 4)
        :param r: repetition number
        :return pp:  photons array of shape (r*num, 4, 4)

        See parallel_input_photons.py for tests/plotting
        """
        if r == 0:
            return p

        o = len(p)          # original number of photons
        pp = np.repeat(p, r, axis=0).reshape(-1,r,4,4)  # shape (8,10,4,4)

        if offset:
            for i in range(o):
                dir = p[i,1,:3]
                pol = p[i,2,:3]   # original polarization, a transverse offset direction vector
                oth = np.cross(pol, dir)
                for j in range(r):
                    jj = j - r//2
                    pp[i,j,0,:3] = jj*oth

        return pp.reshape(-1,4,4)


    @classmethod
    def Parallelize2D(cls, p, rr, offset=True):
        """
        :param p: original photons, shaped (o,4,4)
        :param [rj,rk]: 2d repeat dimension list
        :return pp: shaped (o*rj*rk,4,4)

        See parallel_input_photons.py for tests/plotting
        """
        if len(rr) != 2:
            return p

        rj, rk = rr[0],rr[1]
        o = len(p)          # original number of photons
        pp = np.repeat(p, rj*rk, axis=0).reshape(-1,rj,rk,4,4)

        if offset:
            for i in range(o):
                dir = p[i,1,:3]
                pol = p[i,2,:3]           # original polarization, a transverse offset direction vector
                oth = np.cross(pol, dir)  # other transverse direction perpendicular to pol
                for j in range(rj):
                    jj = j - rj//2
                    for k in range(rk):
                        kk = k - rk//2
                        pp[i,j,k,0,:3] = jj*oth + kk*pol

        return pp.reshape(-1,4,4)

    @classmethod
    def GenerateXZ(cls, n, mom, x0lim=[-49.,49.],y0=0.,z0=-99.  ):
        """

               +-----------------------------------+
               |                                   |
               |                                   |
               |                                   |
               |                                   |
               |                                   |
               |                                   |
               |                                   |
               |                                   |
               |                                   |
               |                                   |
               |                                   |         Z
               |          ^ ^ ^ ^ ^ ^ ^            |         |  Y
               |          | | | | | | |            |         | /
               |          . . . . . . .            |         |/
               +-----------------------------------+         +---> X
             -100        -49    0     49          100


        """
        assert len(x0lim) == 2
        if n < 0:
            n = -n
            xx = sample_linspace(n, x0lim[0], x0lim[1] )
        else:
            xx = sample_linear(n, x0lim[0], x0lim[1] )

        pos = np.zeros((n,3), dtype=cls.DTYPE )
        pos[:,0] = xx
        pos[:,1] = y0
        pos[:,2] = z0

        p = np.zeros( (n, 4, 4), dtype=cls.DTYPE )
        p[:,0,:3] = pos
        p[:,0, 3] = cls.TIME
        p[:,1,:3] = mom           # mom : Up:self.Z or Down:-self.Z
        p[:,1, 3] = cls.WEIGHT
        p[:,2,:3] = cls.Y         # pol
        p[:,2, 3] = cls.WAVELENGTH
        return p

    @classmethod
    def GenerateRandomSpherical(cls, n):
        """
        :param n: number of photons to generate

        spherical distribs not carefully checked

        The start position is offset by the direction vector for easy identification purposes
        so that means the rays will start on a virtual unit sphere and travel radially
        outwards from there.

        """
        spherical = sample_trig(n).T
        assert spherical.shape == (n,3)

        direction = spherical
        polarization = vnorm(np.cross(direction,cls.Y))

        p = np.zeros( (n, 4, 4), dtype=cls.DTYPE )
        p[:,0,:3] = cls.POSITION + direction
        p[:,0, 3] = cls.TIME*(1. + np.arange(n))
        p[:,1,:3] = direction
        p[:,1, 3] = cls.WEIGHT
        p[:,2,:3] = polarization
        p[:,2, 3] = cls.WAVELENGTH
        return p

    @classmethod
    def GenerateRandomDisc(cls, n):
        spherical = sample_trig(n).T
        disc_offset = spherical.copy()
        disc_offset[:,0] *= 100.
        disc_offset[:,1] *= 100.
        disc_offset[:,2] = 0.

        p = np.zeros( (n, 4, 4), dtype=cls.DTYPE )
        p[:,0,:3] = cls.POSITION + disc_offset
        p[:,0, 3] = cls.TIME*(1. + np.arange(n))
        p[:,1,:3] = cls.Z
        p[:,1, 3] = cls.WEIGHT
        p[:,2,:3] = cls.X
        p[:,2, 3] = cls.WAVELENGTH
        return p

    @classmethod
    def GenerateUniformDisc(cls, n, radius=100.):
        offset = sample_disc(n, dtype=cls.DTYPE)
        offset[:,0] *= radius
        offset[:,1] *= radius
        p = np.zeros( (n, 4, 4), dtype=cls.DTYPE )
        p[:,0,:3] = cls.POSITION + offset
        p[:,0, 3] = 0.1
        p[:,1,:3] = -cls.Z
        p[:,1, 3] = cls.WEIGHT
        p[:,2,:3] = cls.X
        p[:,2, 3] = cls.WAVELENGTH
        return p

    @classmethod
    def GenerateGridXY(cls, n, X=100., Z=1000. ):
        sn = int(np.sqrt(n))
        offset = xy_grid_coordinates(nx=sn, ny=sn, sx=X, sy=X )
        offset[:,2] = Z

        p = np.zeros( (n, 4, 4), dtype=cls.DTYPE )
        p[:,0,:3] = cls.POSITION + offset
        p[:,0, 3] = 0.1
        p[:,1,:3] = -cls.Z
        p[:,1, 3] = cls.WEIGHT
        p[:,2,:3] = cls.X
        p[:,2, 3] = cls.WAVELENGTH
        return p


    @classmethod
    def CheckTransverse(cls, direction, polarization, epsilon):
        # check elements should all be very close to zero
        check1 = np.einsum('ij,ij->i',direction,polarization)
        check2 = (direction*polarization).sum(axis=1)
        assert np.abs(check1).min() < epsilon
        assert np.abs(check2).min() < epsilon

    @classmethod
    def Check(cls, p):
        direction = p[:,1,:3]
        polarization = p[:,2,:3]
        cls.CheckTransverse( direction, polarization, 1e-6 )

    CC = "CubeCorners"
    ICC = "InwardsCubeCorners"
    RS = "RandomSpherical"
    RD = "RandomDisc"
    UD = "UniformDisc"
    UXZ = "UpXZ"
    DXZ = "DownXZ"
    RAINXZ = "RainXZ"
    GRIDXY = "GridXY"
    Z230 = "_Z230"
    Z195 = "_Z195"
    Z1000 = "_Z1000"
    X700 = "_X700"
    X1000 = "_X1000"
    R500 = "_R500"

    NAMES = [CC, CC+"10x10", CC+"100", CC+"100x100", RS+"10", RS+"100", ICC+"17699", ICC+"1", RD+"10", RD+"100", UXZ+"1000", DXZ+"1000" ]
    NAMES += [RAINXZ+"100", RAINXZ+"1000", RAINXZ+"100k", RAINXZ+"10k" ]
    NAMES += [RAINXZ+Z230+"_100", RAINXZ+Z230+"_1000", RAINXZ+Z230+"_100k", RAINXZ+Z230+"_10k", RAINXZ+Z230+"_1M" ]
    NAMES += [RAINXZ+Z195+"_100", RAINXZ+Z195+"_1000", RAINXZ+Z195+"_100k", RAINXZ+Z195+"_10k" ]
    NAMES += [RAINXZ+Z230+X700+"_100", RAINXZ+Z230+X700+"_1000", RAINXZ+Z230+X700+"_10k" ]
    NAMES += [UD+R500+"_10k"]
    NAMES += [GRIDXY+X700+Z230+"_10k", GRIDXY+X1000+Z1000+"_40k"    ]

    def generate(self, name, args):
        if args.seed > -1:
            log.info("seeding with %d " % args.seed)
            np.random.seed(args.seed)

        meta = dict(seed=args.seed, name=name, creator="input_photons.py")
        log.info("generate %s " % name)
        if name.startswith(self.RS):
            num = int(name[len(self.RS):])
            p = self.GenerateRandomSpherical(num)
        elif name.startswith(self.UXZ) or name.startswith(self.DXZ):
            num = None
            mom = None
            z0 = None
            xl = None
            if name.startswith(self.UXZ):
                num = int(name[len(self.UXZ):])  # extract num following prefix
                mom = self.Z
                z0 = -99.
                xl = 49.
            elif name.startswith(self.DXZ):
                num = int(name[len(self.DXZ):])
                mom = -self.Z
                z0 = 999.
                xl = 200.
            else:
                pass

            p = self.GenerateXZ(num, mom, x0lim=[-xl,xl],y0=0,z0=z0 )
        elif name.startswith(self.RAINXZ):
            d = parsetail(name, prefix=self.RAINXZ)
            z0 = 1000. if d['Z'] is None else d['Z']
            xl = 250.  if d['X'] is None else d['X']
            num = d['N']
            mom = -self.Z
            p = self.GenerateXZ(-num, mom, x0lim=[-xl,xl],y0=0,z0=z0 )
        elif name.startswith(self.UD):
            d = parsetail(name, prefix=self.UD)
            p = self.GenerateUniformDisc(d["N"], radius=d["R"])
        elif name.startswith(self.GRIDXY):
            d = parsetail(name, prefix=self.GRIDXY)
            p = self.GenerateGridXY(d["N"], X=d["X"], Z=d["Z"])
        elif name.startswith(self.RD):
            num = int(name[len(self.RD):])
            p = self.GenerateRandomDisc(num)
        elif name == self.CC:
            p = self.GenerateCubeCorners()
        elif name.startswith(self.CC):
            o = self.OutwardsCubeCorners()
            sdim = name[len(self.CC):]
            if sdim.find("x") > -1:
                rr = list(map(int, sdim.split("x")))
                p = self.Parallelize2D(o, rr)
                meta["Parallelize2D_rr"] = rr
            else:
                r = int(sdim)
                p = self.Parallelize1D(o, r)
                meta["Parallelize1D_r"] = r

        elif name.startswith(self.ICC):
            sradius = name[len(self.ICC):]
            radius = float(sradius)
            p = self.InwardsCubeCorners(radius)
        else:
            log.fatal("no generate method for name %s " %  name)
            assert 0

        self.Check(p)
        meta.update(num=len(p))
        return p, meta


    def __init__(self, name, args):
        InputPhotons.DTYPE = args.dtype
        npy_path = self.Path(name, ext=".npy")
        json_path = self.Path(name, ext=".json")
        generated = False
        if os.path.exists(npy_path) and os.path.exists(json_path):
            log.info("load %s from %s %s " % (name, npy_path, json_path))
            p = np.load(npy_path)
            meta = json.load(open(json_path,"r"))
        else:
            p, meta = self.generate(name, args)
            generated = True

        self.p = p
        self.meta = meta
        if generated:
            self.save()

    name = property(lambda self:self.meta.get("name", "no-name"))

    def save(self):
        npy_path = self.Path(self.name, ext=".npy")
        json_path = self.Path(self.name, ext=".json")
        fold = os.path.dirname(npy_path)
        if not os.path.isdir(fold):
            log.info("creating folder %s " % fold)
            os.makedirs(fold)

        log.info("save %s to %s and %s " % (self.name, npy_path, json_path))
        np.save(npy_path, self.p)
        json.dump(self.meta, open(json_path,"w"))

    def __repr__(self):
        return "\n".join([str(self.meta),".p %s" % self.p.dtype, str(self.p.reshape(-1,16))])


def parsetail(name, prefix=""):
    """
    """
    d = { 'N':None,'X':None, 'Y':None, 'Z':None, 'R':None }
    assert name.startswith(prefix)
    tail = name[len(prefix):]
    elem = np.array(list(filter(None, tail.split("_"))) if tail.find("_") > -1 else [tail])
    #print(" name:%s. tail:%s. elem:%s." % (name, tail, str(elem)) )

    if len(elem) > 1:
        for e in elem[:-1]:
           if e[0] in 'XYZR':
               d[e[0]] = int(e[1:])

    if elem[-1].endswith("k"):
        num = int(elem[-1][:-1])*1000
    elif elem[-1].endswith("M"):
        num = int(elem[-1][:-1])*1000000
    else:
        num = int(elem[-1])

    assert not num is None
    d['N'] = num
    return d


def test_parsetail():
    assert parsetail("RainXZ1k", prefix="RainXZ") == dict(N=1000,X=None,Y=None,Z=None,R=None)
    assert parsetail("RainXZ10k", prefix="RainXZ") == dict(N=10000,X=None,Y=None,Z=None,R=None)
    assert parsetail("RainXZ1M", prefix="RainXZ") == dict(N=1000000,X=None,Y=None,Z=None,R=None)

    #pt = parsetail("RainXZ_Z230_1k", prefix="RainXZ")
    #print(pt)

    assert parsetail("RainXZ_Z230_1k", prefix="RainXZ") == dict(N=1000,X=None,Y=None,Z=230,R=None)
    assert parsetail("RainXZ_Z230_10k", prefix="RainXZ") == dict(N=10000,X=None,Y=None,Z=230,R=None)
    assert parsetail("RainXZ_Z230_1M", prefix="RainXZ") == dict(N=1000000,X=None,Y=None,Z=230,R=None)

    assert parsetail("RainXZ_Z230_X250_1k", prefix="RainXZ") == dict(N=1000,X=250,Y=None,Z=230,R=None)
    assert parsetail("RainXZ_Z230_X500_10k", prefix="RainXZ") == dict(N=10000,X=500,Y=None,Z=230,R=None)
    assert parsetail("RainXZ_Z230_X1000_1M", prefix="RainXZ") == dict(N=1000000,X=1000,Y=None,Z=230,R=None)


def test_InwardsCubeCorners17699(ip):
    sel = "InwardsCubeCorners17699"
    ip0 = ip[sel]
    p = ip0.p
    m = ip0.meta
    r = np.sqrt(np.sum(p[:,0,:3]*p[:,0,:3], axis=1 ))  # radii of start positions


def _dtype_arg(s):
    if s == 'float32':
        return np.float32
    elif s == 'float64':
        return np.float64
    else:
        raise argparse.ArgumentTypeError("dtype must be 'float32' or 'float64'")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument( "names", nargs="*", default=InputPhotons.NAMES, help="Name stem of InputPhotons array, default %(default)s" )
    parser.add_argument( "--level", default='info', help="logging level, default %(default)s" )
    parser.add_argument( "--seed", type=int, default=0, help="seed for np.random.seed() or -1 for non-reproducible generation, default %(default)s" )
    parser.add_argument( "--dtype", type=_dtype_arg, default=np.float64, help="type of generated values" )

    args = parser.parse_args()

    fmt = '[%(asctime)s] p%(process)s {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s'
    logging.basicConfig(level=getattr(logging, args.level.upper()), format=fmt)

    ip = odict()
    for name in args.names:
        ip[name] = InputPhotons(name, args)
        print(ip[name])


if __name__ == '__main__':
    if "PARSETAIL" in os.environ:
        test_parsetail()
    else:
        ip = main()
        print("ip odict contains all InputPhotons instances ")
        print(ip.keys())
