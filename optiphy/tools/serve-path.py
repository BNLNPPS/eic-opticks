#!/usr/bin/env python3

import argparse

from flask import Flask
from flask_autoindex import AutoIndex

app = Flask(__name__)

def run_autoindex(path):
    AutoIndex(app, browse_root=path)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("path", nargs='?', default='iframe_figures/', help="A path to serve")

    args = parser.parse_args()

    run_autoindex(args.path)
    app.run(host='0.0.0.0', port=8080)
