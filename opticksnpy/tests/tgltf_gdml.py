#!/usr/bin/env python
"""
TODO: 

* migrate into analytic
* rename to gdml2gltf.py 

* gltf should get standard place inside the geocache


grep selected\":\ 1 $TMP/tgltf/tgltf-gdml--.pretty.gltf | wc -l
    9068


"""
import os, logging, sys, numpy as np

log = logging.getLogger(__name__)

from opticks.ana.base import opticks_main
from opticks.analytic.treebase import Tree
from opticks.analytic.gdml import GDML
from opticks.analytic.sc import Sc

args = opticks_main()


wgg = GDML.parse()

tree = Tree(wgg.world)  

tree.apply_selection(args.query)   # set node.selected "volume mask" 
 
sc = Sc(maxcsgheight=3)

sc.extras["verbosity"] = 1
sc.extras["targetnode"] = 0   # args.query.query_range[0]   # hmm get rid of this ?

tg = sc.add_tree_gdml( tree.root, maxdepth=0)

path = args.gltfpath
gltf = sc.save(path)

print path      ## <-- WARNING COMMUNICATION PRINT

#TODO: instead of just passing a path pass a config line or json snippet with the target

