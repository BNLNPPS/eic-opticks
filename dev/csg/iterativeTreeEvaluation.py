#!/usr/bin/env python
"""

Hmm descoping to support complete binary trees up to maximum depth
of three/four would certainly cover all reasonable single 
solid boolean combinations.
Assuming implement transforms in a way that doesnt enlarge the tree.

* http://www.geeksforgeeks.org/iterative-postorder-traversal/
* http://www.techiedelight.com/Tags/lifo/



::

      1
 
      2             3

      4      5      6      7          =>   4 5 2 6 7 3 1

      8  9   10 11  12 13  14 15      =>   (8 9 4) (10 11 5) 2 (12 13 6) (14 15 7) 3 1
                                               (4         5  2)      (6         7  3)
                                                            (2                     3  1)  

Discern where you are based a sequence of 1-based indices, where the 
indices follow level order (aka breadth first order)

* normal left/right/parent triple is  (i,i+1,i/2)   eg (8 9 4) (10 11 5) (12 13 6) (14 15 7) 
* if not a triple then treat the index singly eg 2 corresponding to pseudo-triples ("4" "5" 2) ("6" "7" 3) ("2" "3" 1)
   



csg ray trace algo 

* knowing tree depth can jumpt straight to primitives
* visit left/right/parent where left/right are primitives 

intersect ordering 

( 8 9 4 )
( 10 11 5 )
( 2 )
( 12 13 6 )
( 14 15 7 )
( 3 )
( 1 )





* (8   9 4)   pushLeft -> "4"   
* (10 11 5)   pushRight -> "5"
* ("4"  "5"  2)   popLeft/popRight -> pushLeft "2"    

    ( "4" and "5" child intersects no longer needed, after parent "2" intersect is computed)

* (12 13 6)    pushLeft -> "6" 
* (14 15 7)    pushRight -> "7"
* ("6" "7" 3)      popLeft,popRight -> pushRight "3" 

* ("2" "3" 1)     popLeft, popRight -> "1"



::

     1

     2                                            3
 
     4             5                              6           7

     8       9     10     11                     12    13     14     15
 
     16 17  18 19  20 21  22 23                  24 25 26 27  28 29  30 31 


            postorder ->  (16 17 8) (18 19 9) (4)     (20 21 10) (22 23 11) [5] [2]       24 25 12 26 27 13 6 28 29 14 30 31 15 7 3 1
                                 L8        R9  L4           L4,10       R11 R5 L2

               (16 17  8)->pushL-8 
               (18 19  9)->pushR-9  
               popL-8,popR-9->pushL[4]     L{4}   

               (20 21 10)->pushL-10 
               (22 23 11)->pushR-11 
               popL-10,popR-11->pushR[5]   R{5}

                popL-4,popR-5 (4, 5, 2) -> pushL [2]    L{2}     when left and right are > 0, time to pop-em ? 
                        

               (24 25 12)->pushL-12
               (26 27 13)->pushR-13  
                      [6]  popL-12,popR-13 -> pushL [6]      L{2,6}
               (28 29 14)->pushL-14                          L{2,6,14} 
               (30 31 15)->pushR-15                          R{15}
                      [7]  popL-14, popR-15 -> pushR [7]     L{2,6} R{7} 
                      [3]  popL-6,  popR-7  -> pushR [3]     L{2} R{3}

                    popL-2, popR-3  ->  1
                       1



( 16 17 8 )
( 18 19 9 )
( 4 )
( 20 21 10 )
( 22 23 11 )
( 5 )
( 2 )
( 24 25 12 )
( 26 27 13 )
( 6 )
( 28 29 14 )
( 30 31 15 )
( 7 )
( 3 )
( 1 )





It looks like using L and R intersect stacks will allow to iteratively 
evaluate the ray intersect with the binary tree just by following 
the postorder traverse while pushing and popping from the L/R stacks
which need to be able to hold a maximum of 3 entries.




With 1-based node index, excluding root at node 1 
* left always even
* right always odd


 
*   (16 17 8)  lstack:[8]
*   (18 19 9)  rstack:[9]
*   ( 8  9 



"""


