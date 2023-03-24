#!/usr/bin/env python
"""
svn.py / git.py 
=================

git.py is a symbolic link to svn.py that detects its name
to pick the flavor of version control 


This script enables two svn working copies "local" and "remote"
to be kept in sync with each other without requiring the changes
to be committed to svn or git.  It provides a workaround for operating with 
a remote svn working copy over a slow connection when you do not yet
have permission to commit many of the changes.

This slim script avoids the need to wield the "git svn" sledgehammer
to crack a nut.

Essentially want to be able to locally "svn up" and make edits  
then selectively scp over changed files (with different digests) 
into the remote working copy for compilation and testing.  

Generally it is best to avoid editing on the remote end, but it is sometimes 
unavoidable. This script eases the pain of bringing both working copies
back in sync without having to commit the changes.

NB all the below commands do no harm, they only suggest the scp commands 
that need to be manually run in the shell or piped there.

::

   svn st | perl -ne 'm,\S\s*(\S*), && print "$1\n"' - | xargs md5 % 
   svn st | perl -ne 'm,\S\s*(\S*), && print "$1\n"' - | xargs md5sum % 



TODO:

* record digests of put files into some cache, so a repeating put after edits 
  can put just the updated ? 

* (hmm does cfu do that already ? perhaps do cfu for every put)
* basically assuming that edits are only done locally, the normal situation, can
  minimize puts 



Workflow::

   loc> export PATH=$HOME/opticks/bin:$PATH

   loc> svn up    
   rem> svn up

   loc> vi ...   
       ## local editing, adding files 

   loc> scp ~/opticks/bin/svn.py P:opticks/bin/svn.py 
       ## update this script at remote 

   loc> ssh P opticks/bin/svn.py > ~/rstat.txt     
       ## take snapshot of remote working copy digests 

       ## OR instead do this with : svn.py rup
       ## that can be combined, eg svn.py rup cf 

   loc> svn.py loc   
       ## list status of local working copy with file digests  
   loc> svn.py rem   
       ## ditto for remote, using the ~/rstat.txt snapshot from above
   loc> svn.py cf    
       ## compare status showing which files are only loc/rem and which have different digests

   loc> svn.py put
       ## emit to stdout scp commands to local to remote copy wc files of M/A/? status    
   loc> svn.py get
       ## emit to stdout scp commands to remote to local copy wc files of M/A/? status    
   loc> svn.py sync
       ## show the cf output interleaved with put/get commands to bring the two wc together
       ##
       ## NB where there are digest changes, no command is suggested as it is necessary to 
       ## manually examine the differences to see which is ahead OR to merge changes 
       ## from both ends if there has been a mixup and changes were made in the wrong file 


   loc> svn.py sync -p rem | grep scp 
       ## with remote priority, show the sync scp commands 

   loc> svn.py sync -p rem | grep scp | sh 
       ## pipe those commands to shell

"""
import os, sys, re, argparse, logging, platform

try:
    from commands import getstatusoutput 
except ImportError:
    from subprocess import getstatusoutput 
pass 

from collections import OrderedDict as odict
try: 
    from hashlib import md5 
except ImportError: 
    from md5 import md5 
pass

def md5sum_py3(path):
    with open(path, mode='rb') as f:
        d = md5()
        for buf in iter(partial(f.read, 4096), b''):
            d.update(buf)
        pass
    return d.hexdigest()

def md5sum_py2(path):
    dig = md5()
    with open(path,'rb') as f:  
        for chunk in iter(lambda: f.read(8192),''): 
            dig.update(chunk)
        pass
    pass
    return dig.hexdigest()

def md5sum(path, block_size=8192):
    """ 
    :param path:
    :return: md5 hexdigest of the content of the path or None if non-existing path
    """
    dig = md5()
    with open(path, "rb") as f:
        #while chunk := f.read(block_size):   walrus-operator only available in py38 + it gives error in py27
        while True:
            chunk = f.read(block_size)
            if not chunk:
                break
            pass
            dig.update(chunk)
        pass
    pass
    return dig.hexdigest()



