#!/usr/bin/env python
"""
U4PMTFastSimTest.py
=====================

TODO: restrict to 2D so can create some more informative plots 



::

    In [10]: np.unique(wai, return_counts=True)
    Out[10]: (array([1, 2]), array([32628, 67372]))

    In [2]: np.unique( np.logical_and( wai == 1, trg == 0 ), return_counts=True )
    Out[2]: (array([False,  True]), array([67436, 32564]))

    In [3]: np.unique( np.logical_and( wai == 1, trg == 1 ), return_counts=True )
    Out[3]: (array([False,  True]), array([99936,    64]))

    In [4]: np.unique( np.logical_and( wai == 2, trg == 0 ), return_counts=True )
    Out[4]: (array([False,  True]), array([83308, 16692]))

    In [5]: np.unique( np.logical_and( wai == 2, trg == 1 ), return_counts=True )
    Out[5]: (array([False,  True]), array([49320, 50680]))




"""
import os, numpy as np
from opticks.ana.fold import Fold
NOGUI = "NOGUI" in os.environ
MODE = int(os.environ.get("MODE", 0))
if not NOGUI:
    from opticks.ana.pvplt import * 
pass

if __name__ == '__main__':
    t = Fold.Load(symbol="t")
    print(repr(t))

    fs = t.SFastSim_Debug   
    kInfinity = 9.000e+99    

    pos = fs[:,0,:3]
    tim = fs[:,0,3]

    mom = fs[:,1,:3]
    ds1 = fs[:,1,3]

    pol = fs[:,2,:3]
    ds2 = fs[:,2,3]

    trg = fs[:,3,0].astype(np.int64)
    wai = fs[:,3,1].astype(np.int64)
    c   = fs[:,3,2]
    d   = fs[:,3,3]

    ms1 = np.where(ds1 == kInfinity )[0]
    ht1 = np.where(ds1 != kInfinity )[0]
    ms2 = np.where(ds2 == kInfinity )[0]
    ht2 = np.where(ds2 != kInfinity )[0]
    
    #  enum EWhereAmI { OutOfRegion, kInGlass, kInVacuum };
    glass      = wai == 1 
    vacuum     = wai == 2 
    trg_no     = trg == 0 
    trg_yes    = trg == 1 
    glass_no   = np.logical_and( wai == 1, trg == 0 )
    glass_yes  = np.logical_and( wai == 1, trg == 1 ) 
    vacuum_no  = np.logical_and( wai == 2, trg == 0 )
    vacuum_yes = np.logical_and( wai == 2, trg == 1 )  

    selnames = "glass vacuum trg_no trg_yes glass_no glass_yes vacuum_no vacuum_yes".split()
    for selname in selnames:
        sel = locals()[selname]
        print(" %20s : %s " % (selname, np.count_nonzero(sel) ))
    pass

    #specs = "blue:trg_no:lines cyan:trg_yes:lines"
    #specs = "blue:glass_no:lines cyan:glass_yes:lines"
    specs = "blue:glass_no:lines cyan:glass_yes:lines red:vacuum_yes:lines yellow:vacuum_no:lines"
    label = "U4PMTFastSimTest.py:SFastSim_Debug " + specs

    if NOGUI:
        print("not plotting as NOGUI in environ")
    else:
        pl = pvplt_plotter(label)
        os.environ["EYE"] = "0,100,165"
        os.environ["LOOK"] = "0,0,165"
        pvplt_viewpoint(pl)

        for spec in specs.split():
            color,selname,stylename = spec.split(":")    
            sel = locals()[selname]
            if stylename == "lines":
                pvplt_lines(pl, pos[sel], mom[sel], color=color, factor=10 )
            elif stylename == "points":
                pl.add_points( pos[sel], color=color )
            else:
                pass
            pass
        pass
        pl.show()
    pass
pass