from node import Node, root0, root1, root2, root3, root4
 
def postOrderIterative(root): 
    """
    # iterative postorder traversal using
    # two stacks : nodes processed 

    ::

          1

         [2]                 3

         [4]     [5]         6     7

         [8] [9] [10] [11]  12 13  14 15

    ::

        In [25]: postOrderIterative(root3.l)
        Out[25]: 
        [Node(8),
         Node(9),
         Node(4,l=Node(8),r=Node(9)),
         Node(10),
         Node(11),
         Node(5,l=Node(10),r=Node(11)),
         Node(2,l=Node(4,l=Node(8),r=Node(9)),r=Node(5,l=Node(10),r=Node(11)))]

        In [26]: postOrderIterative(root3.l.l)
        Out[26]: [Node(8), Node(9), Node(4,l=Node(8),r=Node(9))]

    """ 
    if root is None:
        return         
     
    nodes = []
    s = []
     
    nodes.append(root)
     
    while len(nodes) > 0:
         
        node = nodes.pop()
        s.append(node)
     
        if node.l is not None:
            nodes.append(node.l)
        if node.r is not None :
            nodes.append(node.r)
 
    #while len(s) > 0:
    #    node = s.pop()
    #    print node.d,
 
    return list(reversed(s))




def binary_calc(node, left=None, right=None):

    assert hasattr(node,'depth')

    if left and right:
        return "[%s;%s](%s,%s)" % ( node.d, node.depth, left, right )
    else:
        return "%s;%s" % (node.d, node.depth )


def postordereval_r(p):
    """
    * :google:`tree calculation postorder traversal`
    * http://interactivepython.org/runestone/static/pythonds/Trees/TreeTraversals.html
    """
    if not p: return

    l = postordereval_r(p.l)
    r = postordereval_r(p.r)

    return binary_calc(p, l, r )


