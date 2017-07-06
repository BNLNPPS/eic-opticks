#!/usr/bin/env python
"""

Regenerate the cpp for each mesh with::

    gdml2gltf.py   # writes .gltf and extras beside the .gdml in standard IDFOLD location

See::

    opticks-nnt LVID
    opticks-nnt-vi LVID


"""
import logging, os
log = logging.getLogger(__name__)

template_head = r"""

#include "SSys.hh"
#include "NGLMExt.hpp"
#include "NCSG.hpp"
#include "NNode.hpp"
#include "NPrimitives.hpp"
#include "PLOG.hh"
#include "NPY_LOG.hh"

int main(int argc, char** argv)
{
    PLOG_(argc, argv);
    NPY_LOG__ ; 
"""


template_tail = r"""

    %(root)s.update_gtransforms();
    %(root)s.verbosity = SSys::getenvint("VERBOSITY", 1) ; 
    %(root)s.dump() ; 

    const char* boundary = "Rock//perfectAbsorbSurface/Vacuum" ;
    NCSG* csg = NCSG::FromNode(&%(root)s, boundary);
    csg->dump();
    csg->dump_surface_points("dsp", 20);


    return 0 ;        
}
"""

template_body = r"""%(body)s"""

test_body = r"""
nsphere a = make_nsphere(0,0,0,100) ;
"""


from opticks.ana.base import now_


def indent_(s):
    lines = s.split("\n")
    return "\n".join(map(lambda _:"    %s" % _, lines))

def trim_(s):
    lines = filter(None, s.split("\n"))
    return "\n".join(lines)
    


class NNodeTestCPP(dict):
     def __init__(self, *args, **kwa):
         dict.__init__(self, *args, **kwa)

     def _get_stamp(self):
         return "// generated by nnode_test_cpp.py : %s " % ( now_() )
     stamp = property(_get_stamp)

     def _get_runline(self):
         return "\n".join([
                   "// opticks-;opticks-nnt %(name)s " % self,
                   "// opticks-;opticks-nnt-vi %(name)s " % self,""])
     runline = property(_get_runline)

     head = property(lambda self:template_head % self)
     body = property(lambda self:template_body % self)
     tail = property(lambda self:template_tail % self)

     path = property(lambda self:os.path.expandvars("$TMP/tbool%(name)s.cc" % self))

     def save(self):
         log.info("saving to %s " % self.path) 
         file(self.path,"w").write(str(self))

     def test(self):
         print self.stamp
         print self.head
         print self.body
         print self.tail

     def __str__(self):
         return "\n".join([self.stamp, self.runline, self.head, indent_("\n".join([self.stamp,self.runline])), indent_(self.body), self.tail])  


if __name__ == '__main__':

     logging.basicConfig(level=logging.INFO)

     ntc = NNodeTestCPP(name="0", root="a", body=test_body)
     #ntc.test()

     print ntc 

     #ntc.save()

     


