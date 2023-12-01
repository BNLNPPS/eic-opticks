#!/usr/bin/env python
"""
sreport.py
======================

::

    ~/opticks/sysrap/tests/sreport.sh ana

TODO: incorporate sprof_fold_report.py into this

TODO: linear fit like g4ok/tests/G4OpticksProfilePlot.py

"""

import os, numpy as np
from opticks.ana.fold import Fold
from opticks.ana.npmeta import NPMeta

MODE =  int(os.environ.get("MODE", "2"))
PICK =  os.environ.get("PICK", "AB")
TLIM =  np.array(list(map(int,os.environ.get("TLIM", "0,0").split(","))),dtype=np.int32)


if MODE != 0:
    from opticks.ana.pvplt import * 
pass

labels_ = lambda l:l.view("|S%d" % l.shape[1])[:,0]  
tv_     = lambda a:a.view("datetime64[us]")  

# https://matplotlib.org/stable/gallery/color/named_colors.html
palette = ["red","green", "blue", 
           "cyan", "magenta", "yellow", 
           "tab:orange", "tab:pink", "tab:olive",
           "tab:purple", "tab:grey", "tab:cyan"
           ]


def make_title(meta, method, symbol):
    base = meta.base.replace("/data/blyth/opticks/GEOM/", "")
    smry = meta.smry("GPUMeta,prefix,creator")
    sfmt = meta.smry("stampFmt") 
    titl = "%s:%s %s " % (symbol,method, sfmt) 
    title = " ".join([titl,base,smry]) 
    return title

smry__ = lambda _:NPMeta.Summarize(_)
smry_ = lambda _:list(map(smry__, _))


class Subprofile(object):
    """
    Why VM is so large with CUDA

    * https://forums.developer.nvidia.com/t/high-virtual-memory-consumption-on-linux-for-cuda-programs-is-it-possible-to-avoid-it/67706/4

    """
    FONTSIZE = 20 
    XLABEL = "Time from 1st sprof.h stamp (seconds)"
    YLABEL = "VM and/or RSS sprof.h memory (GB) "

    @classmethod
    def Time_VM_RS(cls, f):
        subprofile = f.subprofile
        assert len(subprofile.shape) == 3
        t0 = subprofile[0,0,0] 
        sp = subprofile.reshape(-1,3)   
        tp = (sp[:,0] - t0)/1e6   ## seconds from start
        vm = sp[:,1]/1e6          ## GB
        rs = sp[:,2]/1e6          ## GB
        return tp, vm, rs 

    @classmethod
    def Title(cls, f):
        meta = f.subprofile_meta
        base = meta.base.replace("/data/blyth/opticks/GEOM/", "")
        smry = meta.smry("GPUMeta,prefix,creator")
        sfmt = meta.smry("stampFmt") 
        title = "\n".join([base,smry,sfmt]) 
        return title





class Subprofile_ALL(object):
    def __init__(self, base, symbol="fold.subprofile"):
        title = ["Subprofile_ALL "]  
        for sym in base.ff:
            f = getattr(base, sym)
            title.append(Subprofile.Title(f))
        pass
        label = "\n".join(title)

        fontsize = Subprofile.FONTSIZE
        if MODE == 2:
            fig, axs = mpplt_plotter(nrows=1, ncols=1, label=label, equal=False)
            ax = axs[0]
            for sym in base.ff:
                f = getattr(base, sym)
               
                tp,vm,rs = Subprofile.Time_VM_RS(f)

                if "VM" in os.environ:
                    ax.scatter( tp, vm, label="%s : VM(GB) vs time(s)" % sym.upper())
                    ax.plot(    tp, vm )
                pass
                ax.scatter( tp, rs, label="%s : RSS(GB) vs time(s)" % sym.upper())
                ax.plot( tp, rs )
            pass
            ax.set_xlabel(Subprofile.XLABEL, fontsize=Subprofile.FONTSIZE )
            ax.set_ylabel(Subprofile.YLABEL, fontsize=Subprofile.FONTSIZE )
            ax.legend()
            fig.show()


class Runprofile_ALL(object):
    def __init__(self, rp, symbol="fold.runprofile"):
        label = "Runprofile_ALL " 
        fontsize = Subprofile.FONTSIZE
        if MODE == 2:
            fig, axs = mpplt_plotter(nrows=1, ncols=1, label=label, equal=False)
            ax = axs[0]

            tp = (rp[:,0] - rp[0,0])/1e6
            vm = rp[:,1]/1e6
            rs = rp[:,2]/1e6 

            if "VM" in os.environ:
                ax.scatter( tp, vm, label="%s : VM(GB) vs time(s)" % symbol)
                ax.plot(    tp, vm )
            pass
            ax.scatter( tp, rs, label="%s : RSS(GB) vs time(s)" % symbol )
            ax.plot( tp, rs )
            pass
            ax.set_xlabel(Subprofile.XLABEL, fontsize=Subprofile.FONTSIZE )
            ax.set_ylabel(Subprofile.YLABEL, fontsize=Subprofile.FONTSIZE )

            yl = ax.get_ylim()
            ax.vlines( tp[::4], yl[0], yl[1] ) 
            ax.legend()
            fig.show()
        pass  