def postordereval_i1(node):
    """
    Duplicates postordereval_r recursive tree evaluation using iteration

    Relies upon:
 
    * complete binary tree  
    * nodes carrying 1-based levelorder (aka breadth-first) indices 
    * node ordering into postorder (ie left,right,parent)
    * lhs and rhs stacks 

    1-based levelorder indices::

        1                                 depth:0 [1]    0x1 << 0
  
        2              3                  depth:1 [2]    0x1 << 1

        4     5        6       7          depth:2 [4]    0x1 << 2

        8  9  10 11    12  13  14  15     depth:3 [8]    0x1 << 3
             
                                                 ^^^^^^^^^ nodes at each level
    postorder visits l and r before p

        8 9 4  10 11 5 2 12 13 6 14 15 7 3 1
            L        R L

    lowest level triples of l,r,p have distinctive
    pattern of 1-based levelorder indices

        (2*p 2*p+1 p)

    Consider, while evaluating "2" from (4 5) 

    * loopL -> rerun: "8 9 4"  will pushL then repeating 2 will popL  (R "5" staying unchanged)
    * loopR -> rerun: "10 11 5" will pushR then repeating 2 will popR

    ::

        In [32]: postOrderIterative(root3.l.l)
        Out[32]: [Node(8), Node(9), Node(4,l=Node(8),r=Node(9))]

        In [33]: postOrderIterative(root3.l.r)
        Out[33]: [Node(10), Node(11), Node(5,l=Node(10),r=Node(11))]


    What about more levels, eg while evaluating "1" from (2 3)

                          L       R 
    * loopL -> rerun "8 9 4 10 11 5 2"   which will pushL4,pushR5,popL4,popR5,pushL2  
                             (equal push-pop on R, so the former R3 should still be 
                              ready at the top of the stack)
 
                  actually before doing the repeat need to push the right, as it was just
                  popped for the initial evaluation of "2" 

    ::

        In [31]: postOrderIterative(root3.l)
        Out[31]: 
        [Node(8),
         Node(9),
         Node(4,l=Node(8),r=Node(9)),
         Node(10),
         Node(11),
         Node(5,l=Node(10),r=Node(11)),
         Node(2,l=Node(4,l=Node(8),r=Node(9)),r=Node(5,l=Node(10),r=Node(11)))]

 

    The above suggests that repeating the appropriate postorder sub-sequence 
    will do the right thing, so long as push the other non-looped side first.
 
    How to get the appropriate sub-sequence ? Actually the 
    sequence is just postorder again, the issue is how many steps 
    back to reiterate. Knowing the maxdepth of the tree 
    and the depth of node being advanced tell you that... 

    Nope, doesnt work: as it repeats things that should not 
    be repeated, the nodes to revisit are not a contiguous block of
    post order behind the current pos.

    * loopL "2" -> "8 9 4 10 11 5 2"     4+2+1=7   step back 7

      * hmm thats repeating "10 11 5" when it is wrong to do so, as
        using an advanced tmin may give a different result  

    * loopR "3" -> "12 13 6 14 15 7 3"   

      * this repeats the left progeny of "3" : "12 13 6" when wrong to do so
      * need a way to identify left or right progeny of some node,
        so can set reright releft 


        3 
        
        6=3*2                7=3*2+1 

        12=3*2*2 13=3*2*2+1  14=3*2*2+2  15=3*2*2+2+1
        




    Labelling the nodes with depth will make this easier, as the 
    trees are constant this can be done ahead of time CPU side.

    node "2" is depth 1 

    """

    roots = []
    roots.append([node,0,"Start"])

    lhs = []
    rhs = []

    debug = 1

    while len(roots) > 0:
        root,c,istage = roots.pop()
        nodes = postOrderIterative(root)
        nn = len(nodes)

        if debug>0:
            print "istage: %s " % istage
            print "c:%d nn:%d root:%r " % (c, nn, root) 
        if debug>2:
            print "nodes:%r " % nodes 
                
        while c < nn: 
            if nn >= 3 and c < nn - 2 and nodes[c+1].d - nodes[c+0].d == 1 and 2*nodes[c+2].d == nodes[c+0].d:
                # triplet : two prims and one operation,  2*idx_parent = idx_left 
                increment = 3   
                l = nodes[c+0]
                r = nodes[c+1]
                p = nodes[c+2]

                el = binary_calc(l)  
                er = binary_calc(r) 

                if debug>2:
                    print "lrp (%d %d %d) " % (l.d, r.d, p.d )
            else:
                # op applied to two op results popped up from lower level
                increment = 1
                p = nodes[c+0]
                el = lhs.pop() 
                er = rhs.pop() 

                if debug>2:
                    print "el,er,p (%s %s %d) " % (el, er, p.d )
            pass

            ep = binary_calc(p,el,er)

            # faking LoopL,LoopR 
            # record position in current postorder
            # before queing up the Looper postorder 

            if hasattr(p, "LoopL"):
                delattr(p, "LoopL")

                if debug>2:
                    print "LoopL detected at p:%r " % p

                if increment == 1: 
                    # just popped lhs and rhs, but LoopL means are reiterating lhs, so put back rhs
                    rhs.append(er)
                    roots.append([root,c,"ResumeFromLoopL"])  
                    roots.append([p.l,0,"LoopL"])
                    break 
                elif increment == 3:
                    if debug>2:
                        print "LoopL at prim level, just needs direct rerun with tmin advanced"
                    el = binary_calc(l) 
                    ep = binary_calc(p,el,er)
                pass

            elif hasattr(p, "LoopR"):
                delattr(p, "LoopR")

                if debug>2:
                    print "LoopR detected at %r " % p

                if increment == 1: 
                    # just popped lhs and rhs, but LoopR means are reiterating rhs, so put back lhs
                    lhs.append(el)
                    roots.append([root,c,"ResumeFromLoopR"])
                    roots.append([p.r,0,"LoopR"])
                    break 
                elif increment == 3:
                    if debug>2:
                        print "LoopR at prim level, just needs a direct rerun with tmin advanced"
                    er = binary_calc(r) 
                    ep = binary_calc(p,el,er)
                pass
            else:
                pass    


            c += increment

            if p.d % 2 == 0:
                lhs.append(ep)
            else:
                rhs.append(ep)
            pass
        pass

    assert c == nn , (c, nn)
    assert nodes[c-1].d == 1
    assert len(lhs) == 0, lhs
    assert len(rhs) == 1, rhs

    return rhs[0]
        
 

