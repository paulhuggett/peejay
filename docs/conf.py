# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

import os
import pathlib
import subprocess

project = 'peejay'
copyright = '2026, Paul Bowen-Huggett'
author = 'Paul Bowen-Huggett'
version = '0.1'
release = version

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
     'breathe',
     'sphinx.ext.autosectionlabel',
     'sphinx.ext.graphviz',
     'sphinx_design'
]

# Breathe configuration
breathe_projects = { project: './xml/' }
breathe_default_project = project

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 'xml']

# Section labels are referenced as 'document/path:Section title'.
autosectionlabel_prefix_document = True

# The documentation is about C++: make it the default for code and cross-references.
primary_domain = 'cpp'
highlight_language = 'cpp'

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'furo'

if os.environ.get('READTHEDOCS', None) == 'True':
     # Run doxygen to produce the XML consumed by Breathe. The Doxyfile's
     # clang-assisted parsing options hard-code host-specific include paths
     # that don't exist on Read the Docs, so turn that off and append the
     # overrides to the Doxyfile (later settings win).
     root = pathlib.Path(__file__).resolve().parent.parent
     overrides = '\n'.join([
          'CLANG_ASSISTED_PARSING = NO',
          'GENERATE_HTML = NO',
          'GENERATE_XML = YES',
     ])
     doxyfile = (root / 'Doxyfile').read_text() + '\n' + overrides + '\n'
     subprocess.run(['doxygen', '-'], cwd=root, input=doxyfile, text=True, check=True)