def md5sum_alt(path):
    system =  platform.system()
    if system == "Darwin":
        cmd = "md5 -q %s"  ## just outputs the digest 
        rc,out = getstatusoutput(cmd % path)
        assert rc == 0 
        dig = out      
    elif system == "Linux":
        cmd = "md5sum %s"   ## outputs the digest and the path 
        rc,out = getstatusoutput(cmd % path)
        assert rc == 0 
        dig = out.split(" ")[0]
    else:
        dig = None
        assert 0, system
    pass
    return dig 


log = logging.getLogger(__name__)

expand_ = lambda p:os.path.expandvars(os.path.expanduser(p))  

class Path(dict):
    def __init__(self, *args, **kwa):
        dict.__init__(self, *args, **kwa)
        if not 'dig' in self:
            self["dig"] = md5sum(self["path"])  
            if self.get('check', False) == True:
                self["dig_alt"] = md5sum_alt(self["path"])  
                match = self["dig_alt"] == self["dig"]

                fmt = "%(path)s %(dig)s %(dig_alt)s"
                if not match:
                    log.fatal(" check FAIL : " + fmt % self ) 
                else:
                    log.debug(" check OK   : " + fmt % self ) 
                pass
                assert match, (self["dig_alt"], self["dig"])
            pass
        pass
        if not 'dig5' in self:
            self["dig5"] = self["dig"][:5]
        pass
    def __str__(self):
        dig = self["dig"]
        ldig = self.get("ldig", -1)
        if ldig < 0: ldig = len(dig)
        return "%1s %s %s " % (self["st"], dig[:ldig], self["path"])