def depth_r(node, depth=0):
    """
    Marking up the tree with depth, can be done CPU side 
    during conversion : so recursive is fine
    """
    if node is None:
        return 

    #print node
    node.depth = depth

    maxd = depth
    if node.l is not None: maxd = depth_r(node.l, depth+1)
    if node.r is not None: maxd = depth_r(node.r, depth+1)
    
    return maxd


def levelorder_i(root):
    q = []
    q.append(root)

    idx = 1 
    while len(q) > 0:
       node = q.pop(0)   # bottom of q (ie fifo)

       assert node.d == idx
       idx += 1

       if not node.l is None:q.append(node.l)
       if not node.r is None:q.append(node.r)
    pass
    return idx - 1


def progeny_i(root):
    """

    1

    2          3

    4    5     6      7

    8 9  10 11 12 13 14 

    """

    nodes = []
    q = []
    q.append(root)

    idx = 1 
    while len(q) > 0:
       node = q.pop(0)   # bottom of q (ie fifo)
       nodes.append(node)
       if not node.l is None:q.append(node.l)
       if not node.r is None:q.append(node.r)
    pass
    return nodes





def postordereval_i2(root): 
    """
    Iterative binary tree evaluation
    """ 
    assert root
     
    nodes = []
    s = []
     
    nodes.append(root)
    while len(nodes) > 0:
        node = nodes.pop()
        s.append(node)
        if node.l is not None:
            nodes.append(node.l)
        if node.r is not None :
            nodes.append(node.r)
        pass
    pass

    # s collects all nodes in reverse postorder
    # instead of reversing s, use from the back indexing  

    lhs = []
    rhs = []
    nn = len(s)

    debug = False

    c = nn - 1
    while c >= 0: 
        
        if c > 2 and s[c-1].d - s[c].d == 1 and s[c-2].d*2 == s[c].d: 
            l = s[c-0]
            r = s[c-1]
            p = s[c-2]

            if debug:
                print "c %d lrp %s %s %s " % (c,l,r,p)

            c -= 3

            el = binary_calc(l)  
            er = binary_calc(r) 
        else:
            p = s[c-0]
            if debug:
                print "c %d el %s er %s p %s " % (c, el, er, p)
            c -= 1

            # pop results of prior lower level calculation
            el = lhs.pop()
            er = rhs.pop()
        pass

        ep = binary_calc(p,el,er)

        pleft = p.d % 2 == 0
        if pleft:
            lhs.append(ep)
        else:
            rhs.append(ep)
        pass
    pass

    assert c == -1 , (c, nn)
    assert s[c+1].d == 1
    assert len(lhs) == 0, lhs
    assert len(rhs) == 1, rhs

    return ep
 


def fake_label_r(node, idx, label):
    if node.d == idx:
        setattr(node, label, 1)

    if node.l is not None:fake_label_r(node.l, idx, label)
    if node.r is not None:fake_label_r(node.l, idx, label)


if __name__ == '__main__':


    #roots = [root2]
    roots = [root2, root3, root4]
    #roots = [root3, root4]
    #roots = [root3]

    for root in roots:

        # tree labelling
        root.maxidx = levelorder_i(root)
        root.maxdepth = depth_r(root)

        fake_label_r(root, 2, "LoopL") 

        # just dumping
        nodes = postOrderIterative(root)
        print root.name 
        print " maxdepth:%d maxidx:%d " % (root.maxdepth, root.maxidx ) 
        print " postorder:" + " ".join(map(lambda node:str(node.d), nodes))
        print 

        # compare the imps
        ret0 = None
        for fn in [postordereval_r,postordereval_i1,postordereval_i2]:
            ret = fn(root) 
            print "%20s : %s " % ( fn.__name__, ret )

            if ret0 is None:
                ret0 = ret
            else:
                assert ret == ret0, (ret, ret0)
            pass
        pass
    pass
        





