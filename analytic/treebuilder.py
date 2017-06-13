#!/usr/bin/env python
"""

"""
import os, logging, numpy as np
log = logging.getLogger(__name__)

from opticks.analytic.csg import CSG 


class TreeBuilder(object):

    @classmethod 
    def balance(cls, tree):
        pass 
 

    def __init__(self, primitives, operator="union"):
        """
        :param primitives: list of CSG instance primitives
        """
        self.operator = operator 
        nprim = len(map(lambda n:n.is_primitive, primitives))
        assert nprim == len(primitives) and nprim > 0

        self.nprim = nprim
        self.primitives = primitives

        height = -1
        for h in range(10):
            tprim = 1 << h 
            if tprim >= nprim:
                height = h
                break

        assert height > -1 
        log.debug("TreeBuilder nprim:%d required height:%d " % (nprim, height))
        self.height = height

        if height == 0:
            assert len(primitives) == 1
            root = primitives[0]
        else: 
            root = self.build(height)
            self.populate(root, primitives[::-1]) 
            self.prune(root)
        pass
        self.root = root 

    def build(self, height):
        """
        Build complete binary tree with all operators the same
        and CSG.ZERO placeholders for the primitives at elevation 0.
        """
        def build_r(elevation, operator):
            if elevation > 1:
                node = CSG(operator, name="treebuilder_midop")
                node.left  = build_r(elevation-1, operator) 
                node.right = build_r(elevation-1, operator)
            else:
                node = CSG(operator, name="treebuilder_bileaf")
                node.left = CSG(CSG.ZERO, name="treebuilder_bileaf_left")
                node.right = CSG(CSG.ZERO, name="treebuilder_bilead_right")
            pass
            return node  
        pass
        root = build_r(height, self.operator ) 
        return root

    def populate(self, root, primitives):
        """
        Replace the CSG.ZERO placeholders with the primitives
        """
        for node in root.inorder_():
            if node.is_operator:
                try:
                    if node.left.is_zero:
                        node.left = primitives.pop()
                    if node.right.is_zero:
                        node.right = primitives.pop()
                    pass
                except IndexError:
                    pass
                pass
            pass
        pass

    def prune(self, root):
        """
        Pulling leaves partnered with CSG.ZERO up to a higher elevation. 
        """
        def prune_r(node):
            if node is None:return
            if node.is_operator:
                prune_r(node.left)
                prune_r(node.right)

                if node.left.is_lrzero:
                    node.left = CSG(CSG.ZERO)
                elif node.left.is_rzero:
                    ll = node.left.left
                    node.left = ll
                pass
                if node.right.is_lrzero:
                    node.right = CSG(CSG.ZERO)
                elif node.right.is_rzero:
                    rl = node.right.left
                    node.right = rl
                pass
            pass
        pass
        prune_r(root)




def test_treebuilder():
    log.info("test_treebuilder")
    sprim = "sphere box cone zsphere cylinder trapezoid"
    primitives = map(CSG, sprim.split() + sprim.split() )
    nprim = len(primitives)

    for n in range(0,nprim):
        tb = TreeBuilder(primitives[0:n+1])
        tb.root.analyse()
        print tb.root.txt 

def test_uniontree():
    log.info("test_uniontree")
 
    sprim = "sphere box cone zsphere cylinder trapezoid"
    primitives = map(CSG, sprim.split())

    for n in range(0,len(primitives)):
        root = CSG.uniontree(primitives[0:n+1], name="test_uniontree")
        print "\n",root.txt



if __name__ == '__main__':
    pass
    logging.basicConfig(level=logging.INFO)

    test_treebuilder()
    test_uniontree()


