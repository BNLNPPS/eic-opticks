#!/usr/bin/env python

import logging
log = logging.getLogger(__name__)

from opticks.ana.npmeta import NPMeta
from opticks.ana.axes import * 
from opticks.ana.pvplt import mpplt_focus, SIZE

import os, numpy as np
eary_ = lambda ekey, edef:np.array( list(map(float, os.environ.get(ekey,edef).split(","))) )

from opticks.ana.eget import efloat_


class sframe(object):
    @classmethod
    def Load(cls, fold=None, name="sframe.npy"):
        if fold is None:
            fold = os.environ.get("FOLD", "")
        pass
        if fold.endswith(name):
            path = fold
        else:   
            path = os.path.join(fold, name)
        pass
        return cls(path)


    @classmethod
    def DetermineAxes(cls, nx, ny, nz):
        """
        :param nx:
        :param nx:
        :param nx:

        With planar axes the order is arranged to make the longer axis the first horizontal one 
        followed by the shorter axis as the vertical one.  

            +------+
            |      |  nz     ->  ( Y, Z )    ny_over_nz > 1 
            +------+
               ny

            +------+
            |      |  ny     ->  ( Z, Y )    ny_over_nz < 1 
            +------+
               nz

        """
        if nx == 0 and ny > 0 and nz > 0:
            ny_over_nz = float(ny)/float(nz)
            axes = (Y,Z) if ny_over_nz > 1 else (Z,Y)
        elif nx > 0 and ny == 0 and nz > 0:
            nx_over_nz = float(nx)/float(nz)
            axes = (X,Z) if nx_over_nz > 1 else (Z,X)
        elif nx > 0 and ny > 0 and nz == 0:
            nx_over_ny = float(nx)/float(ny)
            axes = (X,Y) if nx_over_ny > 1 else (Y,X)
        else:
            axes = (X,Y,Z)
        pass
        return axes


    @classmethod
    def CombineFrame(cls, framelist_):
        """
        HMM: probably should do more consistency checks between all the frames 
        """
        framelist = list(filter(None, framelist_))
        axes = None
        for f in framelist:
            if axes is None:
                axes = f.axes
            else:
                assert axes == f.axes 
            pass
        pass
        return framelist[0] if len(framelist) > 0 else None
 

    def __init__(self, path, clear_identity=True ):
        """
        Whether clear_identity makes any material difference depends on the identity values. 
        But it should be done anyhow.  For some identity values they will appear as nan in float.
        """
        if path is None:
            return
        pass

        metapath = path.replace(".npy", "_meta.txt")
        if os.path.exists(metapath):
            meta = NPMeta.Load(metapath)
        else:
            meta = None
        pass   
        self.meta = meta 

        a = np.load(path)
        i = a.view(np.int32)

        self.shape = a.shape  # for fold to treat like np.array
        self.path = path 
        self.a = a
        self.i = i

        ce = a[0,0]
        ix0,ix1,iy0,iy1 = i[0,1,:4]        # q1.i.xyzw
        iz0,iz1,num_photon = i[0,2,:3]     # q2.i.xyz  
        gridscale = a[0,2,3]               # q2.f.w


        midx, mord, gord = i[0,3,:3]       # q3.i.xyz
        inst = i[0,3,3]     # q3.i.w



        self.midx = midx
        self.mord = mord
        self.gord = gord
        self.inst = inst
         
        ## replacing ana/gridspec.py:GridSpec
         


        grid = "".join(["ix0 %(ix0)4d ix1 %(ix1)4d ",
                        "iy0 %(iy0)4d iy1 %(iy1)4d ",
                        "iz0 %(iz0)4d iz1 %(iz1)4d ",
                        "num_photon %(num_photon)4d gridscale %(gridscale)10.4f"]) % locals() 






        self.grid = grid
        self.ce = ce 
        self.ix0 = ix0
        self.ix1 = ix1
        self.iy0 = iy0
        self.iy1 = iy1
        self.iz0 = iz0
        self.iz1 = iz1
        self.num_photon = num_photon
        self.gridscale = gridscale



        target = "midx %(midx)6d mord %(mord)6d gord %(gord)6d       inst %(inst)7d   " % locals() 
        self.target = target 


        ins_idx_1 = i[1,0,3] - 1   # q0.u.w-1  see sqat4.h qat4::getIdentity qat4::setIdentity
        gas_idx_1 = i[1,1,3] - 1   # q1.u.w-1 
        ias_idx_1 = i[1,2,3] - 1   # q2.u.w-1 

        ins_idx_2 = i[2,0,3] - 1   # q0.u.w-1  see sqat4.h qat4::getIdentity qat4::setIdentity
        gas_idx_2 = i[2,1,3] - 1   # q1.u.w-1 
        ias_idx_2 = i[2,2,3] - 1   # q2.u.w-1 

        assert ins_idx_1 == ins_idx_2 
        assert gas_idx_1 == gas_idx_2 
        assert ias_idx_1 == ias_idx_2 

        ins_idx = ins_idx_1
        gas_idx = gas_idx_1
        ias_idx = ias_idx_1
        qat4id = "ins_idx %(ins_idx)6d gas_idx %(gas_idx)4d %(ias_idx)4d " % locals()

        self.ins_idx = ins_idx
        self.gas_idx = gas_idx
        self.ias_idx = ias_idx
        self.qat4id = qat4id

        m2w = a[1].copy()
        w2m = a[2].copy()

        if clear_identity:
            m2w[0,3] = 0. 
            m2w[1,3] = 0. 
            m2w[2,3] = 0. 
            m2w[3,3] = 1. 

            w2m[0,3] = 0. 
            w2m[1,3] = 0. 
            w2m[2,3] = 0. 
            w2m[3,3] = 1. 
        pass

        self.m2w = m2w
        self.w2m = w2m
        self.id = np.dot( m2w, w2m )  


        ins = i[3,0,0]   # aux.q0.i.x  
        gas = i[3,0,1]
        ias = i[3,0,2]

        propagate_epsilon = a[3,1,0]   # aux.q1.f.x 


        ins_gas_ias = " ins %(ins)6d gas %(gas)4d ias %(ias)4d " % locals()

        self.ins = ins
        self.gas = gas
        self.ias = ias
        self.ins_gas_ias = ins_gas_ias
        self.propagate_epsilon = propagate_epsilon 

        self.init_grid()
        self.init_view()


    def init_grid(self):
        """
        replacing ana/gridspec.py 
        """
        gord = self.gord  
        ix0 = self.ix0 
        ix1 = self.ix1 
        iy0 = self.iy0 
        iy1 = self.iy1 
        iz0 = self.iz0 
        iz1 = self.iz1 
        gridscale = self.gridscale
        ce = self.ce
        extent = ce[3]
        s_extent = "e:%7.3f" % extent 

        nx = (ix1 - ix0)//2   
        ny = (iy1 - iy0)//2
        nz = (iz1 - iz0)//2
        coords = "RTP" if gord == -3 else "XYZ"   ## NB RTP IS CORRECT ORDERING radiusUnitVec:thetaUnitVec:phiUnitVec
        axes = self.DetermineAxes(nx, ny, nz)
        other_axis = Axes.OtherAxis(axes)
        planar = len(axes) == 2 

        bbox = np.zeros( (2,3), dtype=np.float32 )
        bbox[0] = list(map(float,[ix0,iy0,iz0]))
        bbox[1] = list(map(float,[ix1,iy1,iz1]))
        bbox *= gridscale*ce[3]


        if planar:
            H, V = axes
            axlabels =  coords[H], coords[V]
            s_bbox = "bb "
            s_bbox += "  %7.4f %7.4f " % tuple(bbox[0,axes])    
            s_bbox += "  %7.4f %7.4f " % tuple(bbox[1,axes])    
        else:
            H, V, D = axes
            axlabels =  coords[H], coords[V], coords[D]
            s_bbox = "bb %7.4f %7.4f %7.4f    %7.4f %7.4f %7.4f " % tuple(bbox.ravel())    
        pass 


        self.nx = nx
        self.ny = ny
        self.nz = nz
        self.coords = coords
        self.axes = axes 
        self.other_axis = other_axis
        self.axlabels = axlabels 

        self.bbox = bbox  # without ce_offset 
        self.s_bbox = s_bbox 

        self.extent = extent 
        self.s_extent = s_extent 
   


    def init_view(self):
        """
        HMM: input EYE is ignored for planar 
        """
        ce = self.ce 
        axes = self.axes
        coords = self.coords
        planar = len(axes) == 2 

        # below default from envvars are overridden for planar data
        eye = eary_("EYE","1.,1.,1.")
        up  = eary_("UP","0.,0.,1.")
        off  = eary_("OFF","0.,0.,1.")
        look = eary_("LOOK","0.,0.,0.")
        #look = ce[:3]

        EYES = efloat_("EYES", "6.")   # TODO: why 6 ? how to control FOV to gain more control of this
        extent = ce[3]

        if planar:
            H, V = axes
            up  = Axes.Up(H,V)
            off = Axes.Off(H,V)
            eye = look + extent*off*EYES
        else:
            H, V, D = axes
            axlabels =  coords[H], coords[V], coords[D]

            up = XYZ.up
            off = XYZ.off
            ## hmm in 3D case makes less sense : better to just use the input EYE

            eye = ce[3]*eye*EYES 
        pass 

        self.look = look
        self.up = up
        self.off = off
        self.eye = eye 
        self.thirdline = self.s_bbox + self.s_extent


    def pv_compose(self, pl, local=True):
        """
        #pl.view_xz()   ## TODO: see if view_xz is doing anything when subsequently set_focus/viewup/position 
        """

        RESET = "RESET" in os.environ 
        PARA = "PARA" in os.environ 
        ZOOM = efloat_("ZOOM", "1.")
        PVGRID = "PVGRID" in os.environ

        if PARA:
            pl.camera.ParallelProjectionOn()
        pass 

        look = self.look if local else self.ce[:3]   ## HMM: same ?
        eye = look + self.off
        up = self.up

        ## for reset=True to succeed to auto-set the view, must do this after add_points etc.. 
        
        #eye = look + self.off

        look = self.look 
        eye = self.eye
        up = self.up

        log.info("frame.pv_compose look:%s eye: %s up:%s  PARA:%s RESET:%d ZOOM:%s  " % (str(look), str(eye), str(up), RESET, PARA, ZOOM ))

        pl.set_focus(    look )
        pl.set_viewup(   up )
        pl.set_position( eye, reset=RESET )   ## for reset=True to succeed to auto-set the view, must do this after add_points etc.. 
        pl.camera.Zoom(ZOOM)

        if PVGRID:
            pl.show_grid()
        pass



    def __repr__(self):

        l_ = lambda k,v:"%-12s : %s" % (k, v) 

        return "\n".join(
                  [ 
                    l_("sframe",""),
                    l_("path",self.path), 
                    l_("meta",repr(self.meta)), 
                    l_("ce", repr(self.ce)), 
                    l_("grid", self.grid), 
                    l_("bbox", repr(self.bbox)), 
                    l_("target", self.target), 
                    l_("qat4id", self.qat4id), 
                    l_("m2w",""), repr(self.m2w), "", 
                    l_("w2m",""), repr(self.w2m), "", 
                    l_("id",""),  repr(self.id) ,
                    l_("ins_gas_ias",self.ins_gas_ias)  
                   ])


    @classmethod
    def FakeXZ(cls, e=10):
        path = None
        fr = cls(path)
        fr.axes = X,Z
        fr.axlabels = "X","Z"
        fr.bbox = np.array( [[-e,-e,-e],[e,e,e]] )
        return fr 


    def mp_subplots(self, mp):
        pass
        H,V = self.axes 
        _H,_V = self.axlabels

        xlim = self.bbox[:,H]  
        ylim = self.bbox[:,V]  

        log.info("mp_subplots xlim %s ylim %s " % (str(xlim), str(ylim)))  
        #xlim, ylim = mpplt_focus(xlim, ylim)

        topline = os.environ.get("TOPLINE", "sframe.py:mp_subplots:TOPLINE")
        botline = os.environ.get("BOTLINE", "sframe.py:mp_subplots:BOTLINE")
        thirdline = getattr(self, "thirdline", "thirdline" )

        title = [topline, botline, thirdline]

        fig, ax = mp.subplots(figsize=SIZE/100.)
        fig.suptitle("\n".join(title))
    
        ax.set_aspect('equal')
        ax.set_xlim(xlim)
        ax.set_ylim(ylim)
        ax.set_xlabel(_H)
        ax.set_ylabel(_V)

        self.fig = fig
        self.ax = ax

        return fig, ax

    def mp_scatter(self, pos, **kwa):
        assert pos.ndim == 2 and pos.shape[1] == 3
        H,V = self.axes 
        self.ax.scatter( pos[:,H], pos[:,V], **kwa)

    def mp_arrow(self, pos, mom, **kwa):
        assert pos.ndim == 2 and pos.shape[1] == 3
        assert mom.ndim == 2 and mom.shape[1] == 3
        assert len(pos) == len(mom)
        H,V = self.axes 

        for i in range(len(pos)):
            self.ax.arrow( pos[i,H], pos[i,V], mom[i,H], mom[i,V] )
        pass


    def mp_legend(self):
        NOLEGEND = "NOLEGEND" in os.environ
        LSIZ = float(os.environ.get("LSIZ",50))
        LOC = os.environ.get("LOC", "upper right")
        log.info("mp_legend LSIZ %s LOC %s NOLEGEND %s " % (LSIZ,LOC,NOLEGEND) )
        if not NOLEGEND:
            lgnd = self.ax.legend(loc=LOC)
            for handle in lgnd.legendHandles:
                handle.set_sizes([LSIZ])
            pass
        pass


 

if __name__ == '__main__':

    fr = sframe.Load()
    print(fr)

     