class Subprofile_ONE(object):
    def __init__(self, f, symbol="fold.subprofile.a"):

        meta = f.subprofile_meta
        names = f.subprofile_names
        labels = f.subprofile_labels
        title = make_title(meta, method="Subprofile_ONE", symbol=symbol)

        tp,vm,rs = Subprofile.Time_VM_RS(f)

        if MODE == 2:
            fig, axs = mpplt_plotter(nrows=1, ncols=1, label=title, equal=False)
            ax = axs[0]
            if "VM" in os.environ:
                ax.scatter( tp, vm, label="VM(GB) vs time(s)")
                ax.plot( tp, vm )
            pass
            ax.scatter( tp, rs, label="RSS(GB) vs time(s)")
            ax.plot( tp, rs )
            ax.set_xlabel(Subprofile.XLABEL, fontsize=Subprofile.FONTSIZE )
            ax.set_ylabel(Subprofile.YLABEL, fontsize=Subprofile.FONTSIZE )
            ax.legend()
            fig.show()
        pass  


class Substamp(object):
    @classmethod
    def ETime(cls, f):
        etime = f.delta_substamp[:,-1]/1e6  
        return etime
    @classmethod
    def Subcount(cls, f, label="photon"):
        _icol = np.where(f.subcount_labels == label)[0] 
        icol = _icol[0] if len(_icol) == 1 else -1 
        subcount = f.subcount[:,icol]/1e6 if icol > -1 else None
        return subcount
    @classmethod
    def Labels(cls, f):
        labels = f.substamp_labels
        labels_s = smry_(labels)
        return labels_s
    @classmethod
    def Hdr(cls, f):
        labels_s = cls.Labels(f)
        hdr = (" " * 8  + " %4s " * len(labels_s) ) % tuple(labels_s) 
        return hdr
    @classmethod
    def Title(cls, f, symbol="a"):
        meta = f.substamp_meta
        title = make_title(meta, method="Substamp", symbol=symbol)
        return title 


class Substamp_ONE_Etime(object):
    def __init__(self, f, symbol="fold.substamp.a"):

        substamp = f.substamp
        meta = f.substamp_meta
        names = f.substamp_names
        delta = f.delta_substamp 

        etime = Substamp.ETime(f) 
        subcount_photon = Substamp.Subcount(f, "photon")
        title = Substamp.Title(f, symbol=symbol)
        hdr = Substamp.Hdr(f)

        desc = "\n".join([title, hdr, symbol, repr(delta)])
        print(desc)

        assert len(substamp.shape) == 2 
        assert delta.shape == substamp.shape 
        fontsize = 20 

        deg = 1  # linear   
        linefit = np.poly1d(np.polyfit(subcount_photon, etime, deg))
        linefit_label = "line fit:  slope %10.2f    intercept %10.2f " % (linefit.coef[0], linefit.coef[1])

        if MODE == 2:
            fig, axs = mpplt_plotter(nrows=1, ncols=1, label=title, equal=False)
            ax = axs[0]
            ax.scatter( subcount_photon, etime, label="etime(s)_vs_photon (millions)")
            ax.plot( subcount_photon, linefit(subcount_photon), label=linefit_label )

            ax.set_yscale('log')
            ax.set_ylabel("Event time (seconds)", fontsize=fontsize )
            ax.set_xlabel("Number of Photons (Millions)", fontsize=fontsize )
            ax.legend()
            fig.show()
        pass  


class Substamp_ALL_Hit_vs_Photon(object):
    def __init__(self, base, symbol="fold.substamp"):
        title = "Substamp_ALL_Hit_vs_Photon"
        fontsize = 20 
        if MODE == 2:
            fig, axs = mpplt_plotter(nrows=1, ncols=1, label=title, equal=False)
            ax = axs[0]
            for sym in base.ff:
                f = getattr(base, sym)

                subcount_photon = Substamp.Subcount(f, "photon")
                subcount_hit = Substamp.Subcount(f, "hit")

                deg = 1  # linear   
                linefit = np.poly1d(np.polyfit(subcount_photon, subcount_hit, deg))
                linefit_label = "%s:line fit:  slope %10.2f    intercept %10.2f " % (sym.upper(), linefit.coef[0], linefit.coef[1])

                ax.scatter( subcount_photon, subcount_hit, label="%s : hit_vs_photon (millions)" % sym.upper() )
                ax.plot( subcount_photon, linefit(subcount_photon), label=linefit_label )
            pass
            #ax.set_yscale('log')
            ax.set_ylabel("Number of Hits (Millions)", fontsize=fontsize )
            ax.set_xlabel("Number of Photons (Millions)", fontsize=fontsize )
            ax.legend()
            ax.legend()
            fig.show()
        pass


