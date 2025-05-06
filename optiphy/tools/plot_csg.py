#!/usr/bin/env python3

import argparse
import logging
import pathlib as pl
import plotly.graph_objects as go
import numpy as np

from optiphy import csg

logging.basicConfig(level=logging.INFO)


def read_mesh(path_csg, path_rec, path_out):
    tri = np.load(path_csg/'tri.npy')
    vtx = np.load(path_csg/'vtx.npy')

    rays = []
    if path_rec:
        rec = np.load(path_rec/'record.npy')
        rays = csg.rays(rec)

    faces, edges = csg.mesh(tri, vtx)

    logging.info(path_csg.name)

    fig = go.Figure(data=[faces, edges, *rays])
    html_file = path_out / (str(path_csg).replace("/", "_").replace("\\", "_").replace(".", "") + '.html')
    fig.update_layout(scene_camera=dict( eye=dict(x=0, y=0, z=2), up=dict(x=0, y=1, z=0) ))
    fig.write_html(html_file, include_plotlyjs='cdn')

    logging.info(f'Saved image to: {html_file}')


def read_meshgroup(path_csg: pl.Path, path_rec: pl.Path, path_out: pl.Path):
    name_index = path_csg/'NPFold_names.txt'

    volume_names = []

    if name_index.is_file():
        with name_index.open('r') as f:
            volume_names = f.read().splitlines()
    else:
        logging.fatal(f"NPFold_names.txt not found in {path_csg}")

    logging.debug(volume_names)

    fig = go.Figure()

    for subdir_idx, volume_name in enumerate(volume_names):
        tri_file = path_csg / f'{subdir_idx}/tri.npy'
        vtx_file = path_csg / f'{subdir_idx}/vtx.npy'

        if tri_file.is_file() and vtx_file.is_file():
            tri = np.load(tri_file)
            vtx = np.load(vtx_file)

            faces, edges = csg.mesh(tri, vtx, 'legendonly' if subdir_idx == 0 else True)
            faces.name = f'{volume_name} faces'
            edges.name = f'{volume_name} edges'

            fig.add_traces([faces, edges])

    rays = []
    suffix = ''
    if path_rec:
        rec = np.load(path_rec/'record.npy')
        rays = csg.rays(rec)
        fig.add_traces(rays)
        suffix = '_' + str(path_rec).replace("/", "_").replace("\\", "_").replace(".", "")

    path_out.mkdir(parents=True, exist_ok=True)
    html_file = path_out / (str(path_csg).replace("/", "_").replace("\\", "_").replace(".", "") + suffix + '.html')
    fig.update_layout(scene_camera=dict( eye=dict(x=-2, y=0, z=0), up=dict(x=0, y=1, z=0) ), scene_dragmode='orbit')
    fig.write_html(html_file, include_plotlyjs='cdn')

    logging.info(f'Saved image to: {html_file}')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('path', help="A path to CSG dir with tri.npy and vtx.npy files")
    parser.add_argument('-r', '--rays', help="A path to record.npy file")
    parser.add_argument('-o', '--outdir', default='/home/web/optiphy', help="A path to output dir")

    args = parser.parse_args()

    path_csg = pl.Path(args.path)
    path_rec = pl.Path(args.rays) if args.rays else None
    path_out = pl.Path(args.outdir)
    logging.info(f'Reading data from dir: {path_csg}')

    read_meshgroup(path_csg, path_rec, path_out)


if __name__ == '__main__':
    main()
