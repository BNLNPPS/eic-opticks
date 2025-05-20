#!/usr/bin/env python

import pyvista as pv

if __name__ == '__main__':
    sphere = pv.Sphere(start_theta=0, end_theta=150, start_phi=10, end_phi=120)
    pl = pv.Plotter()
    pl.add_text("pyvista sphere", position="upper_left")
    pl.add_mesh(sphere, opacity=0.7, show_edges=True, lighting=True)
    pl.show()