class Substamp_ALL_Etime_vs_Photon(object):
    def __init__(self, base, symbol="fold.substamp"):
        title = "Substamp_ALL_Etime_vs_Photon"
        fontsize = 20 
        if MODE == 2:
            fig, axs = mpplt_plotter(nrows=1, ncols=1, label=title, equal=False)
            ax = axs[0]
            for sym in base.ff:
                f = getattr(base, sym)

                etime = Substamp.ETime(f) 
                subcount_photon = Substamp.Subcount(f, "photon")

                deg = 1  # linear   
                linefit = np.poly1d(np.polyfit(subcount_photon, etime, deg))
                linefit_label = "%s : line fit:  slope %10.2f    intercept %10.2f " % (sym.upper(), linefit.coef[0], linefit.coef[1])

                ax.scatter( subcount_photon, etime, label="%s : etime(s)_vs_photon (millions)" % sym.upper() )
                ax.plot( subcount_photon, linefit(subcount_photon), label=linefit_label )
            pass
            ax.set_yscale('log')
            ax.set_ylabel("Event time (seconds)", fontsize=fontsize )
            ax.set_xlabel("Number of Photons (Millions)", fontsize=fontsize )
            ax.legend()
            ax.legend()
            fig.show()
        pass


class Substamp_ALL_RATIO_vs_Photon(object):
    def __init__(self, base, symbol="fold.substamp"):
        title = "Substamp_ALL_RATIO_vs_Photon"
        fontsize = 20 
        if MODE == 2:
            fig, axs = mpplt_plotter(nrows=1, ncols=1, label=title, equal=False)
            ax = axs[0]
            assert "a" in base.ff
            assert "b" in base.ff

            a_photon = Substamp.Subcount(base.a, "photon")
            b_photon = Substamp.Subcount(base.b, "photon")
            assert np.all( a_photon == b_photon )

            a_etime = Substamp.ETime(base.a)
            b_etime = Substamp.ETime(base.b)
            boa_etime = b_etime/a_etime 

            ax.scatter( a_photon, boa_etime, label="boa_etime(s)_vs_photon (millions)"  )
            pass
            ax.set_ylabel("B/A Event time ratio", fontsize=fontsize )
            ax.set_xlabel("Number of Photons (Millions)", fontsize=fontsize )
            ax.set_yscale('log')
            ax.legend(loc="lower right")
            fig.show()
        pass


class Substamp_ONE_Delta(object):
    def __init__(self, f, symbol="fold.substamp.a"):

        delta = f.delta_substamp 
        title = Substamp.Title(f, symbol=symbol)
        labels_s = Substamp.Labels(f)
 
        ax = None
        if MODE == 2:
            fig, axs = mpplt_plotter(nrows=1, ncols=1, label=title, equal=False)
            ax = axs[0]
            if TLIM[1] > TLIM[0]:
                ax.set_xlim(*TLIM)
            pass
            for i in range(len(delta)):
                for j in range(len(delta[i])):
                    label = None if i > 0 else labels_s[j]
                    color = palette[j % len(palette)]
                    ax.vlines( delta[i,j], i-0.5, i+0.5, label=label , colors=[color] ) 
                pass
            pass
            ax.legend(loc="center")
            fig.show()
        pass  
        self.ax = ax


if __name__ == '__main__':
    fold = Fold.Load(symbol="fold")

    print(repr(fold))
    print("MODE:%d PICK:%s " % (MODE, PICK) ) 

    if "substamp_ONE" in os.environ and hasattr(fold, "substamp"):
        for e in PICK:
            f = getattr(fold.substamp, e.lower(), None)
            symbol = "fold.substamp.%s" % e.lower() 
            if f is None: continue 
            if True or "delta" in os.environ: Substamp_ONE_Delta(f, symbol=symbol)
            if True or "etime" in os.environ: Substamp_ONE_Etime(f, symbol=symbol)
        pass
    if "substamp_ALL" in os.environ and hasattr(fold, "substamp"):
        Substamp_ALL_Etime_vs_Photon(  fold.substamp, symbol="fold.substamp")
        Substamp_ALL_Hit_vs_Photon(    fold.substamp, symbol="fold.substamp" )
        Substamp_ALL_RATIO_vs_Photon(  fold.substamp, symbol="fold.substamp")
    pass

    if "subprofile" in os.environ and hasattr(fold, "subprofile"):
        for e in PICK:
            f = getattr(fold.subprofile, e.lower(), None)
            symbol = "fold.subprofile.%s" % e.lower() 
            if f is None: continue
            Subprofile_ONE(f, symbol=symbol)
        pass
        Subprofile_ALL(fold.subprofile, symbol="fold.subprofile")
    pass

    if "runprof" in os.environ and hasattr(fold, "runprof"):
        Runprofile_ALL(fold.runprof, symbol="fold.runprof" )
    pass
pass

