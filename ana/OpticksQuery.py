#!/usr/bin/env python

import os, logging
log = logging.getLogger(__name__)


class OpticksQuery(object):
    """
    Analogue of okc/OpticksQuery.cc
    """
    RANGE_ = "range:"

    INDEX_ = "index:"
    DEPTH_ = "depth:"

    NONE = 0
    RANGE = 1
    INDEX = 2

    def __init__(self, query):
        elem = query.split(",")

        self.type_ = self.NONE 
        self.range_ = []
        self.index = 0
        self.index_depth = 0   # depth of the index
        self.depth = 0         # relative depth from the index

        for q in elem:
            self.parseQueryElement(q)
        pass
        self.query = query 

        if self.type_ == self.RANGE:
            self.expect = self.expected_range_count()
        else:
            self.expect = None
        pass

    def __repr__(self):
        return "OpticksQuery %s range %r index %s depth %s  " % (self.query, self.range_, self.index, self.depth)  


    def check_selected_count(self, count):
        if self.type_ != self.RANGE: return
        assert self.expect == count, ( "expect %s count %s " % (self.expect, count))
        log.info("count %s matches expectation " % count )

    def expected_range_count(self):
        """
        * slice style range selection 0:10 corresponds to 10 items from 0 to 9 inclusive 
        """
        expected_ = 0 
        for i in range(len(self.range_)/2):
            expected_ += self.range_[i*2+1] - self.range_[i*2+0] 
            pass
        return expected_

    def selected(self, name, index, depth, recursive_select_=False):
        """
        """
        selected_ = False
        if self.type_ == self.NONE:
             selected_ = True
        elif self.type_ == self.RANGE and len(self.range_) > 0:
            assert len(self.range_) % 2 == 0 
            for i in range(len(self.range_)/2):
                if index >= self.range_[i*2+0] and index < self.range_[i*2+1]:
                    selected_ = True
                pass
            pass
        elif self.type_ == self.INDEX and self.index > 0:
            if index == self.index:
                self.index_depth = depth 
                log.info("index found at depth %s " % self.index_depth ) # departs from OpticksQuery.cc
                selected_ = True
                recursive_select_ = True
            elif recursive_select_:
                if self.index_depth > 0 and depth < self.index_depth + self.depth:
                    selected_ = True 
                pass
            pass
        else:
            pass
        pass
        return selected_, recursive_select_


    def __call__(self, index):
        return self.selected("dummy", index, -1)

        
    def parseQueryElement(self, q):
        if q.startswith(self.RANGE_):
            self.type_ = self.RANGE
            elem = q[len(self.RANGE_):].split(":")
            assert len(elem) == 2 
            self.range_.append(int(elem[0]))
            self.range_.append(int(elem[1]))
        elif q.startswith(self.INDEX_):
            self.type_ = self.INDEX
            self.index = int(q[len(self.INDEX_):])
        elif q.startswith(self.DEPTH_):
            self.depth = int(q[len(self.DEPTH_):])
        else:
            pass
        pass



if __name__ == '__main__':

    logging.basicConfig(level=logging.INFO)
    #q_ = "range:10:21"
    q_ = "index:4,depth:2"


    q = OpticksQuery(q_)

    print q 




   
    