class WC(object):
    fstpat = re.compile("^(?P<st>\S)\s*(?P<dig>\S*)\s*(?P<path>\S*)$")
    lstpat = re.compile("^\s*(?P<st>\S*)\s*(?P<path>\S*)$")

    @classmethod
    def detect_vc_from_dir(cls):
        if os.path.isdir(".svn"):
            vc = "svn"
        elif os.path.isdir(".git"):
            vc = "git"
        else:
            print("FATAL must invoke from svn or git top level working copy directory")
        pass
        #print("detected vc %s " % vc)
        return vc 

    @classmethod
    def detect_repo_from_scriptname(cls):
        scriptname = os.path.basename(sys.argv[0])  
        if scriptname == "svn.py":
            repo = "offline_svn"
        elif scriptname == "git.py":
            repo = os.environ.get("REPO", "opticks_git" )
        else:
            assert 0 
        pass
        #print("detected repo %s " % repo)
        return repo

    @classmethod
    def detect_repo_from_cwd(cls):
        cwd = os.getcwd()
        home = os.environ["HOME"]
        assert(cwd.startswith(home))
        reldir = cwd[len(home)+1:]
        topdir = reldir.split("/")[0]  


        if topdir == "opticks":
            repo = "opticks"
        elif reldir.startswith("junotop/junosw"):
            repo = "junosw"
        elif reldir.startswith("junotop/junoenv"):
            repo = "junoenv"
        elif topdir == "j":
            repo = "j"
        elif topdir == "env":
            repo = "env"
        else:
            print("FATAL : FAILED TO DETECT REPO : RUN THIS FROM TOP DIR OF REPO ") 
            print(" cwd %s home %s reldir %s topdir %s " % (cwd, home, reldir, topdir))
            print("detected repo %s " % repo)
            assert 0 
        pass
        #print("detect_repo_from_cwd home:%s reldir:%s topdir:%s repo:%s " % (home, reldir, topdir, repo))
        return repo



    @classmethod
    def parse_args(cls, doc):
        repo = cls.detect_repo_from_cwd() 
        defaults = {}
        if repo == "offline_svn":
            vc = "svn"
            defaults["chdir"] = "~/junotop/offline" 
            defaults["rbase"] = "P:junotop/offline" 
            defaults["rstatpath"] = "~/rstat.txt" 
            defaults["lstatpath"] = "~/lstat.txt" 
            defaults["rstatcmd"] = "ssh P opticks/bin/svn.py"
            defaults["lstatcmd"] = "svn.py"
            defaults["statcmd"] = "svn status"
        elif repo == "junosw":
            vc = "git"
            defaults["chdir"] = "~/junotop/junosw" 
            defaults["rbase"] = "P:junotop/junosw" 
            defaults["rstatpath"] = "~/rstat_junosw.txt" 
            defaults["lstatpath"] = "~/lstat_junosw.txt" 
            defaults["rstatcmd"] = "ssh P opticks/bin/git.py"
            defaults["lstatcmd"] = "git.py"
            defaults["statcmd"] = "git status --porcelain"
        elif repo == "junoenv":
            vc = "git"
            defaults["chdir"] = "~/junotop/junoenv" 
            defaults["rbase"] = "P:junotop/junoenv" 
            defaults["rstatpath"] = "~/rstat_junoenv.txt" 
            defaults["lstatpath"] = "~/lstat_junoenv.txt" 
            defaults["rstatcmd"] = "ssh P opticks/bin/git.py"
            defaults["lstatcmd"] = "git.py"
            defaults["statcmd"] = "git status --porcelain"
        elif repo == "customgeant4":
            vc = "git"
            defaults["chdir"] = "~/customgeant4" 
            defaults["rbase"] = "P:customgeant4" 
            defaults["rstatpath"] = "~/rstat_customgeant4.txt" 
            defaults["lstatpath"] = "~/lstat_customgeant4.txt" 
            defaults["rstatcmd"] = "ssh P opticks/bin/git.py"
            defaults["lstatcmd"] = "git.py"
            defaults["statcmd"] = "git status --porcelain"

        elif repo == "opticks":
            vc = "git"
            defaults["chdir"] = "~/opticks" 
            defaults["rbase"] = "P:opticks" 
            defaults["rstatpath"] = "~/rstat_opticks.txt" 
            defaults["lstatpath"] = "~/lstat_opticks.txt" 
            defaults["rstatcmd"] = "ssh P opticks/bin/git.py"
            defaults["lstatcmd"] = "git.py"
            defaults["statcmd"] = "git status --porcelain"
        elif repo == "env":
            vc = "git"
            defaults["chdir"] = "~/env" 
            defaults["rbase"] = "P:env" 
            defaults["rstatpath"] = "~/rstat_env.txt" 
            defaults["lstatpath"] = "~/lstat_env.txt" 
            defaults["rstatcmd"] = "ssh P opticks/bin/git.py"
            defaults["lstatcmd"] = "git.py"
            defaults["statcmd"] = "git status --porcelain"
        elif repo == "j":
            vc = "git"
            defaults["chdir"] = "~/j" 
            defaults["rbase"] = "P:j" 
            defaults["rstatpath"] = "~/rstat_j.txt" 
            defaults["lstatpath"] = "~/lstat_j.txt" 
            defaults["rstatcmd"] = "ssh P opticks/bin/git.py"
            defaults["lstatcmd"] = "git.py"
            defaults["statcmd"] = "git status --porcelain"
        else:
            print("repo:%s has no defaults" % repo )
            pass
        pass
        parser = argparse.ArgumentParser(doc)
        parser.add_argument( "cmd", default=["st"], nargs="*", choices=["rup","lup","loc","rem","st","get","put","cf","cfu","sync","scp", ["st"]], 
            help="command specifying what to do with the working copy" ) 
        parser.add_argument( "--chdir", default=defaults["chdir"], help="chdir here" ) 
        parser.add_argument( "--rstatpath", default=defaults["rstatpath"], help="local path to remote status file" ) 
        parser.add_argument( "--lstatpath", default=defaults["lstatpath"], help="local path to local status file" ) 
        parser.add_argument( "--rstatcmd", default=defaults["rstatcmd"], help="command to invoke the remote version of this script" )
        parser.add_argument( "--lstatcmd", default=defaults["lstatcmd"], help="command to invoke the local version of this script" )
        parser.add_argument( "--rbase", default=defaults["rbase"], help="remote svn working copy" ) 
        parser.add_argument( "--check", default=False, action="store_true", help="check digest with os alternative md5 or md5sum" ) 
        parser.add_argument( "--ldig", type=int, default=-1, help="length of digest" ) 
        parser.add_argument( "-p", "--priority", choices=["loc","rem"], default="loc", help="Which version wins when a file exists at both ends" ) 
        parser.add_argument( "--level", default="info", help="logging level" ) 
        args = parser.parse_args()
        fmt = '[%(asctime)s] p%(process)s {%(pathname)s:%(lineno)d} %(levelname)s - %(message)s'
        logging.basicConfig(level=getattr(logging,args.level.upper()), format=fmt)
        args.chdir = expand_(args.chdir)
        args.rstatpath = expand_(args.rstatpath)
        args.lstatpath = expand_(args.lstatpath)

        args.vc = vc
        args.repo = repo
        args.statcmd = defaults["statcmd"]
        return args

    @classmethod
    def FromStatusFile(cls, statpath, ldig):
        """
        :param statpath:
        :param ldig: int length of digest
        :return rem: WC instance

        Parse the status output of a remote instance of this script
        """
        log.debug("reading %s " % statpath) 
        lines = map(str.rstrip, open(statpath, "r").readlines())
        paths = []
        for line in lines:
            if line.startswith("Warning: Permanently added"): continue
            if len(line) == 3: continue  # skip the rem/loc title
            m = cls.fstpat.match(line)
            assert m, line
            d = m.groupdict()
            d["ldig"] = ldig
            paths.append(Path(d))
        pass
        return cls(paths, "rem")

    @classmethod         
    def FromStatus(cls, args):
        """
        :param args:
        :return loc: WC instance

        Parse the output of "svn status" collecting status strings and paths
        """

        log.debug("ldig %s check %s statcmd %s " % (args.ldig,args.check, args.statcmd))

        rc, out = getstatusoutput(args.statcmd)
        assert rc == 0 
      
        log.debug(out)  

        paths = []
        for line in filter(None,out.split("\n")):
            log.debug("[%s]"%line)
            m = cls.lstpat.match(line)
            assert m, line
            d = m.groupdict()
            d["ldig"] = args.ldig
            d["check"] = args.check 
            assert d["st"] in ["M","MM","A", "?", "??"], d["st"]

            if d["st"] == "??": d["st"] = "?"   # bring git into line

            if os.path.isdir(d["path"]):
                log.debug("skip dir %s " % d["path"] )
            else:    
                paths.append(Path(d))
            pass
        pass
        return cls(paths, "loc")

    @classmethod         
    def FromComparison(cls, loc, rem, ldig):
        """
        :param loc: WC instance
        :param rem: WC instance
        :param ldig: length of digest
        :return cf: WC instance
        """
        l = loc.d
        r = rem.d 
        u = set(l).union(set(r))
        paths = []
        stfmt = "%2s %1s%1s %1s"
        dgfmt = "%5s|%5s" 

        index_ = lambda ls,val:ls.index(val) if val in ls else -1 
       
        for k in sorted(list(u), key=lambda k:max(index_(list(l.keys()),k),index_(list(r.keys()),k))):
            st = "".join(["l" if k in l else " ","r" if k in r else " "])
            rk = r.get(k, None)
            lk = l.get(k, None)

            d = dict(path=k)
            d["ldig"] = ldig

            stdig = " "

            if st == "lr":
                stdig = "=" if lk["dig"] == rk["dig"] else "*"
                stdat = (st, lk["st"], rk["st"], stdig )
                d["dig"] = "%s|%s" % (lk["dig"],rk["dig"])
                d["dig5"] = dgfmt % (lk["dig5"],rk["dig5"] )
            elif st == "l ":
                stdat = (st, lk["st"], "", " " )
                d["dig"] = "%s|%s" % (lk["dig"],"-" * 32 )
                d["dig5"] = dgfmt % (lk["dig5"], "-" * 5 )
            elif st == " r":
                stdat = (st, "", rk["st"], " " )
                d["dig"] = "%s|%s" % ("-" * 32, rk["dig"])
                d["dig5"] = dgfmt % ("-" * 5, rk["dig5"] )
            pass
            d["st"] = stfmt % stdat  
            d["stlr"] = st 
            d["stdig"] = stdig
            paths.append(Path(d))
        pass
        return cls(paths, "cf")

    def __init__(self, paths, name):
        self.paths = paths
        self.name = name
        d = odict()
        for p in paths:
            d[p["path"]] = p
        pass
        self.d = d

    @classmethod
    def PutCmd(cls, path, rbase, chdir):
        return "scp %s/%s %s/%s" % (chdir,path,rbase,path)

    @classmethod
    def GetCmd(cls, path, rbase, chdir):
        return "scp %s/%s %s/%s" % (rbase,path,chdir,path)

    def scp_put_cmds(self, rbase, chdir):
        """put from local to remote"""
        return "\n".join(map(lambda d:self.PutCmd(d["path"],rbase,chdir), self.paths))

    def scp_get_cmds(self, rbase, chdir):
        """get from remote to local"""
        return "\n".join(map(lambda d:self.GetCmd(d["path"],rbase,chdir), self.paths))

    def _get_hdr(self):
        name = getattr(self, 'name', "noname")
        return "%s" % name

    hdr = property(_get_hdr)

    def __str__(self):
        return "\n".join([self.hdr]+list(map(str,self.paths)))


