project = "eic-opticks"
copyright = "2025, BNLNPPS"
author = "BNLNPPS"
version = "0.1"
release = "0.1.0"

extensions = [
    "myst_parser",
    "sphinx.ext.autodoc",
    "sphinx.ext.viewcode",
]

source_suffix = {
    ".rst": "restructuredtext",
    ".md": "markdown",
}

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

html_theme = "sphinx_rtd_theme"
html_static_path = ["_static"]

html_theme_options = {
    "navigation_depth": 3,
    "collapse_navigation": False,
}

html_context = {
    "display_github": True,
    "github_user": "BNLNPPS",
    "github_repo": "eic-opticks",
    "github_version": "main",
    "conf_py_path": "/docs/",
}

myst_enable_extensions = [
    "colon_fence",
    "deflist",
]
myst_heading_anchors = 3