if __name__ == '__main__':
    args = WC.parse_args(__doc__)

    if "rup" in args.cmd or "cfu" in args.cmd:
        log.info("running args.rstatcmd : %s " % args.rstatcmd )
        rc,rup_out = getstatusoutput(args.rstatcmd)
        assert rc == 0, (rc, "maybe the ssh tunnel is not running") 
        #print(rup_out)
        log.info("writing rup_out to args.rstatpath : %s " % args.rstatpath)
        open(args.rstatpath,"w").write(rup_out)
    pass

    if "lup" in args.cmd or "cfu" in args.cmd:
        log.info("running args.lstatcmd : %s " % args.lstatcmd )
        rc,lup_out = getstatusoutput(args.lstatcmd)
        assert rc == 0, (rc, "lstatcmd failed") 
        #print(lup_out)
        log.info("writing lup_out to args.lstatpath : %s " % args.lstatpath)
        open(args.lstatpath,"w").write(lup_out)
    pass

    if os.path.exists(args.rstatpath):
        rem = WC.FromStatusFile(args.rstatpath, args.ldig)
    else:
        rem = None
    pass 

    if os.path.exists(args.lstatpath):
        lup = WC.FromStatusFile(args.lstatpath, args.ldig)
    else:
        lup = None
    pass 

    os.chdir(args.chdir)
    loc = WC.FromStatus(args)

    if loc and rem:
        cf = WC.FromComparison(loc, rem, args.ldig)
    else:
        cf = None
    pass

    for cmd in args.cmd:
        log.debug(cmd)
        if cmd == "loc" or cmd == "st":
            print(loc)
        elif cmd == "rem":
            print(rem)
        elif cmd == "put": # scp local to remote
            print(loc.scp_put_cmds(args.rbase, args.chdir))
        elif cmd == "get": # scp remote to local 
            print(rem.scp_get_cmds(args.rbase, args.chdir))
        elif cmd == "cf" or cmd == "cfu":
            assert cf
            print(cf)
        elif cmd == "sync" or cmd == "scp":
            assert cf
            for p in cf.paths:
                if cmd == "sync":
                    print(str(p))
                pass
                stlr = p["stlr"]
                stdig = p["stdig"]
                if stlr == "l ":
                    print(WC.PutCmd(p["path"], args.rbase, args.chdir))
                elif stlr == " r":
                    print(WC.GetCmd(p["path"], args.rbase, args.chdir))
                elif stlr == "lr":
                    if stdig == "*": 
                        if args.priority == "rem":
                            print(WC.GetCmd(p["path"], args.rbase, args.chdir))
                        elif args.priority == "loc":
                            print(WC.PutCmd(p["path"], args.rbase, args.chdir))
                        else:
                            assert 0, args.priority
                        pass
                    pass
                else:
                    assert 0, stlr
                pass
            pass
        elif cmd == "rup" or cmd == "lup":
            pass
        else:
            assert 0, cmd
        pass
    pass
